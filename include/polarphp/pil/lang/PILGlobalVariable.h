//===--- PILGlobalVariable.h - Defines PILGlobalVariable class --*- C++ -*-===//
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
// This file defines the PILGlobalVariable class.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILGLOBALVARIABLE_H
#define POLARPHP_PIL_PILGLOBALVARIABLE_H

#include "polarphp/pil/lang/PILLinkage.h"
#include "polarphp/pil/lang/PILLocation.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/lang/PILType.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/ADT/ilist.h"

namespace polar {

class AstContext;
class PILFunction;
class PILInstruction;
class PILModule;
class VarDecl;

/// A global variable that has been referenced in PIL.
class PILGlobalVariable
   : public llvm::ilist_node<PILGlobalVariable>,
     public PILAllocated<PILGlobalVariable>
{
private:
   friend class PILModule;
   friend class PILBuilder;

   /// The PIL module that the global variable belongs to.
   PILModule &Module;

   /// The mangled name of the variable, which will be propagated to the
   /// binary.  A pointer into the module's lookup table.
   StringRef Name;

   /// The lowered type of the variable.
   PILType LoweredType;

   /// The PIL location of the variable, which provides a link back to the AST.
   /// The variable only gets a location after it's been emitted.
   Optional<PILLocation> Location;

   /// The linkage of the global variable.
   unsigned Linkage : NumPILLinkageBits;

   /// The global variable's serialized attribute.
   /// Serialized means that the variable can be "inlined" into another module.
   /// Currently this flag is set for all global variables in the stdlib.
   unsigned Serialized : 1;

   /// Whether this is a 'let' property, which can only be initialized
   /// once (either in its declaration, or once later), making it immutable.
   unsigned IsLet : 1;

   /// The VarDecl associated with this PILGlobalVariable. Must by nonnull for
   /// language-level global variables.
   VarDecl *VDecl;

   /// Whether or not this is a declaration.
   bool IsDeclaration;

   /// If this block is not empty, the global variable has a static initializer.
   ///
   /// The last instruction of this block is the top-level value of the static
   /// initializer.
   ///
   /// The block is just used as a container for the instructions. So the
   /// instructions still have a parent PILBasicBlock (but no parent function).
   /// It would be somehow cleaner to just store an instruction list here and
   /// make the PILGlobalVariable the parent pointer of the instructions.
   PILBasicBlock StaticInitializerBlock;

   PILGlobalVariable(PILModule &M, PILLinkage linkage,
                     IsSerialized_t IsSerialized,
                     StringRef mangledName, PILType loweredType,
                     Optional<PILLocation> loc, VarDecl *decl);

public:
   static PILGlobalVariable *create(PILModule &Module, PILLinkage Linkage,
                                    IsSerialized_t IsSerialized,
                                    StringRef MangledName, PILType LoweredType,
                                    Optional<PILLocation> Loc = None,
                                    VarDecl *Decl = nullptr);

   ~PILGlobalVariable();

   PILModule &getModule() const { return Module; }

   PILType getLoweredType() const { return LoweredType; }
   CanPILFunctionType getLoweredFunctionType() const {
      return LoweredType.castTo<PILFunctionType>();
   }
   PILType getLoweredTypeInContext(TypeExpansionContext context) const;
   CanPILFunctionType
   getLoweredFunctionTypeInContext(TypeExpansionContext context) const {
      return getLoweredTypeInContext(context).castTo<PILFunctionType>();
   }

   StringRef getName() const { return Name; }

   void setDeclaration(bool isD) { IsDeclaration = isD; }

   /// True if this is a definition of the variable.
   bool isDefinition() const { return !IsDeclaration; }

   /// Get this function's linkage attribute.
   PILLinkage getLinkage() const { return PILLinkage(Linkage); }
   void setLinkage(PILLinkage linkage) { Linkage = unsigned(linkage); }

   /// Get this global variable's serialized attribute.
   IsSerialized_t isSerialized() const;
   void setSerialized(IsSerialized_t isSerialized);

   /// Is this an immutable 'let' property?
   bool isLet() const { return IsLet; }
   void setLet(bool isLet) { IsLet = isLet; }

   VarDecl *getDecl() const { return VDecl; }

   /// Initialize the source location of the function.
   void setLocation(PILLocation L) { Location = L; }

   /// Check if the function has a location.
   /// FIXME: All functions should have locations, so this method should not be
   /// necessary.
   bool hasLocation() const {
      return Location.hasValue();
   }

   /// Get the source location of the function.
   PILLocation getLocation() const {
      assert(Location.hasValue());
      return Location.getValue();
   }

   /// Returns the value of the static initializer or null if the global has no
   /// static initializer.
   PILInstruction *getStaticInitializerValue();

   /// Returns true if the global is a statically initialized heap object.
   bool isInitializedObject() {
      return dyn_cast_or_null<ObjectInst>(getStaticInitializerValue()) != nullptr;
   }

   /// Returns true if \p I is a valid instruction to be contained in the
   /// static initializer.
   static bool isValidStaticInitializerInst(const PILInstruction *I,
                                            PILModule &M);

   /// Returns the usub_with_overflow builtin if \p TE extracts the result of
   /// such a subtraction, which is required to have an integer_literal as right
   /// operand.
   static BuiltinInst *getOffsetSubtract(const TupleExtractInst *TE, PILModule &M);

   void dropAllReferences() {
      StaticInitializerBlock.dropAllReferences();
   }

   /// Return whether this variable corresponds to a Clang node.
   bool hasClangNode() const;

   /// Return the Clang node associated with this variable if it has one.
   ClangNode getClangNode() const;

   const clang::Decl *getClangDecl() const;

   //===--------------------------------------------------------------------===//
   // Miscellaneous
   //===--------------------------------------------------------------------===//

   /// verify - Run the IR verifier to make sure that the variable follows
   /// invariants.
   void verify() const;

   /// Pretty-print the variable.
   void dump(bool Verbose) const;
   /// Pretty-print the variable.
   ///
   /// This is a separate entry point for ease of debugging.
   void dump() const LLVM_ATTRIBUTE_USED;

   /// Pretty-print the variable to the designated stream as a 'sil_global'
   /// definition.
   ///
   /// \param Verbose In verbose mode, print the PIL locations.
   void print(raw_ostream &OS, bool Verbose = false) const;

   /// Pretty-print the variable name using PIL syntax,
   /// '@var_mangled_name'.
   void printName(raw_ostream &OS) const;

   AstContext &getAstContext() const;
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                     const PILGlobalVariable &F) {
   F.print(OS);
   return OS;
}

} // end swift namespace

//===----------------------------------------------------------------------===//
// ilist_traits for PILGlobalVariable
//===----------------------------------------------------------------------===//

namespace llvm {

template <>
struct ilist_traits<::polar::PILGlobalVariable> :
   public ilist_node_traits<::polar::PILGlobalVariable> {
   using PILGlobalVariable = ::polar::PILGlobalVariable;

public:
   static void deleteNode(PILGlobalVariable *V) {}

private:
   void createNode(const PILGlobalVariable &);
};

} // end llvm namespace

//===----------------------------------------------------------------------===//
// Utilities for verification and optimization.
//===----------------------------------------------------------------------===//

namespace polar {

/// Given an addressor, AddrF, return the global variable being addressed, or
/// return nullptr if the addressor isn't a recognized pattern.
PILGlobalVariable *getVariableOfGlobalInit(PILFunction *AddrF);

/// Return the callee of a once call.
PILFunction *getCalleeOfOnceCall(BuiltinInst *BI);

/// Helper for getVariableOfGlobalInit(), so GlobalOpts can deeply inspect and
/// rewrite the initialization pattern.
///
/// Given an addressor, AddrF, find the call to the global initializer if
/// present, otherwise return null. If an initializer is returned, then
/// `CallToOnce` is initialized to the corresponding builtin "once" call.
PILFunction *findInitializer(PILModule *Module, PILFunction *AddrF,
                             BuiltinInst *&CallToOnce);

/// Helper for getVariableOfGlobalInit(), so GlobalOpts can deeply inspect and
/// rewrite the initialization pattern.
///
/// Given a global initializer, InitFunc, return the GlobalVariable that it
/// statically initializes or return nullptr if it isn't an obvious static
/// initializer. If a global variable is returned, InitVal is initialized to the
/// the instruction producing the global's initial value.
PILGlobalVariable *getVariableOfStaticInitializer(
   PILFunction *InitFunc, SingleValueInstruction *&InitVal);

} // namespace polar

#endif
