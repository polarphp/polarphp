//===--- PILGenGlobalVariable.cpp - Lowering for global variables ---------===//
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

#include "polarphp/pil/gen/PILGenFunction.h"
#include "polarphp/pil/gen/ManagedValue.h"
#include "polarphp/pil/gen/Scope.h"
#include "polarphp/ast/AstMangler.h"
#include "polarphp/ast/GenericSignature.h"
#include "polarphp/pil/lang/FormalLinkage.h"

using namespace polar;
using namespace lowering;

/// Get or create PILGlobalVariable for a given global VarDecl.
PILGlobalVariable *PILGenModule::getPILGlobalVariable(VarDecl *gDecl,
                                                      ForDefinition_t forDef) {
   // First, get a mangled name for the declaration.
   std::string mangledName;

   {
      auto PILGenName = gDecl->getAttrs().getAttribute<PILGenNameAttr>();
      if (PILGenName && !PILGenName->Name.empty()) {
         mangledName = PILGenName->Name;
      } else {
         mangle::AstMangler NewMangler;
         mangledName = NewMangler.mangleGlobalVariableFull(gDecl);
      }
   }

   // Get the linkage for PILGlobalVariable.
   FormalLinkage formalLinkage;
   if (gDecl->isResilient())
      formalLinkage = FormalLinkage::Private;
   else
      formalLinkage = getDeclLinkage(gDecl);
   auto silLinkage = getPILLinkage(formalLinkage, forDef);

   // Check if it is already created, and update linkage if necessary.
   if (auto gv = M.lookUpGlobalVariable(mangledName)) {
      // Update the PILLinkage here if this is a definition.
      if (forDef == ForDefinition) {
         gv->setLinkage(silLinkage);
         gv->setDeclaration(false);
      }
      return gv;
   }

   PILType silTy = PILType::getPrimitiveObjectType(
      M.Types.getLoweredTypeOfGlobal(gDecl));

   auto *pilGlobal = PILGlobalVariable::create(M, silLinkage, IsNotSerialized,
                                               mangledName, silTy,
                                               None, gDecl);
   pilGlobal->setDeclaration(!forDef);

   return pilGlobal;
}

ManagedValue
PILGenFunction::emitGlobalVariableRef(PILLocation loc, VarDecl *var) {
   assert(!VarLocs.count(var));

   if (var->isLazilyInitializedGlobal()) {
      // Call the global accessor to get the variable's address.
      PILFunction *accessorFn = SGM.getFunction(
         PILDeclRef(var, PILDeclRef::Kind::GlobalAccessor),
         NotForDefinition);
      PILValue accessor = B.createFunctionRefFor(loc, accessorFn);
      PILValue addr = B.createApply(loc, accessor, SubstitutionMap(), {});
      // FIXME: It'd be nice if the result of the accessor was natively an
      // address.
      addr = B.createPointerToAddress(
         loc, addr, getLoweredType(var->getInterfaceType()).getAddressType(),
         /*isStrict*/ true, /*isInvariant*/ false);
      return ManagedValue::forLValue(addr);
   }

   // Global variables can be accessed directly with global_addr.  Emit this
   // instruction into the prolog of the function so we can memoize/CSE it in
   // VarLocs.
   auto *entryBB = &*getFunction().begin();
   PILGenBuilder prologueB(*this, entryBB, entryBB->begin());
   prologueB.setTrackingList(B.getTrackingList());

   auto *pilG = SGM.getPILGlobalVariable(var, NotForDefinition);
   PILValue addr = prologueB.createGlobalAddr(var, pilG);

   VarLocs[var] = PILGenFunction::VarLoc::get(addr);
   return ManagedValue::forLValue(addr);
}

//===----------------------------------------------------------------------===//
// Global initialization
//===----------------------------------------------------------------------===//

namespace {

/// A visitor for traversing a pattern, creating
/// global accessor functions for all of the global variables declared inside.
struct GenGlobalAccessors : public PatternVisitor<GenGlobalAccessors>
{
   /// The module generator.
   PILGenModule &SGM;
   /// The Builtin.once token guarding the global initialization.
   PILGlobalVariable *OnceToken;
   /// The function containing the initialization code.
   PILFunction *OnceFunc;

   /// A reference to the Builtin.once declaration.
   FuncDecl *BuiltinOnceDecl;

   GenGlobalAccessors(PILGenModule &SGM,
                      PILGlobalVariable *OnceToken,
                      PILFunction *OnceFunc)
      : SGM(SGM), OnceToken(OnceToken), OnceFunc(OnceFunc)
   {
      // Find Builtin.once.
      auto &C = SGM.M.getAstContext();
      SmallVector<ValueDecl*, 2> found;
      C.TheBuiltinModule->lookupValue(C.getIdentifier("once"),
                                      NLKind::QualifiedLookup, found);

      assert(found.size() == 1 && "didn't find Builtin.once?!");

      BuiltinOnceDecl = cast<FuncDecl>(found[0]);
   }

   // Walk through non-binding patterns.
   void visitParenPattern(ParenPattern *P) {
      return visit(P->getSubPattern());
   }
   void visitTypedPattern(TypedPattern *P) {
      return visit(P->getSubPattern());
   }
   void visitVarPattern(VarPattern *P) {
      return visit(P->getSubPattern());
   }
   void visitTuplePattern(TuplePattern *P) {
      for (auto &elt : P->getElements())
         visit(elt.getPattern());
   }
   void visitAnyPattern(AnyPattern *P) {}

   // When we see a variable binding, emit its global accessor.
   void visitNamedPattern(NamedPattern *P) {
      SGM.emitGlobalAccessor(P->getDecl(), OnceToken, OnceFunc);
   }

#define INVALID_PATTERN(Id, Parent) \
  void visit##Id##Pattern(Id##Pattern *) { \
    llvm_unreachable("pattern not valid in argument or var binding"); \
  }
#define PATTERN(Id, Parent)
#define REFUTABLE_PATTERN(Id, Parent) INVALID_PATTERN(Id, Parent)
#include "polarphp/ast/PatternNodesDef.h"
#undef INVALID_PATTERN
};

} // end anonymous namespace

/// Emit a global initialization.
void PILGenModule::emitGlobalInitialization(PatternBindingDecl *pd,
                                            unsigned pbdEntry) {
   // Generic and dynamic static properties require lazy initialization, which
   // isn't implemented yet.
   if (pd->isStatic()) {
      assert(!pd->getDeclContext()->isGenericContext()
             || pd->getDeclContext()->getGenericSignatureOfContext()
                ->areAllParamsConcrete());
   }

   // Emit the lazy initialization token for the initialization expression.
   auto counter = anonymousSymbolCounter++;

   // Pick one variable of the pattern. Usually it's only one variable, but it
   // can also be something like: var (a, b) = ...
   Pattern *pattern = pd->getPattern(pbdEntry);
   VarDecl *varDecl = nullptr;
   pattern->forEachVariable([&](VarDecl *D) {
      varDecl = D;
   });
   assert(varDecl);

   mangle::AstMangler TokenMangler;
   std::string onceTokenBuffer = TokenMangler.mangleGlobalInit(varDecl, counter,
                                                               false);

   auto onceTy = BuiltinIntegerType::getWordType(M.getAstContext());
   auto oncePILTy
      = PILType::getPrimitiveObjectType(onceTy->getCanonicalType());

   // TODO: include the module in the onceToken's name mangling.
   // Then we can make it fragile.
   auto onceToken = PILGlobalVariable::create(M, PILLinkage::Private,
                                              IsNotSerialized,
                                              onceTokenBuffer, oncePILTy);
   onceToken->setDeclaration(false);

   // Emit the initialization code into a function.
   mangle::AstMangler FuncMangler;
   std::string onceFuncBuffer = FuncMangler.mangleGlobalInit(varDecl, counter,
                                                             true);

   PILFunction *onceFunc = emitLazyGlobalInitializer(onceFuncBuffer, pd,
                                                     pbdEntry);

   // Generate accessor functions for all of the declared variables, which
   // Builtin.once the lazy global initializer we just generated then return
   // the address of the individual variable.
   GenGlobalAccessors(*this, onceToken, onceFunc)
      .visit(pd->getPattern(pbdEntry));
}

void PILGenFunction::emitLazyGlobalInitializer(PatternBindingDecl *binding,
                                               unsigned pbdEntry) {
   MagicFunctionName = PILGenModule::getMagicFunctionName(binding->getDeclContext());

   {
      Scope scope(Cleanups, binding);

      // Emit the initialization sequence.
      emitPatternBinding(binding, pbdEntry);
   }

   // Return void.
   auto ret = emitEmptyTuple(binding);
   B.createReturn(ImplicitReturnLocation::getImplicitReturnLoc(binding), ret);
}

static void emitOnceCall(PILGenFunction &SGF, VarDecl *global,
                         PILGlobalVariable *onceToken, PILFunction *onceFunc) {
   PILType rawPointerPILTy
      = SGF.getLoweredLoadableType(SGF.getAstContext().TheRawPointerType);

   // Emit a reference to the global token.
   PILValue onceTokenAddr = SGF.B.createGlobalAddr(global, onceToken);
   onceTokenAddr = SGF.B.createAddressToPointer(global, onceTokenAddr,
                                                rawPointerPILTy);

   // Emit a reference to the function to execute.
   PILValue onceFuncRef = SGF.B.createFunctionRefFor(global, onceFunc);

   // Call Builtin.once.
   PILValue onceArgs[] = {onceTokenAddr, onceFuncRef};
   SGF.B.createBuiltin(global, SGF.getAstContext().getIdentifier("once"),
                       SGF.SGM.Types.getEmptyTupleType(), {}, onceArgs);
}

void PILGenFunction::emitGlobalAccessor(VarDecl *global,
                                        PILGlobalVariable *onceToken,
                                        PILFunction *onceFunc) {
   emitOnceCall(*this, global, onceToken, onceFunc);

   // Return the address of the global variable.
   // FIXME: It'd be nice to be able to return a PIL address directly.
   auto *pilG = SGM.getPILGlobalVariable(global, NotForDefinition);
   PILValue addr = B.createGlobalAddr(global, pilG);

   PILType rawPointerPILTy
      = getLoweredLoadableType(getAstContext().TheRawPointerType);
   addr = B.createAddressToPointer(global, addr, rawPointerPILTy);
   auto *ret = B.createReturn(global, addr);
   (void)ret;
   assert(ret->getDebugScope() && "instruction without scope");
}

