//===--- PILNode.h - Node base class for PIL --------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file defines the PILNode class.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILNODE_H
#define POLARPHP_PIL_PILNODE_H

#include "llvm/Support/Compiler.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "polarphp/basic/InlineBitfield.h"
#include "polarphp/basic/LLVM.h"
#include <type_traits>

namespace polar {

class PILBasicBlock;
class PILFunction;
class PILInstruction;
class PILModule;
class SingleValueInstruction;
class ValueBase;

using polar::bitmax;
using polar::count_bits_used;

/// An enumeration which contains values for all the nodes in PILNodes.def.
/// Other enumerators, like ValueKind and PILInstructionKind, ultimately
/// take their values from this enumerator.
enum class PILNodeKind {
#define NODE(ID, PARENT) \
  ID,
#define NODE_RANGE(ID, FIRST, LAST) \
  First_##ID = FIRST, \
  Last_##ID = LAST,
#include "polarphp/pil/lang/PILNodesDef.h"
};

enum { NumPILNodeKindBits =
   count_bits_used(static_cast<unsigned>(PILNodeKind::Last_PILNode)) };

enum class PILInstructionKind : std::underlying_type<PILNodeKind>::type;

/// A PILNode is a node in the use-def graph of a PILFunction.  It is
/// either an instruction or a defined value which can be used by an
/// instruction.  A defined value may be an instruction result, a basic
/// block argument, or the special 'undef' value.
///
/// The 'node' intuition is slightly imprecise because a single instruction
/// may be composed of multiple PILNodes: one for the instruction itself
/// and one for each value it produces.  When an instruction kind always
/// produces exactly one value, the cast machinery (isa, cast, and dyn_cast)
/// works to make both nodes appear to be the same object: there is a value
/// kind exactly equal to the instruction kind and the value node can be
/// directly cast to the instruction's class.  When an instruction kind
/// never produces values, it has no corresponding value kind, and it is
/// a compile-time error to attempt to cast a value node to the instruction
/// class.  When an instruction kind can have multiple values (not yet
/// implemented), its value nodes have a different kind from the
/// instruction kind and it is a static error to attempt to cast a value
/// node to the instruction kind.
///
/// Another way of interpreting PILNode is that there is a PILNode for
/// everything that can be numbered in PIL assembly (plus 'undef', which
/// is not conventionally numbered).  Instructions without results are
/// still numbered in PIL in order to describe the users lists of an
/// instruction or argument.  Instructions with multiple results are
/// numbered using their first result.
///
/// PILNode is a base class of both PILInstruction and ValueBase.
/// Because there can be multiple PILNodes within a single instruction
/// object, some care must be taken when working with PILNode pointers.
/// These precautions only apply to PILNode* and not its subclasses.
///
/// - There may have multiple PILNode* values that refer to the same
///   instruction.  Data structures and algorithms that rely on uniqueness of a
///   PILNode* should generally make sure that they're working with the
///   representative PILNode*; see getRepresentativePILNodeInObject().
///
/// - Do not use builtin C++ casts to downcast a PILNode*.  A static_cast
///   from PILNode* to PILInstruction* only works if the referenced
///   PILNode is the base subobject of the object's PILInstruction
///   subobject.  If the PILNode is actually the base subobject of a
///   ValueBase subobject, the cast will yield a corrupted value.
///   Always use the LLVM casts (cast<>, dyn_cast<>, etc.) instead.
class alignas(8) PILNode {
public:
   enum { NumVOKindBits = 3 };
   enum { NumStoreOwnershipQualifierBits = 2 };
   enum { NumLoadOwnershipQualifierBits = 2 };
   enum { NumAssignOwnershipQualifierBits = 2 };
   enum { NumPILAccessKindBits = 2 };
   enum { NumPILAccessEnforcementBits = 2 };

protected:
   union {
      uint64_t OpaqueBits;

      POLAR_INLINE_BITFIELD_BASE(PILNode, bitmax(NumPILNodeKindBits,8)+1+1,
                                 Kind : bitmax(NumPILNodeKindBits,8),
                                 StorageLoc : 1,
                                 IsRepresentativeNode : 1
      );

      POLAR_INLINE_BITFIELD_EMPTY(ValueBase, PILNode);

      POLAR_INLINE_BITFIELD(PILArgument, ValueBase, NumVOKindBits,
                            VOKind : NumVOKindBits
      );

      // No MultipleValueInstructionResult subclass needs inline bits right now,
      // therefore let's naturally align and size the Index for speed.
      POLAR_INLINE_BITFIELD_FULL(MultipleValueInstructionResult, ValueBase,
                                 NumVOKindBits+32,
                                 VOKind : NumVOKindBits,
                                 : NumPadBits,
                                 Index : 32
      );

      POLAR_INLINE_BITFIELD_EMPTY(PILInstruction, PILNode);

      // Special handling for UnaryInstructionWithTypeDependentOperandsBase
      POLAR_INLINE_BITFIELD(IBWTO, PILNode, 64-NumPILNodeBits,
      // DO NOT allocate bits at the front!
      // IBWTO is a template, and templates must allocate bits from back to front
      // so that "normal" subclassing can allocate bits from front to back.
      // If you update this, then you must update the IBWTO_BITFIELD macros.

      /*pad*/ : 32-NumPILNodeBits,

      // Total number of operands of this instruction.
      // It is number of type dependent operands + 1.
                            NumOperands : 32;
                               template<PILInstructionKind Kind, typename, typename, typename...>
                               friend class InstructionBaseWithTrailingOperands
      );

#define IBWTO_BITFIELD(T, U, C, ...) \
  POLAR_INLINE_BITFIELD_TEMPLATE(T, U, (C), 32, __VA_ARGS__)
#define IBWTO_BITFIELD_EMPTY(T, U) \
  POLAR_INLINE_BITFIELD_EMPTY(T, U)

#define UIWTDOB_BITFIELD(T, U, C, ...) \
  IBWTO_BITFIELD(T, U, (C), __VA_ARGS__)
#define UIWTDOB_BITFIELD_EMPTY(T, U) \
  IBWTO_BITFIELD_EMPTY(T, U)

      POLAR_INLINE_BITFIELD_EMPTY(SingleValueInstruction, PILInstruction);
      POLAR_INLINE_BITFIELD_EMPTY(DeallocationInst, PILInstruction);
      POLAR_INLINE_BITFIELD_EMPTY(LiteralInst, SingleValueInstruction);
      POLAR_INLINE_BITFIELD_EMPTY(AllocationInst, SingleValueInstruction);

      // Ensure that StructInst bitfield does not overflow.
      IBWTO_BITFIELD_EMPTY(StructInst, SingleValueInstruction);

      // Ensure that TupleInst bitfield does not overflow.
      IBWTO_BITFIELD_EMPTY(TupleInst, SingleValueInstruction);

      IBWTO_BITFIELD(ObjectInst, SingleValueInstruction,
                     32-NumSingleValueInstructionBits,
                     NumBaseElements : 32-NumSingleValueInstructionBits
      );

      IBWTO_BITFIELD(SelectEnumInstBase, SingleValueInstruction, 1,
                     HasDefault : 1
      );

      POLAR_INLINE_BITFIELD_FULL(IntegerLiteralInst, LiteralInst, 32,
                                 : NumPadBits,
                                 numBits : 32
      );

      POLAR_INLINE_BITFIELD_FULL(FloatLiteralInst, LiteralInst, 32,
                                 : NumPadBits,
                                 numBits : 32
      );

      POLAR_INLINE_BITFIELD_FULL(StringLiteralInst, LiteralInst, 2+32,
                                 TheEncoding : 2,
                                 : NumPadBits,
                                 Length : 32
      );

      POLAR_INLINE_BITFIELD(DeallocRefInst, DeallocationInst, 1,
                            OnStack : 1
      );

      // Ensure that AllocBoxInst bitfield does not overflow.
      IBWTO_BITFIELD_EMPTY(AllocBoxInst, AllocationInst);
      // Ensure that AllocExistentialBoxInst bitfield does not overflow.
      IBWTO_BITFIELD_EMPTY(AllocExistentialBoxInst, AllocationInst);
      POLAR_INLINE_BITFIELD_FULL(AllocStackInst, AllocationInst,
                                 64-NumAllocationInstBits,
                                 NumOperands : 32-NumAllocationInstBits,
                                 VarInfo : 32
      );
      IBWTO_BITFIELD(AllocRefInstBase, AllocationInst, 32-NumAllocationInstBits,
                     ObjC : 1,
                     OnStack : 1,
                     NumTailTypes : 32-1-1-NumAllocationInstBits
      );
      static_assert(32-1-1-NumAllocationInstBits >= 16, "Reconsider bitfield use?");

      UIWTDOB_BITFIELD_EMPTY(AllocValueBufferInst, AllocationInst);

      // TODO: Sort the following in PILNodes.def order

      POLAR_INLINE_BITFIELD_EMPTY(NonValueInstruction, PILInstruction);
      POLAR_INLINE_BITFIELD(RefCountingInst, NonValueInstruction, 1,
                            atomicity : 1
      );

      // Ensure that BindMemoryInst bitfield does not overflow.
      IBWTO_BITFIELD_EMPTY(BindMemoryInst, NonValueInstruction);

      // Ensure that MarkFunctionEscapeInst bitfield does not overflow.
      IBWTO_BITFIELD_EMPTY(MarkFunctionEscapeInst, NonValueInstruction);

      // Ensure that MetatypeInst bitfield does not overflow.
      IBWTO_BITFIELD_EMPTY(MetatypeInst, SingleValueInstruction);

      POLAR_INLINE_BITFIELD(CopyAddrInst, NonValueInstruction, 1+1,
      /// IsTakeOfSrc - True if ownership will be taken from the value at the
      /// source memory location.
                            IsTakeOfSrc : 1,

      /// IsInitializationOfDest - True if this is the initialization of the
      /// uninitialized destination memory location.
                            IsInitializationOfDest : 1
      );

      POLAR_INLINE_BITFIELD(LoadReferenceInstBaseT, NonValueInstruction, 1,
                            IsTake : 1;
                               template<PILInstructionKind K>
                               friend class LoadReferenceInstBase
      );

      POLAR_INLINE_BITFIELD(StoreReferenceInstBaseT, NonValueInstruction, 1,
                            IsInitializationOfDest : 1;
                               template<PILInstructionKind K>
                               friend class StoreReferenceInstBase
      );

      POLAR_INLINE_BITFIELD(BeginAccessInst, SingleValueInstruction,
                            NumPILAccessKindBits+NumPILAccessEnforcementBits
                            + 1 + 1,
                            AccessKind : NumPILAccessKindBits,
                            Enforcement : NumPILAccessEnforcementBits,
                            NoNestedConflict : 1,
                            FromBuiltin : 1
      );
      POLAR_INLINE_BITFIELD(BeginUnpairedAccessInst, NonValueInstruction,
                            NumPILAccessKindBits + NumPILAccessEnforcementBits
                            + 1 + 1,
                            AccessKind : NumPILAccessKindBits,
                            Enforcement : NumPILAccessEnforcementBits,
                            NoNestedConflict : 1,
                            FromBuiltin : 1);

      POLAR_INLINE_BITFIELD(EndAccessInst, NonValueInstruction, 1,
                            Aborting : 1
      );
      POLAR_INLINE_BITFIELD(EndUnpairedAccessInst, NonValueInstruction,
                            NumPILAccessEnforcementBits + 1 + 1,
                            Enforcement : NumPILAccessEnforcementBits,
                            Aborting : 1,
                            FromBuiltin : 1);

      POLAR_INLINE_BITFIELD(StoreInst, NonValueInstruction,
                            NumStoreOwnershipQualifierBits,
                            OwnershipQualifier : NumStoreOwnershipQualifierBits
      );
      POLAR_INLINE_BITFIELD(LoadInst, SingleValueInstruction,
                            NumLoadOwnershipQualifierBits,
                            OwnershipQualifier : NumLoadOwnershipQualifierBits
      );
      POLAR_INLINE_BITFIELD(AssignInst, NonValueInstruction,
                            NumAssignOwnershipQualifierBits,
                            OwnershipQualifier : NumAssignOwnershipQualifierBits
      );
      POLAR_INLINE_BITFIELD(AssignByWrapperInst, NonValueInstruction,
                            NumAssignOwnershipQualifierBits,
                            OwnershipQualifier : NumAssignOwnershipQualifierBits
      );

      POLAR_INLINE_BITFIELD(UncheckedOwnershipConversionInst,SingleValueInstruction,
                            NumVOKindBits,
                            Kind : NumVOKindBits
      );

      POLAR_INLINE_BITFIELD_FULL(TupleExtractInst, SingleValueInstruction, 32,
                                 : NumPadBits,
                                 FieldNo : 32
      );
      POLAR_INLINE_BITFIELD_FULL(TupleElementAddrInst, SingleValueInstruction, 32,
                                 : NumPadBits,
                                 FieldNo : 32
      );

      POLAR_INLINE_BITFIELD_FULL(FieldIndexCacheBase, SingleValueInstruction, 32,
                                 : NumPadBits,
                                 FieldIndex : 32);

      POLAR_INLINE_BITFIELD_EMPTY(MethodInst, SingleValueInstruction);
      // Ensure that WitnessMethodInst bitfield does not overflow.
      IBWTO_BITFIELD_EMPTY(WitnessMethodInst, MethodInst);
      UIWTDOB_BITFIELD_EMPTY(ObjCMethodInst, MethodInst);

      POLAR_INLINE_BITFIELD_EMPTY(ConversionInst, SingleValueInstruction);
      POLAR_INLINE_BITFIELD(PointerToAddressInst, ConversionInst, 1+1,
                            IsStrict : 1,
                            IsInvariant : 1
      );

      UIWTDOB_BITFIELD(ConvertFunctionInst, ConversionInst, 1,
                       WithoutActuallyEscaping : 1);
      UIWTDOB_BITFIELD_EMPTY(PointerToThinFunctionInst, ConversionInst);
      UIWTDOB_BITFIELD_EMPTY(UnconditionalCheckedCastInst, ConversionInst);
      UIWTDOB_BITFIELD_EMPTY(UpcastInst, ConversionInst);
      UIWTDOB_BITFIELD_EMPTY(UncheckedRefCastInst, ConversionInst);
      UIWTDOB_BITFIELD_EMPTY(UncheckedAddrCastInst, ConversionInst);
      UIWTDOB_BITFIELD_EMPTY(UncheckedTrivialBitCastInst, ConversionInst);
      UIWTDOB_BITFIELD_EMPTY(UncheckedBitwiseCastInst, ConversionInst);
      UIWTDOB_BITFIELD_EMPTY(ThinToThickFunctionInst, ConversionInst);
      UIWTDOB_BITFIELD_EMPTY(UnconditionalCheckedCastValueInst, ConversionInst);
      UIWTDOB_BITFIELD_EMPTY(InitExistentialAddrInst, SingleValueInstruction);
      UIWTDOB_BITFIELD_EMPTY(InitExistentialValueInst, SingleValueInstruction);
      UIWTDOB_BITFIELD_EMPTY(InitExistentialRefInst, SingleValueInstruction);
      UIWTDOB_BITFIELD_EMPTY(InitExistentialMetatypeInst, SingleValueInstruction);

      POLAR_INLINE_BITFIELD_EMPTY(TermInst, PILInstruction);
      UIWTDOB_BITFIELD_EMPTY(CheckedCastBranchInst, SingleValueInstruction);
      UIWTDOB_BITFIELD_EMPTY(CheckedCastValueBranchInst, SingleValueInstruction);

      // Ensure that BranchInst bitfield does not overflow.
      IBWTO_BITFIELD_EMPTY(BranchInst, TermInst);
      // Ensure that YieldInst bitfield does not overflow.
      IBWTO_BITFIELD_EMPTY(YieldInst, TermInst);
      IBWTO_BITFIELD(CondBranchInst, TermInst, 32-NumTermInstBits,
                     NumTrueArgs : 32-NumTermInstBits
      );
      IBWTO_BITFIELD(SwitchValueInst, TermInst, 1,
                     HasDefault : 1
      );
      POLAR_INLINE_BITFIELD_FULL(SwitchEnumInstBase, TermInst, 1+32,
                                 HasDefault : 1,
                                 : NumPadBits,
                                 NumCases : 32
      );

   } Bits;

   enum class PILNodeStorageLocation : uint8_t { Value, Instruction };

   enum class IsRepresentative : bool {
      No = false,
      Yes = true,
   };

private:

   PILNodeStorageLocation getStorageLoc() const {
      return PILNodeStorageLocation(Bits.PILNode.StorageLoc);
   }

   const PILNode *getRepresentativePILNodeSlowPath() const;

protected:
   PILNode(PILNodeKind kind, PILNodeStorageLocation storageLoc,
           IsRepresentative isRepresentative) {
      Bits.OpaqueBits = 0;
      Bits.PILNode.Kind = unsigned(kind);
      Bits.PILNode.StorageLoc = unsigned(storageLoc);
      Bits.PILNode.IsRepresentativeNode = unsigned(isRepresentative);
   }

public:
   /// Does the given kind of node inherit from multiple multiple PILNode base
   /// classes?
   ///
   /// This enables one to know if their is a diamond in the inheritence
   /// hierarchy for this PILNode.
   static bool hasMultiplePILNodeBases(PILNodeKind kind) {
      // Currently only SingleValueInstructions.  Note that multi-result
      // instructions shouldn't return true for this.
      return kind >= PILNodeKind::First_SingleValueInstruction &&
             kind <= PILNodeKind::Last_SingleValueInstruction;
   }

   /// Is this PILNode the representative PILNode subobject in this object?
   bool isRepresentativePILNodeInObject() const {
      return Bits.PILNode.IsRepresentativeNode;
   }

   /// Return a pointer to the representative PILNode subobject in this object.
   PILNode *getRepresentativePILNodeInObject() {
      if (isRepresentativePILNodeInObject())
         return this;
      return const_cast<PILNode *>(getRepresentativePILNodeSlowPath());
   }

   const PILNode *getRepresentativePILNodeInObject() const {
      if (isRepresentativePILNodeInObject())
         return this;
      return getRepresentativePILNodeSlowPath();
   }

   LLVM_ATTRIBUTE_ALWAYS_INLINE
   PILNodeKind getKind() const {
      return PILNodeKind(Bits.PILNode.Kind);
   }

   /// Return the PILNodeKind of this node's representative PILNode.
   PILNodeKind getKindOfRepresentativePILNodeInObject() const {
      return getRepresentativePILNodeInObject()->getKind();
   }

   /// If this is a PILArgument or a PILInstruction get its parent basic block,
   /// otherwise return null.
   PILBasicBlock *getParentBlock() const;

   /// If this is a PILArgument or a PILInstruction get its parent function,
   /// otherwise return null.
   PILFunction *getFunction() const;

   /// If this is a PILArgument or a PILInstruction get its parent module,
   /// otherwise return null.
   PILModule *getModule() const;

   /// Pretty-print the node.  If the node is an instruction, the output
   /// will be valid PIL assembly; otherwise, it will be an arbitrary
   /// format suitable for debugging.
   void print(raw_ostream &OS) const;
   void dump() const;

   /// Pretty-print the node in context, preceded by its operands (if the
   /// value represents the result of an instruction) and followed by its
   /// users.
   void printInContext(raw_ostream &OS) const;
   void dumpInContext() const;

   // Cast to SingleValueInstruction.  This is an implementation detail
   // of the cast machinery.  At a high level, all you need to know is to
   // never use static_cast to downcast a PILNode.
   SingleValueInstruction *castToSingleValueInstruction();
   const SingleValueInstruction *castToSingleValueInstruction() const {
      return const_cast<PILNode*>(this)->castToSingleValueInstruction();
   }

   static bool classof(const PILNode *node) {
      return true;
   }
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                     const PILNode &node) {
   node.print(OS);
   return OS;
}

template <class To> struct cast_sil_node_is_unambiguous {
   // The only ambiguity right now is between the value and instruction
   // nodes on a SingleValueInstruction.
   static constexpr bool value =
      // If the destination type isn't a subclass of ValueBase or
      // PILInstruction, there's no ambiguity.
      (!std::is_base_of<PILInstruction, To>::value &&
       !std::is_base_of<ValueBase, To>::value)

      // If the destination type is a proper subclass of ValueBase
      // that isn't a subclass of PILInstruction, there's no ambiguity.
      || (std::is_base_of<ValueBase, To>::value &&
          !std::is_same<ValueBase, To>::value &&
          !std::is_base_of<PILInstruction, To>::value)

      // If the destination type is a proper subclass of PILInstruction
      // that isn't a subclass of ValueBase, there's no ambiguity.
      || (std::is_base_of<PILInstruction, To>::value &&
          !std::is_same<PILInstruction, To>::value &&
          !std::is_base_of<ValueBase, To>::value);
};

template <class To,
   bool IsSingleValueInstruction =
   std::is_base_of<SingleValueInstruction, To>::value,
   bool IsKnownUnambiguous =
   cast_sil_node_is_unambiguous<To>::value>
struct cast_sil_node;

// If all complete objects of the destination type are known to only
// contain a single node, we can just use a static_cast.
template <class To>
struct cast_sil_node<To, /*single value*/ false, /*unambiguous*/ true> {
   static To *doit(PILNode *node) {
      return &static_cast<To&>(*node);
   }
};

// If we're casting to a subclass of SingleValueInstruction, we don't
// need to dynamically check whether the node is an SVI.  In fact,
// we can't, because the static_cast will be ambiguous.
template <class To>
struct cast_sil_node<To, /*single value*/ true, /*unambiguous*/ false> {
   static To *doit(PILNode *node) {
      auto svi = node->castToSingleValueInstruction();
      return &static_cast<To&>(*svi);
   }
};

// Otherwise, we need to dynamically check which case we're in.
template <class To>
struct cast_sil_node<To, /*single value*/ false, /*unambiguous*/ false> {
   static To *doit(PILNode *node) {
      // If the node isn't dynamically a SingleValueInstruction, then this
      // is indeed the PILNode subobject that's statically observable in To.
      if (!PILNode::hasMultiplePILNodeBases(node->getKind())) {
         return &static_cast<To&>(*node);
      }

      auto svi = node->castToSingleValueInstruction();
      return &static_cast<To&>(*svi);
   }
};

} // end namespace polar

namespace llvm {

/// Completely take over cast<>'ing from PILNode*.  A static_cast to
/// ValueBase* or PILInstruction* can be quite wrong.
template <class To>
struct cast_convert_val<To, polar::PILNode*, polar::PILNode*> {
   using ret_type = typename cast_retty<To, polar::PILNode*>::ret_type;
   static ret_type doit(polar::PILNode *node) {
      return polar::cast_sil_node<To>::doit(node);
   }
};
template <class To>
struct cast_convert_val<To, const polar::PILNode *, const polar::PILNode *> {
   using ret_type = typename cast_retty<To, const polar::PILNode*>::ret_type;
   static ret_type doit(const polar::PILNode *node) {
      return polar::cast_sil_node<To>::doit(const_cast<polar::PILNode*>(node));
   }
};

// We don't support casting from PILNode references yet.
template <class To, class From>
struct cast_convert_val<To, polar::PILNode, From>;
template <class To, class From>
struct cast_convert_val<To, const polar::PILNode, From>;

/// ValueBase * is always at least eight-byte aligned; make the three tag bits
/// available through PointerLikeTypeTraits.
template<>
struct PointerLikeTypeTraits<polar::PILNode *> {
public:
   static inline void *getAsVoidPointer(polar::PILNode *I) {
      return (void*)I;
   }
   static inline polar::PILNode *getFromVoidPointer(void *P) {
      return (polar::PILNode *)P;
   }
   enum { NumLowBitsAvailable = 3 };
};

} // end namespace llvm

#endif
