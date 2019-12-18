//===--- PILInstruction.h - Instructions for PIL code -----------*- C++ -*-===//
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
// This file defines the high-level PILInstruction class used for PIL code.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_INSTRUCTION_H
#define POLARPHP_PIL_INSTRUCTION_H

#include "polarphp/ast/BuiltinTypes.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/GenericSignature.h"
#include "polarphp/ast/InterfaceConformanceRef.h"
#include "polarphp/ast/SubstitutionMap.h"
#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/basic/NullablePtr.h"
#include "polarphp/basic/ProfileCounter.h"
#include "polarphp/basic/Range.h"
#include "polarphp/pil/lang/Consumption.h"
#include "polarphp/pil/lang/PILAllocated.h"
#include "polarphp/pil/lang/PILArgumentArrayRef.h"
#include "polarphp/pil/lang/PILDeclRef.h"
#include "polarphp/pil/lang/PILFunctionConventions.h"
#include "polarphp/pil/lang/PILLocation.h"
#include "polarphp/pil/lang/PILSuccessor.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/lang/ValueUtils.h"
#include "polarphp/global/NameStrings.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ilist.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/Support/TrailingObjects.h"

#include <type_traits>

namespace polar {

class Stmt;
class ValueDecl;
class FuncDecl;
class VarDecl;
class AllocationInst;
class DeclRefExpr;
class FloatLiteralExpr;

class IntegerLiteralExpr;
class SingleValueInstruction;
class MultipleValueInstruction;
class MultipleValueInstructionResult;
class DestructureTupleInst;
class DestructureStructInst;
class NonValueInstruction;
class PILBasicBlock;
class PILBuilder;
class PILDebugLocation;
class PILDebugScope;
class PILFunction;
class PILGlobalVariable;
class PILInstructionResultArray;
class PILOpenedArchetypesState;
class PILType;
class PILArgument;
class PILPhiArgument;
class PILUndef;
class TermInst;
class StringLiteralExpr;
class FunctionRefBaseInst;

class PILBoxType;
class AbstractStorageDecl;
class Identifier;
class IntrinsicInfo;
class BuiltinInfo;
enum class BuiltinValueKind;
class ReferenceStorageType;
class InterfaceDecl;
enum class AccessKind : uint8_t;
class ExistentialMetatypeType;
class MetatypeType;

template <typename ImplClass> class PILClonerWithScopes;

// An enum class for PILInstructions that enables exhaustive switches over
// instructions.
enum class PILInstructionKind : std::underlying_type<PILNodeKind>::type {
#define INST(ID, PARENT) \
  ID = unsigned(PILNodeKind::ID),
#define INST_RANGE(ID, FIRST, LAST) \
  First_##ID = unsigned(PILNodeKind::First_##ID), \
  Last_##ID = unsigned(PILNodeKind::Last_##ID),
#include "polarphp/pil/lang/PILNodesDef.h"
};

/// Return a range which can be used to easily iterate over all
/// PILInstructionKinds.
inline IntRange<PILInstructionKind> allPILInstructionKinds() {
   return IntRange<PILInstructionKind>(
      PILInstructionKind(PILNodeKind::First_PILInstruction),
      PILInstructionKind(unsigned(PILNodeKind::Last_PILInstruction) + 1));
}

/// Map PILInstruction's mnemonic name to its PILInstructionKind.
PILInstructionKind getPILInstructionKind(StringRef InstName);

/// Map PILInstructionKind to a corresponding PILInstruction name.
StringRef getPILInstructionName(PILInstructionKind Kind);

/// A formal PIL reference to a list of values, suitable for use as the result
/// of a PILInstruction.
///
/// *NOTE* Most multiple value instructions will not have many results, so if we
/// want we can cache up to 3 bytes in the lower bits of the value.
///
/// *NOTE* Most of this defined out of line further down in the file to work
/// around forward declaration issues.
///
/// *NOTE* The reason why this does not store the size of the stored element is
/// that just from the number of elements we can infer the size of each element
/// due to the restricted problem space. Specificially:
///
/// 1. Size == 0 implies nothing is stored and thus element size is irrelevent.
/// 2. Size == 1 implies we either had a single value instruction or a multiple
/// value instruction, but no matter what instruction we had, we are going to
/// store the results at the same starting location so element size is
/// irrelevent.
/// 3. Size > 1 implies we must be storing multiple value instruction results
/// implying that the size of each stored element must be
/// sizeof(MultipleValueInstructionResult).
///
/// If we ever allow for subclasses of MultipleValueInstructionResult of
/// different sizes, we will need to store a stride into
/// PILInstructionResultArray. We always assume all results are the same
/// subclass of MultipleValueInstructionResult.
class PILInstructionResultArray {
   friend class MultipleValueInstruction;

   /// Byte pointer to our data. nullptr for empty arrays.
   const uint8_t *Pointer;

   /// The number of stored elements.
   unsigned Size;

public:
   PILInstructionResultArray() : Pointer(nullptr), Size(0) {}
   PILInstructionResultArray(const SingleValueInstruction *SVI);
   PILInstructionResultArray(ArrayRef<MultipleValueInstructionResult> results);

   template <class Result>
   PILInstructionResultArray(ArrayRef<Result> results);

   PILInstructionResultArray(const PILInstructionResultArray &Other) = default;
   PILInstructionResultArray &
   operator=(const PILInstructionResultArray &Other) = default;
   PILInstructionResultArray(PILInstructionResultArray &&Other) = default;
   PILInstructionResultArray &
   operator=(PILInstructionResultArray &&Other) = default;

   PILValue operator[](size_t Index) const;

   bool empty() const { return Size == 0; }

   size_t size() const { return Size; }

   class iterator;

   friend bool operator==(iterator, iterator);
   friend bool operator!=(iterator, iterator);

   iterator begin() const;
   iterator end() const;

   using reverse_iterator = std::reverse_iterator<iterator>;
   reverse_iterator rbegin() const;
   reverse_iterator rend() const;

   using range = iterator_range<iterator>;
   range getValues() const;
   using reverse_range = iterator_range<reverse_iterator>;
   reverse_range getReversedValues() const;

   using type_range = iterator_range<
      llvm::mapped_iterator<iterator, PILType(*)(PILValue), PILType>>;
   type_range getTypes() const;

   bool operator==(const PILInstructionResultArray &rhs);
   bool operator!=(const PILInstructionResultArray &other) {
      return !(*this == other);
   }

   /// Returns true if both this and \p rhs have the same result types.
   ///
   /// *NOTE* This does not imply that the actual return PILValues are the
   /// same. Just that the types are the same.
   bool hasSameTypes(const PILInstructionResultArray &rhs);

private:
   /// Return the first element of the array. Asserts if the array is empty.
   ///
   /// Please do not use this outside of this class. It is only meant to speedup
   /// MultipleValueInstruction::getIndexOfResult(PILValue).
   const ValueBase *front() const;

   /// Return the last element of the array. Asserts if the array is empty.
   ///
   /// Please do not use this outside of this class. It is only meant to speedup
   /// MultipleValueInstruction::getIndexOfResult(PILValue).
   const ValueBase *back() const;
};

class PILInstructionResultArray::iterator {
   /// Our "parent" array.
   ///
   /// This is actually a value type reference into a PILInstruction of some
   /// sort. So we can just have our own copy. This also allows us to not worry
   /// about our underlying array having too short of a lifetime.
   PILInstructionResultArray Parent;

   /// The index into the parent array.
   unsigned Index;

public:
   using difference_type = int;
   using value_type = PILValue;
   using pointer = void;
   using reference = PILValue;
   using iterator_category = std::bidirectional_iterator_tag;

   iterator() = default;
   iterator(const PILInstructionResultArray &Parent, unsigned Index = 0)
      : Parent(Parent), Index(Index) {}

   PILValue operator*() const { return Parent[Index]; }
   PILValue operator->() const { return operator*(); }

   iterator &operator++() {
      ++Index;
      return *this;
   }

   iterator operator++(int) {
      iterator copy = *this;
      ++Index;
      return copy;
   }

   iterator &operator--() {
      --Index;
      return *this;
   }

   iterator operator--(int) {
      iterator copy = *this;
      --Index;
      return copy;
   }

   friend bool operator==(iterator lhs, iterator rhs) {
      assert(lhs.Parent.Pointer == rhs.Parent.Pointer);
      return lhs.Index == rhs.Index;
   }

   friend bool operator!=(iterator lhs, iterator rhs) { return !(lhs == rhs); }
};

inline PILInstructionResultArray::iterator
PILInstructionResultArray::begin() const {
   return iterator(*this, 0);
}

inline PILInstructionResultArray::iterator
PILInstructionResultArray::end() const {
   return iterator(*this, size());
}

inline PILInstructionResultArray::reverse_iterator
PILInstructionResultArray::rbegin() const {
   return llvm::make_reverse_iterator(end());
}

inline PILInstructionResultArray::reverse_iterator
PILInstructionResultArray::rend() const {
   return llvm::make_reverse_iterator(begin());
}

inline PILInstructionResultArray::range
PILInstructionResultArray::getValues() const {
   return {begin(), end()};
}

inline PILInstructionResultArray::reverse_range
PILInstructionResultArray::getReversedValues() const {
   return {rbegin(), rend()};
}

/// This is the root class for all instructions that can be used as the
/// contents of a Swift PILBasicBlock.
///
/// Most instructions are defined in terms of two basic kinds of
/// structure: a list of operand values upon which the instruction depends
/// and a list of result values upon which other instructions can depend.
///
/// The operands can be divided into two sets:
///   - the formal operands of the instruction, which reflect its
///     direct value dependencies, and
///   - the type-dependent operands, which reflect dependencies that are
///     not captured by the formal operands; currently, these dependencies
///     only arise due to certain instructions (e.g. open_existential_addr)
///     that bind new archetypes in the local context.
class PILInstruction
   : public PILNode, public llvm::ilist_node<PILInstruction> {
   friend llvm::ilist_traits<PILInstruction>;
   friend llvm::ilist_traits<PILBasicBlock>;
   friend PILBasicBlock;

   /// A backreference to the containing basic block.  This is maintained by
   /// ilist_traits<PILInstruction>.
   PILBasicBlock *ParentBB;

   /// This instruction's containing lexical scope and source location
   /// used for debug info and diagnostics.
   PILDebugLocation Location;

   PILInstruction() = delete;
   void operator=(const PILInstruction &) = delete;

   /// Check any special state of instructions that are not represented in the
   /// instructions operands/type.
   bool hasIdenticalState(const PILInstruction *RHS) const;

   /// Update this instruction's PILDebugScope. This function should
   /// never be called directly. Use PILBuilder, PILBuilderWithScope or
   /// PILClonerWithScope instead.
   void setDebugScope(const PILDebugScope *DS);

   /// Total number of created and deleted PILInstructions.
   /// It is used only for collecting the compiler statistics.
   static int NumCreatedInstructions;
   static int NumDeletedInstructions;

   // Helper functions used by the ArrayRefViews below.
   static PILValue projectValueBaseAsPILValue(const ValueBase &value) {
      return &value;
   }
   static PILType projectValueBaseType(const ValueBase &value) {
      return value.getType();
   }

   /// An internal method which retrieves the result values of the
   /// instruction as an array of ValueBase objects.
   PILInstructionResultArray getResultsImpl() const;

protected:
   PILInstruction(PILInstructionKind kind, PILDebugLocation DebugLoc)
      : PILNode(PILNodeKind(kind), PILNodeStorageLocation::Instruction,
                IsRepresentative::Yes),
        ParentBB(nullptr), Location(DebugLoc) {
      NumCreatedInstructions++;
   }

   ~PILInstruction() {
      NumDeletedInstructions++;
   }
public:
   /// @todo
   void operator delete(void *Ptr, size_t) POLAR_DELETE_OPERATOR_DELETED;
   /// Instructions should be allocated using a dedicated instruction allocation
   /// function from the ContextTy.
   template <typename ContextTy>
   void *operator new(size_t Bytes, const ContextTy &C,
                      size_t Alignment = alignof(ValueBase)) {
      return C.allocateInst(Bytes, Alignment);
   }

   enum class MemoryBehavior {
      None,
      /// The instruction may read memory.
         MayRead,
      /// The instruction may write to memory.
         MayWrite,
      /// The instruction may read or write memory.
         MayReadWrite,
      /// The instruction may have side effects not captured
      ///        solely by its users. Specifically, it can return,
      ///        release memory, or store. Note, alloc is not considered
      ///        to have side effects because its result/users represent
      ///        its effect.
         MayHaveSideEffects,
   };

   /// Enumeration representing whether the execution of an instruction can
   /// result in memory being released.
   enum class ReleasingBehavior {
      DoesNotRelease,
      MayRelease,
   };

   LLVM_ATTRIBUTE_ALWAYS_INLINE
   PILInstructionKind getKind() const {
      return PILInstructionKind(PILNode::getKind());
   }

   const PILBasicBlock *getParent() const { return ParentBB; }
   PILBasicBlock *getParent() { return ParentBB; }

   PILFunction *getFunction();
   const PILFunction *getFunction() const;

   /// Is this instruction part of a static initializer of a PILGlobalVariable?
   bool isStaticInitializerInst() const { return getFunction() == nullptr; }

   PILModule &getModule() const;

   /// This instruction's source location (AST node).
   PILLocation getLoc() const;
   const PILDebugScope *getDebugScope() const;
   PILDebugLocation getDebugLocation() const { return Location; }

   /// Sets the debug location.
   /// Note: Usually it should not be needed to use this function as the location
   /// is already set in when creating an instruction.
   void setDebugLocation(PILDebugLocation Loc) { Location = Loc; }

   /// This method unlinks 'self' from the containing basic block and deletes it.
   void eraseFromParent();

   /// Unlink this instruction from its current basic block and insert the
   /// instruction such that it is the first instruction of \p Block.
   void moveFront(PILBasicBlock *Block);

   /// Unlink this instruction from its current basic block and insert it into
   /// the basic block that Later lives in, right before Later.
   void moveBefore(PILInstruction *Later);

   /// Unlink this instruction from its current basic block and insert it into
   /// the basic block that Earlier lives in, right after Earlier.
   void moveAfter(PILInstruction *Earlier);

   /// Drops all uses that belong to this instruction.
   void dropAllReferences();

   /// Replace all uses of all results of this instruction with undef.
   void replaceAllUsesOfAllResultsWithUndef();

   /// Replace all uses of all results of this instruction
   /// with the parwise-corresponding results of the given instruction.
   void replaceAllUsesPairwiseWith(PILInstruction *other);

   /// Replace all uses of all results of this instruction with the
   /// parwise-corresponding results of the passed in array.
   void
   replaceAllUsesPairwiseWith(const llvm::SmallVectorImpl<PILValue> &NewValues);

   /// Are there uses of any of the results of this instruction?
   bool hasUsesOfAnyResult() const {
      for (auto result : getResults()) {
         if (!result->use_empty())
            return true;
      }
      return false;
   }

   /// Return the array of operands for this instruction.
   ArrayRef<Operand> getAllOperands() const;

   /// Return the array of type dependent operands for this instruction.
   ///
   /// Type dependent operands are hidden operands, i.e. not part of the PIL
   /// syntax (although they are printed as "type-defs" in comments).
   /// Their purpose is to establish a def-use relationship between
   ///   -) an instruction/argument which defines a type, e.g. open_existential
   /// and
   ///   -) this instruction, which uses the type, but doesn't use the defining
   ///      instruction as value-operand, e.g. a type in the substitution list.
   ///
   /// Currently there are two kinds of type dependent operands:
   ///
   /// 1. for opened archetypes:
   ///     %o = open_existential_addr %0 : $*P to $*@opened("UUID") P
   ///     %w = witness_method $@opened("UUID") P, ... // type-defs: %o
   ///
   /// 2. for the dynamic self argument:
   ///     sil @foo : $@convention(method) (@thick X.Type) {
   ///     bb0(%0 : $@thick X.Type):
   ///       %a = apply %f<@dynamic_self X>() ... // type-defs: %0
   ///
   /// The type dependent operands are just there to let optimizations know that
   /// there is a dependency between the instruction/argument which defines the
   /// type and the instruction which uses the type.
   ArrayRef<Operand> getTypeDependentOperands() const;

   /// Return the array of mutable operands for this instruction.
   MutableArrayRef<Operand> getAllOperands();

   /// Return the array of mutable type dependent operands for this instruction.
   MutableArrayRef<Operand> getTypeDependentOperands();

   unsigned getNumOperands() const { return getAllOperands().size(); }

   unsigned getNumTypeDependentOperands() const {
      return getTypeDependentOperands().size();
   }

   bool isTypeDependentOperand(unsigned i) const {
      return i >= getNumOperands() - getNumTypeDependentOperands();
   }

   bool isTypeDependentOperand(const Operand &Op) const {
      assert(Op.getUser() == this &&
             "Operand does not belong to a PILInstruction");
      return isTypeDependentOperand(Op.getOperandNumber());
   }

private:
   /// Predicate used to filter OperandValueRange.
   struct OperandToValue;

public:
   using OperandValueRange =
   OptionalTransformRange<ArrayRef<Operand>, OperandToValue>;
   OperandValueRange
   getOperandValues(bool skipTypeDependentOperands = false) const;

   PILValue getOperand(unsigned Num) const {
      return getAllOperands()[Num].get();
   }
   void setOperand(unsigned Num, PILValue V) { getAllOperands()[Num].set(V); }
   void swapOperands(unsigned Num1, unsigned Num2) {
      getAllOperands()[Num1].swap(getAllOperands()[Num2]);
   }

private:
   /// Predicate used to filter OperandTypeRange.
   struct OperandToType;

public:
   using OperandTypeRange =
   OptionalTransformRange<ArrayRef<Operand>, OperandToType>;
   // NOTE: We always skip type dependent operands.
   OperandTypeRange getOperandTypes() const;

   /// Return the list of results produced by this instruction.
   bool hasResults() const { return !getResults().empty(); }
   PILInstructionResultArray getResults() const { return getResultsImpl(); }
   unsigned getNumResults() const { return getResults().size(); }

   PILValue getResult(unsigned index) const { return getResults()[index]; }

   /// Return the types of the results produced by this instruction.
   PILInstructionResultArray::type_range getResultTypes() const {
      return getResultsImpl().getTypes();
   }

   MemoryBehavior getMemoryBehavior() const;
   ReleasingBehavior getReleasingBehavior() const;

   /// Returns true if the instruction may release any object.
   bool mayRelease() const;

   /// Returns true if the instruction may release or may read the reference
   /// count of any object.
   bool mayReleaseOrReadRefCount() const;

   /// Can this instruction abort the program in some manner?
   bool mayTrap() const;

   /// Returns true if the given instruction is completely identical to RHS.
   bool isIdenticalTo(const PILInstruction *RHS) const {
      return isIdenticalTo(RHS,
                           [](const PILValue &Op1, const PILValue &Op2) -> bool {
                              return Op1 == Op2; });
   }

   /// Returns true if the given instruction is completely identical to RHS,
   /// using \p opEqual to compare operands.
   ///
   template <typename OpCmp>
   bool isIdenticalTo(const PILInstruction *RHS, OpCmp &&opEqual) const {
      // Quick check if both instructions have the same kind, number of operands,
      // and types. This should filter out most cases.
      if (getKind() != RHS->getKind() ||
          getNumOperands() != RHS->getNumOperands()) {
         return false;
      }

      if (!getResults().hasSameTypes(RHS->getResults()))
         return false;

      // Check operands.
      for (unsigned i = 0, e = getNumOperands(); i != e; ++i)
         if (!opEqual(getOperand(i), RHS->getOperand(i)))
            return false;

      // Check any special state of instructions that are not represented in the
      // instructions operands/type.
      return hasIdenticalState(RHS);
   }

   /// Returns true if the instruction may have side effects.
   ///
   /// Instructions that store into memory or change retain counts as well as
   /// calls and deallocation instructions are considered to have side effects
   /// that are not visible by merely examining their uses.
   bool mayHaveSideEffects() const;

   /// Returns true if the instruction may write to memory.
   bool mayWriteToMemory() const {
      MemoryBehavior B = getMemoryBehavior();
      return B == MemoryBehavior::MayWrite ||
             B == MemoryBehavior::MayReadWrite ||
             B == MemoryBehavior::MayHaveSideEffects;
   }

   /// Returns true if the instruction may read from memory.
   bool mayReadFromMemory() const {
      MemoryBehavior B = getMemoryBehavior();
      return B == MemoryBehavior::MayRead ||
             B == MemoryBehavior::MayReadWrite ||
             B == MemoryBehavior::MayHaveSideEffects;
   }

   /// Returns true if the instruction may read from or write to memory.
   bool mayReadOrWriteMemory() const {
      return getMemoryBehavior() != MemoryBehavior::None;
   }

   /// Return true if the instruction is "pure" in the sense that it may execute
   /// multiple times without affecting behavior. This implies that it can be
   /// trivially cloned at multiple use sites without preserving path
   /// equivalence.
   bool isPure() const {
      return !mayReadOrWriteMemory() && !mayTrap() && !isa<AllocationInst>(this)
             && !isa<TermInst>(this);
   }

   /// Returns true if the result of this instruction is a pointer to stack
   /// allocated memory. In this case there must be an adjacent deallocating
   /// instruction.
   bool isAllocatingStack() const;

   /// Returns true if this is the deallocation of a stack allocating instruction.
   /// The first operand must be the allocating instruction.
   bool isDeallocatingStack() const;

   /// Create a new copy of this instruction, which retains all of the operands
   /// and other information of this one.  If an insertion point is specified,
   /// then the new instruction is inserted before the specified point, otherwise
   /// the new instruction is returned without a parent.
   PILInstruction *clone(PILInstruction *InsertPt = nullptr);

   /// Invoke an Instruction's destructor. This dispatches to the appropriate
   /// leaf class destructor for the type of the instruction. This does not
   /// deallocate the instruction.
   static void destroy(PILInstruction *I);

   /// Returns true if the instruction can be duplicated without any special
   /// additional handling. It is important to know this information when
   /// you perform such optimizations like e.g. jump-threading.
   bool isTriviallyDuplicatable() const;

   /// Returns true if the instruction is only relevant for debug
   /// informations and has no other impact on program semantics.
   bool isDebugInstruction() const {
      return getKind() == PILInstructionKind::DebugValueInst ||
             getKind() == PILInstructionKind::DebugValueAddrInst;
   }

   /// Returns true if the instruction is a meta instruction which is
   /// relevant for debug information and does not get lowered to a real
   /// instruction.
   bool isMetaInstruction() const;

   /// Verify that all operands of this instruction have compatible ownership
   /// with this instruction.
   void verifyOperandOwnership() const;

   /// Get the number of created PILInstructions.
   static int getNumCreatedInstructions() {
      return NumCreatedInstructions;
   }

   /// Get the number of deleted PILInstructions.
   static int getNumDeletedInstructions() {
      return NumDeletedInstructions;
   }

   /// Pretty-print the value.
   void dump() const;
   void print(raw_ostream &OS) const;

   /// Pretty-print the value in context, preceded by its operands (if the
   /// value represents the result of an instruction) and followed by its
   /// users.
   void dumpInContext() const;
   void printInContext(raw_ostream &OS) const;

   static bool classof(const PILNode *N) {
      return N->getKind() >= PILNodeKind::First_PILInstruction &&
             N->getKind() <= PILNodeKind::Last_PILInstruction;
   }
   static bool classof(const PILInstruction *I) { return true; }

   /// This is supportable but usually suggests a logic mistake.
   static bool classof(const ValueBase *) = delete;
};

struct PILInstruction::OperandToValue {
   const PILInstruction &i;
   bool skipTypeDependentOps;

   OperandToValue(const PILInstruction &i, bool skipTypeDependentOps)
      : i(i), skipTypeDependentOps(skipTypeDependentOps) {}

   Optional<PILValue> operator()(const Operand &use) const {
      if (skipTypeDependentOps && i.isTypeDependentOperand(use))
         return None;
      return use.get();
   }
};

inline auto
PILInstruction::getOperandValues(bool skipTypeDependentOperands) const
-> OperandValueRange {
   return OperandValueRange(getAllOperands(),
                            OperandToValue(*this, skipTypeDependentOperands));
}

struct PILInstruction::OperandToType {
   const PILInstruction &i;

   OperandToType(const PILInstruction &i) : i(i) {}

   Optional<PILType> operator()(const Operand &use) const {
      if (i.isTypeDependentOperand(use))
         return None;
      return use.get()->getType();
   }
};

inline auto PILInstruction::getOperandTypes() const -> OperandTypeRange {
   return OperandTypeRange(getAllOperands(), OperandToType(*this));
}

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                     const PILInstruction &I) {
   I.print(OS);
   return OS;
}

/// Returns the combined behavior of \p B1 and \p B2.
inline PILInstruction::MemoryBehavior
combineMemoryBehavior(PILInstruction::MemoryBehavior B1,
                      PILInstruction::MemoryBehavior B2) {
   // Basically the combined behavior is the maximum of both operands.
   auto Result = std::max(B1, B2);

   // With one exception: MayRead, MayWrite -> MayReadWrite.
   if (Result == PILInstruction::MemoryBehavior::MayWrite &&
       (B1 == PILInstruction::MemoryBehavior::MayRead ||
        B2 == PILInstruction::MemoryBehavior::MayRead))
      return PILInstruction::MemoryBehavior::MayReadWrite;
   return Result;
}

/// Pretty-print the MemoryBehavior.
llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              PILInstruction::MemoryBehavior B);
/// Pretty-print the ReleasingBehavior.
llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              PILInstruction::ReleasingBehavior B);

/// An instruction which always produces a single value.
///
/// Because this instruction is both a PILInstruction and a ValueBase,
/// both of which inherit from PILNode, it introduces the need for
/// some care when working with PILNodes.  See the comment on PILNode.
class SingleValueInstruction : public PILInstruction, public ValueBase {
   static bool isSingleValueInstKind(PILNodeKind kind) {
      return kind >= PILNodeKind::First_SingleValueInstruction &&
             kind <= PILNodeKind::Last_SingleValueInstruction;
   }

   friend class PILInstruction;
   PILInstructionResultArray getResultsImpl() const {
      return PILInstructionResultArray(this);
   }
public:
   SingleValueInstruction(PILInstructionKind kind, PILDebugLocation loc,
                          PILType type)
      : PILInstruction(kind, loc),
        ValueBase(ValueKind(kind), type, IsRepresentative::No) {}

   using PILInstruction::operator new;
   using PILInstruction::dumpInContext;
   using PILInstruction::print;
   using PILInstruction::printInContext;

   // Redeclare because lldb currently doesn't know about using-declarations
   void dump() const;
   PILFunction *getFunction() { return PILInstruction::getFunction(); }
   const PILFunction *getFunction() const {
      return PILInstruction::getFunction();
   }
   PILModule &getModule() const { return PILInstruction::getModule(); }
   PILInstructionKind getKind() const { return PILInstruction::getKind(); }

   void operator delete(void *Ptr, size_t) POLAR_DELETE_OPERATOR_DELETED;

      ValueKind getValueKind() const {
      return ValueBase::getKind();
   }

   SingleValueInstruction *clone(PILInstruction *insertPt = nullptr) {
      return cast<SingleValueInstruction>(PILInstruction::clone(insertPt));
   }

   /// Override this to reflect the more efficient access pattern.
   PILInstructionResultArray getResults() const { return getResultsImpl(); }

   static bool classof(const PILNode *node) {
      return isSingleValueInstKind(node->getKind());
   }
};

// Resolve ambiguities.
inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                     const SingleValueInstruction &I) {
   I.print(OS);
   return OS;
}

inline SingleValueInstruction *PILNode::castToSingleValueInstruction() {
   assert(isa<SingleValueInstruction>(this));

   // We do reference static_casts to convince the host compiler to do
   // null-unchecked conversions.

   // If we're in the value slot, cast through ValueBase.
   if (getStorageLoc() == PILNodeStorageLocation::Value) {
      return &static_cast<SingleValueInstruction&>(
         static_cast<ValueBase&>(*this));

      // Otherwise, cast through PILInstruction.
   } else {
      return &static_cast<SingleValueInstruction&>(
         static_cast<PILInstruction&>(*this));
   }
}

#define DEFINE_ABSTRACT_SINGLE_VALUE_INST_BOILERPLATE(ID)       \
  static bool classof(const PILNode *node) {                    \
    return node->getKind() >= PILNodeKind::First_##ID &&        \
           node->getKind() <= PILNodeKind::Last_##ID;           \
  }                                                             \
  static bool classof(const SingleValueInstruction *inst) {     \
    return inst->getKind() >= PILInstructionKind::First_##ID && \
           inst->getKind() <= PILInstructionKind::Last_##ID;    \
  }

/// A single value inst that forwards a static ownership from one (or all) of
/// its operands.
///
/// The ownership kind is set on construction and afterwards must be changed
/// explicitly using setOwnershipKind().
class OwnershipForwardingSingleValueInst : public SingleValueInstruction {
   ValueOwnershipKind ownershipKind;

protected:
   OwnershipForwardingSingleValueInst(PILInstructionKind kind,
                                      PILDebugLocation debugLoc, PILType ty,
                                      ValueOwnershipKind ownershipKind)
      : SingleValueInstruction(kind, debugLoc, ty),
        ownershipKind(ownershipKind) {}

public:
   ValueOwnershipKind getOwnershipKind() const { return ownershipKind; }
   void setOwnershipKind(ValueOwnershipKind newOwnershipKind) {
      ownershipKind = newOwnershipKind;
   }
};

/// A value base result of a multiple value instruction.
///
/// *NOTE* We want this to be a pure abstract class that does not add /any/ size
/// to subclasses.
class MultipleValueInstructionResult : public ValueBase {
public:
   /// Create a new multiple value instruction result.
   ///
   /// \arg subclassDeltaOffset This is the delta offset in our parent object's
   /// layout in between the end of the MultipleValueInstruction object and the
   /// end of the specific subclass object.
   ///
   /// *NOTE* subclassDeltaOffset must be use only 5 bits. This gives us to
   /// support subclasses up to 32 bytes in size. We can scavange up to 6 more
   /// bits from ValueBase if this is not large enough.
   MultipleValueInstructionResult(ValueKind valueKind, unsigned index,
                                  PILType type,
                                  ValueOwnershipKind ownershipKind);

   /// Return the parent instruction of this result.
   MultipleValueInstruction *getParent();

   const MultipleValueInstruction *getParent() const {
      return const_cast<MultipleValueInstructionResult *>(this)->getParent();
   }

   unsigned getIndex() const {
      return Bits.MultipleValueInstructionResult.Index;
   }

   /// Get the ownership kind assigned to this result by its parent.
   ///
   /// This is stored in the bottom 3 bits of ValueBase's subclass data.
   ValueOwnershipKind getOwnershipKind() const;

   /// Set the ownership kind assigned to this result.
   ///
   /// This is stored in PILNode in the subclass data.
   void setOwnershipKind(ValueOwnershipKind Kind);

   static bool classof(const PILInstruction *) = delete;
   static bool classof(const PILUndef *) = delete;
   static bool classof(const PILArgument *) = delete;
   static bool classof(const MultipleValueInstructionResult *) { return true; }
   static bool classof(const PILNode *node) {
      // This is an abstract class without anything implementing it right now, so
      // just return false. This will be fixed in a subsequent commit.
      PILNodeKind kind = node->getKind();
      return kind >= PILNodeKind::First_MultipleValueInstructionResult &&
             kind <= PILNodeKind::Last_MultipleValueInstructionResult;
   }

protected:
   /// Set the index of this result.
   void setIndex(unsigned NewIndex);
};

template <class Result>
PILInstructionResultArray::PILInstructionResultArray(ArrayRef<Result> results)
   : PILInstructionResultArray(
   ArrayRef<MultipleValueInstructionResult>(results.data(),
                                            results.size())) {
   static_assert(sizeof(Result) == sizeof(MultipleValueInstructionResult),
                 "MultipleValueInstructionResult subclass has wrong size");
}

/// An instruction that may produce an arbitrary number of values.
class MultipleValueInstruction : public PILInstruction {
   friend class PILInstruction;
   friend class PILInstructionResultArray;

protected:
   MultipleValueInstruction(PILInstructionKind kind, PILDebugLocation loc)
      : PILInstruction(kind, loc) {}

public:
   void operator delete(void *Ptr, size_t) POLAR_DELETE_OPERATOR_DELETED;

   MultipleValueInstruction *clone(PILInstruction *insertPt = nullptr) {
      return cast<MultipleValueInstruction>(PILInstruction::clone(insertPt));
   }

   PILValue getResult(unsigned Index) const { return getResults()[Index]; }

   /// Return the index of \p Target if it is a result in the given
   /// MultipleValueInstructionResult. Otherwise, returns None.
   Optional<unsigned> getIndexOfResult(PILValue Target) const;

   unsigned getNumResults() const { return getResults().size(); }

   static bool classof(const PILNode *node) {
      PILNodeKind kind = node->getKind();
      return kind >= PILNodeKind::First_MultipleValueInstruction &&
             kind <= PILNodeKind::Last_MultipleValueInstruction;
   }
};

template <typename...> class InitialTrailingObjects;
template <typename...> class FinalTrailingObjects;

/// A utility mixin class that must be used by /all/ subclasses of
/// MultipleValueInstruction to store their results.
///
/// The exact ordering of trailing types matters quite a lot because
/// it's vital that the fields used by preceding numTrailingObjects
/// implementations be initialized before this base class is (and
/// conversely that this base class be initialized before any of the
/// succeeding numTrailingObjects implementations are called).
template <typename Derived, typename DerivedResult,
   typename Init = InitialTrailingObjects<>,
   typename Final = FinalTrailingObjects<>>
class MultipleValueInstructionTrailingObjects;

template <typename Derived, typename DerivedResult,
   typename... InitialOtherTrailingTypes,
   typename... FinalOtherTrailingTypes>
class MultipleValueInstructionTrailingObjects<Derived, DerivedResult,
   InitialTrailingObjects<InitialOtherTrailingTypes...>,
   FinalTrailingObjects<FinalOtherTrailingTypes...>>
   : protected llvm::TrailingObjects<Derived,
      InitialOtherTrailingTypes...,
      MultipleValueInstruction *,
      DerivedResult,
      FinalOtherTrailingTypes...> {
   static_assert(std::is_final_v<DerivedResult>,
                 "Expected DerivedResult to be final");
   static_assert(
      std::is_base_of<MultipleValueInstructionResult, DerivedResult>::value,
      "Expected DerivedResult to be a subclass of "
      "MultipleValueInstructionResult");
   static_assert(sizeof(MultipleValueInstructionResult) == sizeof(DerivedResult),
                 "Expected DerivedResult to be the same size as a "
                 "MultipleValueInstructionResult");

protected:
   using TrailingObjects =
   llvm::TrailingObjects<Derived,
      InitialOtherTrailingTypes...,
      MultipleValueInstruction *, DerivedResult,
      FinalOtherTrailingTypes...>;
   friend TrailingObjects;

   using TrailingObjects::totalSizeToAlloc;
   using TrailingObjects::getTrailingObjects;

   unsigned NumResults;

   size_t numTrailingObjects(typename TrailingObjects::template OverloadToken<
      MultipleValueInstruction *>) const {
      return 1;
   }

   size_t numTrailingObjects(
      typename TrailingObjects::template OverloadToken<DerivedResult>) const {
      return NumResults;
   }

   template <typename... Args>
   MultipleValueInstructionTrailingObjects(
      Derived *Parent, ArrayRef<PILType> Types,
      ArrayRef<ValueOwnershipKind> OwnershipKinds, Args &&... OtherArgs)
      : NumResults(Types.size()) {

      // If we do not have any results, then we do not need to initialize even the
      // parent pointer since we do not have any results that will attempt to get
      // our parent pointer.
      if (!NumResults)
         return;

      auto **ParentPtr = this->TrailingObjects::template
         getTrailingObjects<MultipleValueInstruction *>();
      *ParentPtr = static_cast<MultipleValueInstruction *>(Parent);

      auto *DataPtr = this->TrailingObjects::template
         getTrailingObjects<DerivedResult>();
      for (unsigned i : range(NumResults)) {
         ::new (&DataPtr[i]) DerivedResult(i, Types[i], OwnershipKinds[i],
                                           std::forward<Args>(OtherArgs)...);
         assert(DataPtr[i].getParent() == Parent &&
                "Failed to setup parent reference correctly?!");
      }
   }

   // Destruct the Derived Results.
   ~MultipleValueInstructionTrailingObjects() {
      if (!NumResults)
         return;
      auto *DataPtr = this->TrailingObjects::template
         getTrailingObjects<DerivedResult>();
      // We call the DerivedResult destructors to ensure that:
      //
      // 1. If our derived results have any stored data that need to be cleaned
      // up, we clean them up. *NOTE* Today, no results have this property.
      // 2. In ~ValueBase, we validate via an assert that a ValueBase no longer
      // has any uses when it is being destroyed. Rather than re-implement that in
      // result, we get that for free.
      for (unsigned i : range(NumResults))
         DataPtr[i].~DerivedResult();
   }

public:
   ArrayRef<DerivedResult> getAllResultsBuffer() const {
      auto *ptr = this->TrailingObjects::template
         getTrailingObjects<DerivedResult>();
      return { ptr, NumResults };
   }

   PILInstructionResultArray getAllResults() const {
      // Our results start at element 1 since we stash the pointer to our parent
      // MultipleValueInstruction in the 0 elt slot. This allows all
      // MultipleValueInstructionResult to find their parent
      // MultipleValueInstruction by using pointer arithmetic.
      return PILInstructionResultArray(getAllResultsBuffer());
   };
};

/// A subclass of PILInstruction which does not produce any values.
class NonValueInstruction : public PILInstruction {
public:
   NonValueInstruction(PILInstructionKind kind, PILDebugLocation loc)
      : PILInstruction(kind, loc) {}

   /// Doesn't produce any results.
   PILType getType() const = delete;
   PILInstructionResultArray getResults() const = delete;

   static bool classof(const ValueBase *value) = delete;
   static bool classof(const PILNode *N) {
      return N->getKind() >= PILNodeKind::First_NonValueInstruction &&
             N->getKind() <= PILNodeKind::Last_NonValueInstruction;
   }
   static bool classof(const NonValueInstruction *) { return true; }
};
#define DEFINE_ABSTRACT_NON_VALUE_INST_BOILERPLATE(ID)          \
  static bool classof(const ValueBase *value) = delete;         \
  static bool classof(const PILNode *node) {                    \
    return node->getKind() >= PILNodeKind::First_##ID &&        \
           node->getKind() <= PILNodeKind::Last_##ID;           \
  }

/// A helper class for defining some basic boilerplate.
template <PILInstructionKind Kind, typename InstBase,
   bool IsSingleResult =
   std::is_base_of<SingleValueInstruction, InstBase>::value>
class InstructionBase;

template <PILInstructionKind Kind, typename InstBase>
class InstructionBase<Kind, InstBase, /*HasResult*/ true> : public InstBase {
protected:
   template <typename... As>
   InstructionBase(As &&... args) : InstBase(Kind, std::forward<As>(args)...) {}

public:
   /// Override to statically return the kind.
   static constexpr PILInstructionKind getKind() {
      return Kind;
   }

   static bool classof(const PILNode *node) {
      return node->getKind() == PILNodeKind(Kind);
   }
   static bool classof(const SingleValueInstruction *I) { // resolve ambiguities
      return I->getKind() == Kind;
   }
};

template <PILInstructionKind Kind, typename InstBase>
class InstructionBase<Kind, InstBase, /*HasResult*/ false> : public InstBase {
protected:
   template <typename... As>
   InstructionBase(As &&... args) : InstBase(Kind, std::forward<As>(args)...) {}

public:
   static constexpr PILInstructionKind getKind() {
      return Kind;
   }

   /// Can never dynamically succeed.
   static bool classof(const ValueBase *value) = delete;

   static bool classof(const PILNode *node) {
      return node->getKind() == PILNodeKind(Kind);
   }
};

/// A template base class for instructions that take a single PILValue operand
/// and has no result or a single value result.
template<PILInstructionKind Kind, typename Base>
class UnaryInstructionBase : public InstructionBase<Kind, Base> {
   // Space for 1 operand.
   FixedOperandList<1> Operands;

public:
   template <typename... A>
   UnaryInstructionBase(PILDebugLocation loc, PILValue op, A &&... args)
      : InstructionBase<Kind, Base>(loc, std::forward<A>(args)...),
        Operands(this, op) {}

   PILValue getOperand() const { return Operands[0].get(); }
   void setOperand(PILValue V) { Operands[0].set(V); }

   Operand &getOperandRef() { return Operands[0]; }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }

   ArrayRef<Operand> getTypeDependentOperands() const {
      return {};
   }

   MutableArrayRef<Operand> getTypeDependentOperands() {
      return {};
   }
};

/// A template base class for instructions that a variable number of PILValue
/// operands, and has zero or one value results. The operands are tail allocated
/// after the instruction. Further trailing data can be allocated as well if
/// OtherTrailingTypes are provided.
template<PILInstructionKind Kind,
   typename Derived,
   typename Base,
   typename... OtherTrailingTypes>
class InstructionBaseWithTrailingOperands
   : public InstructionBase<Kind, Base>,
     protected llvm::TrailingObjects<Derived, Operand, OtherTrailingTypes...> {

protected:
   friend llvm::TrailingObjects<Derived, Operand, OtherTrailingTypes...>;

   using TrailingObjects =
   llvm::TrailingObjects<Derived, Operand, OtherTrailingTypes...>;

   using TrailingObjects::totalSizeToAlloc;

public:
   template <typename... Args>
   InstructionBaseWithTrailingOperands(ArrayRef<PILValue> Operands,
                                       Args &&...args)
      : InstructionBase<Kind, Base>(std::forward<Args>(args)...) {
      PILInstruction::Bits.IBWTO.NumOperands = Operands.size();
      TrailingOperandsList::InitOperandsList(getAllOperands().begin(), this,
                                             Operands);
   }

   template <typename... Args>
   InstructionBaseWithTrailingOperands(PILValue Operand0,
                                       ArrayRef<PILValue> Operands,
                                       Args &&...args)
      : InstructionBase<Kind, Base>(std::forward<Args>(args)...) {
      PILInstruction::Bits.IBWTO.NumOperands = Operands.size() + 1;
      TrailingOperandsList::InitOperandsList(getAllOperands().begin(), this,
                                             Operand0, Operands);
   }

   template <typename... Args>
   InstructionBaseWithTrailingOperands(PILValue Operand0,
                                       PILValue Operand1,
                                       ArrayRef<PILValue> Operands,
                                       Args &&...args)
      : InstructionBase<Kind, Base>(std::forward<Args>(args)...) {
      PILInstruction::Bits.IBWTO.NumOperands = Operands.size() + 2;
      TrailingOperandsList::InitOperandsList(getAllOperands().begin(), this,
                                             Operand0, Operand1, Operands);
   }

   // Destruct tail allocated objects.
   ~InstructionBaseWithTrailingOperands() {
      Operand *Operands = TrailingObjects::template getTrailingObjects<Operand>();
      auto end = PILInstruction::Bits.IBWTO.NumOperands;
      for (unsigned i = 0; i < end; ++i) {
         Operands[i].~Operand();
      }
   }

   size_t numTrailingObjects(typename TrailingObjects::template
   OverloadToken<Operand>) const {
      return PILInstruction::Bits.IBWTO.NumOperands;
   }

   ArrayRef<Operand> getAllOperands() const {
      return {TrailingObjects::template getTrailingObjects<Operand>(),
              PILInstruction::Bits.IBWTO.NumOperands};
   }

   MutableArrayRef<Operand> getAllOperands() {
      return {TrailingObjects::template getTrailingObjects<Operand>(),
              PILInstruction::Bits.IBWTO.NumOperands};
   }
};

/// A template base class for instructions that take a single regular PILValue
/// operand, a set of type dependent operands and has no result
/// or a single value result. The operands are tail allocated after the
/// instruction. Further trailing data can be allocated as well if
/// TRAILING_TYPES are provided.
template<PILInstructionKind Kind,
   typename Derived,
   typename Base,
   typename... OtherTrailingTypes>
class UnaryInstructionWithTypeDependentOperandsBase
   : public InstructionBaseWithTrailingOperands<Kind, Derived, Base,
      OtherTrailingTypes...> {
protected:
   friend InstructionBaseWithTrailingOperands<Kind, Derived, Operand,
      OtherTrailingTypes...>;

   using TrailingObjects =
   InstructionBaseWithTrailingOperands<Kind, Derived, Operand,
      OtherTrailingTypes...>;

public:
   template <typename... Args>
   UnaryInstructionWithTypeDependentOperandsBase(PILDebugLocation debugLoc,
                                                 PILValue operand,
                                                 ArrayRef<PILValue> typeDependentOperands,
                                                 Args &&...args)
      : InstructionBaseWithTrailingOperands<Kind, Derived, Base,
      OtherTrailingTypes...>(
      operand, typeDependentOperands,
      debugLoc,
      std::forward<Args>(args)...) {}

   unsigned getNumTypeDependentOperands() const {
      return this->getAllOperands().size() - 1;
   }

   PILValue getOperand() const {
      return this->getAllOperands()[0].get();
   }
   void setOperand(PILValue V) {
      this->getAllOperands()[0].set(V);
   }

   Operand &getOperandRef() {
      return this->getAllOperands()[0];
   }

   ArrayRef<Operand> getTypeDependentOperands() const {
      return this->getAllOperands().slice(1);
   }

   MutableArrayRef<Operand> getTypeDependentOperands() {
      return this->getAllOperands().slice(1);
   }
};

/// Holds common debug information about local variables and function
/// arguments that are needed by DebugValueInst, DebugValueAddrInst,
/// AllocStackInst, and AllocBoxInst.
struct PILDebugVariable {
   StringRef Name;
   unsigned ArgNo : 16;
   unsigned Constant : 1;

   PILDebugVariable() : ArgNo(0), Constant(false) {}
   PILDebugVariable(bool Constant, uint16_t ArgNo)
      : ArgNo(ArgNo), Constant(Constant) {}
   PILDebugVariable(StringRef Name, bool Constant, unsigned ArgNo)
      : Name(Name), ArgNo(ArgNo), Constant(Constant) {}
   bool operator==(const PILDebugVariable &V) {
      return ArgNo == V.ArgNo && Constant == V.Constant && Name == V.Name;
   }
};

/// A DebugVariable where storage for the strings has been
/// tail-allocated following the parent PILInstruction.
class TailAllocatedDebugVariable {
   using int_type = uint32_t;
   union {
      int_type RawValue;
      struct {
         /// Whether this is a debug variable at all.
         int_type HasValue : 1;
         /// True if this is a let-binding.
         int_type Constant : 1;
         /// When this is nonzero there is a tail-allocated string storing
         /// variable name present. This typically only happens for
         /// instructions that were created from parsing PIL assembler.
         int_type NameLength : 14;
         /// The source function argument position from left to right
         /// starting with 1 or 0 if this is a local variable.
         int_type ArgNo : 16;
      } Data;
   } Bits;
public:
   TailAllocatedDebugVariable(Optional<PILDebugVariable>, char *buf);
   TailAllocatedDebugVariable(int_type RawValue) { Bits.RawValue = RawValue; }
   int_type getRawValue() const { return Bits.RawValue; }

   unsigned getArgNo() const { return Bits.Data.ArgNo; }
   void setArgNo(unsigned N) { Bits.Data.ArgNo = N; }
   /// Returns the name of the source variable, if it is stored in the
   /// instruction.
   StringRef getName(const char *buf) const;
   bool isLet() const { return Bits.Data.Constant; }

   Optional<PILDebugVariable> get(VarDecl *VD, const char *buf) const {
      if (!Bits.Data.HasValue)
         return None;
      if (VD)
         return PILDebugVariable(VD->getName().empty() ? "" : VD->getName().str(),
                                 VD->isLet(), getArgNo());
      else
         return PILDebugVariable(getName(buf), isLet(), getArgNo());
   }
};
static_assert(sizeof(TailAllocatedDebugVariable) == 4,
              "PILNode inline bitfield needs updating");

//===----------------------------------------------------------------------===//
// Allocation Instructions
//===----------------------------------------------------------------------===//

/// Abstract base class for allocation instructions, like alloc_stack, alloc_box
/// and alloc_ref, etc.
class AllocationInst : public SingleValueInstruction {
protected:
   AllocationInst(PILInstructionKind Kind, PILDebugLocation DebugLoc, PILType Ty)
      : SingleValueInstruction(Kind, DebugLoc, Ty) {}

public:
   DEFINE_ABSTRACT_SINGLE_VALUE_INST_BOILERPLATE(AllocationInst)

   /// Return the underlying variable declaration associated with this
   /// allocation, or null if this allocation inst is associated with a temporary
   /// allocation.
   VarDecl *getDecl() const;
};

class DeallocStackInst;

/// AllocStackInst - This represents the allocation of an unboxed (i.e., no
/// reference count) stack memory.  The memory is provided uninitialized.
class AllocStackInst final
   : public InstructionBase<PILInstructionKind::AllocStackInst,
      AllocationInst>,
     private llvm::TrailingObjects<AllocStackInst, Operand, char> {
   friend TrailingObjects;
   friend PILBuilder;

   bool dynamicLifetime = false;

   AllocStackInst(PILDebugLocation Loc, PILType elementType,
                  ArrayRef<PILValue> TypeDependentOperands,
                  PILFunction &F,
                  Optional<PILDebugVariable> Var, bool hasDynamicLifetime);

   static AllocStackInst *create(PILDebugLocation Loc, PILType elementType,
                                 PILFunction &F,
                                 PILOpenedArchetypesState &OpenedArchetypes,
                                 Optional<PILDebugVariable> Var,
                                 bool hasDynamicLifetime);

   size_t numTrailingObjects(OverloadToken<Operand>) const {
      return PILInstruction::Bits.AllocStackInst.NumOperands;
   }

public:
   ~AllocStackInst() {
      Operand *Operands = getTrailingObjects<Operand>();
      size_t end = PILInstruction::Bits.AllocStackInst.NumOperands;
      for (unsigned i = 0; i < end; ++i) {
         Operands[i].~Operand();
      }
   }

   void setDynamicLifetime() { dynamicLifetime = true; }
   bool hasDynamicLifetime() const { return dynamicLifetime; }

   /// Return the debug variable information attached to this instruction.
   Optional<PILDebugVariable> getVarInfo() const {
      auto RawValue = PILInstruction::Bits.AllocStackInst.VarInfo;
      auto VI = TailAllocatedDebugVariable(RawValue);
      return VI.get(getDecl(), getTrailingObjects<char>());
   };
   void setArgNo(unsigned N) {
      auto RawValue = PILInstruction::Bits.AllocStackInst.VarInfo;
      auto VI = TailAllocatedDebugVariable(RawValue);
      VI.setArgNo(N);
      PILInstruction::Bits.AllocStackInst.VarInfo = VI.getRawValue();
   }

   /// getElementType - Get the type of the allocated memory (as opposed to the
   /// type of the instruction itself, which will be an address type).
   PILType getElementType() const {
      return getType().getObjectType();
   }

   ArrayRef<Operand> getAllOperands() const {
      return { getTrailingObjects<Operand>(),
               static_cast<size_t>(PILInstruction::Bits.AllocStackInst.NumOperands) };
   }

   MutableArrayRef<Operand> getAllOperands() {
      return { getTrailingObjects<Operand>(),
               static_cast<size_t>(PILInstruction::Bits.AllocStackInst.NumOperands) };
   }

   ArrayRef<Operand> getTypeDependentOperands() const {
      return getAllOperands();
   }

   MutableArrayRef<Operand> getTypeDependentOperands() {
      return getAllOperands();
   }

   /// Return a single dealloc_stack user or null.
   DeallocStackInst *getSingleDeallocStack() const;
};

/// The base class for AllocRefInst and AllocRefDynamicInst.
///
/// The first NumTailTypes operands are counts for the tail allocated
/// elements, the remaining operands are opened archetype operands.
class AllocRefInstBase : public AllocationInst {
protected:

   AllocRefInstBase(PILInstructionKind Kind,
                    PILDebugLocation DebugLoc,
                    PILType ObjectType,
                    bool objc, bool canBeOnStack,
                    ArrayRef<PILType> ElementTypes);

   PILType *getTypeStorage();
   const PILType *getTypeStorage() const {
      return const_cast<AllocRefInstBase*>(this)->getTypeStorage();
   }

   unsigned getNumTailTypes() const {
      return PILInstruction::Bits.AllocRefInstBase.NumTailTypes;
   }

public:
   bool canAllocOnStack() const {
      return PILInstruction::Bits.AllocRefInstBase.OnStack;
   }

   void setStackAllocatable(bool OnStack = true) {
      PILInstruction::Bits.AllocRefInstBase.OnStack = OnStack;
   }

   ArrayRef<PILType> getTailAllocatedTypes() const {
      return {getTypeStorage(), getNumTailTypes()};
   }

   MutableArrayRef<PILType> getTailAllocatedTypes() {
      return {getTypeStorage(), getNumTailTypes()};
   }

   ArrayRef<Operand> getTailAllocatedCounts() const {
      return getAllOperands().slice(0, getNumTailTypes());
   }

   MutableArrayRef<Operand> getTailAllocatedCounts() {
      return getAllOperands().slice(0, getNumTailTypes());
   }

   ArrayRef<Operand> getAllOperands() const;
   MutableArrayRef<Operand> getAllOperands();

   /// Whether to use Objective-C's allocation mechanism (+allocWithZone:).
   bool isObjC() const {
      return PILInstruction::Bits.AllocRefInstBase.ObjC;
   }
};

/// AllocRefInst - This represents the primitive allocation of an instance
/// of a reference type. Aside from the reference count, the instance is
/// returned uninitialized.
/// Optionally, the allocated instance contains space for one or more tail-
/// allocated arrays.
class AllocRefInst final
   : public InstructionBaseWithTrailingOperands<
      PILInstructionKind::AllocRefInst,
      AllocRefInst,
      AllocRefInstBase, PILType> {
   friend AllocRefInstBase;
   friend PILBuilder;

   AllocRefInst(PILDebugLocation DebugLoc, PILFunction &F,
                PILType ObjectType,
                bool objc, bool canBeOnStack,
                ArrayRef<PILType> ElementTypes,
                ArrayRef<PILValue> AllOperands)
      : InstructionBaseWithTrailingOperands(AllOperands, DebugLoc, ObjectType,
                                            objc, canBeOnStack, ElementTypes) {
      assert(AllOperands.size() >= ElementTypes.size());
      std::uninitialized_copy(ElementTypes.begin(), ElementTypes.end(),
                              getTrailingObjects<PILType>());
   }

   static AllocRefInst *create(PILDebugLocation DebugLoc, PILFunction &F,
                               PILType ObjectType,
                               bool objc, bool canBeOnStack,
                               ArrayRef<PILType> ElementTypes,
                               ArrayRef<PILValue> ElementCountOperands,
                               PILOpenedArchetypesState &OpenedArchetypes);

public:
   ArrayRef<Operand> getTypeDependentOperands() const {
      return getAllOperands().slice(getNumTailTypes());
   }

   MutableArrayRef<Operand> getTypeDependentOperands() {
      return getAllOperands().slice(getNumTailTypes());
   }
};

/// AllocRefDynamicInst - This represents the primitive allocation of
/// an instance of a reference type whose runtime type is provided by
/// the given metatype value. Aside from the reference count, the
/// instance is returned uninitialized.
/// Optionally, the allocated instance contains space for one or more tail-
/// allocated arrays.
class AllocRefDynamicInst final
   : public InstructionBaseWithTrailingOperands<
      PILInstructionKind::AllocRefDynamicInst,
      AllocRefDynamicInst,
      AllocRefInstBase, PILType> {
   friend AllocRefInstBase;
   friend PILBuilder;

   AllocRefDynamicInst(PILDebugLocation DebugLoc,
                       PILType ty,
                       bool objc,
                       ArrayRef<PILType> ElementTypes,
                       ArrayRef<PILValue> AllOperands)
      : InstructionBaseWithTrailingOperands(AllOperands, DebugLoc, ty, objc,
                                            false, ElementTypes) {
      assert(AllOperands.size() >= ElementTypes.size() + 1);
      std::uninitialized_copy(ElementTypes.begin(), ElementTypes.end(),
                              getTrailingObjects<PILType>());
   }

   static AllocRefDynamicInst *
   create(PILDebugLocation DebugLoc, PILFunction &F,
          PILValue metatypeOperand, PILType ty, bool objc,
          ArrayRef<PILType> ElementTypes,
          ArrayRef<PILValue> ElementCountOperands,
          PILOpenedArchetypesState &OpenedArchetypes);

public:
   PILValue getMetatypeOperand() const {
      return getAllOperands()[getNumTailTypes()].get();
   }

   ArrayRef<Operand> getTypeDependentOperands() const {
      return getAllOperands().slice(getNumTailTypes() + 1);
   }

   MutableArrayRef<Operand> getTypeDependentOperands() {
      return getAllOperands().slice(getNumTailTypes() + 1);
   }
};

/// AllocValueBufferInst - Allocate memory in a value buffer.
class AllocValueBufferInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::AllocValueBufferInst,
      AllocValueBufferInst,
      AllocationInst> {
   friend PILBuilder;

   AllocValueBufferInst(PILDebugLocation DebugLoc, PILType valueType,
                        PILValue operand,
                        ArrayRef<PILValue> TypeDependentOperands);

   static AllocValueBufferInst *
   create(PILDebugLocation DebugLoc, PILType valueType, PILValue operand,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes);

public:
   PILType getValueType() const { return getType().getObjectType(); }
};

/// This represents the allocation of a heap box for a Swift value of some type.
/// The instruction returns two values.  The first return value is the object
/// pointer with Builtin.NativeObject type.  The second return value
/// is an address pointing to the contained element. The contained
/// element is uninitialized.
class AllocBoxInst final
   : public InstructionBaseWithTrailingOperands<
      PILInstructionKind::AllocBoxInst,
      AllocBoxInst, AllocationInst, char> {
   friend PILBuilder;

   TailAllocatedDebugVariable VarInfo;

   bool dynamicLifetime = false;

   AllocBoxInst(PILDebugLocation DebugLoc, CanPILBoxType BoxType,
                ArrayRef<PILValue> TypeDependentOperands, PILFunction &F,
                Optional<PILDebugVariable> Var, bool hasDynamicLifetime);

   static AllocBoxInst *create(PILDebugLocation Loc, CanPILBoxType boxType,
                               PILFunction &F,
                               PILOpenedArchetypesState &OpenedArchetypes,
                               Optional<PILDebugVariable> Var,
                               bool hasDynamicLifetime);

public:
   CanPILBoxType getBoxType() const {
      return getType().castTo<PILBoxType>();
   }

   void setDynamicLifetime() { dynamicLifetime = true; }
   bool hasDynamicLifetime() const { return dynamicLifetime; }

   // Return the type of the memory stored in the alloc_box.
   PILType getAddressType() const;

   /// Return the debug variable information attached to this instruction.
   Optional<PILDebugVariable> getVarInfo() const {
      return VarInfo.get(getDecl(), getTrailingObjects<char>());
   };

   ArrayRef<Operand> getTypeDependentOperands() const {
      return getAllOperands();
   }

   MutableArrayRef<Operand> getTypeDependentOperands() {
      return getAllOperands();
   }
};

/// This represents the allocation of a heap box for an existential container.
/// The instruction returns two values.  The first return value is the owner
/// pointer, which has the existential type.  The second return value
/// is an address pointing to the contained element. The contained
/// value is uninitialized.
class AllocExistentialBoxInst final
   : public InstructionBaseWithTrailingOperands<
      PILInstructionKind::AllocExistentialBoxInst,
      AllocExistentialBoxInst, AllocationInst> {
   friend PILBuilder;
   CanType ConcreteType;
   ArrayRef<InterfaceConformanceRef> Conformances;

   AllocExistentialBoxInst(PILDebugLocation DebugLoc, PILType ExistentialType,
                           CanType ConcreteType,
                           ArrayRef<InterfaceConformanceRef> Conformances,
                           ArrayRef<PILValue> TypeDependentOperands,
                           PILFunction *Parent)
      : InstructionBaseWithTrailingOperands(TypeDependentOperands, DebugLoc,
                                            ExistentialType.getObjectType()),
        ConcreteType(ConcreteType), Conformances(Conformances) {}

   static AllocExistentialBoxInst *
   create(PILDebugLocation DebugLoc, PILType ExistentialType,
          CanType ConcreteType, ArrayRef<InterfaceConformanceRef> Conformances,
          PILFunction *Parent, PILOpenedArchetypesState &OpenedArchetypes);

public:
   CanType getFormalConcreteType() const { return ConcreteType; }

   PILType getExistentialType() const { return getType(); }

   ArrayRef<InterfaceConformanceRef> getConformances() const {
      return Conformances;
   }

   ArrayRef<Operand> getTypeDependentOperands() const {
      return getAllOperands();
   }

   MutableArrayRef<Operand> getTypeDependentOperands() {
      return getAllOperands();
   }
};

/// GenericSpecializationInformation - provides information about a generic
/// specialization. This meta-information is created for each generic
/// specialization, which allows for tracking of dependencies between
/// specialized generic functions and can be used to detect specialization loops
/// during generic specialization.
class GenericSpecializationInformation {
   /// The caller function that triggered this specialization.
   PILFunction *Caller;
   /// The original function that was specialized.
   PILFunction *Parent;
   /// Substitutions used to produce this specialization.
   SubstitutionMap Subs;

   GenericSpecializationInformation(PILFunction *Caller, PILFunction *Parent,
                                    SubstitutionMap Subs);

public:
   static const GenericSpecializationInformation *create(PILFunction *Caller,
                                                         PILFunction *Parent,
                                                         SubstitutionMap Subs);
   static const GenericSpecializationInformation *create(PILInstruction *Inst,
                                                         PILBuilder &B);
   const PILFunction *getCaller() const { return Caller; }
   const PILFunction *getParent() const { return Parent; }
   SubstitutionMap getSubstitutions() const { return Subs; }
};

class PartialApplyInst;

// There's no good reason for the OverloadToken type to be internal
// or protected, and it makes it very difficult to write our CRTP classes
// if it is, so pull it out.  TODO: just fix LLVM.
struct TerribleOverloadTokenHack :
   llvm::trailing_objects_internal::TrailingObjectsBase {
   template <class T>
   using Hack = OverloadToken<T>;
};
template <class T>
using OverloadToken = TerribleOverloadTokenHack::Hack<T>;

/// ApplyInstBase - An abstract class for different kinds of function
/// application.
template <class Impl, class Base,
   bool IsFullApply = !std::is_same<Impl, PartialApplyInst>::value>
class ApplyInstBase;

// The partial specialization for non-full applies.  Note that the
// partial specialization for full applies inherits from this.
template <class Impl, class Base>
class ApplyInstBase<Impl, Base, false> : public Base {
   enum { Callee, NumStaticOperands };

   /// The type of the callee with our substitutions applied.
   PILType SubstCalleeType;

   /// Information about specialization and inlining of this apply.
   /// This is only != nullptr if the apply was inlined. And in this case it
   /// points to the specialization info of the inlined function.
   const GenericSpecializationInformation *SpecializationInfo;

   /// Used for apply_inst instructions: true if the called function has an
   /// error result but is not actually throwing.
   unsigned NonThrowing: 1;

   /// The number of call arguments as required by the callee.
   unsigned NumCallArguments : 31;

   /// The total number of type-dependent operands.
   unsigned NumTypeDependentOperands;

   /// The substitutions being applied to the callee.
   SubstitutionMap Substitutions;

   Impl &asImpl() { return static_cast<Impl &>(*this); }
   const Impl &asImpl() const { return static_cast<const Impl &>(*this); }

protected:
   template <class... As>
   ApplyInstBase(PILInstructionKind kind, PILDebugLocation DebugLoc, PILValue callee,
                 PILType substCalleeType, SubstitutionMap subs,
                 ArrayRef<PILValue> args,
                 ArrayRef<PILValue> typeDependentOperands,
                 const GenericSpecializationInformation *specializationInfo,
                 As... baseArgs)
      : Base(kind, DebugLoc, baseArgs...), SubstCalleeType(substCalleeType),
        SpecializationInfo(specializationInfo),
        NonThrowing(false), NumCallArguments(args.size()),
        NumTypeDependentOperands(typeDependentOperands.size()),
        Substitutions(subs) {

      // Initialize the operands.
      auto allOperands = getAllOperands();
      new (&allOperands[Callee]) Operand(this, callee);
      for (size_t i : indices(args)) {
         new (&allOperands[NumStaticOperands + i]) Operand(this, args[i]);
      }
      for (size_t i : indices(typeDependentOperands)) {
         new (&allOperands[NumStaticOperands + args.size() + i])
            Operand(this, typeDependentOperands[i]);
      }
   }

   ~ApplyInstBase() {
      for (auto &operand : getAllOperands())
         operand.~Operand();
   }

   template <class, class...>
   friend class llvm::TrailingObjects;

   unsigned numTrailingObjects(OverloadToken<Operand>) const {
      return getNumAllOperands();
   }

   static size_t getNumAllOperands(ArrayRef<PILValue> args,
                                   ArrayRef<PILValue> typeDependentOperands) {
      return NumStaticOperands + args.size() + typeDependentOperands.size();
   }

   void setNonThrowing(bool isNonThrowing) { NonThrowing = isNonThrowing; }

   bool isNonThrowingApply() const { return NonThrowing; }

public:
   /// The operand number of the first argument.
   static unsigned getArgumentOperandNumber() { return NumStaticOperands; }

   const Operand *getCalleeOperand() const { return &getAllOperands()[Callee]; }
   PILValue getCallee() const { return getCalleeOperand()->get(); }

   /// Gets the origin of the callee by looking through function type conversions
   /// until we find a function_ref, partial_apply, or unrecognized value.
   ///
   /// This is defined out of line to work around incomplete definition
   /// issues. It is at the bottom of the file.
   PILValue getCalleeOrigin() const;

   /// Gets the referenced function by looking through partial apply,
   /// convert_function, and thin to thick function until we find a function_ref.
   ///
   /// This is defined out of line to work around incomplete definition
   /// issues. It is at the bottom of the file.
   PILFunction *getCalleeFunction() const;

   bool isCalleeDynamicallyReplaceable() const;

   /// Gets the referenced function if the callee is a function_ref instruction.
   /// Returns null if the callee is dynamic or a (prev_)dynamic_function_ref
   /// instruction.
   PILFunction *getReferencedFunctionOrNull() const {
      if (auto *FRI = dyn_cast<FunctionRefBaseInst>(getCallee()))
         return FRI->getReferencedFunctionOrNull();
      return nullptr;
   }

   /// Return the referenced function if the callee is a function_ref like
   /// instruction.
   ///
   /// WARNING: This not necessarily the function that will be called at runtime.
   /// If the callee is a (prev_)dynamic_function_ref the actual function called
   /// might be different because it could be dynamically replaced at runtime.
   ///
   /// If the client of this API wants to look at the content of the returned PIL
   /// function it should call getReferencedFunctionOrNull() instead.
   PILFunction *getInitiallyReferencedFunction() const {
      if (auto *FRI = dyn_cast<FunctionRefBaseInst>(getCallee()))
         return FRI->getInitiallyReferencedFunction();
      return nullptr;
   }

   /// Get the type of the callee without the applied substitutions.
   CanPILFunctionType getOrigCalleeType() const {
      return getCallee()->getType().template castTo<PILFunctionType>();
   }
   PILFunctionConventions getOrigCalleeConv() const {
      return PILFunctionConventions(getOrigCalleeType(), this->getModule());
   }

   /// Get the type of the callee with the applied substitutions.
   CanPILFunctionType getSubstCalleeType() const {
      return SubstCalleeType.castTo<PILFunctionType>();
   }
   PILType getSubstCalleePILType() const {
      return SubstCalleeType;
   }
   PILFunctionConventions getSubstCalleeConv() const {
      return PILFunctionConventions(getSubstCalleeType(), this->getModule());
   }

   bool isCalleeNoReturn() const {
      return getSubstCalleePILType().isNoReturnFunction(this->getModule());
   }

   bool isCalleeThin() const {
      auto Rep = getSubstCalleeType()->getRepresentation();
      return Rep == FunctionType::Representation::Thin;
   }

   /// Returns true if the callee function is annotated with
   /// @_semantics("programtermination_point")
   bool isCalleeKnownProgramTerminationPoint() const {
      auto calleeFn = getCalleeFunction();
      if (!calleeFn) return false;
      return calleeFn->hasSemanticsAttr(SEMANTICS_PROGRAMTERMINATION_POINT);
   }

   /// True if this application has generic substitutions.
   bool hasSubstitutions() const {
      return Substitutions.hasAnySubstitutableParams();
   }

   /// The substitutions used to bind the generic arguments of this function.
   SubstitutionMap getSubstitutionMap() const { return Substitutions; }

   /// Return the total number of operands of this instruction.
   unsigned getNumAllOperands() const {
      return NumStaticOperands + NumCallArguments + NumTypeDependentOperands;
   }

   /// Return all the operands of this instruction, which are (in order):
   ///   - the callee
   ///   - the formal arguments
   ///   - the type-dependency arguments
   MutableArrayRef<Operand> getAllOperands() {
      return { asImpl().template getTrailingObjects<Operand>(),
               getNumAllOperands() };
   }

   ArrayRef<Operand> getAllOperands() const {
      return { asImpl().template getTrailingObjects<Operand>(),
               getNumAllOperands() };
   }

   /// Check whether the given operand index is a call-argument index
   /// and, if so, return that index.
   Optional<unsigned> getArgumentIndexForOperandIndex(unsigned index) {
      assert(index < getNumAllOperands());
      if (index < NumStaticOperands) return None;
      index -= NumStaticOperands;
      if (index >= NumCallArguments) return None;
      return index;
   }

   /// The arguments passed to this instruction.
   MutableArrayRef<Operand> getArgumentOperands() {
      return getAllOperands().slice(NumStaticOperands, NumCallArguments);
   }

   ArrayRef<Operand> getArgumentOperands() const {
      return getAllOperands().slice(NumStaticOperands, NumCallArguments);
   }

   /// The arguments passed to this instruction.
   OperandValueArrayRef getArguments() const {
      return OperandValueArrayRef(getArgumentOperands());
   }

   /// Returns the number of arguments being passed by this apply.
   /// If this is a partial_apply, it can be less than the number of
   /// parameters.
   unsigned getNumArguments() const { return NumCallArguments; }

   Operand &getArgumentRef(unsigned i) {
      return getArgumentOperands()[i];
   }

   /// Return the ith argument passed to this instruction.
   PILValue getArgument(unsigned i) const { return getArguments()[i]; }

   /// Set the ith argument of this instruction.
   void setArgument(unsigned i, PILValue V) {
      return getArgumentOperands()[i].set(V);
   }

   ArrayRef<Operand> getTypeDependentOperands() const {
      return getAllOperands().slice(NumStaticOperands + NumCallArguments);
   }

   MutableArrayRef<Operand> getTypeDependentOperands() {
      return getAllOperands().slice(NumStaticOperands + NumCallArguments);
   }

   const GenericSpecializationInformation *getSpecializationInfo() const {
      return SpecializationInfo;
   }
};

/// Given the callee operand of an apply or try_apply instruction,
/// does it have the given semantics?
bool doesApplyCalleeHaveSemantics(PILValue callee, StringRef semantics);

/// The partial specialization of ApplyInstBase for full applications.
/// Adds some methods relating to 'self' and to result types that don't
/// make sense for partial applications.
template <class Impl, class Base>
class ApplyInstBase<Impl, Base, true>
   : public ApplyInstBase<Impl, Base, false> {
   using super = ApplyInstBase<Impl, Base, false>;
protected:
   template <class... As>
   ApplyInstBase(As &&...args)
      : ApplyInstBase<Impl, Base, false>(std::forward<As>(args)...) {}

public:
   using super::getCallee;
   using super::getSubstCalleeType;
   using super::getSubstCalleeConv;
   using super::hasSubstitutions;
   using super::getNumArguments;
   using super::getArgument;
   using super::getArguments;
   using super::getArgumentOperands;

   /// The collection of following routines wrap the representation difference in
   /// between the self substitution being first, but the self parameter of a
   /// function being last.
   ///
   /// The hope is that this will prevent any future bugs from coming up related
   /// to this.
   ///
   /// Self is always the last parameter, but self substitutions are always
   /// first. The reason to add this method is to wrap that dichotomy to reduce
   /// errors.
   ///
   /// FIXME: Could this be standardized? It has and will lead to bugs. IMHO.
   PILValue getSelfArgument() const {
      assert(hasSelfArgument() && "Must have a self argument");
      assert(getNumArguments() && "Should only be called when Callee has "
                                  "arguments.");
      return getArgument(getNumArguments()-1);
   }

   Operand &getSelfArgumentOperand() {
      assert(hasSelfArgument() && "Must have a self argument");
      assert(getNumArguments() && "Should only be called when Callee has "
                                  "arguments.");
      return getArgumentOperands()[getNumArguments()-1];
   }

   void setSelfArgument(PILValue V) {
      assert(hasSelfArgument() && "Must have a self argument");
      assert(getNumArguments() && "Should only be called when Callee has "
                                  "arguments.");
      getArgumentOperands()[getNumArguments() - 1].set(V);
   }

   OperandValueArrayRef getArgumentsWithoutSelf() const {
      assert(hasSelfArgument() && "Must have a self argument");
      assert(getNumArguments() && "Should only be called when Callee has "
                                  "at least a self parameter.");
      ArrayRef<Operand> ops = this->getArgumentOperands();
      ArrayRef<Operand> opsWithoutSelf = ArrayRef<Operand>(&ops[0],
                                                           ops.size()-1);
      return OperandValueArrayRef(opsWithoutSelf);
   }

   Optional<PILResultInfo> getSingleResult() const {
      auto SubstCallee = getSubstCalleeType();
      if (SubstCallee->getNumAllResults() != 1)
         return None;
      return SubstCallee->getSingleResult();
   }

   bool hasIndirectResults() const {
      return getSubstCalleeConv().hasIndirectPILResults();
   }
   unsigned getNumIndirectResults() const {
      return getSubstCalleeConv().getNumIndirectPILResults();
   }

   bool hasSelfArgument() const {
      return getSubstCalleeType()->hasSelfParam();
   }

   bool hasGuaranteedSelfArgument() const {
      auto C = getSubstCalleeType()->getSelfParameter().getConvention();
      return C == ParameterConvention::Direct_Guaranteed;
   }

   OperandValueArrayRef getIndirectPILResults() const {
      return getArguments().slice(0, getNumIndirectResults());
   }

   OperandValueArrayRef getArgumentsWithoutIndirectResults() const {
      return getArguments().slice(getNumIndirectResults());
   }

   bool hasSemantics(StringRef semanticsString) const {
      return doesApplyCalleeHaveSemantics(getCallee(), semanticsString);
   }
};

/// ApplyInst - Represents the full application of a function value.
class ApplyInst final
   : public InstructionBase<PILInstructionKind::ApplyInst,
      ApplyInstBase<ApplyInst, SingleValueInstruction>>,
     public llvm::TrailingObjects<ApplyInst, Operand> {
   friend PILBuilder;

   ApplyInst(PILDebugLocation DebugLoc, PILValue Callee,
             PILType SubstCalleeType, PILType ReturnType,
             SubstitutionMap Substitutions,
             ArrayRef<PILValue> Args,
             ArrayRef<PILValue> TypeDependentOperands,
             bool isNonThrowing,
             const GenericSpecializationInformation *SpecializationInfo);

   static ApplyInst *
   create(PILDebugLocation DebugLoc, PILValue Callee,
          SubstitutionMap Substitutions, ArrayRef<PILValue> Args,
          bool isNonThrowing, Optional<PILModuleConventions> ModuleConventions,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes,
          const GenericSpecializationInformation *SpecializationInfo);

public:
   /// Returns true if the called function has an error result but is not actually
   /// throwing an error.
   bool isNonThrowing() const {
      return isNonThrowingApply();
   }
};

/// PartialApplyInst - Represents the creation of a closure object by partial
/// application of a function value.
class PartialApplyInst final
   : public InstructionBase<PILInstructionKind::PartialApplyInst,
      ApplyInstBase<PartialApplyInst,
         SingleValueInstruction>>,
     public llvm::TrailingObjects<PartialApplyInst, Operand> {
   friend PILBuilder;

public:
   enum OnStackKind {
      NotOnStack, OnStack
   };

private:
   PartialApplyInst(PILDebugLocation DebugLoc, PILValue Callee,
                    PILType SubstCalleeType,
                    SubstitutionMap Substitutions,
                    ArrayRef<PILValue> Args,
                    ArrayRef<PILValue> TypeDependentOperands,
                    PILType ClosureType,
                    const GenericSpecializationInformation *SpecializationInfo);

   static PartialApplyInst *
   create(PILDebugLocation DebugLoc, PILValue Callee, ArrayRef<PILValue> Args,
          SubstitutionMap Substitutions, ParameterConvention CalleeConvention,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes,
          const GenericSpecializationInformation *SpecializationInfo,
          OnStackKind onStack);

public:
   /// Return the result function type of this partial apply.
   CanPILFunctionType getFunctionType() const {
      return getType().castTo<PILFunctionType>();
   }
   bool hasCalleeGuaranteedContext() const {
      return getType().castTo<PILFunctionType>()->isCalleeGuaranteed();
   }

   OnStackKind isOnStack() const {
      return getFunctionType()->isNoEscape() ? OnStack : NotOnStack;
   }
};

class BeginApplyInst;
class BeginApplyResult final : public MultipleValueInstructionResult {
public:
   BeginApplyResult(unsigned index, PILType type,
                    ValueOwnershipKind ownershipKind)
      : MultipleValueInstructionResult(ValueKind::BeginApplyResult,
                                       index, type, ownershipKind) {}

   BeginApplyInst *getParent(); // inline below
   const BeginApplyInst *getParent() const {
      return const_cast<BeginApplyResult *>(this)->getParent();
   }

   /// Is this result the token result of the begin_apply, which abstracts
   /// over the implicit coroutine state?
   bool isTokenResult() const; // inline below

   static bool classof(const PILNode *N) {
      return N->getKind() == PILNodeKind::BeginApplyResult;
   }
};

class EndApplyInst;
class AbortApplyInst;

/// BeginApplyInst - Represents the beginning of the full application of
/// a yield_once coroutine (up until the coroutine yields a value back).
class BeginApplyInst final
   : public InstructionBase<PILInstructionKind::BeginApplyInst,
      ApplyInstBase<BeginApplyInst,
         MultipleValueInstruction>>,
     public MultipleValueInstructionTrailingObjects<
        BeginApplyInst, BeginApplyResult,
        // These must be earlier trailing objects because their
        // count fields are initialized by an earlier base class.
        InitialTrailingObjects<Operand>> {
   friend PILBuilder;

   template <class, class...>
   friend class llvm::TrailingObjects;
   using InstructionBase::numTrailingObjects;
   using MultipleValueInstructionTrailingObjects::numTrailingObjects;

   friend class ApplyInstBase<BeginApplyInst, MultipleValueInstruction, false>;
   using MultipleValueInstructionTrailingObjects::getTrailingObjects;

   BeginApplyInst(PILDebugLocation debugLoc, PILValue callee,
                  PILType substCalleeType,
                  ArrayRef<PILType> allResultTypes,
                  ArrayRef<ValueOwnershipKind> allResultOwnerships,
                  SubstitutionMap substitutions,
                  ArrayRef<PILValue> args,
                  ArrayRef<PILValue> typeDependentOperands,
                  bool isNonThrowing,
                  const GenericSpecializationInformation *specializationInfo);

   static BeginApplyInst *
   create(PILDebugLocation debugLoc, PILValue Callee,
          SubstitutionMap substitutions, ArrayRef<PILValue> args,
          bool isNonThrowing, Optional<PILModuleConventions> moduleConventions,
          PILFunction &F, PILOpenedArchetypesState &openedArchetypes,
          const GenericSpecializationInformation *specializationInfo);

public:
   using MultipleValueInstructionTrailingObjects::totalSizeToAlloc;

   PILValue getTokenResult() const {
      return &getAllResultsBuffer().back();
   }

   PILInstructionResultArray getYieldedValues() const {
      return getAllResultsBuffer().drop_back();
   }

   /// Returns true if the called coroutine has an error result but is not
   /// actually throwing an error.
   bool isNonThrowing() const {
      return isNonThrowingApply();
   }

   void getCoroutineEndPoints(
      SmallVectorImpl<EndApplyInst *> &endApplyInsts,
      SmallVectorImpl<AbortApplyInst *> &abortApplyInsts) const;

   void getCoroutineEndPoints(SmallVectorImpl<Operand *> &endApplyInsts,
                              SmallVectorImpl<Operand *> &abortApplyInsts) const;
};

inline BeginApplyInst *BeginApplyResult::getParent() {
   auto *Parent = MultipleValueInstructionResult::getParent();
   return cast<BeginApplyInst>(Parent);
}
inline bool BeginApplyResult::isTokenResult() const {
   return getIndex() == getParent()->getNumResults() - 1;
}

/// AbortApplyInst - Unwind the full application of a yield_once coroutine.
class AbortApplyInst
   : public UnaryInstructionBase<PILInstructionKind::AbortApplyInst,
      NonValueInstruction> {
   friend PILBuilder;

   AbortApplyInst(PILDebugLocation debugLoc, PILValue beginApplyToken)
      : UnaryInstructionBase(debugLoc, beginApplyToken) {
      assert(isa<BeginApplyResult>(beginApplyToken) &&
             cast<BeginApplyResult>(beginApplyToken)->isTokenResult());
   }

public:
   BeginApplyInst *getBeginApply() const {
      return cast<BeginApplyResult>(getOperand())->getParent();
   }
};

/// EndApplyInst - Resume the full application of a yield_once coroutine
/// normally.
class EndApplyInst
   : public UnaryInstructionBase<PILInstructionKind::EndApplyInst,
      NonValueInstruction> {
   friend PILBuilder;

   EndApplyInst(PILDebugLocation debugLoc, PILValue beginApplyToken)
      : UnaryInstructionBase(debugLoc, beginApplyToken) {
      assert(isa<BeginApplyResult>(beginApplyToken) &&
             cast<BeginApplyResult>(beginApplyToken)->isTokenResult());
   }

public:
   BeginApplyInst *getBeginApply() const {
      return cast<BeginApplyResult>(getOperand())->getParent();
   }
};

//===----------------------------------------------------------------------===//
// Literal instructions.
//===----------------------------------------------------------------------===//

/// Abstract base class for literal instructions.
class LiteralInst : public SingleValueInstruction {
protected:
   LiteralInst(PILInstructionKind Kind, PILDebugLocation DebugLoc, PILType Ty)
      : SingleValueInstruction(Kind, DebugLoc, Ty) {}

public:

   DEFINE_ABSTRACT_SINGLE_VALUE_INST_BOILERPLATE(LiteralInst)
};

class FunctionRefBaseInst : public LiteralInst {
   PILFunction *f;

protected:
   FunctionRefBaseInst(PILInstructionKind Kind, PILDebugLocation DebugLoc,
                       PILFunction *F, TypeExpansionContext context);

public:
   ~FunctionRefBaseInst();

   /// Return the referenced function if this is a function_ref instruction and
   /// therefore a client can rely on the dynamically called function being equal
   /// to the returned value and null otherwise.
   PILFunction *getReferencedFunctionOrNull() const {
      auto kind = getKind();
      if (kind == PILInstructionKind::FunctionRefInst)
         return f;
      assert(kind == PILInstructionKind::DynamicFunctionRefInst ||
             kind == PILInstructionKind::PreviousDynamicFunctionRefInst);
      return nullptr;
   }

   /// Return the initially referenced function.
   ///
   /// WARNING: This not necessarily the function that will be called at runtime.
   /// If the callee is a (prev_)dynamic_function_ref the actual function called
   /// might be different because it could be dynamically replaced at runtime.
   ///
   /// If the client of this API wants to look at the content of the returned PIL
   /// function it should call getReferencedFunctionOrNull() instead.
   PILFunction *getInitiallyReferencedFunction() const { return f; }

   void dropReferencedFunction();

   CanPILFunctionType getFunctionType() const {
      return getType().castTo<PILFunctionType>();
   }
   PILFunctionConventions getConventions() const {
      return PILFunctionConventions(getFunctionType(), getModule());
   }

   ArrayRef<Operand> getAllOperands() const { return {}; }
   MutableArrayRef<Operand> getAllOperands() { return {}; }

   static bool classof(const PILNode *node) {
      return (node->getKind() == PILNodeKind::FunctionRefInst ||
              node->getKind() == PILNodeKind::DynamicFunctionRefInst ||
              node->getKind() == PILNodeKind::PreviousDynamicFunctionRefInst);
   }
   static bool classof(const SingleValueInstruction *node) {
      return (node->getKind() == PILInstructionKind::FunctionRefInst ||
              node->getKind() == PILInstructionKind::DynamicFunctionRefInst ||
              node->getKind() == PILInstructionKind::PreviousDynamicFunctionRefInst);
   }
};

/// FunctionRefInst - Represents a reference to a PIL function.
class FunctionRefInst : public FunctionRefBaseInst {
   friend PILBuilder;

   /// Construct a FunctionRefInst.
   ///
   /// \param DebugLoc  The location of the reference.
   /// \param F         The function being referenced.
   /// \param context   The type expansion context of the function reference.
   FunctionRefInst(PILDebugLocation DebugLoc, PILFunction *F,
                   TypeExpansionContext context);

public:
   static bool classof(const PILNode *node) {
      return node->getKind() == PILNodeKind::FunctionRefInst;
   }
   static bool classof(const SingleValueInstruction *node) {
      return node->getKind() == PILInstructionKind::FunctionRefInst;
   }
};

class DynamicFunctionRefInst : public FunctionRefBaseInst {
   friend PILBuilder;

   /// Construct a DynamicFunctionRefInst.
   ///
   /// \param DebugLoc  The location of the reference.
   /// \param F         The function being referenced.
   /// \param context   The type expansion context of the function reference.
   DynamicFunctionRefInst(PILDebugLocation DebugLoc, PILFunction *F,
                          TypeExpansionContext context);

public:
   static bool classof(const PILNode *node) {
      return node->getKind() == PILNodeKind::DynamicFunctionRefInst;
   }
   static bool classof(const SingleValueInstruction *node) {
      return node->getKind() == PILInstructionKind::DynamicFunctionRefInst;
   }
};

class PreviousDynamicFunctionRefInst : public FunctionRefBaseInst {
   friend PILBuilder;

   /// Construct a PreviousDynamicFunctionRefInst.
   ///
   /// \param DebugLoc  The location of the reference.
   /// \param F         The function being referenced.
   /// \param context   The type expansion context of the function reference.
   PreviousDynamicFunctionRefInst(PILDebugLocation DebugLoc, PILFunction *F,
                                  TypeExpansionContext context);

public:
   static bool classof(const PILNode *node) {
      return node->getKind() == PILNodeKind::PreviousDynamicFunctionRefInst;
   }
   static bool classof(const SingleValueInstruction *node) {
      return node->getKind() ==
             PILInstructionKind::PreviousDynamicFunctionRefInst;
   }
};

/// Component of a KeyPathInst.
class KeyPathPatternComponent {
public:
   /// Computed property components require an identifier so they can be stably
   /// identified at runtime. This has to correspond to the ABI of the property--
   /// whether a reabstracted stored property, a property dispatched through a
   /// vtable or witness table, or a computed property.
   class ComputedPropertyId {
      friend KeyPathPatternComponent;
   public:
      enum KindType {
         Property, Function, DeclRef,
      };
   private:

      union ValueType {
         AbstractStorageDecl *Property;
         PILFunction *Function;
         PILDeclRef DeclRef;

         ValueType() : Property(nullptr) {}
         ValueType(AbstractStorageDecl *p) : Property(p) {}
         ValueType(PILFunction *f) : Function(f) {}
         ValueType(PILDeclRef d) : DeclRef(d) {}
      } Value;

      KindType Kind;

      explicit ComputedPropertyId(ValueType Value, KindType Kind)
         : Value(Value), Kind(Kind)
      {}

   public:
      ComputedPropertyId() : Value(), Kind(Property) {}

      /*implicit*/ ComputedPropertyId(VarDecl *property)
         : Value{property}, Kind{Property}
      {
      }

      /*implicit*/ ComputedPropertyId(PILFunction *function)
         : Value{function}, Kind{Function}
      {}

      /*implicit*/ ComputedPropertyId(PILDeclRef declRef)
         : Value{declRef}, Kind{DeclRef}
      {}

      KindType getKind() const { return Kind; }

      VarDecl *getProperty() const {
         assert(getKind() == Property);
         return cast<VarDecl>(Value.Property);
      }

      PILFunction *getFunction() const {
         assert(getKind() == Function);
         return Value.Function;
      }

      PILDeclRef getDeclRef() const {
         assert(getKind() == DeclRef);
         return Value.DeclRef;
      }
   };

   enum class Kind: unsigned {
      StoredProperty,
      GettableProperty,
      SettableProperty,
      TupleElement,
      OptionalChain,
      OptionalForce,
      OptionalWrap,
   };

   // Description of a captured index value and its Hashable conformance for a
   // subscript keypath.
   struct Index {
      unsigned Operand;
      CanType FormalType;
      PILType LoweredType;
      InterfaceConformanceRef Hashable;
   };

private:
   enum PackedKind: unsigned {
      PackedStored,
      PackedComputed,
      Unpacked,
   };

   static const unsigned KindPackingBits = 2;

   static unsigned getPackedKind(Kind k) {
      switch (k) {
         case Kind::StoredProperty:
         case Kind::TupleElement:
            return PackedStored;
         case Kind::GettableProperty:
         case Kind::SettableProperty:
            return PackedComputed;
         case Kind::OptionalChain:
         case Kind::OptionalForce:
         case Kind::OptionalWrap:
            return Unpacked;
      }
   }

   // Value is the VarDecl* for StoredProperty, the PILFunction* of the
   // Getter for computed properties, or the Kind for other kinds
   llvm::PointerIntPair<void *, KindPackingBits, unsigned> ValueAndKind;
   llvm::PointerIntPair<PILFunction *, 2,
      ComputedPropertyId::KindType> SetterAndIdKind;

   // If this component refers to a tuple element then TupleIndex is the
   // 1-based index of the element in the tuple, in order to allow the
   // discrimination of the TupleElement Kind from the StoredProperty Kind
   union {
      unsigned TupleIndex = 0;
      ComputedPropertyId::ValueType IdValue;
   };

   ArrayRef<Index> Indices;
   struct {
      PILFunction *Equal;
      PILFunction *Hash;
   } IndexEquality;
   CanType ComponentType;
   AbstractStorageDecl *ExternalStorage;
   SubstitutionMap ExternalSubstitutions;

   /// Constructor for stored components
   KeyPathPatternComponent(VarDecl *storedProp,
                           CanType ComponentType)
      : ValueAndKind(storedProp, PackedStored),
        ComponentType(ComponentType) {}

   /// Constructor for computed components
   KeyPathPatternComponent(ComputedPropertyId id,
                           PILFunction *getter,
                           PILFunction *setter,
                           ArrayRef<Index> indices,
                           PILFunction *indicesEqual,
                           PILFunction *indicesHash,
                           AbstractStorageDecl *externalStorage,
                           SubstitutionMap externalSubstitutions,
                           CanType ComponentType)
      : ValueAndKind(getter, PackedComputed),
        SetterAndIdKind{setter, id.Kind},
        IdValue{id.Value},
        Indices(indices),
        IndexEquality{indicesEqual, indicesHash},
        ComponentType(ComponentType),
        ExternalStorage(externalStorage),
        ExternalSubstitutions(externalSubstitutions)
   {
   }

   /// Constructor for optional components.
   KeyPathPatternComponent(Kind kind, CanType componentType)
      : ValueAndKind((void*)((uintptr_t)kind << KindPackingBits), Unpacked),
        ComponentType(componentType) {
      assert((unsigned)kind >= (unsigned)Kind::OptionalChain
             && "not an optional component");
   }

   /// Constructor for tuple element.
   KeyPathPatternComponent(unsigned tupleIndex, CanType componentType)
      : ValueAndKind((void*)((uintptr_t)Kind::TupleElement << KindPackingBits), PackedStored),
        TupleIndex(tupleIndex + 1),
        ComponentType(componentType)
   {
   }

public:
   KeyPathPatternComponent() : ValueAndKind(nullptr, 0) {}

   bool isNull() const {
      return ValueAndKind.getPointer() == nullptr;
   }

   Kind getKind() const {
      auto packedKind = ValueAndKind.getInt();
      switch ((PackedKind)packedKind) {
         case PackedStored:
            return TupleIndex
                   ? Kind::TupleElement : Kind::StoredProperty;
         case PackedComputed:
            return SetterAndIdKind.getPointer()
                   ? Kind::SettableProperty : Kind::GettableProperty;
         case Unpacked:
            return (Kind)((uintptr_t)ValueAndKind.getPointer() >> KindPackingBits);
      }
      llvm_unreachable("unhandled kind");
   }

   CanType getComponentType() const {
      return ComponentType;
   }

   VarDecl *getStoredPropertyDecl() const {
      switch (getKind()) {
         case Kind::StoredProperty:
            return static_cast<VarDecl*>(ValueAndKind.getPointer());
         case Kind::GettableProperty:
         case Kind::SettableProperty:
         case Kind::OptionalChain:
         case Kind::OptionalForce:
         case Kind::OptionalWrap:
         case Kind::TupleElement:
            llvm_unreachable("not a stored property");
      }
      llvm_unreachable("unhandled kind");
   }

   ComputedPropertyId getComputedPropertyId() const {
      switch (getKind()) {
         case Kind::StoredProperty:
         case Kind::OptionalChain:
         case Kind::OptionalForce:
         case Kind::OptionalWrap:
         case Kind::TupleElement:
            llvm_unreachable("not a computed property");
         case Kind::GettableProperty:
         case Kind::SettableProperty:
            return ComputedPropertyId(IdValue,
                                      SetterAndIdKind.getInt());
      }
      llvm_unreachable("unhandled kind");
   }

   PILFunction *getComputedPropertyGetter() const {
      switch (getKind()) {
         case Kind::StoredProperty:
         case Kind::OptionalChain:
         case Kind::OptionalForce:
         case Kind::OptionalWrap:
         case Kind::TupleElement:
            llvm_unreachable("not a computed property");
         case Kind::GettableProperty:
         case Kind::SettableProperty:
            return static_cast<PILFunction*>(ValueAndKind.getPointer());
      }
      llvm_unreachable("unhandled kind");
   }

   PILFunction *getComputedPropertySetter() const {
      switch (getKind()) {
         case Kind::StoredProperty:
         case Kind::GettableProperty:
         case Kind::OptionalChain:
         case Kind::OptionalForce:
         case Kind::OptionalWrap:
         case Kind::TupleElement:
            llvm_unreachable("not a settable computed property");
         case Kind::SettableProperty:
            return SetterAndIdKind.getPointer();
      }
      llvm_unreachable("unhandled kind");
   }

   ArrayRef<Index> getSubscriptIndices() const {
      switch (getKind()) {
         case Kind::StoredProperty:
         case Kind::OptionalChain:
         case Kind::OptionalForce:
         case Kind::OptionalWrap:
         case Kind::TupleElement:
            return {};
         case Kind::GettableProperty:
         case Kind::SettableProperty:
            return Indices;
      }
      llvm_unreachable("unhandled kind");
   }

   PILFunction *getSubscriptIndexEquals() const {
      switch (getKind()) {
         case Kind::StoredProperty:
         case Kind::OptionalChain:
         case Kind::OptionalForce:
         case Kind::OptionalWrap:
         case Kind::TupleElement:
            llvm_unreachable("not a computed property");
         case Kind::GettableProperty:
         case Kind::SettableProperty:
            return IndexEquality.Equal;
      }
      llvm_unreachable("unhandled kind");
   }
   PILFunction *getSubscriptIndexHash() const {
      switch (getKind()) {
         case Kind::StoredProperty:
         case Kind::OptionalChain:
         case Kind::OptionalForce:
         case Kind::OptionalWrap:
         case Kind::TupleElement:
            llvm_unreachable("not a computed property");
         case Kind::GettableProperty:
         case Kind::SettableProperty:
            return IndexEquality.Hash;
      }
      llvm_unreachable("unhandled kind");
   }

   bool isComputedSettablePropertyMutating() const;

   static KeyPathPatternComponent forStoredProperty(VarDecl *property,
                                                    CanType ty) {
      return KeyPathPatternComponent(property, ty);
   }

   AbstractStorageDecl *getExternalDecl() const {
      switch (getKind()) {
         case Kind::StoredProperty:
         case Kind::OptionalChain:
         case Kind::OptionalForce:
         case Kind::OptionalWrap:
         case Kind::TupleElement:
            llvm_unreachable("not a computed property");
         case Kind::GettableProperty:
         case Kind::SettableProperty:
            return ExternalStorage;
      }
      llvm_unreachable("unhandled kind");
   }

   SubstitutionMap getExternalSubstitutions() const {
      switch (getKind()) {
         case Kind::StoredProperty:
         case Kind::OptionalChain:
         case Kind::OptionalForce:
         case Kind::OptionalWrap:
         case Kind::TupleElement:
            llvm_unreachable("not a computed property");
         case Kind::GettableProperty:
         case Kind::SettableProperty:
            return ExternalSubstitutions;
      }
      llvm_unreachable("unhandled kind");
   }

   unsigned getTupleIndex() const {
      switch (getKind()) {
         case Kind::StoredProperty:
         case Kind::OptionalChain:
         case Kind::OptionalForce:
         case Kind::OptionalWrap:
         case Kind::GettableProperty:
         case Kind::SettableProperty:
            llvm_unreachable("not a tuple element");
         case Kind::TupleElement:
            return TupleIndex - 1;
      }
      llvm_unreachable("unhandled kind");
   }

   static KeyPathPatternComponent
   forComputedGettableProperty(ComputedPropertyId identifier,
                               PILFunction *getter,
                               ArrayRef<Index> indices,
                               PILFunction *indicesEquals,
                               PILFunction *indicesHash,
                               AbstractStorageDecl *externalDecl,
                               SubstitutionMap externalSubs,
                               CanType ty) {
      return KeyPathPatternComponent(identifier,
                                     getter, nullptr, indices,
                                     indicesEquals, indicesHash,
                                     externalDecl, externalSubs,
                                     ty);
   }

   static KeyPathPatternComponent
   forComputedSettableProperty(ComputedPropertyId identifier,
                               PILFunction *getter,
                               PILFunction *setter,
                               ArrayRef<Index> indices,
                               PILFunction *indicesEquals,
                               PILFunction *indicesHash,
                               AbstractStorageDecl *externalDecl,
                               SubstitutionMap externalSubs,
                               CanType ty) {
      return KeyPathPatternComponent(identifier,
                                     getter, setter, indices,
                                     indicesEquals, indicesHash,
                                     externalDecl, externalSubs,
                                     ty);
   }

   static KeyPathPatternComponent
   forOptional(Kind kind, CanType ty) {
      switch (kind) {
         case Kind::OptionalChain:
         case Kind::OptionalForce:
            break;
         case Kind::OptionalWrap:
            assert(ty->getOptionalObjectType() &&
                   "optional wrap didn't form optional?!");
            break;
         case Kind::StoredProperty:
         case Kind::GettableProperty:
         case Kind::SettableProperty:
         case Kind::TupleElement:
            llvm_unreachable("not an optional kind");
      }
      return KeyPathPatternComponent(kind, ty);
   }

   static KeyPathPatternComponent forTupleElement(unsigned tupleIndex,
                                                  CanType ty) {
      return KeyPathPatternComponent(tupleIndex, ty);
   }

   void visitReferencedFunctionsAndMethods(
      std::function<void (PILFunction *)> functionCallBack,
      std::function<void (PILDeclRef)> methodCallBack) const;

   void incrementRefCounts() const;
   void decrementRefCounts() const;

   void Profile(llvm::FoldingSetNodeID &ID);
};

/// An abstract description of a key path pattern.
class KeyPathPattern final
   : public llvm::FoldingSetNode,
     private llvm::TrailingObjects<KeyPathPattern,
        KeyPathPatternComponent>
{
   friend TrailingObjects;

   unsigned NumOperands, NumComponents;
   CanGenericSignature Signature;
   CanType RootType, ValueType;
   StringRef ObjCString;

   KeyPathPattern(CanGenericSignature signature,
                  CanType rootType,
                  CanType valueType,
                  ArrayRef<KeyPathPatternComponent> components,
                  StringRef ObjCString,
                  unsigned numOperands);

   static KeyPathPattern *create(PILModule &M,
                                 CanGenericSignature signature,
                                 CanType rootType,
                                 CanType valueType,
                                 ArrayRef<KeyPathPatternComponent> components,
                                 StringRef ObjCString,
                                 unsigned numOperands);
public:
   CanGenericSignature getGenericSignature() const {
      return Signature;
   }

   CanType getRootType() const {
      return RootType;
   }

   CanType getValueType() const {
      return ValueType;
   }

   unsigned getNumOperands() const {
      return NumOperands;
   }

   StringRef getObjCString() const {
      return ObjCString;
   }

   ArrayRef<KeyPathPatternComponent> getComponents() const;

   void visitReferencedFunctionsAndMethods(
      std::function<void (PILFunction *)> functionCallBack,
      std::function<void (PILDeclRef)> methodCallBack) {
      for (auto &component : getComponents()) {
         component.visitReferencedFunctionsAndMethods(functionCallBack,
                                                      methodCallBack);
      }
   }

   static KeyPathPattern *get(PILModule &M,
                              CanGenericSignature signature,
                              CanType rootType,
                              CanType valueType,
                              ArrayRef<KeyPathPatternComponent> components,
                              StringRef ObjCString);

   static void Profile(llvm::FoldingSetNodeID &ID,
                       CanGenericSignature signature,
                       CanType rootType,
                       CanType valueType,
                       ArrayRef<KeyPathPatternComponent> components,
                       StringRef ObjCString);

   void Profile(llvm::FoldingSetNodeID &ID) {
      Profile(ID, getGenericSignature(), getRootType(), getValueType(),
              getComponents(), getObjCString());
   }
};

/// Instantiates a key path object.
class KeyPathInst final
   : public InstructionBase<PILInstructionKind::KeyPathInst,
      SingleValueInstruction>,
     private llvm::TrailingObjects<KeyPathInst, Operand> {
   friend PILBuilder;
   friend TrailingObjects;

   KeyPathPattern *Pattern;
   unsigned NumOperands;
   SubstitutionMap Substitutions;

   static KeyPathInst *create(PILDebugLocation Loc,
                              KeyPathPattern *Pattern,
                              SubstitutionMap Subs,
                              ArrayRef<PILValue> Args,
                              PILType Ty,
                              PILFunction &F);

   KeyPathInst(PILDebugLocation Loc,
               KeyPathPattern *Pattern,
               SubstitutionMap Subs,
               ArrayRef<PILValue> Args,
               PILType Ty);

   size_t numTrailingObjects(OverloadToken<Operand>) const {
      return NumOperands;
   }

public:
   KeyPathPattern *getPattern() const;
   bool hasPattern() const { return (bool)Pattern; }

   ArrayRef<Operand> getAllOperands() const {
      return const_cast<KeyPathInst*>(this)->getAllOperands();
   }
   MutableArrayRef<Operand> getAllOperands();

   SubstitutionMap getSubstitutions() const { return Substitutions; }

   void dropReferencedPattern();

   ~KeyPathInst();
};

/// Represents an invocation of builtin functionality provided by the code
/// generator.
class BuiltinInst final
   : public InstructionBaseWithTrailingOperands<
      PILInstructionKind::BuiltinInst, BuiltinInst,
      SingleValueInstruction> {
   friend PILBuilder;

   /// The name of the builtin to invoke.
   Identifier Name;

   /// The substitutions.
   SubstitutionMap Substitutions;

   BuiltinInst(PILDebugLocation DebugLoc, Identifier Name, PILType ReturnType,
               SubstitutionMap Substitutions, ArrayRef<PILValue> Args);

   static BuiltinInst *create(PILDebugLocation DebugLoc, Identifier Name,
                              PILType ReturnType,
                              SubstitutionMap Substitutions,
                              ArrayRef<PILValue> Args, PILModule &M);

public:
   /// Return the name of the builtin operation.
   Identifier getName() const { return Name; }
   void setName(Identifier I) { Name = I; }

   /// Looks up the llvm intrinsic ID and type for the builtin function.
   ///
   /// \returns Returns llvm::Intrinsic::not_intrinsic if the function is not an
   /// intrinsic. The particular intrinsic functions which correspond to the
   /// returned value are defined in llvm/Intrinsics.h.
   const IntrinsicInfo &getIntrinsicInfo() const;

   /// Looks up the lazily cached identification for the builtin function.
   const BuiltinInfo &getBuiltinInfo() const;

   /// Looks up the llvm intrinsic ID of this builtin. Returns None if
   /// this is not an intrinsic.
   llvm::Optional<llvm::Intrinsic::ID> getIntrinsicID() const {
      auto I = getIntrinsicInfo();
      if (I.ID == llvm::Intrinsic::not_intrinsic)
         return None;
      return I.ID;
   }

   /// Looks up the BuiltinKind of this builtin. Returns None if this is
   /// not a builtin.
   Optional<BuiltinValueKind> getBuiltinKind() const {
      auto I = getBuiltinInfo();
      if (I.ID == BuiltinValueKind::None)
         return None;
      return I.ID;
   }

   /// True if this builtin application has substitutions, which represent type
   /// parameters to the builtin.
   bool hasSubstitutions() const {
      return Substitutions.hasAnySubstitutableParams();
   }

   /// Return the type parameters to the builtin.
   SubstitutionMap getSubstitutions() const { return Substitutions; }

   /// The arguments to the builtin.
   OperandValueArrayRef getArguments() const {
      return OperandValueArrayRef(getAllOperands());
   }
};

/// Initializes a PIL global variable. Only valid once, before any
/// usages of the global via GlobalAddrInst.
class AllocGlobalInst
   : public InstructionBase<PILInstructionKind::AllocGlobalInst,
      PILInstruction> {
   friend PILBuilder;

   PILGlobalVariable *Global;

   AllocGlobalInst(PILDebugLocation DebugLoc, PILGlobalVariable *Global);

public:
   /// Return the referenced global variable.
   PILGlobalVariable *getReferencedGlobal() const { return Global; }

   void setReferencedGlobal(PILGlobalVariable *v) { Global = v; }

   ArrayRef<Operand> getAllOperands() const { return {}; }
   MutableArrayRef<Operand> getAllOperands() { return {}; }
};

/// The base class for global_addr and global_value.
class GlobalAccessInst : public LiteralInst {
   PILGlobalVariable *Global;

protected:
   GlobalAccessInst(PILInstructionKind kind, PILDebugLocation loc,
                    PILType ty, PILGlobalVariable *global)
      : LiteralInst(kind, loc, ty), Global(global) { }

public:
   /// Return the referenced global variable.
   PILGlobalVariable *getReferencedGlobal() const { return Global; }

   void setReferencedGlobal(PILGlobalVariable *v) { Global = v; }

   ArrayRef<Operand> getAllOperands() const { return {}; }
   MutableArrayRef<Operand> getAllOperands() { return {}; }
};

/// Gives the address of a PIL global variable. Only valid after an
/// AllocGlobalInst.
class GlobalAddrInst
   : public InstructionBase<PILInstructionKind::GlobalAddrInst,
      GlobalAccessInst> {
   friend PILBuilder;

   GlobalAddrInst(PILDebugLocation DebugLoc, PILGlobalVariable *Global,
                  TypeExpansionContext context);

public:
   // FIXME: This constructor should be private but is currently used
   //        in the PILParser.

   /// Create a placeholder instruction with an unset global reference.
   GlobalAddrInst(PILDebugLocation DebugLoc, PILType Ty)
      : InstructionBase(DebugLoc, Ty, nullptr) {}
};

/// Gives the value of a global variable.
///
/// The referenced global variable must be a statically initialized object.
/// TODO: in future we might support global variables in general.
class GlobalValueInst
   : public InstructionBase<PILInstructionKind::GlobalValueInst,
      GlobalAccessInst> {
   friend PILBuilder;

   GlobalValueInst(PILDebugLocation DebugLoc, PILGlobalVariable *Global,
                   TypeExpansionContext context);
};

/// IntegerLiteralInst - Encapsulates an integer constant, as defined originally
/// by an IntegerLiteralExpr.
class IntegerLiteralInst final
   : public InstructionBase<PILInstructionKind::IntegerLiteralInst,
      LiteralInst>,
     private llvm::TrailingObjects<IntegerLiteralInst, llvm::APInt::WordType> {
   friend TrailingObjects;
   friend PILBuilder;

   IntegerLiteralInst(PILDebugLocation Loc, PILType Ty, const APInt &Value);

   static IntegerLiteralInst *create(IntegerLiteralExpr *E,
                                     PILDebugLocation Loc, PILModule &M);
   static IntegerLiteralInst *create(PILDebugLocation Loc, PILType Ty,
                                     intmax_t Value, PILModule &M);
   static IntegerLiteralInst *create(PILDebugLocation Loc, PILType Ty,
                                     const APInt &Value, PILModule &M);

public:
   /// getValue - Return the APInt for the underlying integer literal.
   APInt getValue() const;

   ArrayRef<Operand> getAllOperands() const { return {}; }
   MutableArrayRef<Operand> getAllOperands() { return {}; }
};

/// FloatLiteralInst - Encapsulates a floating point constant, as defined
/// originally by a FloatLiteralExpr.
class FloatLiteralInst final
   : public InstructionBase<PILInstructionKind::FloatLiteralInst,
      LiteralInst>,
     private llvm::TrailingObjects<FloatLiteralInst, llvm::APInt::WordType> {
   friend TrailingObjects;
   friend PILBuilder;

   FloatLiteralInst(PILDebugLocation Loc, PILType Ty, const APInt &Bits);

   static FloatLiteralInst *create(FloatLiteralExpr *E, PILDebugLocation Loc,
                                   PILModule &M);
   static FloatLiteralInst *create(PILDebugLocation Loc, PILType Ty,
                                   const APFloat &Value, PILModule &M);

public:
   /// Return the APFloat for the underlying FP literal.
   APFloat getValue() const;

   /// Return the bitcast representation of the FP literal as an APInt.
   APInt getBits() const;

   ArrayRef<Operand> getAllOperands() const { return {}; }
   MutableArrayRef<Operand> getAllOperands() { return {}; }
};

/// StringLiteralInst - Encapsulates a string constant, as defined originally by
/// a StringLiteralExpr.  This produces the address of the string data as a
/// Builtin.RawPointer.
class StringLiteralInst final
   : public InstructionBase<PILInstructionKind::StringLiteralInst,
      LiteralInst>,
     private llvm::TrailingObjects<StringLiteralInst, char> {
   friend TrailingObjects;
   friend PILBuilder;

public:
   enum class Encoding {
      Bytes,
      UTF8,
      UTF16,
      /// UTF-8 encoding of an Objective-C selector.
         ObjCSelector,
   };

private:
   StringLiteralInst(PILDebugLocation DebugLoc, StringRef text,
                     Encoding encoding, PILType ty);

   static StringLiteralInst *create(PILDebugLocation DebugLoc, StringRef Text,
                                    Encoding encoding, PILModule &M);

public:
   /// getValue - Return the string data for the literal, in UTF-8.
   StringRef getValue() const {
      return {getTrailingObjects<char>(),
              PILInstruction::Bits.StringLiteralInst.Length};
   }

   /// getEncoding - Return the desired encoding of the text.
   Encoding getEncoding() const {
      return Encoding(PILInstruction::Bits.StringLiteralInst.TheEncoding);
   }

   /// getCodeUnitCount - Return encoding-based length of the string
   /// literal in code units.
   uint64_t getCodeUnitCount();

   ArrayRef<Operand> getAllOperands() const { return {}; }
   MutableArrayRef<Operand> getAllOperands() { return {}; }
};

//===----------------------------------------------------------------------===//
// Memory instructions.
//===----------------------------------------------------------------------===//

/// StringLiteralInst::Encoding hashes to its underlying integer representation.
static inline llvm::hash_code hash_value(StringLiteralInst::Encoding E) {
   return llvm::hash_value(size_t(E));
}

// *NOTE* When serializing, we can only represent up to 4 values here. If more
// qualifiers are added, PIL serialization must be updated.
enum class LoadOwnershipQualifier {
   Unqualified, Take, Copy, Trivial
};
static_assert(2 == PILNode::NumLoadOwnershipQualifierBits, "Size mismatch");

/// LoadInst - Represents a load from a memory location.
class LoadInst
   : public UnaryInstructionBase<PILInstructionKind::LoadInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   /// Constructs a LoadInst.
   ///
   /// \param DebugLoc The location of the expression that caused the load.
   ///
   /// \param LValue The PILValue representing the lvalue (address) to
   ///        use for the load.
   LoadInst(PILDebugLocation DebugLoc, PILValue LValue,
            LoadOwnershipQualifier Q = LoadOwnershipQualifier::Unqualified)
      : UnaryInstructionBase(DebugLoc, LValue,
                             LValue->getType().getObjectType()) {
      PILInstruction::Bits.LoadInst.OwnershipQualifier = unsigned(Q);
   }

public:
   LoadOwnershipQualifier getOwnershipQualifier() const {
      return LoadOwnershipQualifier(
         PILInstruction::Bits.LoadInst.OwnershipQualifier);
   }
   void setOwnershipQualifier(LoadOwnershipQualifier qualifier) {
      PILInstruction::Bits.LoadInst.OwnershipQualifier = unsigned(qualifier);
   }
};

// *NOTE* When serializing, we can only represent up to 4 values here. If more
// qualifiers are added, PIL serialization must be updated.
enum class StoreOwnershipQualifier {
   Unqualified, Init, Assign, Trivial
};
static_assert(2 == PILNode::NumStoreOwnershipQualifierBits, "Size mismatch");

/// StoreInst - Represents a store from a memory location.
class StoreInst
   : public InstructionBase<PILInstructionKind::StoreInst,
      NonValueInstruction> {
   friend PILBuilder;

private:
   FixedOperandList<2> Operands;

   StoreInst(PILDebugLocation DebugLoc, PILValue Src, PILValue Dest,
             StoreOwnershipQualifier Qualifier);

public:
   enum {
      /// the value being stored
         Src,
      /// the lvalue being stored to
         Dest
   };

   PILValue getSrc() const { return Operands[Src].get(); }
   PILValue getDest() const { return Operands[Dest].get(); }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }

   StoreOwnershipQualifier getOwnershipQualifier() const {
      return StoreOwnershipQualifier(
         PILInstruction::Bits.StoreInst.OwnershipQualifier);
   }
   void setOwnershipQualifier(StoreOwnershipQualifier qualifier) {
      PILInstruction::Bits.StoreInst.OwnershipQualifier = unsigned(qualifier);
   }
};

class EndBorrowInst;

/// Represents a load of a borrowed value. Must be paired with an end_borrow
/// instruction in its use-def list.
class LoadBorrowInst :
   public UnaryInstructionBase<PILInstructionKind::LoadBorrowInst,
      SingleValueInstruction> {
   friend class PILBuilder;

public:
   LoadBorrowInst(PILDebugLocation DebugLoc, PILValue LValue)
      : UnaryInstructionBase(DebugLoc, LValue,
                             LValue->getType().getObjectType()) {}

   using EndBorrowRange =
   decltype(std::declval<ValueBase>().getUsersOfType<EndBorrowInst>());

   /// Return a range over all EndBorrow instructions for this BeginBorrow.
   EndBorrowRange getEndBorrows() const;
};

inline auto LoadBorrowInst::getEndBorrows() const -> EndBorrowRange {
   return getUsersOfType<EndBorrowInst>();
}

/// Represents the begin scope of a borrowed value. Must be paired with an
/// end_borrow instruction in its use-def list.
class BeginBorrowInst
   : public UnaryInstructionBase<PILInstructionKind::BeginBorrowInst,
      SingleValueInstruction> {
   friend class PILBuilder;

   BeginBorrowInst(PILDebugLocation DebugLoc, PILValue LValue)
      : UnaryInstructionBase(DebugLoc, LValue,
                             LValue->getType().getObjectType()) {}

public:
   using EndBorrowRange =
   decltype(std::declval<ValueBase>().getUsersOfType<EndBorrowInst>());

   /// Return a range over all EndBorrow instructions for this BeginBorrow.
   EndBorrowRange getEndBorrows() const;

   /// Return the single use of this BeginBorrowInst, not including any
   /// EndBorrowInst uses, or return nullptr if the borrow is dead or has
   /// multiple uses.
   ///
   /// Useful for matching common PILGen patterns that emit one borrow per use,
   /// and simplifying pass logic.
   Operand *getSingleNonEndingUse() const;
};

inline auto BeginBorrowInst::getEndBorrows() const -> EndBorrowRange {
   return getUsersOfType<EndBorrowInst>();
}

/// Represents a store of a borrowed value into an address. Returns the borrowed
/// address. Must be paired with an end_borrow in its use-def list.
class StoreBorrowInst
   : public InstructionBase<PILInstructionKind::StoreBorrowInst,
      SingleValueInstruction> {
   friend class PILBuilder;

public:
   enum {
      /// The source of the value being borrowed.
         Src,
      /// The destination of the borrowed value.
         Dest
   };

private:
   FixedOperandList<2> Operands;
   StoreBorrowInst(PILDebugLocation DebugLoc, PILValue Src, PILValue Dest);

public:
   PILValue getSrc() const { return Operands[Src].get(); }
   PILValue getDest() const { return Operands[Dest].get(); }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }
};

/// Represents the end of a borrow scope of a value %val from a
/// value or address %src.
///
/// While %val is "live" in a region then,
///
///   1. If %src is an object, it is undefined behavior for %src to be
///   destroyed. This is enforced by the ownership verifier.
///
///   2. If %src is an address, it is undefined behavior for %src to be
///   destroyed or written to.
class EndBorrowInst
   : public UnaryInstructionBase<PILInstructionKind::EndBorrowInst,
      NonValueInstruction> {
   friend class PILBuilder;

   EndBorrowInst(PILDebugLocation debugLoc, PILValue borrowedValue)
      : UnaryInstructionBase(debugLoc, borrowedValue) {}

public:
   /// Return the value that this end_borrow is ending the borrow of if we are
   /// borrowing a single value.
   PILValue getSingleOriginalValue() const {
      PILValue v = getOperand();
      if (auto *bbi = dyn_cast<BeginBorrowInst>(v))
         return bbi->getOperand();
      if (auto *lbi = dyn_cast<LoadBorrowInst>(v))
         return lbi->getOperand();
      return PILValue();
   }

   /// Return the set of guaranteed values that have scopes ended by this
   /// end_borrow.
   ///
   /// Discussion: We can only have multiple values associated with an end_borrow
   /// in the case of having Phi arguments with guaranteed inputs. This is
   /// necessary to represent certain conditional operations such as:
   ///
   /// class Klass {
   ///   let k1: Klass
   ///   let k2: Klass
   /// }
   ///
   /// func useKlass(k: Klass) { ... }
   /// var boolValue : Bool { ... }
   ///
   /// func f(k: Klass) {
   ///   useKlass(boolValue ? k.k1 : k.k2)
   /// }
   ///
   /// Today, when we PILGen such code, we copy k.k1 and k.k2 before the Phi when
   /// it could potentially be avoided. So today this just appends
   /// getSingleOriginalValue() to originalValues.
   ///
   /// TODO: Once this changes, this code must be update.
   void getOriginalValues(SmallVectorImpl<PILValue> &originalValues) const {
      PILValue value = getSingleOriginalValue();
      assert(value && "Guaranteed phi arguments are not supported now");
      originalValues.emplace_back(value);
   }
};

/// Different kinds of access.
enum class PILAccessKind : uint8_t {
   /// An access which takes uninitialized memory and initializes it.
      Init,

   /// An access which reads the value of initialized memory, but doesn't
   /// modify it.
      Read,

   /// An access which changes the value of initialized memory.
      Modify,

   /// An access which takes initialized memory and leaves it uninitialized.
      Deinit,

   // This enum is encoded.
      Last = Deinit,
};
enum { NumPILAccessKindBits = 2 };

StringRef getPILAccessKindName(PILAccessKind kind);

/// Different kinds of exclusivity enforcement for accesses.
enum class PILAccessEnforcement : uint8_t {
   /// The access's enforcement has not yet been determined.
      Unknown,

   /// The access is statically known to not conflict with other accesses.
      Static,

   /// TODO: maybe add InitiallyStatic for when the access is statically
   /// known to not interfere with any accesses when it begins but where
   /// it's possible that other accesses might be started during this access.

   /// The access is not statically known to not conflict with anything
   /// and must be dynamically checked.
      Dynamic,

   /// The access is not statically known to not conflict with anything
   /// but dynamic checking should be suppressed, leaving it undefined
   /// behavior.
      Unsafe,

   // This enum is encoded.
      Last = Unsafe
};
StringRef getPILAccessEnforcementName(PILAccessEnforcement enforcement);

class EndAccessInst;

/// Begins an access scope. Must be paired with an end_access instruction
/// along every path.
class BeginAccessInst
   : public UnaryInstructionBase<PILInstructionKind::BeginAccessInst,
      SingleValueInstruction> {
   friend class PILBuilder;

   BeginAccessInst(PILDebugLocation loc, PILValue lvalue,
                   PILAccessKind accessKind, PILAccessEnforcement enforcement,
                   bool noNestedConflict, bool fromBuiltin)
      : UnaryInstructionBase(loc, lvalue, lvalue->getType()) {
      PILInstruction::Bits.BeginAccessInst.AccessKind = unsigned(accessKind);
      PILInstruction::Bits.BeginAccessInst.Enforcement = unsigned(enforcement);
      PILInstruction::Bits.BeginAccessInst.NoNestedConflict =
         unsigned(noNestedConflict);
      PILInstruction::Bits.BeginAccessInst.FromBuiltin =
         unsigned(fromBuiltin);

      static_assert(unsigned(PILAccessKind::Last) < (1 << 2),
                    "reserve sufficient bits for serialized PIL");
      static_assert(unsigned(PILAccessEnforcement::Last) < (1 << 2),
                    "reserve sufficient bits for serialized PIL");

      static_assert(unsigned(PILAccessKind::Last) <
                    (1 << PILNode::NumPILAccessKindBits),
                    "PILNode needs updating");
      static_assert(unsigned(PILAccessEnforcement::Last) <
                    (1 << PILNode::NumPILAccessEnforcementBits),
                    "PILNode needs updating");
   }

public:
   PILAccessKind getAccessKind() const {
      return PILAccessKind(PILInstruction::Bits.BeginAccessInst.AccessKind);
   }
   void setAccessKind(PILAccessKind kind) {
      PILInstruction::Bits.BeginAccessInst.AccessKind = unsigned(kind);
   }

   PILAccessEnforcement getEnforcement() const {
      return
         PILAccessEnforcement(PILInstruction::Bits.BeginAccessInst.Enforcement);
   }
   void setEnforcement(PILAccessEnforcement enforcement) {
      PILInstruction::Bits.BeginAccessInst.Enforcement = unsigned(enforcement);
   }

   /// If hasNoNestedConflict is true, then it is a static guarantee against
   /// inner conflicts. No subsequent access between this point and the
   /// corresponding end_access could cause an enforcement failure. Consequently,
   /// the access will not need to be tracked by the runtime for the duration of
   /// its scope. This access may still conflict with an outer access scope;
   /// therefore may still require dynamic enforcement at a single point.
   bool hasNoNestedConflict() const {
      return PILInstruction::Bits.BeginAccessInst.NoNestedConflict;
   }
   void setNoNestedConflict(bool noNestedConflict) {
      PILInstruction::Bits.BeginAccessInst.NoNestedConflict = noNestedConflict;
   }

   /// Return true if this access marker was emitted for a user-controlled
   /// Builtin. Return false if this access marker was auto-generated by the
   /// compiler to enforce formal access that derives from the language.
   bool isFromBuiltin() const {
      return PILInstruction::Bits.BeginAccessInst.FromBuiltin;
   }

   PILValue getSource() const {
      return getOperand();
   }

   using EndAccessRange =
   decltype(std::declval<ValueBase>().getUsersOfType<EndAccessInst>());

   /// Find all the associated end_access instructions for this begin_access.
   EndAccessRange getEndAccesses() const;
};

/// Represents the end of an access scope.
class EndAccessInst
   : public UnaryInstructionBase<PILInstructionKind::EndAccessInst,
      NonValueInstruction> {
   friend class PILBuilder;

private:
   EndAccessInst(PILDebugLocation loc, PILValue access, bool aborting = false)
      : UnaryInstructionBase(loc, access) {
      PILInstruction::Bits.EndAccessInst.Aborting = aborting;
   }

public:
   /// An aborted access is one that did not perform the expected
   /// transition described by the begin_access instruction before it
   /// reached this end_access.
   ///
   /// Only AccessKind::Init and AccessKind::Deinit accesses can be
   /// aborted.
   bool isAborting() const {
      return PILInstruction::Bits.EndAccessInst.Aborting;
   }
   void setAborting(bool aborting = true) {
      PILInstruction::Bits.EndAccessInst.Aborting = aborting;
   }

   BeginAccessInst *getBeginAccess() const {
      return cast<BeginAccessInst>(getOperand());
   }

   PILValue getSource() const {
      return getBeginAccess()->getSource();
   }
};

inline auto BeginAccessInst::getEndAccesses() const -> EndAccessRange {
   return getUsersOfType<EndAccessInst>();
}

/// Begins an access without requiring a paired end_access.
/// Dynamically, an end_unpaired_access does still need to be called, though.
///
/// This should only be used in materializeForSet, and eventually it should
/// be removed entirely.
class BeginUnpairedAccessInst
   : public InstructionBase<PILInstructionKind::BeginUnpairedAccessInst,
      NonValueInstruction> {
   friend class PILBuilder;

   FixedOperandList<2> Operands;

   BeginUnpairedAccessInst(PILDebugLocation loc, PILValue addr, PILValue buffer,
                           PILAccessKind accessKind,
                           PILAccessEnforcement enforcement,
                           bool noNestedConflict,
                           bool fromBuiltin)
      : InstructionBase(loc), Operands(this, addr, buffer) {
      PILInstruction::Bits.BeginUnpairedAccessInst.AccessKind =
         unsigned(accessKind);
      PILInstruction::Bits.BeginUnpairedAccessInst.Enforcement =
         unsigned(enforcement);
      PILInstruction::Bits.BeginUnpairedAccessInst.NoNestedConflict =
         unsigned(noNestedConflict);
      PILInstruction::Bits.BeginUnpairedAccessInst.FromBuiltin =
         unsigned(fromBuiltin);
   }

public:
   PILAccessKind getAccessKind() const {
      return PILAccessKind(
         PILInstruction::Bits.BeginUnpairedAccessInst.AccessKind);
   }
   void setAccessKind(PILAccessKind kind) {
      PILInstruction::Bits.BeginUnpairedAccessInst.AccessKind = unsigned(kind);
   }

   PILAccessEnforcement getEnforcement() const {
      return PILAccessEnforcement(
         PILInstruction::Bits.BeginUnpairedAccessInst.Enforcement);
   }
   void setEnforcement(PILAccessEnforcement enforcement) {
      PILInstruction::Bits.BeginUnpairedAccessInst.Enforcement
         = unsigned(enforcement);
   }

   /// If hasNoNestedConflict is true, then it is a static guarantee against
   /// inner conflicts. No subsequent access between this point and the
   /// corresponding end_access could cause an enforcement failure. Consequently,
   /// the access will not need to be tracked by the runtime for the duration of
   /// its scope. This access may still conflict with an outer access scope;
   /// therefore may still require dynamic enforcement at a single point.
   bool hasNoNestedConflict() const {
      return PILInstruction::Bits.BeginUnpairedAccessInst.NoNestedConflict;
   }
   void setNoNestedConflict(bool noNestedConflict) {
      PILInstruction::Bits.BeginUnpairedAccessInst.NoNestedConflict =
         noNestedConflict;
   }

   /// Return true if this access marker was emitted for a user-controlled
   /// Builtin. Return false if this access marker was auto-generated by the
   /// compiler to enforce formal access that derives from the language.
   bool isFromBuiltin() const {
      return PILInstruction::Bits.BeginUnpairedAccessInst.FromBuiltin;
   }

   PILValue getSource() const {
      return Operands[0].get();
   }

   PILValue getBuffer() const {
      return Operands[1].get();
   }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }

   ArrayRef<Operand> getTypeDependentOperands() const {
      return {};
   }

   MutableArrayRef<Operand> getTypeDependentOperands() {
      return {};
   }
};

/// Ends an unpaired access.
class EndUnpairedAccessInst
   : public UnaryInstructionBase<PILInstructionKind::EndUnpairedAccessInst,
      NonValueInstruction> {
   friend class PILBuilder;

private:
   EndUnpairedAccessInst(PILDebugLocation loc, PILValue buffer,
                         PILAccessEnforcement enforcement, bool aborting,
                         bool fromBuiltin)
      : UnaryInstructionBase(loc, buffer) {
      PILInstruction::Bits.EndUnpairedAccessInst.Enforcement
         = unsigned(enforcement);
      PILInstruction::Bits.EndUnpairedAccessInst.Aborting = aborting;
      PILInstruction::Bits.EndUnpairedAccessInst.FromBuiltin = fromBuiltin;
   }

public:
   /// An aborted access is one that did not perform the expected
   /// transition described by the begin_access instruction before it
   /// reached this end_access.
   ///
   /// Only AccessKind::Init and AccessKind::Deinit accesses can be
   /// aborted.
   bool isAborting() const {
      return PILInstruction::Bits.EndUnpairedAccessInst.Aborting;
   }
   void setAborting(bool aborting) {
      PILInstruction::Bits.EndUnpairedAccessInst.Aborting = aborting;
   }

   PILAccessEnforcement getEnforcement() const {
      return PILAccessEnforcement(
         PILInstruction::Bits.EndUnpairedAccessInst.Enforcement);
   }
   void setEnforcement(PILAccessEnforcement enforcement) {
      PILInstruction::Bits.EndUnpairedAccessInst.Enforcement =
         unsigned(enforcement);
   }

   /// Return true if this access marker was emitted for a user-controlled
   /// Builtin. Return false if this access marker was auto-generated by the
   /// compiler to enforce formal access that derives from the language.
   bool isFromBuiltin() const {
      return PILInstruction::Bits.EndUnpairedAccessInst.FromBuiltin;
   }

   PILValue getBuffer() const {
      return getOperand();
   }
};

// *NOTE* When serializing, we can only represent up to 4 values here. If more
// qualifiers are added, PIL serialization must be updated.
enum class AssignOwnershipQualifier {
   /// Unknown initialization method
      Unknown,

   /// The box contains a fully-initialized value.
      Reassign,

   /// The box contains a class instance that we own, but the instance has
   /// not been initialized and should be freed with a special PIL
   /// instruction made for this purpose.
      Reinit,

   /// The box contains an undefined value that should be ignored.
      Init,
};
static_assert(2 == PILNode::NumAssignOwnershipQualifierBits, "Size mismatch");

template <PILInstructionKind Kind, int NumOps>
class AssignInstBase
   : public InstructionBase<Kind, NonValueInstruction> {

protected:
   FixedOperandList<NumOps> Operands;

   template <class... T>
   AssignInstBase(PILDebugLocation DebugLoc, T&&...args) :
      InstructionBase<Kind, NonValueInstruction>(DebugLoc),
      Operands(this, std::forward<T>(args)...) { }

public:
   enum {
      /// the value being stored
         Src,
      /// the lvalue being stored to
         Dest
   };

   PILValue getSrc() const { return Operands[Src].get(); }
   PILValue getDest() const { return Operands[Dest].get(); }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }
};

/// AssignInst - Represents an abstract assignment to a memory location, which
/// may either be an initialization or a store sequence.  This is only valid in
/// Raw PIL.
class AssignInst
   : public AssignInstBase<PILInstructionKind::AssignInst, 2> {
   friend PILBuilder;

   AssignInst(PILDebugLocation DebugLoc, PILValue Src, PILValue Dest,
              AssignOwnershipQualifier Qualifier =
              AssignOwnershipQualifier::Unknown);

public:
   AssignOwnershipQualifier getOwnershipQualifier() const {
      return AssignOwnershipQualifier(
         PILInstruction::Bits.AssignInst.OwnershipQualifier);
   }
   void setOwnershipQualifier(AssignOwnershipQualifier qualifier) {
      PILInstruction::Bits.AssignInst.OwnershipQualifier = unsigned(qualifier);
   }
};

/// AssignByWrapperInst - Represents an abstract assignment via a wrapper,
/// which may either be an initialization or a store sequence.  This is only
/// valid in Raw PIL.
class AssignByWrapperInst
   : public AssignInstBase<PILInstructionKind::AssignByWrapperInst, 4> {
   friend PILBuilder;

   AssignByWrapperInst(PILDebugLocation DebugLoc, PILValue Src, PILValue Dest,
                       PILValue Initializer, PILValue Setter,
                       AssignOwnershipQualifier Qualifier =
                       AssignOwnershipQualifier::Unknown);

public:

   PILValue getInitializer() { return Operands[2].get(); }
   PILValue getSetter() { return  Operands[3].get(); }

   AssignOwnershipQualifier getOwnershipQualifier() const {
      return AssignOwnershipQualifier(
         PILInstruction::Bits.AssignByWrapperInst.OwnershipQualifier);
   }
   void setOwnershipQualifier(AssignOwnershipQualifier qualifier) {
      PILInstruction::Bits.AssignByWrapperInst.OwnershipQualifier = unsigned(qualifier);
   }
};

/// Indicates that a memory location is uninitialized at this point and needs to
/// be initialized by the end of the function and before any escape point for
/// this instruction. This is only valid in Raw PIL.
class MarkUninitializedInst
   : public UnaryInstructionBase<PILInstructionKind::MarkUninitializedInst,
      OwnershipForwardingSingleValueInst> {
   friend PILBuilder;

public:
   /// This enum captures what the mark_uninitialized instruction is designating.
   enum Kind {
      /// Var designates the start of a normal variable live range.
         Var,

      /// RootSelf designates "self" in a struct, enum, or root class.
         RootSelf,

      /// CrossModuleRootSelf is the same as "RootSelf", but in a case where
      /// it's not really safe to treat 'self' as root because the original
      /// module might add more stored properties.
      ///
      /// This is only used for Swift 4 compatibility. In Swift 5, cross-module
      /// initializers are always DelegatingSelf.
         CrossModuleRootSelf,

      /// DerivedSelf designates "self" in a derived (non-root) class.
         DerivedSelf,

      /// DerivedSelfOnly designates "self" in a derived (non-root)
      /// class whose stored properties have already been initialized.
         DerivedSelfOnly,

      /// DelegatingSelf designates "self" on a struct, enum, or class
      /// in a delegating constructor (one that calls self.init).
         DelegatingSelf,

      /// DelegatingSelfAllocated designates "self" in a delegating class
      /// initializer where memory has already been allocated.
         DelegatingSelfAllocated,
   };
private:
   Kind ThisKind;

   MarkUninitializedInst(PILDebugLocation DebugLoc, PILValue Value, Kind K)
      : UnaryInstructionBase(DebugLoc, Value, Value->getType(),
                             Value.getOwnershipKind()),
        ThisKind(K) {}

public:

   Kind getKind() const { return ThisKind; }

   bool isVar() const { return ThisKind == Var; }
   bool isRootSelf() const {
      return ThisKind == RootSelf;
   }
   bool isCrossModuleRootSelf() const {
      return ThisKind == CrossModuleRootSelf;
   }
   bool isDerivedClassSelf() const {
      return ThisKind == DerivedSelf;
   }
   bool isDerivedClassSelfOnly() const {
      return ThisKind == DerivedSelfOnly;
   }
   bool isDelegatingSelf() const {
      return ThisKind == DelegatingSelf;
   }
   bool isDelegatingSelfAllocated() const {
      return ThisKind == DelegatingSelfAllocated;
   }
};

/// MarkFunctionEscape - Represents the escape point of set of variables due to
/// a function definition which uses the variables.  This is only valid in Raw
/// PIL.
class MarkFunctionEscapeInst final
   : public InstructionBaseWithTrailingOperands<
      PILInstructionKind::MarkFunctionEscapeInst,
      MarkFunctionEscapeInst, NonValueInstruction> {
   friend PILBuilder;

   /// Private constructor.  Because this is variadic, object creation goes
   /// through 'create()'.
   MarkFunctionEscapeInst(PILDebugLocation DebugLoc, ArrayRef<PILValue> Elements)
      : InstructionBaseWithTrailingOperands(Elements, DebugLoc) {}

   /// Construct a MarkFunctionEscapeInst.
   static MarkFunctionEscapeInst *create(PILDebugLocation DebugLoc,
                                         ArrayRef<PILValue> Elements,
                                         PILFunction &F);

public:
   /// The elements referenced by this instruction.
   MutableArrayRef<Operand> getElementOperands() {
      return getAllOperands();
   }

   /// The elements referenced by this instruction.
   OperandValueArrayRef getElements() const {
      return OperandValueArrayRef(getAllOperands());
   }
};

/// Define the start or update to a symbolic variable value (for loadable
/// types).
class DebugValueInst final
   : public UnaryInstructionBase<PILInstructionKind::DebugValueInst,
      NonValueInstruction>,
     private llvm::TrailingObjects<DebugValueInst, char> {
   friend TrailingObjects;
   friend PILBuilder;
   TailAllocatedDebugVariable VarInfo;

   DebugValueInst(PILDebugLocation DebugLoc, PILValue Operand,
                  PILDebugVariable Var);
   static DebugValueInst *create(PILDebugLocation DebugLoc, PILValue Operand,
                                 PILModule &M, PILDebugVariable Var);

   size_t numTrailingObjects(OverloadToken<char>) const { return 1; }

public:
   /// Return the underlying variable declaration that this denotes,
   /// or null if we don't have one.
   VarDecl *getDecl() const;
   /// Return the debug variable information attached to this instruction.
   Optional<PILDebugVariable> getVarInfo() const {
      return VarInfo.get(getDecl(), getTrailingObjects<char>());
   }
};

/// Define the start or update to a symbolic variable value (for address-only
/// types) .
class DebugValueAddrInst final
   : public UnaryInstructionBase<PILInstructionKind::DebugValueAddrInst,
      NonValueInstruction>,
     private llvm::TrailingObjects<DebugValueAddrInst, char> {
   friend TrailingObjects;
   friend PILBuilder;
   TailAllocatedDebugVariable VarInfo;

   DebugValueAddrInst(PILDebugLocation DebugLoc, PILValue Operand,
                      PILDebugVariable Var);
   static DebugValueAddrInst *create(PILDebugLocation DebugLoc,
                                     PILValue Operand, PILModule &M,
                                     PILDebugVariable Var);

public:
   /// Return the underlying variable declaration that this denotes,
   /// or null if we don't have one.
   VarDecl *getDecl() const;
   /// Return the debug variable information attached to this instruction.
   Optional<PILDebugVariable> getVarInfo() const {
      return VarInfo.get(getDecl(), getTrailingObjects<char>());
   };
};


/// An abstract class representing a load from some kind of reference storage.
template <PILInstructionKind K>
class LoadReferenceInstBase
   : public UnaryInstructionBase<K, SingleValueInstruction> {
   static PILType getResultType(PILType operandTy) {
      assert(operandTy.isAddress() && "loading from non-address operand?");
      auto refType = cast<ReferenceStorageType>(operandTy.getAstType());
      return PILType::getPrimitiveObjectType(refType.getReferentType());
   }

protected:
   LoadReferenceInstBase(PILDebugLocation loc, PILValue lvalue, IsTake_t isTake)
      : UnaryInstructionBase<K, SingleValueInstruction>(loc, lvalue,
                                                        getResultType(lvalue->getType())) {
      PILInstruction::Bits.LoadReferenceInstBaseT.IsTake = unsigned(isTake);
   }

public:
   IsTake_t isTake() const {
      return IsTake_t(PILInstruction::Bits.LoadReferenceInstBaseT.IsTake);
   }
};

/// An abstract class representing a store to some kind of reference storage.
template <PILInstructionKind K>
class StoreReferenceInstBase : public InstructionBase<K, NonValueInstruction> {
   enum { Src, Dest };
   FixedOperandList<2> Operands;
protected:
   StoreReferenceInstBase(PILDebugLocation loc, PILValue src, PILValue dest,
                          IsInitialization_t isInit)
      : InstructionBase<K, NonValueInstruction>(loc),
        Operands(this, src, dest) {
      PILInstruction::Bits.StoreReferenceInstBaseT.IsInitializationOfDest =
         unsigned(isInit);
   }

public:
   PILValue getSrc() const { return Operands[Src].get(); }
   PILValue getDest() const { return Operands[Dest].get(); }

   IsInitialization_t isInitializationOfDest() const {
      return IsInitialization_t(
         PILInstruction::Bits.StoreReferenceInstBaseT.IsInitializationOfDest);
   }
   void setIsInitializationOfDest(IsInitialization_t I) {
      PILInstruction::Bits.StoreReferenceInstBaseT.IsInitializationOfDest =
         (bool)I;
   }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }
};

/// Represents a load from a dynamic reference storage memory location.
/// This is required for address-only scenarios; for loadable references,
/// it's better to use a load and a strong_retain_#name.
///
/// \param loc The location of the expression that caused the load.
/// \param lvalue The PILValue representing the address to
///        use for the load.
#define NEVER_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
class Load##Name##Inst \
    : public LoadReferenceInstBase<PILInstructionKind::Load##Name##Inst> { \
  friend PILBuilder; \
  Load##Name##Inst(PILDebugLocation loc, PILValue lvalue, IsTake_t isTake) \
    : LoadReferenceInstBase(loc, lvalue, isTake) {} \
};
#include "polarphp/ast/ReferenceStorageDef.h"

/// Represents a store to a dynamic reference storage memory location.
/// This is only required for address-only scenarios; for loadable
/// references, it's better to use a ref_to_##name and a store.
#define NEVER_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
class Store##Name##Inst \
    : public StoreReferenceInstBase<PILInstructionKind::Store##Name##Inst> { \
  friend PILBuilder; \
  Store##Name##Inst(PILDebugLocation loc, PILValue src, PILValue dest, \
                IsInitialization_t isInit) \
    : StoreReferenceInstBase(loc, src, dest, isInit) {} \
};
#include "polarphp/ast/ReferenceStorageDef.h"

/// CopyAddrInst - Represents a copy from one memory location to another. This
/// is similar to:
///   %1 = load %src
///   store %1 to %dest
/// but a copy instruction must be used for address-only types.
class CopyAddrInst
   : public InstructionBase<PILInstructionKind::CopyAddrInst,
      NonValueInstruction> {
   friend PILBuilder;

public:
   enum {
      /// The lvalue being loaded from.
         Src,

      /// The lvalue being stored to.
         Dest
   };

private:
   FixedOperandList<2> Operands;

   CopyAddrInst(PILDebugLocation DebugLoc, PILValue Src, PILValue Dest,
                IsTake_t isTakeOfSrc, IsInitialization_t isInitializationOfDest);

public:
   PILValue getSrc() const { return Operands[Src].get(); }
   PILValue getDest() const { return Operands[Dest].get(); }

   void setSrc(PILValue V) { Operands[Src].set(V); }
   void setDest(PILValue V) { Operands[Dest].set(V); }

   IsTake_t isTakeOfSrc() const {
      return IsTake_t(PILInstruction::Bits.CopyAddrInst.IsTakeOfSrc);
   }
   IsInitialization_t isInitializationOfDest() const {
      return IsInitialization_t(
         PILInstruction::Bits.CopyAddrInst.IsInitializationOfDest);
   }

   void setIsTakeOfSrc(IsTake_t T) {
      PILInstruction::Bits.CopyAddrInst.IsTakeOfSrc = (bool)T;
   }
   void setIsInitializationOfDest(IsInitialization_t I) {
      PILInstruction::Bits.CopyAddrInst.IsInitializationOfDest = (bool)I;
   }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }
};

/// BindMemoryInst -
/// "bind_memory %0 : $Builtin.RawPointer, %1 : $Builtin.Word to $T"
/// Binds memory at the raw pointer %0 to type $T with enough capacity
/// to hold $1 values.
class BindMemoryInst final :
   public InstructionBaseWithTrailingOperands<
      PILInstructionKind::BindMemoryInst,
      BindMemoryInst, NonValueInstruction> {
   friend PILBuilder;

   PILType BoundType;

   static BindMemoryInst *create(
      PILDebugLocation Loc, PILValue Base, PILValue Index, PILType BoundType,
      PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes);

   BindMemoryInst(PILDebugLocation Loc, PILValue Base, PILValue Index,
                  PILType BoundType,
                  ArrayRef<PILValue> TypeDependentOperands)
      : InstructionBaseWithTrailingOperands(Base, Index, TypeDependentOperands,
                                            Loc), BoundType(BoundType) {}

public:
   enum { BaseOperIdx, IndexOperIdx, NumFixedOpers };

   PILValue getBase() const { return getAllOperands()[BaseOperIdx].get(); }

   PILValue getIndex() const { return getAllOperands()[IndexOperIdx].get(); }

   PILType getBoundType() const { return BoundType; }

   ArrayRef<Operand> getTypeDependentOperands() const {
      return getAllOperands().slice(NumFixedOpers);
   }

   MutableArrayRef<Operand> getTypeDependentOperands() {
      return getAllOperands().slice(NumFixedOpers);
   }
};

//===----------------------------------------------------------------------===//
// Conversion instructions.
//===----------------------------------------------------------------------===//

/// ConversionInst - Abstract class representing instructions that convert
/// values.
///
class ConversionInst : public SingleValueInstruction {
protected:
   ConversionInst(PILInstructionKind Kind, PILDebugLocation DebugLoc, PILType Ty)
      : SingleValueInstruction(Kind, DebugLoc, Ty) {}

public:
   /// All conversion instructions take the converted value, whose reference
   /// identity is expected to be preserved through the conversion chain, as their
   /// first operand. Some instructions may take additional operands that do not
   /// affect the reference identity.
   PILValue getConverted() const { return getOperand(0); }

   DEFINE_ABSTRACT_SINGLE_VALUE_INST_BOILERPLATE(ConversionInst)
};

/// A conversion inst that produces a static OwnershipKind set upon the
/// instruction's construction.
class OwnershipForwardingConversionInst : public ConversionInst {
   ValueOwnershipKind ownershipKind;

protected:
   OwnershipForwardingConversionInst(PILInstructionKind kind,
                                     PILDebugLocation debugLoc, PILType ty,
                                     ValueOwnershipKind ownershipKind)
      : ConversionInst(kind, debugLoc, ty), ownershipKind(ownershipKind) {}

public:
   ValueOwnershipKind getOwnershipKind() const { return ownershipKind; }
   void setOwnershipKind(ValueOwnershipKind newOwnershipKind) {
      ownershipKind = newOwnershipKind;
   }
};

/// ConvertFunctionInst - Change the type of a function value without
/// affecting how it will codegen.
class ConvertFunctionInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::ConvertFunctionInst, ConvertFunctionInst,
      OwnershipForwardingConversionInst> {
   friend PILBuilder;

   ConvertFunctionInst(PILDebugLocation DebugLoc, PILValue Operand,
                       ArrayRef<PILValue> TypeDependentOperands, PILType Ty,
                       bool WithoutActuallyEscaping)
      : UnaryInstructionWithTypeDependentOperandsBase(
      DebugLoc, Operand, TypeDependentOperands, Ty,
      Operand.getOwnershipKind()) {
      PILInstruction::Bits.ConvertFunctionInst.WithoutActuallyEscaping =
         WithoutActuallyEscaping;
      assert((Operand->getType().castTo<PILFunctionType>()->isNoEscape() ==
              Ty.castTo<PILFunctionType>()->isNoEscape() ||
              Ty.castTo<PILFunctionType>()->getRepresentation() !=
              PILFunctionType::Representation::Thick) &&
             "Change of escapeness is not ABI compatible");
   }

   static ConvertFunctionInst *create(PILDebugLocation DebugLoc,
                                      PILValue Operand, PILType Ty,
                                      PILFunction &F,
                                      PILOpenedArchetypesState &OpenedArchetypes,
                                      bool WithoutActuallyEscaping);

public:
   /// Returns `true` if this converts a non-escaping closure into an escaping
   /// function type. `True` must be returned whenever the closure operand has an
   /// unboxed capture (via @inout_aliasable) *and* the resulting function type
   /// is escaping. (This only happens as a result of
   /// withoutActuallyEscaping()). If `true` is returned, then the resulting
   /// function type must be escaping, but the operand's function type may or may
   /// not be @noescape. Note that a non-escaping closure may have unboxed
   /// captured even though its PIL function type is "escaping".
   bool withoutActuallyEscaping() const {
      return PILInstruction::Bits.ConvertFunctionInst.WithoutActuallyEscaping;
   }
};

/// ConvertEscapeToNoEscapeInst - Change the type of a escaping function value
/// to a trivial function type (@noescape T -> U).
class ConvertEscapeToNoEscapeInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::ConvertEscapeToNoEscapeInst,
      ConvertEscapeToNoEscapeInst, ConversionInst> {
   friend PILBuilder;

   bool lifetimeGuaranteed;

   ConvertEscapeToNoEscapeInst(PILDebugLocation DebugLoc, PILValue Operand,
                               ArrayRef<PILValue> TypeDependentOperands,
                               PILType Ty,
                               bool isLifetimeGuaranteed)
      : UnaryInstructionWithTypeDependentOperandsBase(
      DebugLoc, Operand, TypeDependentOperands, Ty),
        lifetimeGuaranteed(isLifetimeGuaranteed) {
      assert(!Operand->getType().castTo<PILFunctionType>()->isNoEscape());
      assert(Ty.castTo<PILFunctionType>()->isNoEscape());
   }

   static ConvertEscapeToNoEscapeInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes,
          bool lifetimeGuaranteed);
public:
   /// Return true if we have extended the lifetime of the argument of the
   /// convert_escape_to_no_escape to be over all uses of the trivial type.
   bool isLifetimeGuaranteed() const {
      return lifetimeGuaranteed;
   }

   /// Mark that we have extended the lifetime of the argument of the
   /// convert_escape_to_no_escape to be over all uses of the trivial type.
   ///
   /// NOTE: This is a one way operation.
   void setLifetimeGuaranteed() { lifetimeGuaranteed = true; }
};

/// ThinFunctionToPointerInst - Convert a thin function pointer to a
/// Builtin.RawPointer.
class ThinFunctionToPointerInst
   : public UnaryInstructionBase<PILInstructionKind::ThinFunctionToPointerInst,
      ConversionInst>
{
   friend PILBuilder;

   ThinFunctionToPointerInst(PILDebugLocation DebugLoc, PILValue operand,
                             PILType ty)
      : UnaryInstructionBase(DebugLoc, operand, ty) {}
};

/// PointerToThinFunctionInst - Convert a Builtin.RawPointer to a thin
/// function pointer.
class PointerToThinFunctionInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::PointerToThinFunctionInst,
      PointerToThinFunctionInst,
      ConversionInst> {
   friend PILBuilder;

   PointerToThinFunctionInst(PILDebugLocation DebugLoc, PILValue operand,
                             ArrayRef<PILValue> TypeDependentOperands,
                             PILType ty)
      : UnaryInstructionWithTypeDependentOperandsBase(
      DebugLoc, operand, TypeDependentOperands, ty) {}

   static PointerToThinFunctionInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes);
};

/// UpcastInst - Perform a conversion of a class instance to a supertype.
class UpcastInst final : public UnaryInstructionWithTypeDependentOperandsBase<
   PILInstructionKind::UpcastInst, UpcastInst,
   OwnershipForwardingConversionInst> {
   friend PILBuilder;

   UpcastInst(PILDebugLocation DebugLoc, PILValue Operand,
              ArrayRef<PILValue> TypeDependentOperands, PILType Ty)
      : UnaryInstructionWithTypeDependentOperandsBase(
      DebugLoc, Operand, TypeDependentOperands, Ty,
      Operand.getOwnershipKind()) {}

   static UpcastInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes);
};

/// AddressToPointerInst - Convert a PIL address to a Builtin.RawPointer value.
class AddressToPointerInst
   : public UnaryInstructionBase<PILInstructionKind::AddressToPointerInst,
      ConversionInst>
{
   friend PILBuilder;

   AddressToPointerInst(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty)
      : UnaryInstructionBase(DebugLoc, Operand, Ty) {}
};

/// PointerToAddressInst - Convert a Builtin.RawPointer value to a PIL address.
class PointerToAddressInst
   : public UnaryInstructionBase<PILInstructionKind::PointerToAddressInst,
      ConversionInst>
{
   friend PILBuilder;

   PointerToAddressInst(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty,
                        bool IsStrict, bool IsInvariant)
      : UnaryInstructionBase(DebugLoc, Operand, Ty) {
      PILInstruction::Bits.PointerToAddressInst.IsStrict = IsStrict;
      PILInstruction::Bits.PointerToAddressInst.IsInvariant = IsInvariant;
   }

public:
   /// Whether the returned address adheres to strict aliasing.
   /// If true, then the type of each memory access dependent on
   /// this address must be consistent with the memory's bound type.
   bool isStrict() const {
      return PILInstruction::Bits.PointerToAddressInst.IsStrict;
   }
   /// Whether the returned address is invariant.
   /// If true, then loading from an address derived from this pointer always
   /// produces the same value.
   bool isInvariant() const {
      return PILInstruction::Bits.PointerToAddressInst.IsInvariant;
   }
};

/// Convert a heap object reference to a different type without any runtime
/// checks.
class UncheckedRefCastInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::UncheckedRefCastInst, UncheckedRefCastInst,
      OwnershipForwardingConversionInst> {
   friend PILBuilder;

   UncheckedRefCastInst(PILDebugLocation DebugLoc, PILValue Operand,
                        ArrayRef<PILValue> TypeDependentOperands, PILType Ty)
      : UnaryInstructionWithTypeDependentOperandsBase(
      DebugLoc, Operand, TypeDependentOperands, Ty,
      Operand.getOwnershipKind()) {}
   static UncheckedRefCastInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes);
};

/// Converts a heap object reference to a different type without any runtime
/// checks. This is a variant of UncheckedRefCast that works on address types,
/// thus encapsulates an implicit load and take of the reference followed by a
/// store and initialization of a new reference.
class UncheckedRefCastAddrInst
   : public InstructionBase<PILInstructionKind::UncheckedRefCastAddrInst,
      NonValueInstruction> {
public:
   enum {
      /// the value being stored
         Src,
      /// the lvalue being stored to
         Dest
   };

private:
   FixedOperandList<2> Operands;
   CanType SourceType;
   CanType TargetType;
public:
   UncheckedRefCastAddrInst(PILDebugLocation Loc, PILValue src, CanType srcType,
                            PILValue dest, CanType targetType);

   PILValue getSrc() const { return Operands[Src].get(); }
   PILValue getDest() const { return Operands[Dest].get(); }

   PILType getSourceLoweredType() const { return getSrc()->getType(); }
   CanType getSourceFormalType() const { return SourceType; }

   PILType getTargetLoweredType() const { return getDest()->getType(); }
   CanType getTargetFormalType() const { return TargetType; }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }
};

class UncheckedAddrCastInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::UncheckedAddrCastInst,
      UncheckedAddrCastInst,
      ConversionInst>
{
   friend PILBuilder;

   UncheckedAddrCastInst(PILDebugLocation DebugLoc, PILValue Operand,
                         ArrayRef<PILValue> TypeDependentOperands, PILType Ty)
      : UnaryInstructionWithTypeDependentOperandsBase(DebugLoc, Operand,
                                                      TypeDependentOperands, Ty) {}
   static UncheckedAddrCastInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes);
};

/// Convert a value's binary representation to a trivial type of the same size.
class UncheckedTrivialBitCastInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::UncheckedTrivialBitCastInst,
      UncheckedTrivialBitCastInst,
      ConversionInst>
{
   friend PILBuilder;

   UncheckedTrivialBitCastInst(PILDebugLocation DebugLoc, PILValue Operand,
                               ArrayRef<PILValue> TypeDependentOperands,
                               PILType Ty)
      : UnaryInstructionWithTypeDependentOperandsBase(DebugLoc, Operand,
                                                      TypeDependentOperands, Ty) {}

   static UncheckedTrivialBitCastInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes);
};

/// Bitwise copy a value into another value of the same size or smaller.
class UncheckedBitwiseCastInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::UncheckedBitwiseCastInst,
      UncheckedBitwiseCastInst,
      ConversionInst>
{
   friend PILBuilder;

   UncheckedBitwiseCastInst(PILDebugLocation DebugLoc, PILValue Operand,
                            ArrayRef<PILValue> TypeDependentOperands,
                            PILType Ty)
      : UnaryInstructionWithTypeDependentOperandsBase(DebugLoc, Operand,
                                                      TypeDependentOperands, Ty) {}
   static UncheckedBitwiseCastInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes);
};

/// Build a Builtin.BridgeObject from a heap object reference by bitwise-or-ing
/// in bits from a word.
class RefToBridgeObjectInst
   : public InstructionBase<PILInstructionKind::RefToBridgeObjectInst,
      OwnershipForwardingConversionInst> {
   friend PILBuilder;

   FixedOperandList<2> Operands;
   RefToBridgeObjectInst(PILDebugLocation DebugLoc, PILValue ConvertedValue,
                         PILValue MaskValue, PILType BridgeObjectTy)
      : InstructionBase(DebugLoc, BridgeObjectTy,
                        ConvertedValue.getOwnershipKind()),
        Operands(this, ConvertedValue, MaskValue) {}

public:
   PILValue getBitsOperand() const { return Operands[1].get(); }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }
};

/// Extract the heap object reference from a BridgeObject.
class ClassifyBridgeObjectInst
   : public UnaryInstructionBase<PILInstructionKind::ClassifyBridgeObjectInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   ClassifyBridgeObjectInst(PILDebugLocation DebugLoc, PILValue Operand,
                            PILType Ty)
      : UnaryInstructionBase(DebugLoc, Operand, Ty) {}
};

/// Extract the heap object reference from a BridgeObject.
class BridgeObjectToRefInst
   : public UnaryInstructionBase<PILInstructionKind::BridgeObjectToRefInst,
      OwnershipForwardingConversionInst> {
   friend PILBuilder;

   BridgeObjectToRefInst(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty)
      : UnaryInstructionBase(DebugLoc, Operand, Ty,
                             Operand.getOwnershipKind()) {}
};

/// Sets the BridgeObject to a tagged pointer representation holding its
/// operands
class ValueToBridgeObjectInst
   : public UnaryInstructionBase<PILInstructionKind::ValueToBridgeObjectInst,
      ConversionInst> {
   friend PILBuilder;

   ValueToBridgeObjectInst(PILDebugLocation DebugLoc, PILValue Operand,
                           PILType BridgeObjectTy)
      : UnaryInstructionBase(DebugLoc, Operand, BridgeObjectTy) {}
};

/// Retrieve the bit pattern of a BridgeObject.
class BridgeObjectToWordInst
   : public UnaryInstructionBase<PILInstructionKind::BridgeObjectToWordInst,
      ConversionInst>
{
   friend PILBuilder;

   BridgeObjectToWordInst(PILDebugLocation DebugLoc, PILValue Operand,
                          PILType Ty)
      : UnaryInstructionBase(DebugLoc, Operand, Ty) {}
};

/// RefToRawPointer - Convert a reference type to a Builtin.RawPointer.
class RefToRawPointerInst
   : public UnaryInstructionBase<PILInstructionKind::RefToRawPointerInst,
      ConversionInst>
{
   friend PILBuilder;

   RefToRawPointerInst(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty)
      : UnaryInstructionBase(DebugLoc, Operand, Ty) {}
};

/// RawPointerToRefInst - Convert a Builtin.RawPointer to a reference type.
class RawPointerToRefInst
   : public UnaryInstructionBase<PILInstructionKind::RawPointerToRefInst,
      ConversionInst>
{
   friend PILBuilder;

   RawPointerToRefInst(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty)
      : UnaryInstructionBase(DebugLoc, Operand, Ty) {}
};

/// Transparent reference storage to underlying reference type conversion.
/// This does nothing at runtime; it just changes the formal type.
#define LOADABLE_REF_STORAGE(Name, ...) \
class RefTo##Name##Inst \
    : public UnaryInstructionBase<PILInstructionKind::RefTo##Name##Inst, \
                                  ConversionInst> { \
  friend PILBuilder; \
  RefTo##Name##Inst(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty) \
      : UnaryInstructionBase(DebugLoc, Operand, Ty) {} \
}; \
class Name##ToRefInst \
  : public UnaryInstructionBase<PILInstructionKind::Name##ToRefInst, \
                                ConversionInst> { \
  friend PILBuilder; \
  Name##ToRefInst(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty) \
      : UnaryInstructionBase(DebugLoc, Operand, Ty) {} \
};
#include "polarphp/ast/ReferenceStorageDef.h"

/// ThinToThickFunctionInst - Given a thin function reference, adds a null
/// context to convert the value to a thick function type.
class ThinToThickFunctionInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::ThinToThickFunctionInst, ThinToThickFunctionInst,
      ConversionInst> {
   friend PILBuilder;

   ThinToThickFunctionInst(PILDebugLocation DebugLoc, PILValue Operand,
                           ArrayRef<PILValue> TypeDependentOperands, PILType Ty)
      : UnaryInstructionWithTypeDependentOperandsBase(
      DebugLoc, Operand, TypeDependentOperands, Ty) {}

   static ThinToThickFunctionInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILType Ty,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes);

public:
   /// Return the callee of the thin_to_thick_function.
   ///
   /// This is not technically necessary, but from a symmetry perspective it
   /// makes sense to follow the lead of partial_apply which also creates
   /// closures.
   PILValue getCallee() const { return getOperand(); }
};
// @todo
/// Given a thick metatype value, produces an Objective-C metatype
/// value.
//class ThickToObjCMetatypeInst
//   : public UnaryInstructionBase<PILInstructionKind::ThickToObjCMetatypeInst,
//      ConversionInst>
//{
//   friend PILBuilder;
//
//   ThickToObjCMetatypeInst(PILDebugLocation DebugLoc, PILValue Operand,
//                           PILType Ty)
//      : UnaryInstructionBase(DebugLoc, Operand, Ty) {}
//};
//
///// Given an Objective-C metatype value, produces a thick metatype
///// value.
//class ObjCToThickMetatypeInst
//   : public UnaryInstructionBase<PILInstructionKind::ObjCToThickMetatypeInst,
//      ConversionInst>
//{
//   friend PILBuilder;
//
//   ObjCToThickMetatypeInst(PILDebugLocation DebugLoc, PILValue Operand,
//                           PILType Ty)
//      : UnaryInstructionBase(DebugLoc, Operand, Ty) {}
//};
//
///// Given an Objective-C metatype value, convert it to an AnyObject value.
//class ObjCMetatypeToObjectInst
//   : public UnaryInstructionBase<PILInstructionKind::ObjCMetatypeToObjectInst,
//      ConversionInst>
//{
//   friend PILBuilder;
//
//   ObjCMetatypeToObjectInst(PILDebugLocation DebugLoc, PILValue Operand,
//                            PILType Ty)
//      : UnaryInstructionBase(DebugLoc, Operand, Ty) {}
//};
//
///// Given an Objective-C existential metatype value, convert it to an AnyObject
///// value.
//class ObjCExistentialMetatypeToObjectInst
//   : public UnaryInstructionBase<PILInstructionKind::ObjCExistentialMetatypeToObjectInst,
//      ConversionInst>
//{
//   friend PILBuilder;
//
//   ObjCExistentialMetatypeToObjectInst(PILDebugLocation DebugLoc,
//                                       PILValue Operand, PILType Ty)
//      : UnaryInstructionBase(DebugLoc, Operand, Ty) {}
//};
//
///// Return the Objective-C Interface class instance for a protocol.
//class ObjCInterfaceInst
//   : public InstructionBase<PILInstructionKind::ObjCInterfaceInst,
//      SingleValueInstruction> {
//   friend PILBuilder;
//
//   InterfaceDecl *Proto;
//   ObjCInterfaceInst(PILDebugLocation DebugLoc, InterfaceDecl *Proto, PILType Ty)
//      : InstructionBase(DebugLoc, Ty),
//        Proto(Proto) {}
//
//public:
//   InterfaceDecl *getInterface() const { return Proto; }
//
//   ArrayRef<Operand> getAllOperands() const { return {}; }
//   MutableArrayRef<Operand> getAllOperands() { return {}; }
//};


/// Perform an unconditional checked cast that aborts if the cast fails.
class UnconditionalCheckedCastInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::UnconditionalCheckedCastInst,
      UnconditionalCheckedCastInst, OwnershipForwardingConversionInst> {
   CanType DestFormalTy;
   friend PILBuilder;

   UnconditionalCheckedCastInst(PILDebugLocation DebugLoc, PILValue Operand,
                                ArrayRef<PILValue> TypeDependentOperands,
                                PILType DestLoweredTy, CanType DestFormalTy)
      : UnaryInstructionWithTypeDependentOperandsBase(
      DebugLoc, Operand, TypeDependentOperands, DestLoweredTy,
      Operand.getOwnershipKind()),
        DestFormalTy(DestFormalTy) {}

   static UnconditionalCheckedCastInst *
   create(PILDebugLocation DebugLoc, PILValue Operand,
          PILType DestLoweredTy, CanType DestFormalTy,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes);

public:
   PILType getSourceLoweredType() const { return getOperand()->getType(); }
   CanType getSourceFormalType() const { return getSourceLoweredType().getAstType(); }

   CanType getTargetFormalType() const { return DestFormalTy; }
   PILType getTargetLoweredType() const { return getType(); }
};

/// Perform an unconditional checked cast that aborts if the cast fails.
/// The result of the checked cast is left in the destination address.
class UnconditionalCheckedCastAddrInst
   : public InstructionBase<PILInstructionKind::UnconditionalCheckedCastAddrInst,
      NonValueInstruction> {
   friend PILBuilder;

   enum {
      /// the value being stored
         Src,
      /// the lvalue being stored to
         Dest
   };
   FixedOperandList<2> Operands;
   CanType SourceType;
   CanType TargetType;

   UnconditionalCheckedCastAddrInst(PILDebugLocation Loc,
                                    PILValue src, CanType sourceType,
                                    PILValue dest, CanType targetType);

public:
   PILValue getSrc() const { return Operands[Src].get(); }
   PILValue getDest() const { return Operands[Dest].get(); }

   PILType getSourceLoweredType() const { return getSrc()->getType(); }
   CanType getSourceFormalType() const { return SourceType; }

   PILType getTargetLoweredType() const { return getDest()->getType(); }
   CanType getTargetFormalType() const { return TargetType; }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }
};

/// Perform an unconditional checked cast that aborts if the cast fails.
/// The result of the checked cast is left in the destination.
class UnconditionalCheckedCastValueInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::UnconditionalCheckedCastValueInst,
      UnconditionalCheckedCastValueInst, ConversionInst> {
   CanType SourceFormalTy;
   CanType DestFormalTy;
   friend PILBuilder;

   UnconditionalCheckedCastValueInst(PILDebugLocation DebugLoc,
                                     PILValue Operand, CanType SourceFormalTy,
                                     ArrayRef<PILValue> TypeDependentOperands,
                                     PILType DestLoweredTy, CanType DestFormalTy)
      : UnaryInstructionWithTypeDependentOperandsBase(
      DebugLoc, Operand, TypeDependentOperands,
      DestLoweredTy),
        SourceFormalTy(SourceFormalTy),
        DestFormalTy(DestFormalTy) {}

   static UnconditionalCheckedCastValueInst *
   create(PILDebugLocation DebugLoc,
          PILValue Operand, CanType SourceFormalTy,
          PILType DestLoweredTy, CanType DestFormalTy,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes);

public:
   PILType getSourceLoweredType() const { return getOperand()->getType(); }
   CanType getSourceFormalType() const { return SourceFormalTy; }

   PILType getTargetLoweredType() const { return getType(); }
   CanType getTargetFormalType() const { return DestFormalTy; }
};

/// StructInst - Represents a constructed loadable struct.
class StructInst final : public InstructionBaseWithTrailingOperands<
   PILInstructionKind::StructInst, StructInst,
   OwnershipForwardingSingleValueInst> {
   friend PILBuilder;

   /// Because of the storage requirements of StructInst, object
   /// creation goes through 'create()'.
   StructInst(PILDebugLocation DebugLoc, PILType Ty, ArrayRef<PILValue> Elements,
              bool HasOwnership);

   /// Construct a StructInst.
   static StructInst *create(PILDebugLocation DebugLoc, PILType Ty,
                             ArrayRef<PILValue> Elements, PILModule &M,
                             bool HasOwnership);

public:
   /// The elements referenced by this StructInst.
   MutableArrayRef<Operand> getElementOperands() {
      return getAllOperands();
   }

   /// The elements referenced by this StructInst.
   OperandValueArrayRef getElements() const {
      return OperandValueArrayRef(getAllOperands());
   }

   PILValue getFieldValue(const VarDecl *V) const {
      return getOperandForField(V)->get();
   }

   /// Return the Operand associated with the given VarDecl.
   const Operand *getOperandForField(const VarDecl *V) const {
      return const_cast<StructInst*>(this)->getOperandForField(V);
   }

   Operand *getOperandForField(const VarDecl *V) {
      // If V is null or is computed, there is no operand associated with it.
      assert(V && V->hasStorage() &&
             "getOperandForField only works with stored fields");

      StructDecl *S = getStructDecl();

      auto Props = S->getStoredProperties();
      for (unsigned I = 0, E = Props.size(); I < E; ++I)
         if (V == Props[I])
            return &getAllOperands()[I];

      // Did not find a matching VarDecl, return nullptr.
      return nullptr;
   }

   /// Search the operands of this struct for a unique non-trivial field. If we
   /// find it, return it. Otherwise return PILValue().
   PILValue getUniqueNonTrivialFieldValue() {
      auto *F = getFunction();
      ArrayRef<Operand> Ops = getAllOperands();

      Optional<unsigned> Index;
      // For each operand...
      for (unsigned i = 0, e = Ops.size(); i != e; ++i) {
         // If the operand is not trivial...
         if (!Ops[i].get()->getType().isTrivial(*F)) {
            // And we have not found an Index yet, set index to i and continue.
            if (!Index.hasValue()) {
               Index = i;
               continue;
            }

            // Otherwise, we have two values that are non-trivial. Bail.
            return PILValue();
         }
      }

      // If we did not find an index, return an empty PILValue.
      if (!Index.hasValue())
         return PILValue();

      // Otherwise, return the value associated with index.
      return Ops[Index.getValue()].get();
   }

   StructDecl *getStructDecl() const {
      auto s = getType().getStructOrBoundGenericStruct();
      assert(s && "A struct should always have a StructDecl associated with it");
      return s;
   }
};

/// RefCountingInst - An abstract class of instructions which
/// manipulate the reference count of their object operand.
class RefCountingInst : public NonValueInstruction {
public:
   /// The atomicity of a reference counting operation to be used.
   enum class Atomicity : bool {
      /// Atomic reference counting operations should be used.
         Atomic,
      /// Non-atomic reference counting operations can be used.
         NonAtomic,
   };
protected:
   RefCountingInst(PILInstructionKind Kind, PILDebugLocation DebugLoc)
      : NonValueInstruction(Kind, DebugLoc) {
      PILInstruction::Bits.RefCountingInst.atomicity = bool(Atomicity::Atomic);
   }

public:
   void setAtomicity(Atomicity flag) {
      PILInstruction::Bits.RefCountingInst.atomicity = bool(flag);
   }
   void setNonAtomic() {
      PILInstruction::Bits.RefCountingInst.atomicity = bool(Atomicity::NonAtomic);
   }
   void setAtomic() {
      PILInstruction::Bits.RefCountingInst.atomicity = bool(Atomicity::Atomic);
   }
   Atomicity getAtomicity() const {
      return Atomicity(PILInstruction::Bits.RefCountingInst.atomicity);
   }
   bool isNonAtomic() const { return getAtomicity() == Atomicity::NonAtomic; }
   bool isAtomic() const { return getAtomicity() == Atomicity::Atomic; }

   DEFINE_ABSTRACT_NON_VALUE_INST_BOILERPLATE(RefCountingInst)
};

/// RetainValueInst - Copies a loadable value.
class RetainValueInst
   : public UnaryInstructionBase<PILInstructionKind::RetainValueInst,
      RefCountingInst> {
   friend PILBuilder;

   RetainValueInst(PILDebugLocation DebugLoc, PILValue operand,
                   Atomicity atomicity)
      : UnaryInstructionBase(DebugLoc, operand) {
      setAtomicity(atomicity);
   }
};

/// RetainValueAddrInst - Copies a loadable value by address.
class RetainValueAddrInst
   : public UnaryInstructionBase<PILInstructionKind::RetainValueAddrInst,
      RefCountingInst> {
   friend PILBuilder;

   RetainValueAddrInst(PILDebugLocation DebugLoc, PILValue operand,
                       Atomicity atomicity)
      : UnaryInstructionBase(DebugLoc, operand) {
      setAtomicity(atomicity);
   }
};

/// ReleaseValueInst - Destroys a loadable value.
class ReleaseValueInst
   : public UnaryInstructionBase<PILInstructionKind::ReleaseValueInst,
      RefCountingInst> {
   friend PILBuilder;

   ReleaseValueInst(PILDebugLocation DebugLoc, PILValue operand,
                    Atomicity atomicity)
      : UnaryInstructionBase(DebugLoc, operand) {
      setAtomicity(atomicity);
   }
};

/// ReleaseValueInst - Destroys a loadable value by address.
class ReleaseValueAddrInst
   : public UnaryInstructionBase<PILInstructionKind::ReleaseValueAddrInst,
      RefCountingInst> {
   friend PILBuilder;

   ReleaseValueAddrInst(PILDebugLocation DebugLoc, PILValue operand,
                        Atomicity atomicity)
      : UnaryInstructionBase(DebugLoc, operand) {
      setAtomicity(atomicity);
   }
};

/// Copies a loadable value in an unmanaged, unbalanced way. Only meant for use
/// in ownership qualified PIL. Please do not use this EVER unless you are
/// implementing a part of the stdlib called Unmanaged.
class UnmanagedRetainValueInst
   : public UnaryInstructionBase<PILInstructionKind::UnmanagedRetainValueInst,
      RefCountingInst> {
   friend PILBuilder;

   UnmanagedRetainValueInst(PILDebugLocation DebugLoc, PILValue operand,
                            Atomicity atomicity)
      : UnaryInstructionBase(DebugLoc, operand) {
      setAtomicity(atomicity);
   }
};

/// Destroys a loadable value in an unmanaged, unbalanced way. Only meant for
/// use in ownership qualified PIL. Please do not use this EVER unless you are
/// implementing a part of the stdlib called Unmanaged.
class UnmanagedReleaseValueInst
   : public UnaryInstructionBase<PILInstructionKind::UnmanagedReleaseValueInst,
      RefCountingInst> {
   friend PILBuilder;

   UnmanagedReleaseValueInst(PILDebugLocation DebugLoc, PILValue operand,
                             Atomicity atomicity)
      : UnaryInstructionBase(DebugLoc, operand) {
      setAtomicity(atomicity);
   }
};

/// Transfers ownership of a loadable value to the current autorelease
/// pool. Unmanaged, so it is ignored from an ownership balancing perspective.
class UnmanagedAutoreleaseValueInst
   : public UnaryInstructionBase<PILInstructionKind::UnmanagedAutoreleaseValueInst,
      RefCountingInst> {
   friend PILBuilder;

   UnmanagedAutoreleaseValueInst(PILDebugLocation DebugLoc, PILValue operand,
                                 Atomicity atomicity)
      : UnaryInstructionBase(DebugLoc, operand) {
      setAtomicity(atomicity);
   }
};

/// Transfers ownership of a loadable value to the current autorelease pool.
class AutoreleaseValueInst
   : public UnaryInstructionBase<PILInstructionKind::AutoreleaseValueInst,
      RefCountingInst> {
   friend PILBuilder;

   AutoreleaseValueInst(PILDebugLocation DebugLoc, PILValue operand,
                        Atomicity atomicity)
      : UnaryInstructionBase(DebugLoc, operand) {
      setAtomicity(atomicity);
   }
};

/// SetDeallocatingInst - Sets the operand in deallocating state.
///
/// This is the same operation what's done by a strong_release immediately
/// before it calls the deallocator of the object.
class SetDeallocatingInst
   : public UnaryInstructionBase<PILInstructionKind::SetDeallocatingInst,
      RefCountingInst> {
   friend PILBuilder;

   SetDeallocatingInst(PILDebugLocation DebugLoc, PILValue operand,
                       Atomicity atomicity)
      : UnaryInstructionBase(DebugLoc, operand) {
      setAtomicity(atomicity);
   }
};

/// ObjectInst - Represents a object value type.
///
/// This instruction can only appear at the end of a gobal variable's
/// static initializer list.
class ObjectInst final : public InstructionBaseWithTrailingOperands<
   PILInstructionKind::ObjectInst, ObjectInst,
   OwnershipForwardingSingleValueInst> {
   friend PILBuilder;

   /// Because of the storage requirements of ObjectInst, object
   /// creation goes through 'create()'.
   ObjectInst(PILDebugLocation DebugLoc, PILType Ty, ArrayRef<PILValue> Elements,
              unsigned NumBaseElements, bool HasOwnership)
      : InstructionBaseWithTrailingOperands(
      Elements, DebugLoc, Ty,
      HasOwnership ? *mergePILValueOwnership(Elements)
                   : ValueOwnershipKind(ValueOwnershipKind::None)) {
      PILInstruction::Bits.ObjectInst.NumBaseElements = NumBaseElements;
   }

   /// Construct an ObjectInst.
   static ObjectInst *create(PILDebugLocation DebugLoc, PILType Ty,
                             ArrayRef<PILValue> Elements,
                             unsigned NumBaseElements, PILModule &M,
                             bool HasOwnership);

public:
   /// All elements referenced by this ObjectInst.
   MutableArrayRef<Operand> getElementOperands() {
      return getAllOperands();
   }

   /// All elements referenced by this ObjectInst.
   OperandValueArrayRef getAllElements() const {
      return OperandValueArrayRef(getAllOperands());
   }

   /// The elements which initialize the stored properties of the object itself.
   OperandValueArrayRef getBaseElements() const {
      return OperandValueArrayRef(getAllOperands().slice(0,
                                                         PILInstruction::Bits.ObjectInst.NumBaseElements));
   }

   /// The elements which initialize the tail allocated elements.
   OperandValueArrayRef getTailElements() const {
      return OperandValueArrayRef(getAllOperands().slice(
         PILInstruction::Bits.ObjectInst.NumBaseElements));
   }
};

/// TupleInst - Represents a constructed loadable tuple.
class TupleInst final : public InstructionBaseWithTrailingOperands<
   PILInstructionKind::TupleInst, TupleInst,
   OwnershipForwardingSingleValueInst> {
   friend PILBuilder;

   /// Because of the storage requirements of TupleInst, object
   /// creation goes through 'create()'.
   TupleInst(PILDebugLocation DebugLoc, PILType Ty, ArrayRef<PILValue> Elems,
             bool HasOwnership)
      : InstructionBaseWithTrailingOperands(
      Elems, DebugLoc, Ty,
      HasOwnership ? *mergePILValueOwnership(Elems)
                   : ValueOwnershipKind(ValueOwnershipKind::None)) {}

   /// Construct a TupleInst.
   static TupleInst *create(PILDebugLocation DebugLoc, PILType Ty,
                            ArrayRef<PILValue> Elements, PILModule &M,
                            bool HasOwnership);

public:
   /// The elements referenced by this TupleInst.
   MutableArrayRef<Operand> getElementOperands() {
      return getAllOperands();
   }

   /// The elements referenced by this TupleInst.
   OperandValueArrayRef getElements() const {
      return OperandValueArrayRef(getAllOperands());
   }

   /// Return the i'th value referenced by this TupleInst.
   PILValue getElement(unsigned i) const {
      return getElements()[i];
   }

   unsigned getElementIndex(Operand *operand) {
      assert(operand->getUser() == this);
      return operand->getOperandNumber();
   }

   TupleType *getTupleType() const {
      return getType().castTo<TupleType>();
   }

   /// Search the operands of this tuple for a unique non-trivial elt. If we find
   /// it, return it. Otherwise return PILValue().
   PILValue getUniqueNonTrivialElt() {
      auto *F = getFunction();
      ArrayRef<Operand> Ops = getAllOperands();

      Optional<unsigned> Index;
      // For each operand...
      for (unsigned i = 0, e = Ops.size(); i != e; ++i) {
         // If the operand is not trivial...
         if (!Ops[i].get()->getType().isTrivial(*F)) {
            // And we have not found an Index yet, set index to i and continue.
            if (!Index.hasValue()) {
               Index = i;
               continue;
            }

            // Otherwise, we have two values that are non-trivial. Bail.
            return PILValue();
         }
      }

      // If we did not find an index, return an empty PILValue.
      if (!Index.hasValue())
         return PILValue();

      // Otherwise, return the value associated with index.
      return Ops[Index.getValue()].get();
   }
};

/// Represents a loadable enum constructed from one of its
/// elements.
class EnumInst : public InstructionBase<PILInstructionKind::EnumInst,
   OwnershipForwardingSingleValueInst> {
   friend PILBuilder;

   Optional<FixedOperandList<1>> OptionalOperand;
   EnumElementDecl *Element;

   EnumInst(PILDebugLocation DebugLoc, PILValue Operand,
            EnumElementDecl *Element, PILType ResultTy)
      : InstructionBase(DebugLoc, ResultTy,
                        Operand ? Operand.getOwnershipKind()
                                : ValueOwnershipKind(ValueOwnershipKind::None)),
        Element(Element) {
      if (Operand) {
         OptionalOperand.emplace(this, Operand);
      }
   }

public:
   EnumElementDecl *getElement() const { return Element; }

   bool hasOperand() const { return OptionalOperand.hasValue(); }
   PILValue getOperand() const { return OptionalOperand->asValueArray()[0]; }

   Operand &getOperandRef() { return OptionalOperand->asArray()[0]; }

   ArrayRef<Operand> getAllOperands() const {
      return OptionalOperand ? OptionalOperand->asArray() : ArrayRef<Operand>{};
   }

   MutableArrayRef<Operand> getAllOperands() {
      return OptionalOperand
             ? OptionalOperand->asArray() : MutableArrayRef<Operand>{};
   }
};

/// Unsafely project the data for an enum case out of an enum without checking
/// the tag.
class UncheckedEnumDataInst
   : public UnaryInstructionBase<PILInstructionKind::UncheckedEnumDataInst,
      OwnershipForwardingSingleValueInst> {
   friend PILBuilder;

   EnumElementDecl *Element;

   UncheckedEnumDataInst(PILDebugLocation DebugLoc, PILValue Operand,
                         EnumElementDecl *Element, PILType ResultTy)
      : UnaryInstructionBase(DebugLoc, Operand, ResultTy,
                             Operand.getOwnershipKind()),
        Element(Element) {}

public:
   EnumElementDecl *getElement() const { return Element; }

   EnumDecl *getEnumDecl() const {
      auto *E = getOperand()->getType().getEnumOrBoundGenericEnum();
      assert(E && "Operand of unchecked_enum_data must be of enum type");
      return E;
   }

   unsigned getElementNo() const {
      unsigned i = 0;
      for (EnumElementDecl *E : getEnumDecl()->getAllElements()) {
         if (E == Element)
            return i;
         ++i;
      }
      llvm_unreachable("An unchecked_enum_data's enumdecl should have at least "
                       "on element, the element that is being extracted");
   }
};

/// Projects the address of the data for a case inside an uninitialized enum in
/// order to initialize the payload for that case.
class InitEnumDataAddrInst
   : public UnaryInstructionBase<PILInstructionKind::InitEnumDataAddrInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   EnumElementDecl *Element;

   InitEnumDataAddrInst(PILDebugLocation DebugLoc, PILValue Operand,
                        EnumElementDecl *Element, PILType ResultTy)
      : UnaryInstructionBase(DebugLoc, Operand, ResultTy), Element(Element) {}

public:
   EnumElementDecl *getElement() const { return Element; }
};

/// InjectEnumAddrInst - Tags an enum as containing a case. The data for
/// that case, if any, must have been written into the enum first.
class InjectEnumAddrInst
   : public UnaryInstructionBase<PILInstructionKind::InjectEnumAddrInst,
      NonValueInstruction>
{
   friend PILBuilder;

   EnumElementDecl *Element;

   InjectEnumAddrInst(PILDebugLocation DebugLoc, PILValue Operand,
                      EnumElementDecl *Element)
      : UnaryInstructionBase(DebugLoc, Operand), Element(Element) {}

public:
   EnumElementDecl *getElement() const { return Element; }
};

/// Invalidate an enum value and take ownership of its payload data
/// without moving it in memory.
class UncheckedTakeEnumDataAddrInst
   : public UnaryInstructionBase<PILInstructionKind::UncheckedTakeEnumDataAddrInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   EnumElementDecl *Element;

   UncheckedTakeEnumDataAddrInst(PILDebugLocation DebugLoc, PILValue Operand,
                                 EnumElementDecl *Element, PILType ResultTy)
      : UnaryInstructionBase(DebugLoc, Operand, ResultTy), Element(Element) {}

public:
   EnumElementDecl *getElement() const { return Element; }

   EnumDecl *getEnumDecl() const {
      auto *E = getOperand()->getType().getEnumOrBoundGenericEnum();
      assert(E && "Operand of unchecked_take_enum_data_addr must be of enum"
                  " type");
      return E;
   }

   unsigned getElementNo() const {
      unsigned i = 0;
      for (EnumElementDecl *E : getEnumDecl()->getAllElements()) {
         if (E == Element)
            return i;
         ++i;
      }
      llvm_unreachable(
         "An unchecked_enum_data_addr's enumdecl should have at least "
         "on element, the element that is being extracted");
   }
};

// Abstract base class of all select instructions like select_enum,
// select_value, etc. The template parameter represents a type of case values
// to be compared with the operand of a select instruction.
//
// Subclasses must provide tail allocated storage.
// The first operand is the operand of select_xxx instruction. The rest of
// the operands are the case values and results of a select instruction.
template <class Derived, class T, class Base = SingleValueInstruction>
class SelectInstBase : public Base {
public:
   template <typename... Args>
   SelectInstBase(PILInstructionKind kind, PILDebugLocation Loc, PILType type,
                  Args &&... otherArgs)
      : Base(kind, Loc, type, std::forward<Args>(otherArgs)...) {}

   PILValue getOperand() const { return getAllOperands()[0].get(); }

   ArrayRef<Operand> getAllOperands() const {
      return static_cast<const Derived *>(this)->getAllOperands();
   }
   MutableArrayRef<Operand> getAllOperands() {
      return static_cast<Derived *>(this)->getAllOperands();
   }

   std::pair<T, PILValue> getCase(unsigned i) const {
      return static_cast<const Derived *>(this)->getCase(i);
   }

   unsigned getNumCases() const {
      return static_cast<const Derived *>(this)->getNumCases();
   }

   bool hasDefault() const {
      return static_cast<const Derived *>(this)->hasDefault();
   }

   PILValue getDefaultResult() const {
      return static_cast<const Derived *>(this)->getDefaultResult();
   }
};

/// Common base class for the select_enum and select_enum_addr instructions,
/// which select one of a set of possible results based on the case of an enum.
class SelectEnumInstBase
   : public SelectInstBase<SelectEnumInstBase, EnumElementDecl *> {
   // Tail-allocated after the operands is an array of `NumCases`
   // EnumElementDecl* pointers, referencing the case discriminators for each
   // operand.

   EnumElementDecl **getEnumElementDeclStorage();
   EnumElementDecl * const* getEnumElementDeclStorage() const {
      return const_cast<SelectEnumInstBase*>(this)->getEnumElementDeclStorage();
   }

protected:
   SelectEnumInstBase(PILInstructionKind kind, PILDebugLocation debugLoc,
                      PILType type, bool defaultValue,
                      Optional<ArrayRef<ProfileCounter>> CaseCounts,
                      ProfileCounter DefaultCount)
      : SelectInstBase(kind, debugLoc, type) {
      PILInstruction::Bits.SelectEnumInstBase.HasDefault = defaultValue;
   }
   template <typename SELECT_ENUM_INST>
   static SELECT_ENUM_INST *
   createSelectEnum(PILDebugLocation DebugLoc, PILValue Enum, PILType Type,
                    PILValue DefaultValue,
                    ArrayRef<std::pair<EnumElementDecl *, PILValue>> CaseValues,
                    PILModule &M, Optional<ArrayRef<ProfileCounter>> CaseCounts,
                    ProfileCounter DefaultCount, bool HasOwnership);

public:
   ArrayRef<Operand> getAllOperands() const;
   MutableArrayRef<Operand> getAllOperands();

   PILValue getOperand() const { return getAllOperands()[0].get(); }
   PILValue getEnumOperand() const { return getOperand(); }

   std::pair<EnumElementDecl*, PILValue>
   getCase(unsigned i) const {
      return std::make_pair(getEnumElementDeclStorage()[i],
                            getAllOperands()[i+1].get());
   }

   /// Return the value that will be used as the result for the specified enum
   /// case.
   PILValue getCaseResult(EnumElementDecl *D) {
      for (unsigned i = 0, e = getNumCases(); i != e; ++i) {
         auto Entry = getCase(i);
         if (Entry.first == D) return Entry.second;
      }
      // select_enum is required to be fully covered, so return the default if we
      // didn't find anything.
      return getDefaultResult();
   }

   /// If the default refers to exactly one case decl, return it.
   NullablePtr<EnumElementDecl> getUniqueCaseForDefault();

   bool hasDefault() const {
      return PILInstruction::Bits.SelectEnumInstBase.HasDefault;
   }

   PILValue getDefaultResult() const {
      assert(hasDefault() && "doesn't have a default");
      return getAllOperands().back().get();
   }

   unsigned getNumCases() const {
      return getAllOperands().size() - 1 - hasDefault();
   }

   /// If there is a single case that returns a literal "true" value (an
   /// "integer_literal $Builtin.Int1, 1" value), return it.
   ///
   /// FIXME: This is used to interoperate with passes that reasoned about the
   /// old enum_is_tag insn. Ideally those passes would become general enough
   /// not to need this.
   NullablePtr<EnumElementDecl> getSingleTrueElement() const;
};

/// A select enum inst that produces a static OwnershipKind.
class OwnershipForwardingSelectEnumInstBase : public SelectEnumInstBase {
   ValueOwnershipKind ownershipKind;

protected:
   OwnershipForwardingSelectEnumInstBase(
      PILInstructionKind kind, PILDebugLocation debugLoc, PILType type,
      bool defaultValue, Optional<ArrayRef<ProfileCounter>> caseCounts,
      ProfileCounter defaultCount, ValueOwnershipKind ownershipKind)
      : SelectEnumInstBase(kind, debugLoc, type, defaultValue, caseCounts,
                           defaultCount),
        ownershipKind(ownershipKind) {}

public:
   ValueOwnershipKind getOwnershipKind() const { return ownershipKind; }
   void setOwnershipKind(ValueOwnershipKind newOwnershipKind) {
      ownershipKind = newOwnershipKind;
   }
};

/// Select one of a set of values based on the case of an enum.
class SelectEnumInst final
   : public InstructionBaseWithTrailingOperands<
      PILInstructionKind::SelectEnumInst, SelectEnumInst,
      OwnershipForwardingSelectEnumInstBase, EnumElementDecl *> {
   friend PILBuilder;

private:
   friend SelectEnumInstBase;

   SelectEnumInst(PILDebugLocation DebugLoc, PILValue Operand, PILType Type,
                  bool DefaultValue, ArrayRef<PILValue> CaseValues,
                  ArrayRef<EnumElementDecl *> CaseDecls,
                  Optional<ArrayRef<ProfileCounter>> CaseCounts,
                  ProfileCounter DefaultCount, bool HasOwnership)
      : InstructionBaseWithTrailingOperands(
      Operand, CaseValues, DebugLoc, Type, bool(DefaultValue), CaseCounts,
      DefaultCount,
      HasOwnership ? *mergePILValueOwnership(CaseValues)
                   : ValueOwnershipKind(ValueOwnershipKind::None)) {
      assert(CaseValues.size() - DefaultValue == CaseDecls.size());
      std::uninitialized_copy(CaseDecls.begin(), CaseDecls.end(),
                              getTrailingObjects<EnumElementDecl *>());
   }
   static SelectEnumInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILType Type,
          PILValue DefaultValue,
          ArrayRef<std::pair<EnumElementDecl *, PILValue>> CaseValues,
          PILModule &M, Optional<ArrayRef<ProfileCounter>> CaseCounts,
          ProfileCounter DefaultCount, bool HasOwnership);
};

/// Select one of a set of values based on the case of an enum.
class SelectEnumAddrInst final
   : public InstructionBaseWithTrailingOperands<
      PILInstructionKind::SelectEnumAddrInst,
      SelectEnumAddrInst,
      SelectEnumInstBase, EnumElementDecl *> {
   friend PILBuilder;
   friend SelectEnumInstBase;

   SelectEnumAddrInst(PILDebugLocation DebugLoc, PILValue Operand, PILType Type,
                      bool DefaultValue, ArrayRef<PILValue> CaseValues,
                      ArrayRef<EnumElementDecl *> CaseDecls,
                      Optional<ArrayRef<ProfileCounter>> CaseCounts,
                      ProfileCounter DefaultCount, bool HasOwnership)
      : InstructionBaseWithTrailingOperands(Operand, CaseValues, DebugLoc, Type,
                                            bool(DefaultValue), CaseCounts,
                                            DefaultCount) {
      (void)HasOwnership;
      assert(CaseValues.size() - DefaultValue == CaseDecls.size());
      std::uninitialized_copy(CaseDecls.begin(), CaseDecls.end(),
                              getTrailingObjects<EnumElementDecl *>());
   }
   static SelectEnumAddrInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILType Type,
          PILValue DefaultValue,
          ArrayRef<std::pair<EnumElementDecl *, PILValue>> CaseValues,
          PILModule &M, Optional<ArrayRef<ProfileCounter>> CaseCounts,
          ProfileCounter DefaultCount);
};

/// Select on a value of a builtin integer type.
///
/// There is 'the' operand, followed by pairs of operands for each case,
/// followed by an optional default operand.
class SelectValueInst final
   : public InstructionBaseWithTrailingOperands<
      PILInstructionKind::SelectValueInst, SelectValueInst,
      SelectInstBase<SelectValueInst, PILValue,
         OwnershipForwardingSingleValueInst>> {
   friend PILBuilder;

   SelectValueInst(PILDebugLocation DebugLoc, PILValue Operand, PILType Type,
                   PILValue DefaultResult,
                   ArrayRef<PILValue> CaseValuesAndResults, bool HasOwnership);

   static SelectValueInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILType Type,
          PILValue DefaultValue,
          ArrayRef<std::pair<PILValue, PILValue>> CaseValues, PILModule &M,
          bool HasOwnership);

public:
   std::pair<PILValue, PILValue>
   getCase(unsigned i) const {
      auto cases = getAllOperands().slice(1);
      return {cases[i*2].get(), cases[i*2+1].get()};
   }

   unsigned getNumCases() const {
      // Ignore the first non-case operand.
      auto count = getAllOperands().size() - 1;
      // This implicitly ignore the optional default operand.
      return count / 2;
   }

   bool hasDefault() const {
      // If the operand count is even, then we have a default value.
      return (getAllOperands().size() & 1) == 0;
   }

   PILValue getDefaultResult() const {
      assert(hasDefault() && "doesn't have a default");
      return getAllOperands().back().get();
   }
};

/// MetatypeInst - Represents the production of an instance of a given metatype
/// named statically.
class MetatypeInst final
   : public InstructionBaseWithTrailingOperands<
      PILInstructionKind::MetatypeInst,
      MetatypeInst, SingleValueInstruction> {
   friend PILBuilder;

   /// Constructs a MetatypeInst
   MetatypeInst(PILDebugLocation DebugLoc, PILType Metatype,
                ArrayRef<PILValue> TypeDependentOperands)
      : InstructionBaseWithTrailingOperands(TypeDependentOperands, DebugLoc,
                                            Metatype) {}

   static MetatypeInst *create(PILDebugLocation DebugLoc, PILType Metatype,
                               PILFunction *F,
                               PILOpenedArchetypesState &OpenedArchetypes);

public:
   ArrayRef<Operand> getTypeDependentOperands() const {
      return getAllOperands();
   }

   MutableArrayRef<Operand> getTypeDependentOperands() {
      return getAllOperands();
   }
};

/// Represents loading a dynamic metatype from a value.
class ValueMetatypeInst
   : public UnaryInstructionBase<PILInstructionKind::ValueMetatypeInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   ValueMetatypeInst(PILDebugLocation DebugLoc, PILType Metatype, PILValue Base)
      : UnaryInstructionBase(DebugLoc, Base, Metatype) {}
};

/// ExistentialMetatype - Represents loading a dynamic metatype from an
/// existential container.
class ExistentialMetatypeInst
   : public UnaryInstructionBase<PILInstructionKind::ExistentialMetatypeInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   ExistentialMetatypeInst(PILDebugLocation DebugLoc, PILType Metatype,
                           PILValue Base)
      : UnaryInstructionBase(DebugLoc, Base, Metatype) {}
};

/// Extract a numbered element out of a value of tuple type.
class TupleExtractInst
   : public UnaryInstructionBase<PILInstructionKind::TupleExtractInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   TupleExtractInst(PILDebugLocation DebugLoc, PILValue Operand,
                    unsigned FieldNo, PILType ResultTy)
      : UnaryInstructionBase(DebugLoc, Operand, ResultTy) {
      PILInstruction::Bits.TupleExtractInst.FieldNo = FieldNo;
   }

public:
   unsigned getFieldNo() const {
      return PILInstruction::Bits.TupleExtractInst.FieldNo;
   }

   TupleType *getTupleType() const {
      return getOperand()->getType().castTo<TupleType>();
   }

   unsigned getNumTupleElts() const {
      return getTupleType()->getNumElements();
   }

   /// Returns true if this is a trivial result of a tuple that is non-trivial
   /// and represents one RCID.
   bool isTrivialEltOfOneRCIDTuple() const;
   bool isEltOnlyNonTrivialElt() const;
};

/// Derive the address of a numbered element from the address of a tuple.
class TupleElementAddrInst
   : public UnaryInstructionBase<PILInstructionKind::TupleElementAddrInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   TupleElementAddrInst(PILDebugLocation DebugLoc, PILValue Operand,
                        unsigned FieldNo, PILType ResultTy)
      : UnaryInstructionBase(DebugLoc, Operand, ResultTy) {
      PILInstruction::Bits.TupleElementAddrInst.FieldNo = FieldNo;
   }

public:
   unsigned getFieldNo() const {
      return PILInstruction::Bits.TupleElementAddrInst.FieldNo;
   }


   TupleType *getTupleType() const {
      return getOperand()->getType().castTo<TupleType>();
   }
};

/// A common base for instructions that require a cached field index.
///
/// "Field" is a term used here to refer to the ordered, accessible stored
/// properties of a class or struct.
///
/// The field's ordinal value is the basis of efficiently comparing and sorting
/// access paths in PIL. For example, whenever a Projection object is created,
/// it stores the field index. Finding the field index initially requires
/// searching the type declaration's array of all stored properties. If this
/// index is not cached, it will cause widespread quadratic complexity in any
/// pass that queries projections, including the PIL verifier.
///
/// FIXME: This cache may not be necessary if the Decl TypeChecker instead
/// caches a field index in the VarDecl itself. This solution would be superior
/// because it would allow constant time lookup of either the VarDecl or the
/// index from a single pointer without referring back to a projection
/// instruction.
class FieldIndexCacheBase : public SingleValueInstruction {
   enum : unsigned { InvalidFieldIndex = ~unsigned(0) };

   VarDecl *field;

public:
   FieldIndexCacheBase(PILInstructionKind kind, PILDebugLocation loc,
                       PILType type, VarDecl *field)
      : SingleValueInstruction(kind, loc, type), field(field) {
      PILInstruction::Bits.FieldIndexCacheBase.FieldIndex = InvalidFieldIndex;
      // This needs to be a concrete class to hold bitfield information. However,
      // it should only be extended by UnaryInstructions.
      assert(getNumOperands() == 1);
   }

   VarDecl *getField() const { return field; }

   // FIXME: this should be called getFieldIndex().
   unsigned getFieldNo() const {
      unsigned idx = PILInstruction::Bits.FieldIndexCacheBase.FieldIndex;
      if (idx != InvalidFieldIndex)
         return idx;

      return const_cast<FieldIndexCacheBase *>(this)->cacheFieldIndex();
   }

   NominalTypeDecl *getParentDecl() const {
      auto s = getOperand(0)->getType().getNominalOrBoundGenericNominal();
      assert(s);
      return s;
   }

private:
   unsigned cacheFieldIndex();
};

/// Extract a physical, fragile field out of a value of struct type.
class StructExtractInst
   : public UnaryInstructionBase<PILInstructionKind::StructExtractInst,
      FieldIndexCacheBase> {
   friend PILBuilder;

   StructExtractInst(PILDebugLocation DebugLoc, PILValue Operand, VarDecl *Field,
                     PILType ResultTy)
      : UnaryInstructionBase(DebugLoc, Operand, ResultTy, Field) {}

public:
   StructDecl *getStructDecl() const {
      return cast<StructDecl>(getParentDecl());
   }

   /// Returns true if this is a trivial result of a struct that is non-trivial
   /// and represents one RCID.
   bool isTrivialFieldOfOneRCIDStruct() const;

   /// Return true if we are extracting the only non-trivial field of out parent
   /// struct. This implies that a ref count operation on the aggregate is
   /// equivalent to a ref count operation on this field.
   bool isFieldOnlyNonTrivialField() const;
};

/// Derive the address of a physical field from the address of a struct.
class StructElementAddrInst
   : public UnaryInstructionBase<PILInstructionKind::StructElementAddrInst,
      FieldIndexCacheBase> {
   friend PILBuilder;

   StructElementAddrInst(PILDebugLocation DebugLoc, PILValue Operand,
                         VarDecl *Field, PILType ResultTy)
      : UnaryInstructionBase(DebugLoc, Operand, ResultTy, Field) {}

public:
   StructDecl *getStructDecl() const {
      return cast<StructDecl>(getParentDecl());
   }
};

/// RefElementAddrInst - Derive the address of a named element in a reference
/// type instance.
class RefElementAddrInst
   : public UnaryInstructionBase<PILInstructionKind::RefElementAddrInst,
      FieldIndexCacheBase> {
   friend PILBuilder;

   RefElementAddrInst(PILDebugLocation DebugLoc, PILValue Operand,
                      VarDecl *Field, PILType ResultTy)
      : UnaryInstructionBase(DebugLoc, Operand, ResultTy, Field) {}

public:
   ClassDecl *getClassDecl() const { return cast<ClassDecl>(getParentDecl()); }
};

/// RefTailAddrInst - Derive the address of the first element of the first
/// tail-allocated array in a reference type instance.
class RefTailAddrInst
   : public UnaryInstructionBase<PILInstructionKind::RefTailAddrInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   RefTailAddrInst(PILDebugLocation DebugLoc, PILValue Operand, PILType ResultTy)
      : UnaryInstructionBase(DebugLoc, Operand, ResultTy) {}

public:
   ClassDecl *getClassDecl() const {
      auto s = getOperand()->getType().getClassOrBoundGenericClass();
      assert(s);
      return s;
   }

   PILType getTailType() const { return getType().getObjectType(); }
};

/// MethodInst - Abstract base for instructions that implement dynamic
/// method lookup.
class MethodInst : public SingleValueInstruction {
   PILDeclRef Member;
public:
   MethodInst(PILInstructionKind Kind, PILDebugLocation DebugLoc, PILType Ty,
              PILDeclRef Member)
      : SingleValueInstruction(Kind, DebugLoc, Ty), Member(Member) {
   }

   PILDeclRef getMember() const { return Member; }

   DEFINE_ABSTRACT_SINGLE_VALUE_INST_BOILERPLATE(MethodInst)
};

/// ClassMethodInst - Given the address of a value of class type and a method
/// constant, extracts the implementation of that method for the dynamic
/// instance type of the class.
class ClassMethodInst
   : public UnaryInstructionBase<PILInstructionKind::ClassMethodInst,
      MethodInst>
{
   friend PILBuilder;

   ClassMethodInst(PILDebugLocation DebugLoc, PILValue Operand,
                   PILDeclRef Member, PILType Ty)
      : UnaryInstructionBase(DebugLoc, Operand, Ty, Member) {}
};

/// SuperMethodInst - Given the address of a value of class type and a method
/// constant, extracts the implementation of that method for the superclass of
/// the static type of the class.
class SuperMethodInst
   : public UnaryInstructionBase<PILInstructionKind::SuperMethodInst, MethodInst>
{
   friend PILBuilder;

   SuperMethodInst(PILDebugLocation DebugLoc, PILValue Operand,
                   PILDeclRef Member, PILType Ty)
      : UnaryInstructionBase(DebugLoc, Operand, Ty, Member) {}
};

/// ObjCMethodInst - Given the address of a value of class type and a method
/// constant, extracts the implementation of that method for the dynamic
/// instance type of the class.
class ObjCMethodInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::ObjCMethodInst,
      ObjCMethodInst,
      MethodInst>
{
   friend PILBuilder;

   ObjCMethodInst(PILDebugLocation DebugLoc, PILValue Operand,
                  ArrayRef<PILValue> TypeDependentOperands,
                  PILDeclRef Member, PILType Ty)
      : UnaryInstructionWithTypeDependentOperandsBase(DebugLoc, Operand,
                                                      TypeDependentOperands, Ty, Member) {}

   static ObjCMethodInst *
   create(PILDebugLocation DebugLoc, PILValue Operand,
          PILDeclRef Member, PILType Ty, PILFunction *F,
          PILOpenedArchetypesState &OpenedArchetypes);
};

/// ObjCSuperMethodInst - Given the address of a value of class type and a method
/// constant, extracts the implementation of that method for the superclass of
/// the static type of the class.
class ObjCSuperMethodInst
   : public UnaryInstructionBase<PILInstructionKind::ObjCSuperMethodInst, MethodInst>
{
   friend PILBuilder;

   ObjCSuperMethodInst(PILDebugLocation DebugLoc, PILValue Operand,
                       PILDeclRef Member, PILType Ty)
      : UnaryInstructionBase(DebugLoc, Operand, Ty, Member) {}
};

/// WitnessMethodInst - Given a type, a protocol conformance,
/// and a protocol method constant, extracts the implementation of that method
/// for the type.
class WitnessMethodInst final
   : public InstructionBaseWithTrailingOperands<
      PILInstructionKind::WitnessMethodInst,
      WitnessMethodInst, MethodInst> {
   friend PILBuilder;

   CanType LookupType;
   InterfaceConformanceRef Conformance;

   WitnessMethodInst(PILDebugLocation DebugLoc, CanType LookupType,
                     InterfaceConformanceRef Conformance, PILDeclRef Member,
                     PILType Ty, ArrayRef<PILValue> TypeDependentOperands)
      : InstructionBaseWithTrailingOperands(TypeDependentOperands,
                                            DebugLoc, Ty, Member),
        LookupType(LookupType), Conformance(Conformance) {}

   /// Create a witness method call of a protocol requirement, passing in a lookup
   /// type and conformance.
   ///
   /// At runtime, the witness is looked up in the conformance of the lookup type
   /// to the protocol.
   ///
   /// The lookup type is usually an archetype, but it will be concrete if the
   /// witness_method instruction is inside a function body that was specialized.
   ///
   /// The conformance must exactly match the requirement; the caller must handle
   /// the case where the requirement is defined in a base protocol that is
   /// refined by the conforming protocol.
   static WitnessMethodInst *
   create(PILDebugLocation DebugLoc, CanType LookupType,
          InterfaceConformanceRef Conformance, PILDeclRef Member, PILType Ty,
          PILFunction *Parent, PILOpenedArchetypesState &OpenedArchetypes);

public:
   CanType getLookupType() const { return LookupType; }
   InterfaceDecl *getLookupInterface() const {
      return getMember().getDecl()->getDeclContext()->getSelfInterfaceDecl();
   }

   InterfaceConformanceRef getConformance() const { return Conformance; }

   ArrayRef<Operand> getTypeDependentOperands() const {
      return getAllOperands();
   }

   MutableArrayRef<Operand> getTypeDependentOperands() {
      return getAllOperands();
   }
};

/// Access allowed to the opened value by the open_existential_addr instruction.
/// Allowing mutable access to the opened existential requires a boxed
/// existential value's box to be unique.
enum class OpenedExistentialAccess { Immutable, Mutable };

OpenedExistentialAccess getOpenedExistentialAccessFor(AccessKind access);

/// Given the address of an existential, "opens" the
/// existential by returning a pointer to a fresh archetype T, which also
/// captures the (dynamic) conformances.
class OpenExistentialAddrInst
   : public UnaryInstructionBase<PILInstructionKind::OpenExistentialAddrInst,
      SingleValueInstruction>
{
   friend PILBuilder;
   OpenedExistentialAccess ForAccess;

   OpenExistentialAddrInst(PILDebugLocation DebugLoc, PILValue Operand,
                           PILType SelfTy, OpenedExistentialAccess AccessKind);

public:
   OpenedExistentialAccess getAccessKind() const { return ForAccess; }
};

/// Given an opaque value referring to an existential, "opens" the
/// existential by returning a pointer to a fresh archetype T, which also
/// captures the (dynamic) conformances.
class OpenExistentialValueInst
   : public UnaryInstructionBase<PILInstructionKind::OpenExistentialValueInst,
      SingleValueInstruction> {
   friend PILBuilder;

   OpenExistentialValueInst(PILDebugLocation DebugLoc, PILValue Operand,
                            PILType SelfTy);
};

/// Given a class existential, "opens" the
/// existential by returning a pointer to a fresh archetype T, which also
/// captures the (dynamic) conformances.
class OpenExistentialRefInst
   : public UnaryInstructionBase<PILInstructionKind::OpenExistentialRefInst,
      OwnershipForwardingSingleValueInst> {
   friend PILBuilder;

   OpenExistentialRefInst(PILDebugLocation DebugLoc, PILValue Operand,
                          PILType Ty, bool HasOwnership);
};

/// Given an existential metatype,
/// "opens" the existential by returning a pointer to a fresh
/// archetype metatype T.Type, which also captures the (dynamic)
/// conformances.
class OpenExistentialMetatypeInst
   : public UnaryInstructionBase<PILInstructionKind::OpenExistentialMetatypeInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   OpenExistentialMetatypeInst(PILDebugLocation DebugLoc, PILValue operand,
                               PILType ty);
};

/// Given a boxed existential container,
/// "opens" the existential by returning a pointer to a fresh
/// archetype T, which also captures the (dynamic) conformances.
class OpenExistentialBoxInst
   : public UnaryInstructionBase<PILInstructionKind::OpenExistentialBoxInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   OpenExistentialBoxInst(PILDebugLocation DebugLoc, PILValue operand,
                          PILType ty);
};

/// Given a boxed existential container, "opens" the existential by returning a
/// fresh archetype T, which also captures the (dynamic) conformances.
class OpenExistentialBoxValueInst
   : public UnaryInstructionBase<PILInstructionKind::OpenExistentialBoxValueInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   OpenExistentialBoxValueInst(PILDebugLocation DebugLoc, PILValue operand,
                               PILType ty);
};

/// Given an address to an uninitialized buffer of
/// a protocol type, initializes its existential container to contain a concrete
/// value of the given type, and returns the address of the uninitialized
/// concrete value inside the existential container.
class InitExistentialAddrInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::InitExistentialAddrInst,
      InitExistentialAddrInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   CanType ConcreteType;
   ArrayRef<InterfaceConformanceRef> Conformances;

   InitExistentialAddrInst(PILDebugLocation DebugLoc, PILValue Existential,
                           ArrayRef<PILValue> TypeDependentOperands,
                           CanType ConcreteType, PILType ConcreteLoweredType,
                           ArrayRef<InterfaceConformanceRef> Conformances)
      : UnaryInstructionWithTypeDependentOperandsBase(DebugLoc, Existential,
                                                      TypeDependentOperands,
                                                      ConcreteLoweredType.getAddressType()),
        ConcreteType(ConcreteType), Conformances(Conformances) {}

   static InitExistentialAddrInst *
   create(PILDebugLocation DebugLoc, PILValue Existential, CanType ConcreteType,
          PILType ConcreteLoweredType,
          ArrayRef<InterfaceConformanceRef> Conformances, PILFunction *Parent,
          PILOpenedArchetypesState &OpenedArchetypes);

public:
   ArrayRef<InterfaceConformanceRef> getConformances() const {
      return Conformances;
   }

   CanType getFormalConcreteType() const {
      return ConcreteType;
   }

   PILType getLoweredConcreteType() const {
      return getType();
   }
};

/// Given an uninitialized buffer of a protocol type,
/// initializes its existential container to contain a concrete
/// value of the given type, and returns the uninitialized
/// concrete value inside the existential container.
class InitExistentialValueInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::InitExistentialValueInst, InitExistentialValueInst,
      SingleValueInstruction> {
   friend PILBuilder;

   CanType ConcreteType;
   ArrayRef<InterfaceConformanceRef> Conformances;

   InitExistentialValueInst(PILDebugLocation DebugLoc, PILType ExistentialType,
                            CanType FormalConcreteType, PILValue Instance,
                            ArrayRef<PILValue> TypeDependentOperands,
                            ArrayRef<InterfaceConformanceRef> Conformances)
      : UnaryInstructionWithTypeDependentOperandsBase(
      DebugLoc, Instance, TypeDependentOperands, ExistentialType),
        ConcreteType(FormalConcreteType), Conformances(Conformances) {}

   static InitExistentialValueInst *
   create(PILDebugLocation DebugLoc, PILType ExistentialType,
          CanType ConcreteType, PILValue Instance,
          ArrayRef<InterfaceConformanceRef> Conformances, PILFunction *Parent,
          PILOpenedArchetypesState &OpenedArchetypes);

public:
   CanType getFormalConcreteType() const { return ConcreteType; }

   ArrayRef<InterfaceConformanceRef> getConformances() const {
      return Conformances;
   }
};

/// InitExistentialRefInst - Given a class instance reference and a set of
/// conformances, creates a class existential value referencing the
/// class instance.
class InitExistentialRefInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::InitExistentialRefInst, InitExistentialRefInst,
      SingleValueInstruction> {
   friend PILBuilder;

   CanType ConcreteType;
   ArrayRef<InterfaceConformanceRef> Conformances;

   InitExistentialRefInst(PILDebugLocation DebugLoc, PILType ExistentialType,
                          CanType FormalConcreteType, PILValue Instance,
                          ArrayRef<PILValue> TypeDependentOperands,
                          ArrayRef<InterfaceConformanceRef> Conformances)
      : UnaryInstructionWithTypeDependentOperandsBase(
      DebugLoc, Instance, TypeDependentOperands, ExistentialType),
        ConcreteType(FormalConcreteType), Conformances(Conformances) {}

   static InitExistentialRefInst *
   create(PILDebugLocation DebugLoc, PILType ExistentialType,
          CanType ConcreteType, PILValue Instance,
          ArrayRef<InterfaceConformanceRef> Conformances, PILFunction *Parent,
          PILOpenedArchetypesState &OpenedArchetypes);

public:
   CanType getFormalConcreteType() const {
      return ConcreteType;
   }

   ArrayRef<InterfaceConformanceRef> getConformances() const {
      return Conformances;
   }
};

/// InitExistentialMetatypeInst - Given a metatype reference and a set
/// of conformances, creates an existential metatype value referencing
/// the metatype.
class InitExistentialMetatypeInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::InitExistentialMetatypeInst,
      InitExistentialMetatypeInst,
      SingleValueInstruction,
      InterfaceConformanceRef>
{
   friend PILBuilder;

   unsigned NumConformances;

   InitExistentialMetatypeInst(PILDebugLocation DebugLoc,
                               PILType existentialMetatypeType,
                               PILValue metatype,
                               ArrayRef<PILValue> TypeDependentOperands,
                               ArrayRef<InterfaceConformanceRef> conformances);

   static InitExistentialMetatypeInst *
   create(PILDebugLocation DebugLoc, PILType existentialMetatypeType,
          PILValue metatype, ArrayRef<InterfaceConformanceRef> conformances,
          PILFunction *parent, PILOpenedArchetypesState &OpenedArchetypes);

public:
   /// Return the object type which was erased.  That is, if this
   /// instruction erases Decoder<T>.Type.Type to Printable.Type.Type,
   /// this method returns Decoder<T>.
   CanType getFormalErasedObjectType() const {
      auto exType = getType().getAstType();
      auto concreteType = getOperand()->getType().getAstType();
      while (auto exMetatype = dyn_cast<ExistentialMetatypeType>(exType)) {
         exType = exMetatype.getInstanceType();
         concreteType = cast<MetatypeType>(concreteType).getInstanceType();
      }
      assert(exType.isExistentialType());
      return concreteType;
   }

   ArrayRef<InterfaceConformanceRef> getConformances() const;
};

/// DeinitExistentialAddrInst - Given an address of an existential that has been
/// partially initialized with an InitExistentialAddrInst but whose value buffer
/// has not been initialized, deinitializes the existential and deallocates
/// the value buffer. This should only be used for partially-initialized
/// existentials; a fully-initialized existential can be destroyed with
/// DestroyAddrInst and deallocated with DeallocStackInst.
class DeinitExistentialAddrInst
   : public UnaryInstructionBase<PILInstructionKind::DeinitExistentialAddrInst,
      NonValueInstruction>
{
   friend PILBuilder;

   DeinitExistentialAddrInst(PILDebugLocation DebugLoc, PILValue Existential)
      : UnaryInstructionBase(DebugLoc, Existential) {}
};

class DeinitExistentialValueInst
   : public UnaryInstructionBase<PILInstructionKind::DeinitExistentialValueInst,
      NonValueInstruction> {
   friend PILBuilder;

   DeinitExistentialValueInst(PILDebugLocation DebugLoc, PILValue Existential)
      : UnaryInstructionBase(DebugLoc, Existential) {}
};

/// Projects the capture storage address from a @block_storage address.
class ProjectBlockStorageInst
   : public UnaryInstructionBase<PILInstructionKind::ProjectBlockStorageInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   ProjectBlockStorageInst(PILDebugLocation DebugLoc, PILValue Operand,
                           PILType DestTy)
      : UnaryInstructionBase(DebugLoc, Operand, DestTy) {}
};


/// Initializes a block header, creating a block that
/// invokes a given thin cdecl function.
class InitBlockStorageHeaderInst
   : public InstructionBase<PILInstructionKind::InitBlockStorageHeaderInst,
      SingleValueInstruction> {
   friend PILBuilder;

   enum { BlockStorage, InvokeFunction };
   SubstitutionMap Substitutions;
   FixedOperandList<2> Operands;

   InitBlockStorageHeaderInst(PILDebugLocation DebugLoc, PILValue BlockStorage,
                              PILValue InvokeFunction, PILType BlockType,
                              SubstitutionMap Subs)
      : InstructionBase(DebugLoc, BlockType),
        Substitutions(Subs),
        Operands(this, BlockStorage, InvokeFunction) {
   }

   static InitBlockStorageHeaderInst *create(PILFunction &F,
                                             PILDebugLocation DebugLoc, PILValue BlockStorage,
                                             PILValue InvokeFunction, PILType BlockType,
                                             SubstitutionMap Subs);
public:
   /// Get the block storage address to be initialized.
   PILValue getBlockStorage() const { return Operands[BlockStorage].get(); }
   /// Get the invoke function to form the block around.
   PILValue getInvokeFunction() const { return Operands[InvokeFunction].get(); }

   SubstitutionMap getSubstitutions() const { return Substitutions; }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }
};

/// StrongRetainInst - Increase the strong reference count of an object.
class StrongRetainInst
   : public UnaryInstructionBase<PILInstructionKind::StrongRetainInst,
      RefCountingInst>
{
   friend PILBuilder;

   StrongRetainInst(PILDebugLocation DebugLoc, PILValue Operand,
                    Atomicity atomicity)
      : UnaryInstructionBase(DebugLoc, Operand) {
      setAtomicity(atomicity);
   }
};

/// StrongReleaseInst - Decrease the strong reference count of an object.
///
/// An object can be destroyed when its strong reference count is
/// zero.  It can be deallocated when both its strong reference and
/// weak reference counts reach zero.
class StrongReleaseInst
   : public UnaryInstructionBase<PILInstructionKind::StrongReleaseInst,
      RefCountingInst>
{
   friend PILBuilder;

   StrongReleaseInst(PILDebugLocation DebugLoc, PILValue Operand,
                     Atomicity atomicity)
      : UnaryInstructionBase(DebugLoc, Operand) {
      setAtomicity(atomicity);
   }
};

/// Simple reference storage logic.
///
/// StrongRetain##Name##Inst - Increase the strong reference count of an object
/// and assert that it has not been deallocated.
/// The operand must be of type @name.
///
/// Name##RetainInst - Increase the 'name' reference count of an object.
///
/// Name##ReleaseInst - Decrease the 'name' reference count of an object.
#define ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
class StrongRetain##Name##Inst \
    : public UnaryInstructionBase<PILInstructionKind::StrongRetain##Name##Inst,\
                                  RefCountingInst> { \
  friend PILBuilder; \
  StrongRetain##Name##Inst(PILDebugLocation DebugLoc, PILValue operand, \
                           Atomicity atomicity) \
      : UnaryInstructionBase(DebugLoc, operand) { \
    setAtomicity(atomicity); \
  } \
}; \
class Name##RetainInst \
    : public UnaryInstructionBase<PILInstructionKind::Name##RetainInst, \
                                RefCountingInst> { \
  friend PILBuilder; \
  Name##RetainInst(PILDebugLocation DebugLoc, PILValue Operand, \
                   Atomicity atomicity) \
      : UnaryInstructionBase(DebugLoc, Operand) { \
    setAtomicity(atomicity); \
  } \
}; \
class Name##ReleaseInst \
    : public UnaryInstructionBase<PILInstructionKind::Name##ReleaseInst, \
                                  RefCountingInst> { \
  friend PILBuilder; \
  Name##ReleaseInst(PILDebugLocation DebugLoc, PILValue Operand, \
                    Atomicity atomicity) \
      : UnaryInstructionBase(DebugLoc, Operand) { \
    setAtomicity(atomicity); \
  } \
};
#include "polarphp/ast/ReferenceStorageDef.h"

/// FixLifetimeInst - An artificial use of a value for the purposes of ARC or
/// RVO optimizations.
class FixLifetimeInst :
   public UnaryInstructionBase<PILInstructionKind::FixLifetimeInst,
      NonValueInstruction>
{
   friend PILBuilder;

   FixLifetimeInst(PILDebugLocation DebugLoc, PILValue Operand)
      : UnaryInstructionBase(DebugLoc, Operand) {}
};

/// EndLifetimeInst - An artificial end lifetime use of a value for the purpose
/// of working around verification problems.
///
/// Specifically, the signature of destroying deinit takes self at +0 and
/// returns self at +1. This is an issue since a deallocating deinit takes in
/// self at +1. Previously, we could rely on the deallocating bit being set in
/// the object header to allow PILGen to statically balance the +1 from the
/// deallocating deinit. This is because deallocating values used to be
/// immortal. The runtime now asserts if we release a deallocating value,
/// meaning such an approach does not work. This instruction acts as a "fake"
/// lifetime ending use allowing for static verification of deallocating
/// destroyers, without an actual release being emitted (avoiding the runtime
/// assert).
class EndLifetimeInst
   : public UnaryInstructionBase<PILInstructionKind::EndLifetimeInst,
      NonValueInstruction> {
   friend PILBuilder;

   EndLifetimeInst(PILDebugLocation DebugLoc, PILValue Operand)
      : UnaryInstructionBase(DebugLoc, Operand) {}
};

/// An unsafe conversion in between ownership kinds.
///
/// This is used today in destructors where due to Objective-C legacy
/// constraints, we need to be able to convert a guaranteed parameter to an owned
/// parameter.
class UncheckedOwnershipConversionInst
   : public UnaryInstructionBase<PILInstructionKind::UncheckedOwnershipConversionInst,
      SingleValueInstruction> {
   friend PILBuilder;

   UncheckedOwnershipConversionInst(PILDebugLocation DebugLoc, PILValue operand,
                                    ValueOwnershipKind Kind)
      : UnaryInstructionBase(DebugLoc, operand, operand->getType()) {
      PILInstruction::Bits.UncheckedOwnershipConversionInst.Kind = Kind;
   }

public:
   ValueOwnershipKind getConversionOwnershipKind() const {
      unsigned kind = PILInstruction::Bits.UncheckedOwnershipConversionInst.Kind;
      return ValueOwnershipKind(kind);
   }
};

/// Indicates that the validity of the first operand ("the value") depends on
/// the value of the second operand ("the base").  Operations that would destroy
/// the base must not be moved before any instructions which depend on the
/// result of this instruction, exactly as if the address had been obviously
/// derived from that operand (e.g. using ``ref_element_addr``). The result is
/// always equal to the first operand and thus forwards ownership through the
/// first operand. This is a "regular" use of the second operand (i.e. the
/// second operand must be live at the use point).
///
/// Example:
///
///   %base = ...
///   %value = ... @trivial value ...
///   %value_dependent_on_base = mark_dependence %value on %base
///   ...
///   use(%value_dependent_on_base)     (1)
///   ...
///   destroy_value %base               (2)
///
/// (2) can never move before (1). In English this is a way for the compiler
/// writer to say to the optimizer: 'This subset of uses of "value" (the uses of
/// result) have a dependence on "base" being alive. Do not allow for things
/// that /may/ destroy base to be moved earlier than any of these uses of
/// "value"'.
class MarkDependenceInst
   : public InstructionBase<PILInstructionKind::MarkDependenceInst,
      OwnershipForwardingSingleValueInst> {
   friend PILBuilder;

   FixedOperandList<2> Operands;

   MarkDependenceInst(PILDebugLocation DebugLoc, PILValue value, PILValue base)
      : InstructionBase(DebugLoc, value->getType(), value.getOwnershipKind()),
        Operands{this, value, base} {}

public:
   enum { Value, Base };

   PILValue getValue() const { return Operands[Value].get(); }

   PILValue getBase() const { return Operands[Base].get(); }

   void setValue(PILValue newVal) {
      Operands[Value].set(newVal);
   }
   void setBase(PILValue newVal) {
      Operands[Base].set(newVal);
   }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }
};

/// Promote an Objective-C block that is on the stack to the heap, or simply
/// retain a block that is already on the heap.
class CopyBlockInst
   : public UnaryInstructionBase<PILInstructionKind::CopyBlockInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   CopyBlockInst(PILDebugLocation DebugLoc, PILValue operand)
      : UnaryInstructionBase(DebugLoc, operand, operand->getType()) {}
};

class CopyBlockWithoutEscapingInst
   : public InstructionBase<PILInstructionKind::CopyBlockWithoutEscapingInst,
      SingleValueInstruction> {
   friend PILBuilder;

   FixedOperandList<2> Operands;

   CopyBlockWithoutEscapingInst(PILDebugLocation DebugLoc, PILValue block,
                                PILValue closure)
      : InstructionBase(DebugLoc, block->getType()), Operands{this, block,
                                                              closure} {}

public:
   enum { Block, Closure };

   PILValue getBlock() const { return Operands[Block].get(); }
   PILValue getClosure() const { return Operands[Closure].get(); }

   void setBlock(PILValue block) {
      Operands[Block].set(block);
   }
   void setClosure(PILValue closure) {
      Operands[Closure].set(closure);
   }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }
};

class CopyValueInst
   : public UnaryInstructionBase<PILInstructionKind::CopyValueInst,
      SingleValueInstruction> {
   friend class PILBuilder;

   CopyValueInst(PILDebugLocation DebugLoc, PILValue operand)
      : UnaryInstructionBase(DebugLoc, operand, operand->getType()) {}
};

#define UNCHECKED_REF_STORAGE(Name, ...)                                       \
  class StrongCopy##Name##ValueInst                                            \
      : public UnaryInstructionBase<                                           \
            PILInstructionKind::StrongCopy##Name##ValueInst,                   \
            SingleValueInstruction> {                                          \
    friend class PILBuilder;                                                   \
    StrongCopy##Name##ValueInst(PILDebugLocation DebugLoc, PILValue operand,   \
                                PILType type)                                  \
        : UnaryInstructionBase(DebugLoc, operand, type) {}                     \
  };

#define ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...)            \
  class StrongCopy##Name##ValueInst                                            \
      : public UnaryInstructionBase<                                           \
            PILInstructionKind::StrongCopy##Name##ValueInst,                   \
            SingleValueInstruction> {                                          \
    friend class PILBuilder;                                                   \
    StrongCopy##Name##ValueInst(PILDebugLocation DebugLoc, PILValue operand,   \
                                PILType type)                                  \
        : UnaryInstructionBase(DebugLoc, operand, type) {}                     \
  };
#include "polarphp/ast/ReferenceStorageDef.h"

class DestroyValueInst
   : public UnaryInstructionBase<PILInstructionKind::DestroyValueInst,
      NonValueInstruction> {
   friend class PILBuilder;

   DestroyValueInst(PILDebugLocation DebugLoc, PILValue operand)
      : UnaryInstructionBase(DebugLoc, operand) {}
};

/// Given an object reference, return true iff it is non-nil and refers
/// to a native swift object with strong reference count of 1.
class IsUniqueInst
   : public UnaryInstructionBase<PILInstructionKind::IsUniqueInst,
      SingleValueInstruction>
{
   friend PILBuilder;

   IsUniqueInst(PILDebugLocation DebugLoc, PILValue Operand, PILType BoolTy)
      : UnaryInstructionBase(DebugLoc, Operand, BoolTy) {}
};

/// Given an escaping closure return true iff it has a non-nil context and the
/// context has a strong reference count greater than 1.
class IsEscapingClosureInst
   : public UnaryInstructionBase<PILInstructionKind::IsEscapingClosureInst,
      SingleValueInstruction> {
   friend PILBuilder;

   unsigned VerificationType;

   IsEscapingClosureInst(PILDebugLocation DebugLoc, PILValue Operand,
                         PILType BoolTy, unsigned VerificationType)
      : UnaryInstructionBase(DebugLoc, Operand, BoolTy),
        VerificationType(VerificationType) {}

public:
   enum { WithoutActuallyEscaping, ObjCEscaping };

   unsigned getVerificationType() const { return VerificationType; }
};

//===----------------------------------------------------------------------===//
// DeallocationInsts
//===----------------------------------------------------------------------===//

/// DeallocationInst - An abstract parent class for Dealloc{Stack, Box, Ref}.
class DeallocationInst : public NonValueInstruction {
protected:
   DeallocationInst(PILInstructionKind Kind, PILDebugLocation DebugLoc)
      : NonValueInstruction(Kind, DebugLoc) {}

public:
   DEFINE_ABSTRACT_NON_VALUE_INST_BOILERPLATE(DeallocationInst)
};

/// DeallocStackInst - Deallocate stack memory allocated by alloc_stack.
class DeallocStackInst :
   public UnaryInstructionBase<PILInstructionKind::DeallocStackInst,
      DeallocationInst> {
   friend PILBuilder;

   DeallocStackInst(PILDebugLocation DebugLoc, PILValue operand)
      : UnaryInstructionBase(DebugLoc, operand) {}
};

/// Deallocate memory for a reference type instance from a destructor or
/// failure path of a constructor.
///
/// This does not destroy the referenced instance; it must be destroyed
/// first.
///
/// It is undefined behavior if the type of the operand does not match the
/// most derived type of the allocated instance.
class DeallocRefInst :
   public UnaryInstructionBase<PILInstructionKind::DeallocRefInst,
      DeallocationInst> {
   friend PILBuilder;

private:
   DeallocRefInst(PILDebugLocation DebugLoc, PILValue Operand,
                  bool canBeOnStack = false)
      : UnaryInstructionBase(DebugLoc, Operand) {
      PILInstruction::Bits.DeallocRefInst.OnStack = canBeOnStack;
   }

public:
   bool canAllocOnStack() const {
      return PILInstruction::Bits.DeallocRefInst.OnStack;
   }

   void setStackAllocatable(bool OnStack) {
      PILInstruction::Bits.DeallocRefInst.OnStack = OnStack;
   }
};

/// Deallocate memory for a reference type instance from a failure path of a
/// constructor.
///
/// The instance is assumed to have been partially initialized, with the
/// initialized portion being all instance variables in classes that are more
/// derived than the given metatype.
///
/// The metatype value can either be the static self type (in a designated
/// initializer) or a dynamic self type (in a convenience initializer).
class DeallocPartialRefInst
   : public InstructionBase<PILInstructionKind::DeallocPartialRefInst,
      DeallocationInst> {
   friend PILBuilder;

private:
   FixedOperandList<2> Operands;

   DeallocPartialRefInst(PILDebugLocation DebugLoc, PILValue Operand,
                         PILValue Metatype)
      : InstructionBase(DebugLoc),
        Operands(this, Operand, Metatype) {}

public:
   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }

   PILValue getInstance() const { return getOperand(0); }
   PILValue getMetatype() const { return getOperand(1); }
};

/// Deallocate memory allocated for an unsafe value buffer.
class DeallocValueBufferInst
   : public UnaryInstructionBase<PILInstructionKind::DeallocValueBufferInst,
      DeallocationInst> {
   friend PILBuilder;

   PILType ValueType;

   DeallocValueBufferInst(PILDebugLocation DebugLoc, PILType valueType,
                          PILValue operand)
      : UnaryInstructionBase(DebugLoc, operand), ValueType(valueType) {}

public:
   PILType getValueType() const { return ValueType; }
};

/// Deallocate memory allocated for a boxed value created by an AllocBoxInst.
/// It is undefined behavior if the type of the boxed type does not match the
/// type the box was allocated for.
///
/// This does not destroy the boxed value instance; it must either be
/// uninitialized or have been manually destroyed.
class DeallocBoxInst
   : public UnaryInstructionBase<PILInstructionKind::DeallocBoxInst,
      DeallocationInst>
{
   friend PILBuilder;

   DeallocBoxInst(PILDebugLocation DebugLoc, PILValue operand)
      : UnaryInstructionBase(DebugLoc, operand) {}
};

/// Deallocate memory allocated for a boxed existential container created by
/// AllocExistentialBox. It is undefined behavior if the given concrete type
/// does not match the concrete type for which the box was allocated.
///
/// This does not destroy the boxed value instance; it must either be
/// uninitialized or have been manually destroyed.
class DeallocExistentialBoxInst
   : public UnaryInstructionBase<PILInstructionKind::DeallocExistentialBoxInst,
      DeallocationInst>
{
   friend PILBuilder;

   CanType ConcreteType;

   DeallocExistentialBoxInst(PILDebugLocation DebugLoc, CanType concreteType,
                             PILValue operand)
      : UnaryInstructionBase(DebugLoc, operand), ConcreteType(concreteType) {}

public:
   CanType getConcreteType() const { return ConcreteType; }
};

/// Destroy the value at a memory location according to
/// its PIL type. This is similar to:
///   %1 = load %operand
///   release_value %1
/// but a destroy instruction can be used for types that cannot be loaded,
/// such as resilient value types.
class DestroyAddrInst
   : public UnaryInstructionBase<PILInstructionKind::DestroyAddrInst,
      NonValueInstruction>
{
   friend PILBuilder;

   DestroyAddrInst(PILDebugLocation DebugLoc, PILValue Operand)
      : UnaryInstructionBase(DebugLoc, Operand) {}
};

/// Project out the address of the value
/// stored in the given Builtin.UnsafeValueBuffer.
class ProjectValueBufferInst
   : public UnaryInstructionBase<PILInstructionKind::ProjectValueBufferInst,
      SingleValueInstruction> {
   friend PILBuilder;

   ProjectValueBufferInst(PILDebugLocation DebugLoc, PILType valueType,
                          PILValue operand)
      : UnaryInstructionBase(DebugLoc, operand, valueType.getAddressType()) {}

public:
   PILType getValueType() const { return getType().getObjectType(); }
};

/// Project out the address of the value in a box.
class ProjectBoxInst
   : public UnaryInstructionBase<PILInstructionKind::ProjectBoxInst,
      SingleValueInstruction> {
   friend PILBuilder;

   unsigned Index;

   ProjectBoxInst(PILDebugLocation DebugLoc,
                  PILValue operand,
                  unsigned fieldIndex,
                  PILType fieldTy)
      : UnaryInstructionBase(DebugLoc, operand, fieldTy.getAddressType()),
        Index(fieldIndex) {}


public:
   unsigned getFieldIndex() const { return Index; }
};

/// Project out the address of the value in an existential box.
class ProjectExistentialBoxInst
   : public UnaryInstructionBase<PILInstructionKind::ProjectExistentialBoxInst,
      SingleValueInstruction> {
   friend PILBuilder;

   ProjectExistentialBoxInst(PILDebugLocation DebugLoc, PILType valueType,
                             PILValue operand)
      : UnaryInstructionBase(DebugLoc, operand, valueType.getAddressType()) {}
};

//===----------------------------------------------------------------------===//
// Runtime failure
//===----------------------------------------------------------------------===//

/// Trigger a runtime failure if the given Int1 value is true.
///
/// Optionally cond_fail has a static failure message, which is displayed in the debugger in case the failure
/// is triggered.
class CondFailInst final
   : public UnaryInstructionBase<PILInstructionKind::CondFailInst,
      NonValueInstruction>,
     private llvm::TrailingObjects<CondFailInst, char>
{
   friend TrailingObjects;
   friend PILBuilder;

   unsigned MessageSize;

   CondFailInst(PILDebugLocation DebugLoc, PILValue Operand, StringRef Message);

   static CondFailInst *create(PILDebugLocation DebugLoc, PILValue Operand,
                               StringRef Message, PILModule &M);

public:
   StringRef getMessage() const {
      return {getTrailingObjects<char>(), MessageSize};
   }
};

//===----------------------------------------------------------------------===//
// Pointer/address indexing instructions
//===----------------------------------------------------------------------===//

/// Abstract base class for indexing instructions.
class IndexingInst : public SingleValueInstruction {
   enum { Base, Index };
   FixedOperandList<2> Operands;
public:
   IndexingInst(PILInstructionKind Kind, PILDebugLocation DebugLoc,
                PILType ResultTy, PILValue Operand, PILValue Index)
      : SingleValueInstruction(Kind, DebugLoc, ResultTy),
        Operands{this, Operand, Index} {}

   PILValue getBase() const { return Operands[Base].get(); }
   PILValue getIndex() const { return Operands[Index].get(); }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }

   DEFINE_ABSTRACT_SINGLE_VALUE_INST_BOILERPLATE(IndexingInst)
};

/// IndexAddrInst - "%2 : $*T = index_addr %0 : $*T, %1 : $Builtin.Word"
/// This takes an address and indexes it, striding by the pointed-
/// to type.  This is used to index into arrays of uniform elements.
class IndexAddrInst
   : public InstructionBase<PILInstructionKind::IndexAddrInst,
      IndexingInst> {
   friend PILBuilder;

   enum { Base, Index };

   IndexAddrInst(PILDebugLocation DebugLoc, PILValue Operand, PILValue Index)
      : InstructionBase(DebugLoc, Operand->getType(), Operand, Index) {}
};

/// TailAddrInst - like IndexingInst, but aligns-up the resulting address to a
/// tail-allocated element type.
class TailAddrInst
   : public InstructionBase<PILInstructionKind::TailAddrInst,
      IndexingInst> {
   friend PILBuilder;

   TailAddrInst(PILDebugLocation DebugLoc, PILValue Operand, PILValue Count,
                PILType ResultTy)
      : InstructionBase(DebugLoc, ResultTy, Operand, Count) {}

public:
   PILType getTailType() const { return getType().getObjectType(); }
};

/// IndexRawPointerInst
/// %2 : $Builtin.RawPointer \
///   = index_raw_pointer %0 : $Builtin.RawPointer, %1 : $Builtin.Word
/// This takes an address and indexes it, striding by the pointed-
/// to type.  This is used to index into arrays of uniform elements.
class IndexRawPointerInst
   : public InstructionBase<PILInstructionKind::IndexRawPointerInst,
      IndexingInst> {
   friend PILBuilder;

   enum { Base, Index };

   IndexRawPointerInst(PILDebugLocation DebugLoc, PILValue Operand,
                       PILValue Index)
      : InstructionBase(DebugLoc, Operand->getType(), Operand, Index) {
   }
};

//===----------------------------------------------------------------------===//
// Instructions representing terminators
//===----------------------------------------------------------------------===//

enum class TermKind {
#define TERMINATOR(Id, TextualName, Parent, MemBehavior, MayRelease) \
  Id = unsigned(PILInstructionKind::Id),
#include "polarphp/pil/lang/PILNodesDef.h"
};

/// This class defines a "terminating instruction" for a PILBasicBlock.
class TermInst : public NonValueInstruction {
protected:
   TermInst(PILInstructionKind K, PILDebugLocation DebugLoc)
      : NonValueInstruction(K, DebugLoc) {}

public:
   using ConstSuccessorListTy = ArrayRef<PILSuccessor>;
   using SuccessorListTy = MutableArrayRef<PILSuccessor>;

   /// The successor basic blocks of this terminator.
   SuccessorListTy getSuccessors();
   ConstSuccessorListTy getSuccessors() const {
      return const_cast<TermInst*>(this)->getSuccessors();
   }

   using const_succ_iterator = ConstSuccessorListTy::const_iterator;
   using succ_iterator = SuccessorListTy::iterator;

   bool succ_empty() const { return getSuccessors().empty(); }
   succ_iterator succ_begin() { return getSuccessors().begin(); }
   succ_iterator succ_end() { return getSuccessors().end(); }
   const_succ_iterator succ_begin() const { return getSuccessors().begin(); }
   const_succ_iterator succ_end() const { return getSuccessors().end(); }

   unsigned getNumSuccessors() const { return getSuccessors().size(); }

   using succblock_iterator =
   TransformIterator<PILSuccessor *,
   PILBasicBlock *(*)(const PILSuccessor &)>;
   using const_succblock_iterator = TransformIterator<
      const PILSuccessor *,
      const PILBasicBlock *(*)(const PILSuccessor &)>;
   succblock_iterator succblock_begin() {
      return succblock_iterator(getSuccessors().begin(),
                                [](const PILSuccessor &succ) -> PILBasicBlock * {
                                   return succ.getBB();
                                });
   }
   succblock_iterator succblock_end() {
      return succblock_iterator(getSuccessors().end(),
                                [](const PILSuccessor &succ) -> PILBasicBlock * {
                                   return succ.getBB();
                                });
   }
   const_succblock_iterator succblock_begin() const {
      return const_succblock_iterator(
         getSuccessors().begin(),
         [](const PILSuccessor &succ) -> const PILBasicBlock * {
            return succ.getBB();
         });
   }
   const_succblock_iterator succblock_end() const {
      return const_succblock_iterator(
         getSuccessors().end(),
         [](const PILSuccessor &succ) -> const PILBasicBlock * {
            return succ.getBB();
         });
   }

   PILBasicBlock *getSingleSuccessorBlock() {
      if (succ_empty() || std::next(succ_begin()) != succ_end())
         return nullptr;
      return *succ_begin();
   }

   const PILBasicBlock *getSingleSuccessorBlock() const {
      return const_cast<TermInst *>(this)->getSingleSuccessorBlock();
   }

   /// Returns true if \p BB is a successor of this block.
   bool isSuccessorBlock(PILBasicBlock *BB) const {
      auto Range = getSuccessorBlocks();
      return any_of(Range, [&BB](const PILBasicBlock *SuccBB) -> bool {
         return BB == SuccBB;
      });
   }

   using SuccessorBlockArgumentsListTy =
   TransformRange<ConstSuccessorListTy, function_ref<PILPhiArgumentArrayRef(
      const PILSuccessor &)>>;

   /// Return the range of Argument arrays for each successor of this
   /// block.
   SuccessorBlockArgumentsListTy getSuccessorBlockArguments() const;

   using SuccessorBlockListTy =
   TransformRange<SuccessorListTy,
   PILBasicBlock *(*)(const PILSuccessor &)>;
   using ConstSuccessorBlockListTy =
   TransformRange<ConstSuccessorListTy,
      const PILBasicBlock *(*)(const PILSuccessor &)>;

   /// Return the range of PILBasicBlocks that are successors of this block.
   SuccessorBlockListTy getSuccessorBlocks() {
      return SuccessorBlockListTy(getSuccessors(),
                                  [](const PILSuccessor &succ) -> PILBasicBlock* {
                                     return succ.getBB();
                                  });
   }

   /// Return the range of PILBasicBlocks that are successors of this block.
   ConstSuccessorBlockListTy getSuccessorBlocks() const {
      return ConstSuccessorBlockListTy(
         getSuccessors(),
         [](const PILSuccessor &succ) -> const PILBasicBlock * {
            return succ.getBB();
         });
   }

   DEFINE_ABSTRACT_NON_VALUE_INST_BOILERPLATE(TermInst)

   bool isBranch() const { return !getSuccessors().empty(); }

   /// Returns true if this terminator exits the function.
   bool isFunctionExiting() const;

   /// Returns true if this terminator terminates the program.
   bool isProgramTerminating() const;

   TermKind getTermKind() const { return TermKind(getKind()); }
};

/// UnreachableInst - Position in the code which would be undefined to reach.
/// These are always implicitly generated, e.g. when falling off the end of a
/// function or after a no-return function call.
class UnreachableInst
   : public InstructionBase<PILInstructionKind::UnreachableInst,
      TermInst> {
   friend PILBuilder;

   UnreachableInst(PILDebugLocation DebugLoc)
      : InstructionBase(DebugLoc) {}

public:
   SuccessorListTy getSuccessors() {
      // No Successors.
      return SuccessorListTy();
   }

   ArrayRef<Operand> getAllOperands() const { return {}; }
   MutableArrayRef<Operand> getAllOperands() { return {}; }
};

/// ReturnInst - Representation of a ReturnStmt.
class ReturnInst
   : public UnaryInstructionBase<PILInstructionKind::ReturnInst, TermInst>
{
   friend PILBuilder;

   /// Constructs a ReturnInst representing a return.
   ///
   /// \param DebugLoc The backing AST location.
   ///
   /// \param ReturnValue The value to be returned.
   ///
   ReturnInst(PILDebugLocation DebugLoc, PILValue ReturnValue)
      : UnaryInstructionBase(DebugLoc, ReturnValue) {}

public:
   SuccessorListTy getSuccessors() {
      // No Successors.
      return SuccessorListTy();
   }
};

/// ThrowInst - Throw a typed error (which, in our system, is
/// essentially just a funny kind of return).
class ThrowInst
   : public UnaryInstructionBase<PILInstructionKind::ThrowInst, TermInst>
{
   friend PILBuilder;

   /// Constructs a ThrowInst representing a throw out of the current
   /// function.
   ///
   /// \param DebugLoc The location of the throw.
   /// \param errorValue The value to be thrown.
   ThrowInst(PILDebugLocation DebugLoc, PILValue errorValue)
      : UnaryInstructionBase(DebugLoc, errorValue) {}

public:
   SuccessorListTy getSuccessors() {
      // No successors.
      return SuccessorListTy();
   }
};

/// UnwindInst - Continue unwinding out of this function.  Currently this is
/// only used in coroutines as the eventual terminator of the unwind edge
/// out of a 'yield'.
class UnwindInst
   : public InstructionBase<PILInstructionKind::UnwindInst,
      TermInst> {
   friend PILBuilder;

   UnwindInst(PILDebugLocation loc)
      : InstructionBase(loc) {}

public:
   SuccessorListTy getSuccessors() {
      // No successors.
      return SuccessorListTy();
   }

   ArrayRef<Operand> getAllOperands() const { return {}; }
   MutableArrayRef<Operand> getAllOperands() { return {}; }
};

/// YieldInst - Yield control temporarily to the caller of this coroutine.
///
/// This is a terminator because the caller can abort the coroutine,
/// e.g. if an error is thrown and an unwind is provoked.
class YieldInst final
   : public InstructionBaseWithTrailingOperands<PILInstructionKind::YieldInst,
      YieldInst, TermInst> {
   friend PILBuilder;

   PILSuccessor DestBBs[2];

   YieldInst(PILDebugLocation loc, ArrayRef<PILValue> yieldedValues,
             PILBasicBlock *normalBB, PILBasicBlock *unwindBB)
      : InstructionBaseWithTrailingOperands(yieldedValues, loc),
        DestBBs{{this, normalBB}, {this, unwindBB}} {

      }

   static YieldInst *create(PILDebugLocation loc,
                            ArrayRef<PILValue> yieldedValues,
                            PILBasicBlock *normalBB, PILBasicBlock *unwindBB,
                            PILFunction &F);

public:
   /// Return the normal resume destination of the yield, which is where the
   /// coroutine resumes when the caller is ready to continue normally.
   ///
   /// This must be the unique predecessor edge of the given block.
   ///
   /// Control flow along every path from this block must either loop or
   /// eventually terminate in a 'return', 'throw', or 'unreachable'
   /// instruction.  In a yield_many coroutine, control is permitted to
   /// first reach a 'yield' instruction; this is prohibited in a
   /// yield_once coroutine.
   PILBasicBlock *getResumeBB() const { return DestBBs[0]; }

   /// Return the 'unwind' destination of the yield, which is where the
   /// coroutine resumes when the caller is unconditionally aborting the
   /// coroutine.
   ///
   /// This must be the unique predecessor edge of the given block.
   ///
   /// Control flow along every path from this block must either loop or
   /// eventually terminate in an 'unwind' or 'unreachable' instruction.
   /// It is not permitted to reach a 'yield' instruction.
   PILBasicBlock *getUnwindBB() const { return DestBBs[1]; }

   OperandValueArrayRef getYieldedValues() const {
      return OperandValueArrayRef(getAllOperands());
   }

   SuccessorListTy getSuccessors() {
      return DestBBs;
   }

   PILYieldInfo getYieldInfoForOperand(const Operand &op) const;

   PILArgumentConvention
   getArgumentConventionForOperand(const Operand &op) const;
};

/// BranchInst - An unconditional branch.
class BranchInst final
   : public InstructionBaseWithTrailingOperands<PILInstructionKind::BranchInst,
      BranchInst, TermInst> {
   friend PILBuilder;

   PILSuccessor DestBB;

   BranchInst(PILDebugLocation DebugLoc, PILBasicBlock *DestBB,
              ArrayRef<PILValue> Args)
      : InstructionBaseWithTrailingOperands(Args, DebugLoc),
        DestBB(this, DestBB) {}

   /// Construct a BranchInst that will branch to the specified block.
   /// The destination block must take no parameters.
   static BranchInst *create(PILDebugLocation DebugLoc, PILBasicBlock *DestBB,
                             PILFunction &F);

   /// Construct a BranchInst that will branch to the specified block with
   /// the given parameters.
   static BranchInst *create(PILDebugLocation DebugLoc, PILBasicBlock *DestBB,
                             ArrayRef<PILValue> Args, PILFunction &F);

public:
   /// returns jump target for the branch.
   PILBasicBlock *getDestBB() const { return DestBB; }

   /// The arguments for the destination BB.
   OperandValueArrayRef getArgs() const {
      return OperandValueArrayRef(getAllOperands());
   }

   SuccessorListTy getSuccessors() {
      return SuccessorListTy(&DestBB, 1);
   }

   unsigned getNumArgs() const { return getAllOperands().size(); }
   PILValue getArg(unsigned i) const { return getAllOperands()[i].get(); }

   /// Return the PILPhiArgument for the given operand.
   ///
   /// See PILArgument.cpp.
   const PILPhiArgument *getArgForOperand(const Operand *oper) const;
};

/// A conditional branch.
class CondBranchInst final
   : public InstructionBaseWithTrailingOperands<
      PILInstructionKind::CondBranchInst,
      CondBranchInst,
      TermInst> {
   friend PILBuilder;

public:
   enum {
      /// The operand index of the condition value used for the branch.
         ConditionIdx,
      NumFixedOpers,
   };
   enum {
      // Map branch targets to block successor indices.
         TrueIdx,
      FalseIdx
   };
private:
   PILSuccessor DestBBs[2];
   /// The number of arguments for the True branch.
   unsigned getNumTrueArgs() const {
      return PILInstruction::Bits.CondBranchInst.NumTrueArgs;
   }
   /// The number of arguments for the False branch.
   unsigned getNumFalseArgs() const {
      return getAllOperands().size() - NumFixedOpers -
             PILInstruction::Bits.CondBranchInst.NumTrueArgs;
   }

   CondBranchInst(PILDebugLocation DebugLoc, PILValue Condition,
                  PILBasicBlock *TrueBB, PILBasicBlock *FalseBB,
                  ArrayRef<PILValue> Args, unsigned NumTrue, unsigned NumFalse,
                  ProfileCounter TrueBBCount, ProfileCounter FalseBBCount);

   /// Construct a CondBranchInst that will branch to TrueBB or FalseBB based on
   /// the Condition value. Both blocks must not take any arguments.
   static CondBranchInst *create(PILDebugLocation DebugLoc, PILValue Condition,
                                 PILBasicBlock *TrueBB, PILBasicBlock *FalseBB,
                                 ProfileCounter TrueBBCount,
                                 ProfileCounter FalseBBCount, PILFunction &F);

   /// Construct a CondBranchInst that will either branch to TrueBB and pass
   /// TrueArgs or branch to FalseBB and pass FalseArgs based on the Condition
   /// value.
   static CondBranchInst *
   create(PILDebugLocation DebugLoc, PILValue Condition, PILBasicBlock *TrueBB,
          ArrayRef<PILValue> TrueArgs, PILBasicBlock *FalseBB,
          ArrayRef<PILValue> FalseArgs, ProfileCounter TrueBBCount,
          ProfileCounter FalseBBCount, PILFunction &F);

public:
   const Operand *getConditionOperand() const {
      return &getAllOperands()[ConditionIdx];
   }
   PILValue getCondition() const { return getConditionOperand()->get(); }
   void setCondition(PILValue newCondition) {
      getAllOperands()[ConditionIdx].set(newCondition);
   }

   SuccessorListTy getSuccessors() {
      return DestBBs;
   }

   PILBasicBlock *getTrueBB() { return DestBBs[0]; }
   const PILBasicBlock *getTrueBB() const { return DestBBs[0]; }
   PILBasicBlock *getFalseBB() { return DestBBs[1]; }
   const PILBasicBlock *getFalseBB() const { return DestBBs[1]; }

   /// The number of times the True branch was executed.
   ProfileCounter getTrueBBCount() const { return DestBBs[0].getCount(); }
   /// The number of times the False branch was executed.
   ProfileCounter getFalseBBCount() const { return DestBBs[1].getCount(); }

   /// Get the arguments to the true BB.
   OperandValueArrayRef getTrueArgs() const {
      return OperandValueArrayRef(getTrueOperands());
   }
   /// Get the arguments to the false BB.
   OperandValueArrayRef getFalseArgs() const {
      return OperandValueArrayRef(getFalseOperands());
   }

   /// Get the operands to the true BB.
   ArrayRef<Operand> getTrueOperands() const {
      return getAllOperands().slice(NumFixedOpers, getNumTrueArgs());
   }
   MutableArrayRef<Operand> getTrueOperands() {
      return getAllOperands().slice(NumFixedOpers, getNumTrueArgs());
   }

   /// Get the operands to the false BB.
   ArrayRef<Operand> getFalseOperands() const {
      // The remaining arguments are 'false' operands.
      return getAllOperands().slice(NumFixedOpers + getNumTrueArgs());
   }
   MutableArrayRef<Operand> getFalseOperands() {
      // The remaining arguments are 'false' operands.
      return getAllOperands().slice(NumFixedOpers + getNumTrueArgs());
   }

   /// Returns true if \p op is mapped to the condition operand of the cond_br.
   bool isConditionOperand(Operand *op) const {
      return getConditionOperand() == op;
   }

   bool isConditionOperandIndex(unsigned OpIndex) const {
      assert(OpIndex < getNumOperands() &&
             "OpIndex must be an index for an actual operand");
      return OpIndex == ConditionIdx;
   }

   /// Is \p OpIndex an operand associated with the true case?
   bool isTrueOperandIndex(unsigned OpIndex) const {
      assert(OpIndex < getNumOperands() &&
             "OpIndex must be an index for an actual operand");
      if (getNumTrueArgs() == 0)
         return false;

      auto Operands = getTrueOperands();
      return Operands.front().getOperandNumber() <= OpIndex &&
             OpIndex <= Operands.back().getOperandNumber();
   }

   /// Is \p OpIndex an operand associated with the false case?
   bool isFalseOperandIndex(unsigned OpIndex) const {
      assert(OpIndex < getNumOperands() &&
             "OpIndex must be an index for an actual operand");
      if (getNumFalseArgs() == 0)
         return false;

      auto Operands = getFalseOperands();
      return Operands.front().getOperandNumber() <= OpIndex &&
             OpIndex <= Operands.back().getOperandNumber();
   }

   /// Returns the argument on the cond_br terminator that will be passed to
   /// DestBB in A.
   PILValue getArgForDestBB(const PILBasicBlock *DestBB,
                            const PILArgument *A) const;

   /// Returns the argument on the cond_br terminator that will be passed as the
   /// \p Index argument to DestBB.
   PILValue getArgForDestBB(const PILBasicBlock *DestBB,
                            unsigned ArgIndex) const;

   /// Return the PILPhiArgument from either the true or false destination for
   /// the given operand.
   ///
   /// Returns nullptr for an operand with no block argument
   /// (i.e the branch condition).
   ///
   /// See PILArgument.cpp.
   const PILPhiArgument *getArgForOperand(const Operand *oper) const;

   void swapSuccessors();
};

/// A switch on a value of a builtin type.
class SwitchValueInst final
   : public InstructionBaseWithTrailingOperands<
      PILInstructionKind::SwitchValueInst,
      SwitchValueInst, TermInst, PILSuccessor> {
   friend PILBuilder;

   SwitchValueInst(PILDebugLocation DebugLoc, PILValue Operand,
                   PILBasicBlock *DefaultBB, ArrayRef<PILValue> Cases,
                   ArrayRef<PILBasicBlock *> BBs);

   // Tail-allocated after the SwitchValueInst record are:
   // - `NumCases` PILValue values, containing
   //   the PILValue references for each case
   // - `NumCases + HasDefault` PILSuccessor records, referencing the
   //   destinations for each case, ending with the default destination if
   //   present.

   OperandValueArrayRef getCaseBuf() const {
      return OperandValueArrayRef(getAllOperands().slice(1));
   }

   PILSuccessor *getSuccessorBuf() {
      return getTrailingObjects<PILSuccessor>();
   }
   const PILSuccessor *getSuccessorBuf() const {
      return getTrailingObjects<PILSuccessor>();
   }

   static SwitchValueInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILBasicBlock *DefaultBB,
          ArrayRef<std::pair<PILValue, PILBasicBlock *>> CaseBBs,
          PILFunction &F);

public:
   /// Clean up tail-allocated successor records for the switch cases.
   ~SwitchValueInst();

   PILValue getOperand() const { return getAllOperands()[0].get(); }

   SuccessorListTy getSuccessors() {
      return MutableArrayRef<PILSuccessor>{getSuccessorBuf(),
                                           static_cast<size_t>(getNumCases() + hasDefault())};
   }

   unsigned getNumCases() const {
      return getAllOperands().size() - 1;
   }
   std::pair<PILValue, PILBasicBlock*>
   getCase(unsigned i) const {
      assert(i < getNumCases() && "case out of bounds");
      return {getCaseBuf()[i], getSuccessorBuf()[i]};
   }

   bool hasDefault() const {
      return PILInstruction::Bits.SwitchValueInst.HasDefault;
   }
   PILBasicBlock *getDefaultBB() const {
      assert(hasDefault() && "doesn't have a default");
      return getSuccessorBuf()[getNumCases()];
   }

   Optional<unsigned> getUniqueCaseForDestination(PILBasicBlock *bb) const {
      for (unsigned i = 0; i < getNumCases(); ++i) {
         if (getCase(i).second == bb) {
            return i + 1;
         }
      }
      return None;
   }
};

/// Common implementation for the switch_enum and
/// switch_enum_addr instructions.
class SwitchEnumInstBase : public TermInst {
   FixedOperandList<1> Operands;

   // Tail-allocated after the SwitchEnumInst record are:
   // - an array of `NumCases` EnumElementDecl* pointers, referencing the case
   //   discriminators
   // - `NumCases + HasDefault` PILSuccessor records, referencing the
   //   destinations for each case, ending with the default destination if
   //   present.
   // FIXME: This should use llvm::TrailingObjects, but it has subclasses
   // (which are empty, of course).

   EnumElementDecl **getCaseBuf() {
      return reinterpret_cast<EnumElementDecl**>(this + 1);

   }
   EnumElementDecl * const* getCaseBuf() const {
      return reinterpret_cast<EnumElementDecl* const*>(this + 1);

   }

   PILSuccessor *getSuccessorBuf() {
      return reinterpret_cast<PILSuccessor*>(getCaseBuf() + getNumCases());
   }
   const PILSuccessor *getSuccessorBuf() const {
      return reinterpret_cast<const PILSuccessor*>(getCaseBuf() + getNumCases());
   }

protected:
   SwitchEnumInstBase(
      PILInstructionKind Kind, PILDebugLocation DebugLoc, PILValue Operand,
      PILBasicBlock *DefaultBB,
      ArrayRef<std::pair<EnumElementDecl *, PILBasicBlock *>> CaseBBs,
      Optional<ArrayRef<ProfileCounter>> Counts, ProfileCounter DefaultCount);

   template <typename SWITCH_ENUM_INST>
   static SWITCH_ENUM_INST *createSwitchEnum(
      PILDebugLocation DebugLoc, PILValue Operand, PILBasicBlock *DefaultBB,
      ArrayRef<std::pair<EnumElementDecl *, PILBasicBlock *>> CaseBBs,
      PILFunction &F, Optional<ArrayRef<ProfileCounter>> Counts,
      ProfileCounter DefaultCount);

public:
   /// Clean up tail-allocated successor records for the switch cases.
   ~SwitchEnumInstBase();

   PILValue getOperand() const { return Operands[0].get(); }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }

   SuccessorListTy getSuccessors() {
      return MutableArrayRef<PILSuccessor>{getSuccessorBuf(),
                                           static_cast<size_t>(getNumCases() + hasDefault())};
   }

   unsigned getNumCases() const {
      return PILInstruction::Bits.SwitchEnumInstBase.NumCases;
   }
   std::pair<EnumElementDecl*, PILBasicBlock*>
   getCase(unsigned i) const {
      assert(i < getNumCases() && "case out of bounds");
      return {getCaseBuf()[i], getSuccessorBuf()[i].getBB()};
   }
   ProfileCounter getCaseCount(unsigned i) const {
      assert(i < getNumCases() && "case out of bounds");
      return getSuccessorBuf()[i].getCount();
   }

   // Swap the cases at indices \p i and \p j.
   void swapCase(unsigned i, unsigned j);

   /// Return the block that will be branched to on the specified enum
   /// case.
   PILBasicBlock *getCaseDestination(EnumElementDecl *D) {
      for (unsigned i = 0, e = getNumCases(); i != e; ++i) {
         auto Entry = getCase(i);
         if (Entry.first == D) return Entry.second;
      }
      // switch_enum is required to be fully covered, so return the default if we
      // didn't find anything.
      return getDefaultBB();
   }

   /// If the default refers to exactly one case decl, return it.
   NullablePtr<EnumElementDecl> getUniqueCaseForDefault();

   /// If the given block only has one enum element decl matched to it,
   /// return it.
   NullablePtr<EnumElementDecl> getUniqueCaseForDestination(PILBasicBlock *BB);

   bool hasDefault() const {
      return PILInstruction::Bits.SwitchEnumInstBase.HasDefault;
   }

   PILBasicBlock *getDefaultBB() const {
      assert(hasDefault() && "doesn't have a default");
      return getSuccessorBuf()[getNumCases()];
   }

   NullablePtr<PILBasicBlock> getDefaultBBOrNull() const;

   ProfileCounter getDefaultCount() const {
      assert(hasDefault() && "doesn't have a default");
      return getSuccessorBuf()[getNumCases()].getCount();
   }

   static bool classof(const PILInstruction *I) {
      return I->getKind() >= PILInstructionKind::SwitchEnumInst &&
             I->getKind() <= PILInstructionKind::SwitchEnumAddrInst;
   }
};

/// A switch on a loadable enum's discriminator. The data for each case is
/// passed into the corresponding destination block as an argument.
class SwitchEnumInst
   : public InstructionBase<PILInstructionKind::SwitchEnumInst,
      SwitchEnumInstBase> {
   friend PILBuilder;

private:
   friend SwitchEnumInstBase;

   SwitchEnumInst(
      PILDebugLocation DebugLoc, PILValue Operand, PILBasicBlock *DefaultBB,
      ArrayRef<std::pair<EnumElementDecl *, PILBasicBlock *>> CaseBBs,
      Optional<ArrayRef<ProfileCounter>> CaseCounts,
      ProfileCounter DefaultCount)
      : InstructionBase(DebugLoc, Operand, DefaultBB, CaseBBs, CaseCounts,
                        DefaultCount) {}
   static SwitchEnumInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILBasicBlock *DefaultBB,
          ArrayRef<std::pair<EnumElementDecl *, PILBasicBlock *>> CaseBBs,
          PILFunction &F, Optional<ArrayRef<ProfileCounter>> CaseCounts,
          ProfileCounter DefaultCount);
};

/// A switch on an enum's discriminator in memory.
class SwitchEnumAddrInst
   : public InstructionBase<PILInstructionKind::SwitchEnumAddrInst,
      SwitchEnumInstBase> {
   friend PILBuilder;

private:
   friend SwitchEnumInstBase;

   SwitchEnumAddrInst(
      PILDebugLocation DebugLoc, PILValue Operand, PILBasicBlock *DefaultBB,
      ArrayRef<std::pair<EnumElementDecl *, PILBasicBlock *>> CaseBBs,
      Optional<ArrayRef<ProfileCounter>> CaseCounts,
      ProfileCounter DefaultCount)
      : InstructionBase(DebugLoc, Operand, DefaultBB, CaseBBs, CaseCounts,
                        DefaultCount) {}
   static SwitchEnumAddrInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILBasicBlock *DefaultBB,
          ArrayRef<std::pair<EnumElementDecl *, PILBasicBlock *>> CaseBBs,
          PILFunction &F, Optional<ArrayRef<ProfileCounter>> CaseCounts,
          ProfileCounter DefaultCount);
};

/// Branch on the existence of an Objective-C method in the dynamic type of
/// an object.
///
/// If the method exists, branches to the first BB, providing it with the
/// method reference; otherwise, branches to the second BB.
class DynamicMethodBranchInst
   : public InstructionBase<PILInstructionKind::DynamicMethodBranchInst,
      TermInst> {
   friend PILBuilder;

   PILDeclRef Member;

   PILSuccessor DestBBs[2];

   /// The operand.
   FixedOperandList<1> Operands;

   DynamicMethodBranchInst(PILDebugLocation DebugLoc, PILValue Operand,
                           PILDeclRef Member, PILBasicBlock *HasMethodBB,
                           PILBasicBlock *NoMethodBB);

   /// Construct a DynamicMethodBranchInst that will branch to \c HasMethodBB or
   /// \c NoMethodBB based on the ability of the object operand to respond to
   /// a message with the same selector as the member.
   static DynamicMethodBranchInst *
   create(PILDebugLocation DebugLoc, PILValue Operand, PILDeclRef Member,
          PILBasicBlock *HasMethodBB, PILBasicBlock *NoMethodBB, PILFunction &F);

public:
   PILValue getOperand() const { return Operands[0].get(); }

   PILDeclRef getMember() const { return Member; }

   SuccessorListTy getSuccessors() {
      return DestBBs;
   }

   PILBasicBlock *getHasMethodBB() { return DestBBs[0]; }
   const PILBasicBlock *getHasMethodBB() const { return DestBBs[0]; }
   PILBasicBlock *getNoMethodBB() { return DestBBs[1]; }
   const PILBasicBlock *getNoMethodBB() const { return DestBBs[1]; }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }
};

/// Perform a checked cast operation and branch on whether the cast succeeds.
/// The success branch destination block receives the cast result as a BB
/// argument.
class CheckedCastBranchInst final:
   public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::CheckedCastBranchInst,
      CheckedCastBranchInst,
      TermInst> {
   friend PILBuilder;

   PILType DestLoweredTy;
   CanType DestFormalTy;
   bool IsExact;

   PILSuccessor DestBBs[2];

   CheckedCastBranchInst(PILDebugLocation DebugLoc, bool IsExact,
                         PILValue Operand,
                         ArrayRef<PILValue> TypeDependentOperands,
                         PILType DestLoweredTy, CanType DestFormalTy,
                         PILBasicBlock *SuccessBB, PILBasicBlock *FailureBB,
                         ProfileCounter Target1Count, ProfileCounter Target2Count)
      : UnaryInstructionWithTypeDependentOperandsBase(DebugLoc, Operand,
                                                      TypeDependentOperands),
        DestLoweredTy(DestLoweredTy),
        DestFormalTy(DestFormalTy),
        IsExact(IsExact), DestBBs{{this, SuccessBB, Target1Count},
                                  {this, FailureBB, Target2Count}} {}

   static CheckedCastBranchInst *
   create(PILDebugLocation DebugLoc, bool IsExact, PILValue Operand,
          PILType DestLoweredTy, CanType DestFormalTy,
          PILBasicBlock *SuccessBB, PILBasicBlock *FailureBB,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes,
          ProfileCounter Target1Count, ProfileCounter Target2Count);

public:
   bool isExact() const { return IsExact; }

   SuccessorListTy getSuccessors() {
      return DestBBs;
   }

   PILType getSourceLoweredType() const { return getOperand()->getType(); }
   CanType getSourceFormalType() const { return getSourceLoweredType().getAstType(); }

   PILType getTargetLoweredType() const { return DestLoweredTy; }
   CanType getTargetFormalType() const { return DestFormalTy; }

   PILBasicBlock *getSuccessBB() { return DestBBs[0]; }
   const PILBasicBlock *getSuccessBB() const { return DestBBs[0]; }
   PILBasicBlock *getFailureBB() { return DestBBs[1]; }
   const PILBasicBlock *getFailureBB() const { return DestBBs[1]; }

   /// The number of times the True branch was executed
   ProfileCounter getTrueBBCount() const { return DestBBs[0].getCount(); }
   /// The number of times the False branch was executed
   ProfileCounter getFalseBBCount() const { return DestBBs[1].getCount(); }
};

/// Perform a checked cast operation and branch on whether the cast succeeds.
/// The success branch destination block receives the cast result as a BB
/// argument.
class CheckedCastValueBranchInst final
   : public UnaryInstructionWithTypeDependentOperandsBase<
      PILInstructionKind::CheckedCastValueBranchInst,
      CheckedCastValueBranchInst,
      TermInst> {
   friend PILBuilder;

   CanType SourceFormalTy;
   PILType DestLoweredTy;
   CanType DestFormalTy;

   PILSuccessor DestBBs[2];

   CheckedCastValueBranchInst(PILDebugLocation DebugLoc,
                              PILValue Operand, CanType SourceFormalTy,
                              ArrayRef<PILValue> TypeDependentOperands,
                              PILType DestLoweredTy, CanType DestFormalTy,
                              PILBasicBlock *SuccessBB, PILBasicBlock *FailureBB)
      : UnaryInstructionWithTypeDependentOperandsBase(DebugLoc, Operand,
                                                      TypeDependentOperands),
        SourceFormalTy(SourceFormalTy),
        DestLoweredTy(DestLoweredTy), DestFormalTy(DestFormalTy),
        DestBBs{{this, SuccessBB}, {this, FailureBB}} {}

   static CheckedCastValueBranchInst *
   create(PILDebugLocation DebugLoc,
          PILValue Operand, CanType SourceFormalTy,
          PILType DestLoweredTy, CanType DestFormalTy,
          PILBasicBlock *SuccessBB, PILBasicBlock *FailureBB,
          PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes);

public:
   SuccessorListTy getSuccessors() { return DestBBs; }

   PILType getSourceLoweredType() const { return getOperand()->getType(); }
   CanType getSourceFormalType() const { return SourceFormalTy; }

   PILType getTargetLoweredType() const { return DestLoweredTy; }
   CanType getTargetFormalType() const { return DestFormalTy; }

   PILBasicBlock *getSuccessBB() { return DestBBs[0]; }
   const PILBasicBlock *getSuccessBB() const { return DestBBs[0]; }
   PILBasicBlock *getFailureBB() { return DestBBs[1]; }
   const PILBasicBlock *getFailureBB() const { return DestBBs[1]; }
};

/// Perform a checked cast operation and branch on whether the cast succeeds.
/// The result of the checked cast is left in the destination address.
class CheckedCastAddrBranchInst
   : public InstructionBase<PILInstructionKind::CheckedCastAddrBranchInst,
      TermInst> {
   friend PILBuilder;

   CastConsumptionKind ConsumptionKind;

   FixedOperandList<2> Operands;
   PILSuccessor DestBBs[2];

   CanType SourceType;
   CanType TargetType;

   CheckedCastAddrBranchInst(PILDebugLocation DebugLoc,
                             CastConsumptionKind consumptionKind, PILValue src,
                             CanType srcType, PILValue dest, CanType targetType,
                             PILBasicBlock *successBB, PILBasicBlock *failureBB,
                             ProfileCounter Target1Count,
                             ProfileCounter Target2Count)
      : InstructionBase(DebugLoc), ConsumptionKind(consumptionKind),
        Operands{this, src, dest}, DestBBs{{this, successBB, Target1Count},
                                           {this, failureBB, Target2Count}},
        SourceType(srcType), TargetType(targetType) {
      assert(ConsumptionKind != CastConsumptionKind::BorrowAlways &&
             "BorrowAlways is not supported on addresses");
   }

public:
   enum {
      /// the value being stored
         Src,
      /// the lvalue being stored to
         Dest
   };

   CastConsumptionKind getConsumptionKind() const { return ConsumptionKind; }

   PILValue getSrc() const { return Operands[Src].get(); }
   PILValue getDest() const { return Operands[Dest].get(); }

   PILType getSourceLoweredType() const { return getSrc()->getType(); }
   CanType getSourceFormalType() const { return SourceType; }

   PILType getTargetLoweredType() const { return getDest()->getType(); }
   CanType getTargetFormalType() const { return TargetType; }

   ArrayRef<Operand> getAllOperands() const { return Operands.asArray(); }
   MutableArrayRef<Operand> getAllOperands() { return Operands.asArray(); }

   SuccessorListTy getSuccessors() {
      return DestBBs;
   }

   PILBasicBlock *getSuccessBB() { return DestBBs[0]; }
   const PILBasicBlock *getSuccessBB() const { return DestBBs[0]; }
   PILBasicBlock *getFailureBB() { return DestBBs[1]; }
   const PILBasicBlock *getFailureBB() const { return DestBBs[1]; }

   /// The number of times the True branch was executed.
   ProfileCounter getTrueBBCount() const { return DestBBs[0].getCount(); }
   /// The number of times the False branch was executed.
   ProfileCounter getFalseBBCount() const { return DestBBs[1].getCount(); }
};

/// A private abstract class to store the destinations of a TryApplyInst.
class TryApplyInstBase : public TermInst {
public:
   enum {
      // Map branch targets to block successor indices.
         NormalIdx,
      ErrorIdx
   };
private:
   PILSuccessor DestBBs[2];

protected:
   TryApplyInstBase(PILInstructionKind valueKind, PILDebugLocation Loc,
                    PILBasicBlock *normalBB, PILBasicBlock *errorBB);

public:
   SuccessorListTy getSuccessors() {
      return DestBBs;
   }

   bool isNormalSuccessorRef(PILSuccessor *successor) const {
      assert(successor == &DestBBs[0] || successor == &DestBBs[1]);
      return successor == &DestBBs[0];
   }
   bool isErrorSuccessorRef(PILSuccessor *successor) const {
      assert(successor == &DestBBs[0] || successor == &DestBBs[1]);
      return successor == &DestBBs[1];
   }

   PILBasicBlock *getNormalBB() { return DestBBs[NormalIdx]; }
   const PILBasicBlock *getNormalBB() const { return DestBBs[NormalIdx]; }
   PILBasicBlock *getErrorBB() { return DestBBs[ErrorIdx]; }
   const PILBasicBlock *getErrorBB() const { return DestBBs[ErrorIdx]; }
};

/// TryApplyInst - Represents the full application of a function that
/// can produce an error.
class TryApplyInst final
   : public InstructionBase<PILInstructionKind::TryApplyInst,
      ApplyInstBase<TryApplyInst, TryApplyInstBase>>,
     public llvm::TrailingObjects<TryApplyInst, Operand> {
   friend PILBuilder;

   TryApplyInst(PILDebugLocation DebugLoc, PILValue callee,
                PILType substCalleeType, SubstitutionMap substitutions,
                ArrayRef<PILValue> args,
                ArrayRef<PILValue> TypeDependentOperands,
                PILBasicBlock *normalBB, PILBasicBlock *errorBB,
                const GenericSpecializationInformation *SpecializationInfo);

   static TryApplyInst *
   create(PILDebugLocation DebugLoc, PILValue callee,
          SubstitutionMap substitutions, ArrayRef<PILValue> args,
          PILBasicBlock *normalBB, PILBasicBlock *errorBB, PILFunction &F,
          PILOpenedArchetypesState &OpenedArchetypes,
          const GenericSpecializationInformation *SpecializationInfo);
};

// This is defined out of line to work around the fact that this depends on
// PartialApplyInst being defined, but PartialApplyInst is a subclass of
// ApplyInstBase, so we can not place ApplyInstBase after it.
template <class Impl, class Base>
PILValue ApplyInstBase<Impl, Base, false>::getCalleeOrigin() const {
   PILValue Callee = getCallee();
   while (true) {
      if (auto *TTTFI = dyn_cast<ThinToThickFunctionInst>(Callee)) {
         Callee = TTTFI->getCallee();
         continue;
      }
      if (auto *CFI = dyn_cast<ConvertFunctionInst>(Callee)) {
         Callee = CFI->getConverted();
         continue;
      }
      if (auto *CETN = dyn_cast<ConvertEscapeToNoEscapeInst>(Callee)) {
         Callee = CETN->getOperand();
         continue;
      }
      return Callee;
   }
}

template <class Impl, class Base>
bool ApplyInstBase<Impl, Base, false>::isCalleeDynamicallyReplaceable() const {
   PILValue Callee = getCalleeOrigin();

   while (true) {
      if (isa<FunctionRefInst>(Callee))
         return false;

      if (isa<DynamicFunctionRefInst>(Callee))
         return true;
      if (isa<PreviousDynamicFunctionRefInst>(Callee))
         return true;

      if (auto *PAI = dyn_cast<PartialApplyInst>(Callee)) {
         Callee = PAI->getCalleeOrigin();
         continue;
      }
      return false;
   }
}

template <class Impl, class Base>
PILFunction *ApplyInstBase<Impl, Base, false>::getCalleeFunction() const {
   PILValue Callee = getCalleeOrigin();

   while (true) {
      // Intentionally don't lookup throught dynamic_function_ref and
      // previous_dynamic_function_ref as the target of those functions is not
      // statically known.
      if (auto *FRI = dyn_cast<FunctionRefInst>(Callee))
         return FRI->getReferencedFunctionOrNull();

      if (auto *PAI = dyn_cast<PartialApplyInst>(Callee)) {
         Callee = PAI->getCalleeOrigin();
         continue;
      }
      return nullptr;
   }
}

/// A result for the destructure_struct instruction. See documentation for
/// destructure_struct for more information.
class DestructureStructResult final : public MultipleValueInstructionResult {
public:
   DestructureStructResult(unsigned Index, PILType Type,
                           ValueOwnershipKind OwnershipKind)
      : MultipleValueInstructionResult(ValueKind::DestructureStructResult,
                                       Index, Type, OwnershipKind) {}

   static bool classof(const PILNode *N) {
      return N->getKind() == PILNodeKind::DestructureStructResult;
   }

   DestructureStructInst *getParent();
   const DestructureStructInst *getParent() const {
      return const_cast<DestructureStructResult *>(this)->getParent();
   }
};

/// Instruction that takes in a struct value and splits the struct into the
/// struct's fields.
class DestructureStructInst final
   : public UnaryInstructionBase<PILInstructionKind::DestructureStructInst,
      MultipleValueInstruction>,
     public MultipleValueInstructionTrailingObjects<
        DestructureStructInst, DestructureStructResult> {
   friend TrailingObjects;

   DestructureStructInst(PILModule &M, PILDebugLocation Loc, PILValue Operand,
                         ArrayRef<PILType> Types,
                         ArrayRef<ValueOwnershipKind> OwnershipKinds)
      : UnaryInstructionBase(Loc, Operand),
        MultipleValueInstructionTrailingObjects(this, Types, OwnershipKinds) {}

public:
   static DestructureStructInst *create(const PILFunction &F,
                                        PILDebugLocation Loc,
                                        PILValue Operand);
   static bool classof(const PILNode *N) {
      return N->getKind() == PILNodeKind::DestructureStructInst;
   }
};

// Out of line to work around forward declaration issues.
inline DestructureStructInst *DestructureStructResult::getParent() {
   auto *Parent = MultipleValueInstructionResult::getParent();
   return cast<DestructureStructInst>(Parent);
}

/// A result for the destructure_tuple instruction. See documentation for
/// destructure_tuple for more information.
class DestructureTupleResult final : public MultipleValueInstructionResult {
public:
   DestructureTupleResult(unsigned Index, PILType Type,
                          ValueOwnershipKind OwnershipKind)
      : MultipleValueInstructionResult(ValueKind::DestructureTupleResult, Index,
                                       Type, OwnershipKind) {}

   static bool classof(const PILNode *N) {
      return N->getKind() == PILNodeKind::DestructureTupleResult;
   }

   DestructureTupleInst *getParent();
   const DestructureTupleInst *getParent() const {
      return const_cast<DestructureTupleResult *>(this)->getParent();
   }
};

/// Instruction that takes in a tuple value and splits the tuple into the
/// tuples's elements.
class DestructureTupleInst final
   : public UnaryInstructionBase<PILInstructionKind::DestructureTupleInst,
      MultipleValueInstruction>,
     public MultipleValueInstructionTrailingObjects<
        DestructureTupleInst, DestructureTupleResult> {
   friend TrailingObjects;

   DestructureTupleInst(PILModule &M, PILDebugLocation Loc, PILValue Operand,
                        ArrayRef<PILType> Types,
                        ArrayRef<ValueOwnershipKind> OwnershipKinds)
      : UnaryInstructionBase(Loc, Operand),
        MultipleValueInstructionTrailingObjects(this, Types, OwnershipKinds) {}

public:
   static DestructureTupleInst *create(const PILFunction &F,
                                       PILDebugLocation Loc,
                                       PILValue Operand);
   static bool classof(const PILNode *N) {
      return N->getKind() == PILNodeKind::DestructureTupleInst;
   }
};

// Out of line to work around forward declaration issues.
inline DestructureTupleInst *DestructureTupleResult::getParent() {
   auto *Parent = MultipleValueInstructionResult::getParent();
   return cast<DestructureTupleInst>(Parent);
}

inline PILType *AllocRefInstBase::getTypeStorage() {
   // If the size of the subclasses are equal, then all of this compiles away.
   if (auto I = dyn_cast<AllocRefInst>(this))
      return I->getTrailingObjects<PILType>();
   if (auto I = dyn_cast<AllocRefDynamicInst>(this))
      return I->getTrailingObjects<PILType>();
   llvm_unreachable("Unhandled AllocRefInstBase subclass");
}

inline ArrayRef<Operand> AllocRefInstBase::getAllOperands() const {
   // If the size of the subclasses are equal, then all of this compiles away.
   if (auto I = dyn_cast<AllocRefInst>(this))
      return I->getAllOperands();
   if (auto I = dyn_cast<AllocRefDynamicInst>(this))
      return I->getAllOperands();
   llvm_unreachable("Unhandled AllocRefInstBase subclass");
}

inline MutableArrayRef<Operand> AllocRefInstBase::getAllOperands() {
   // If the size of the subclasses are equal, then all of this compiles away.
   if (auto I = dyn_cast<AllocRefInst>(this))
      return I->getAllOperands();
   if (auto I = dyn_cast<AllocRefDynamicInst>(this))
      return I->getAllOperands();
   llvm_unreachable("Unhandled AllocRefInstBase subclass");
}

inline ArrayRef<Operand> SelectEnumInstBase::getAllOperands() const {
   // If the size of the subclasses are equal, then all of this compiles away.
   if (auto I = dyn_cast<SelectEnumInst>(this))
      return I->getAllOperands();
   if (auto I = dyn_cast<SelectEnumAddrInst>(this))
      return I->getAllOperands();
   llvm_unreachable("Unhandled SelectEnumInstBase subclass");
}

inline MutableArrayRef<Operand> SelectEnumInstBase::getAllOperands() {
   // If the size of the subclasses are equal, then all of this compiles away.
   if (auto I = dyn_cast<SelectEnumInst>(this))
      return I->getAllOperands();
   if (auto I = dyn_cast<SelectEnumAddrInst>(this))
      return I->getAllOperands();
   llvm_unreachable("Unhandled SelectEnumInstBase subclass");
}

inline EnumElementDecl **SelectEnumInstBase::getEnumElementDeclStorage() {
   // If the size of the subclasses are equal, then all of this compiles away.
   if (auto I = dyn_cast<SelectEnumInst>(this))
      return I->getTrailingObjects<EnumElementDecl*>();
   if (auto I = dyn_cast<SelectEnumAddrInst>(this))
      return I->getTrailingObjects<EnumElementDecl*>();
   llvm_unreachable("Unhandled SelectEnumInstBase subclass");
}

inline void PILSuccessor::pred_iterator::cacheBasicBlock() {
   if (Cur != nullptr) {
      Block = Cur->ContainingInst->getParent();
      assert(Block != nullptr);
   } else {
      Block = nullptr;
   }
}

// Declared in PILValue.h
inline bool Operand::isTypeDependent() const {
   return getUser()->isTypeDependentOperand(*this);
}

} // end polar namespace

//===----------------------------------------------------------------------===//
// ilist_traits for PILInstruction
//===----------------------------------------------------------------------===//

namespace llvm {

template <>
struct ilist_traits<::polar::PILInstruction> :
   public ilist_node_traits<::polar::PILInstruction> {
   using PILInstruction = ::polar::PILInstruction;

private:
   polar::PILBasicBlock *getContainingBlock();

   using instr_iterator = simple_ilist<PILInstruction>::iterator;

public:
   static void deleteNode(PILInstruction *V) {
      PILInstruction::destroy(V);
   }

   void addNodeToList(PILInstruction *I);
   void removeNodeFromList(PILInstruction *I);
   void transferNodesFromList(ilist_traits<PILInstruction> &L2,
                              instr_iterator first, instr_iterator last);

private:
   void createNode(const PILInstruction &);
};

} // end llvm namespace

#endif
