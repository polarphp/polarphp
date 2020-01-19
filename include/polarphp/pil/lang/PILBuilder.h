//===--- PILBuilder.h - Class for creating PIL Constructs -------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_PILBUILDER_H
#define POLARPHP_PIL_PILBUILDER_H

#include "polarphp/basic/ProfileCounter.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILDebugScope.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/PILOpenedArchetypesTracker.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/StringExtras.h"

namespace polar {

using Atomicity = RefCountingInst::Atomicity;

class PILDebugScope;
class IntegerLiteralExpr;
class FloatLiteralExpr;
class PILGlobalVariable;

/// Manage the state needed for a PIL pass across multiple, independent
/// PILBuilder invocations.
///
/// A PIL pass can instantiate a PILBuilderContext object to track information
/// across multiple, potentially independent invocations of PILBuilder. This
/// allows utilities used within the pass to construct a new PILBuilder instance
/// whenever it is convenient or appropriate. For example, a separate PILBuilder
/// should be constructed whenever the current debug location or insertion point
/// changed. Reusing the same PILBuilder and calling setInsertionPoint() easily
/// leads to incorrect debug information.
class PILBuilderContext {
   friend class PILBuilder;

   PILModule &Module;

   /// Allow the PIL module conventions to be overriden within the builder.
   /// This supports passes that lower PIL to a new stage.
   PILModuleConventions silConv = PILModuleConventions(Module);

   /// If this pointer is non-null, then any inserted instruction is
   /// recorded in this list.
   ///
   /// TODO: Give this ownership of InsertedInstrs and migrate users that
   /// currently provide their own InsertedInstrs.
   SmallVectorImpl<PILInstruction *> *InsertedInstrs = nullptr;

   /// An immutable view on the set of available opened archetypes.
   /// It is passed down to PILInstruction constructors and create
   /// methods.
   PILOpenedArchetypesState OpenedArchetypes;

   /// Maps opened archetypes to their definitions. If provided,
   /// can be used by the builder. It is supposed to be used
   /// only by PILGen or PIL deserializers.
   PILOpenedArchetypesTracker *OpenedArchetypesTracker = nullptr;

public:
   explicit PILBuilderContext(
      PILModule &M, SmallVectorImpl<PILInstruction *> *InsertedInstrs = 0)
      : Module(M), InsertedInstrs(InsertedInstrs) {}

   PILModule &getModule() { return Module; }

   // Allow a pass to override the current PIL module conventions. This should
   // only be done by a pass responsible for lowering PIL to a new stage
   // (e.g. Addresslowering).
   void setPILConventions(PILModuleConventions silConv) {
      this->silConv = silConv;
   }

   void setOpenedArchetypesTracker(PILOpenedArchetypesTracker *Tracker) {
      OpenedArchetypesTracker = Tracker;
      OpenedArchetypes.setOpenedArchetypesTracker(OpenedArchetypesTracker);
   }

   PILOpenedArchetypesTracker *getOpenedArchetypesTracker() const {
      return OpenedArchetypesTracker;
   }

protected:
   /// Notify the context of each new instruction after it is inserted in the
   /// instruction stream.
   void notifyInserted(PILInstruction *Inst) {
      // If the PILBuilder client wants to know about new instructions, record
      // this.
      if (InsertedInstrs)
         InsertedInstrs->push_back(Inst);
   }
};

class PILBuilder {
   friend class PILBuilderWithScope;

   /// Temporary context for clients that don't provide their own.
   PILBuilderContext TempContext;

   /// Reference to the provided PILBuilderContext.
   PILBuilderContext &C;

   /// The PILFunction that we are currently inserting into if we have one.
   ///
   /// If we are building into a block associated with a PILGlobalVariable this
   /// will be a nullptr.
   ///
   /// TODO: This can be made cleaner by using a PointerUnion or the like so we
   /// can store the PILGlobalVariable here as well.
   PILFunction *F;

   /// If this is non-null, the instruction is inserted in the specified
   /// basic block, at the specified InsertPt.  If null, created instructions
   /// are not auto-inserted.
   PILBasicBlock *BB;
   PILBasicBlock::iterator InsertPt;
   const PILDebugScope *CurDebugScope = nullptr;
   Optional<PILLocation> CurDebugLocOverride = None;

public:
   explicit PILBuilder(PILFunction &F)
      : TempContext(F.getModule()), C(TempContext), F(&F), BB(nullptr) {}

   PILBuilder(PILFunction &F, SmallVectorImpl<PILInstruction *> *InsertedInstrs)
      : TempContext(F.getModule(), InsertedInstrs), C(TempContext), F(&F),
        BB(nullptr) {}

   explicit PILBuilder(PILInstruction *I,
                       SmallVectorImpl<PILInstruction *> *InsertedInstrs = 0)
      : TempContext(I->getFunction()->getModule(), InsertedInstrs),
        C(TempContext), F(I->getFunction()) {
      setInsertionPoint(I);
   }

   explicit PILBuilder(PILBasicBlock::iterator I,
                       SmallVectorImpl<PILInstruction *> *InsertedInstrs = 0)
      : PILBuilder(&*I, InsertedInstrs) {}

   explicit PILBuilder(PILBasicBlock *BB,
                       SmallVectorImpl<PILInstruction *> *InsertedInstrs = 0)
      : TempContext(BB->getParent()->getModule(), InsertedInstrs),
        C(TempContext), F(BB->getParent()) {
      setInsertionPoint(BB);
   }

   explicit PILBuilder(PILGlobalVariable *GlobVar,
                       SmallVectorImpl<PILInstruction *> *InsertedInstrs = 0);

   PILBuilder(PILBasicBlock *BB, PILBasicBlock::iterator InsertPt,
              SmallVectorImpl<PILInstruction *> *InsertedInstrs = 0)
      : TempContext(BB->getParent()->getModule(), InsertedInstrs),
        C(TempContext), F(BB->getParent()) {
      setInsertionPoint(BB, InsertPt);
   }

   /// Build instructions before the given insertion point, inheriting the debug
   /// location.
   ///
   /// PILBuilderContext must outlive this PILBuilder instance.
   PILBuilder(PILInstruction *I, const PILDebugScope *DS, PILBuilderContext &C)
      : TempContext(C.getModule()), C(C), F(I->getFunction()) {
      assert(DS && "instruction has no debug scope");
      setCurrentDebugScope(DS);
      setInsertionPoint(I);
   }

   PILBuilder(PILBasicBlock *BB, const PILDebugScope *DS, PILBuilder &B)
      : PILBuilder(BB, DS, B.getBuilderContext()) {}

   /// Build instructions before the given insertion point, inheriting the debug
   /// location.
   ///
   /// PILBuilderContext must outlive this PILBuilder instance.
   PILBuilder(PILBasicBlock *BB, const PILDebugScope *DS, PILBuilderContext &C)
      : TempContext(C.getModule()), C(C), F(BB->getParent()) {
      assert(DS && "block has no debug scope");
      setCurrentDebugScope(DS);
      setInsertionPoint(BB);
   }

   // Allow a pass to override the current PIL module conventions. This should
   // only be done by a pass responsible for lowering PIL to a new stage
   // (e.g. Addresslowering).
   void setPILConventions(PILModuleConventions silConv) { C.silConv = silConv; }

   PILFunction &getFunction() const {
      assert(F && "cannot create this instruction without a function context");
      return *F;
   }

   TypeExpansionContext getTypeExpansionContext() const {
      return TypeExpansionContext(getFunction());
   }

   PILBuilderContext &getBuilderContext() const { return C; }
   PILModule &getModule() const { return C.Module; }
   AstContext &getAstContext() const { return getModule().getAstContext(); }
   const lowering::TypeLowering &getTypeLowering(PILType T) const {

      auto expansion = TypeExpansionContext::maximal(getModule().getTypePHPModule(),
                                                     getModule().isWholeModule());
      // If there's no current PILFunction, we're inserting into a global
      // variable initializer.
      if (F) {
         expansion = TypeExpansionContext(getFunction());
      }
      return getModule().Types.getTypeLowering(T, expansion);
   }

   void setOpenedArchetypesTracker(PILOpenedArchetypesTracker *Tracker) {
      C.setOpenedArchetypesTracker(Tracker);
   }

   PILOpenedArchetypesTracker *getOpenedArchetypesTracker() const {
      return C.getOpenedArchetypesTracker();
   }

   PILOpenedArchetypesState &getOpenedArchetypes() { return C.OpenedArchetypes; }

   void setCurrentDebugScope(const PILDebugScope *DS) { CurDebugScope = DS; }
   const PILDebugScope *getCurrentDebugScope() const { return CurDebugScope; }

   /// Apply a debug location override. If loc is None, the current override is
   /// removed. Otherwise, newly created debug locations use the given location.
   /// Note: the override location does not apply to debug_value[_addr].
   void applyDebugLocOverride(Optional<PILLocation> loc) {
      CurDebugLocOverride = loc;
   }

   /// Get the current debug location override.
   Optional<PILLocation> getCurrentDebugLocOverride() const {
      return CurDebugLocOverride;
   }

   /// Convenience function for building a PILDebugLocation.
   PILDebugLocation getPILDebugLocation(PILLocation Loc) {
      // FIXME: Audit all uses and enable this assertion.
      // assert(getCurrentDebugScope() && "no debug scope");
      auto Scope = getCurrentDebugScope();
      if (!Scope && F)
         Scope = F->getDebugScope();
      auto overriddenLoc = CurDebugLocOverride ? *CurDebugLocOverride : Loc;
      return PILDebugLocation(overriddenLoc, Scope);
   }

   /// If we have a PILFunction, return PILFunction::hasOwnership(). If we have a
   /// PILGlobalVariable, just return false.
   bool hasOwnership() const {
      if (F)
         return F->hasOwnership();
      return false;
   }

   //===--------------------------------------------------------------------===//
   // Insertion Point Management
   //===--------------------------------------------------------------------===//

   bool hasValidInsertionPoint() const { return BB != nullptr; }
   PILBasicBlock *getInsertionBB() { return BB; }
   PILBasicBlock::iterator getInsertionPoint() { return InsertPt; }

   /// insertingAtEndOfBlock - Return true if the insertion point is at the end
   /// of the current basic block.  False if we're inserting before an existing
   /// instruction.
   bool insertingAtEndOfBlock() const {
      assert(hasValidInsertionPoint() &&
             "Must have insertion point to ask about it");
      return InsertPt == BB->end();
   }

   /// clearInsertionPoint - Clear the insertion point: created instructions will
   /// not be inserted into a block.
   void clearInsertionPoint() { BB = nullptr; }

   /// setInsertionPoint - Set the insertion point.
   void setInsertionPoint(PILBasicBlock *BB, PILBasicBlock::iterator InsertPt) {
      this->BB = BB;
      this->InsertPt = InsertPt;
      if (InsertPt == BB->end())
         return;
      // Set the opened archetype context from the instruction.
      addOpenedArchetypeOperands(&*InsertPt);
   }

   /// setInsertionPoint - Set the insertion point to insert before the specified
   /// instruction.
   void setInsertionPoint(PILInstruction *I) {
      assert(I && "can't set insertion point to a null instruction");
      setInsertionPoint(I->getParent(), I->getIterator());
   }

   /// setInsertionPoint - Set the insertion point to insert before the specified
   /// instruction.
   void setInsertionPoint(PILBasicBlock::iterator IIIter) {
      setInsertionPoint(IIIter->getParent(), IIIter);
   }

   /// setInsertionPoint - Set the insertion point to insert at the end of the
   /// specified block.
   void setInsertionPoint(PILBasicBlock *BB) {
      assert(BB && "can't set insertion point to a null basic block");
      setInsertionPoint(BB, BB->end());
   }

   /// setInsertionPoint - Set the insertion point to insert at the end of the
   /// specified block.
   void setInsertionPoint(PILFunction::iterator BBIter) {
      setInsertionPoint(&*BBIter);
   }

   PILBasicBlock *getInsertionPoint() const { return BB; }

   //===--------------------------------------------------------------------===//
   // Instruction Tracking
   //===--------------------------------------------------------------------===//

   /// Clients of PILBuilder who want to know about any newly created
   /// instructions can install a SmallVector into the builder to collect them.
   void setTrackingList(SmallVectorImpl<PILInstruction *> *II) {
      C.InsertedInstrs = II;
   }

   SmallVectorImpl<PILInstruction *> *getTrackingList() {
      return C.InsertedInstrs;
   }

   //===--------------------------------------------------------------------===//
   // Opened archetypes handling
   //===--------------------------------------------------------------------===//
   void addOpenedArchetypeOperands(PILInstruction *I);

   //===--------------------------------------------------------------------===//
   // Type remapping
   //===--------------------------------------------------------------------===//

   static PILType getPartialApplyResultType(
      TypeExpansionContext context, PILType Ty, unsigned ArgCount, PILModule &M,
      SubstitutionMap subs, ParameterConvention calleeConvention,
      PartialApplyInst::OnStackKind onStack =
      PartialApplyInst::OnStackKind::NotOnStack);

   //===--------------------------------------------------------------------===//
   // CFG Manipulation
   //===--------------------------------------------------------------------===//

   /// moveBlockTo - Move a block to immediately before the given iterator.
   void moveBlockTo(PILBasicBlock *BB, PILFunction::iterator IP) {
      assert(PILFunction::iterator(BB) != IP && "moving block before itself?");
      PILFunction *F = BB->getParent();
      auto &Blocks = F->getBlocks();
      Blocks.remove(BB);
      Blocks.insert(IP, BB);
   }

   /// moveBlockTo - Move \p BB to immediately before \p Before.
   void moveBlockTo(PILBasicBlock *BB, PILBasicBlock *Before) {
      moveBlockTo(BB, Before->getIterator());
   }

   /// moveBlockToEnd - Reorder a block to the end of its containing function.
   void moveBlockToEnd(PILBasicBlock *BB) {
      moveBlockTo(BB, BB->getParent()->end());
   }

   /// Move the insertion point to the end of the given block.
   ///
   /// Assumes that no insertion point is currently active.
   void emitBlock(PILBasicBlock *BB) {
      assert(!hasValidInsertionPoint());
      setInsertionPoint(BB);
   }

   /// Branch to the given block if there's an active insertion point,
   /// then move the insertion point to the end of that block.
   void emitBlock(PILBasicBlock *BB, PILLocation BranchLoc);

   /// splitBlockForFallthrough - Prepare for the insertion of a terminator.  If
   /// the builder's insertion point is at the end of the current block (as when
   /// PILGen is creating the initial code for a function), just create and
   /// return a new basic block that will be later used for the continue point.
   ///
   /// If the insertion point is valid (i.e., pointing to an existing
   /// instruction) then split the block at that instruction and return the
   /// continuation block.
   PILBasicBlock *splitBlockForFallthrough();

   /// Convenience for creating a fall-through basic block on-the-fly without
   /// affecting the insertion point.
   PILBasicBlock *createFallthroughBlock(PILLocation loc,
                                         PILBasicBlock *targetBB) {
      auto *newBB = F->createBasicBlock();
      PILBuilder(newBB, this->getCurrentDebugScope(), this->getBuilderContext())
         .createBranch(loc, targetBB);
      return newBB;
   }

   //===--------------------------------------------------------------------===//
   // PILInstruction Creation Methods
   //===--------------------------------------------------------------------===//

   AllocStackInst *createAllocStack(PILLocation Loc, PILType elementType,
                                    Optional<PILDebugVariable> Var = None,
                                    bool hasDynamicLifetime = false) {
      Loc.markAsPrologue();
      assert((!dyn_cast_or_null<VarDecl>(Loc.getAsAstNode<Decl>()) || Var) &&
             "location is a VarDecl, but PILDebugVariable is empty");
      return insert(AllocStackInst::create(getPILDebugLocation(Loc), elementType,
                                           getFunction(), C.OpenedArchetypes,
                                           Var, hasDynamicLifetime));
   }

   AllocRefInst *createAllocRef(PILLocation Loc, PILType ObjectType,
                                bool objc, bool canAllocOnStack,
                                ArrayRef<PILType> ElementTypes,
                                ArrayRef<PILValue> ElementCountOperands) {
      // AllocRefInsts expand to function calls and can therefore not be
      // counted towards the function prologue.
      assert(!Loc.isInPrologue());
      return insert(AllocRefInst::create(getPILDebugLocation(Loc), getFunction(),
                                         ObjectType, objc, canAllocOnStack,
                                         ElementTypes, ElementCountOperands,
                                         C.OpenedArchetypes));
   }

   AllocRefDynamicInst *createAllocRefDynamic(PILLocation Loc, PILValue operand,
                                              PILType type, bool objc,
                                              ArrayRef<PILType> ElementTypes,
                                              ArrayRef<PILValue> ElementCountOperands) {
      // AllocRefDynamicInsts expand to function calls and can therefore
      // not be counted towards the function prologue.
      assert(!Loc.isInPrologue());
      return insert(AllocRefDynamicInst::create(
         getPILDebugLocation(Loc), *F, operand, type, objc, ElementTypes,
         ElementCountOperands, C.OpenedArchetypes));
   }

   AllocValueBufferInst *
   createAllocValueBuffer(PILLocation Loc, PILType valueType, PILValue operand) {
      return insert(AllocValueBufferInst::create(
         getPILDebugLocation(Loc), valueType, operand, *F, C.OpenedArchetypes));
   }

   AllocBoxInst *createAllocBox(PILLocation Loc, CanPILBoxType BoxType,
                                Optional<PILDebugVariable> Var = None,
                                bool hasDynamicLifetime = false) {
      Loc.markAsPrologue();
      assert((!dyn_cast_or_null<VarDecl>(Loc.getAsAstNode<Decl>()) || Var) &&
             "location is a VarDecl, but PILDebugVariable is empty");
      return insert(AllocBoxInst::create(getPILDebugLocation(Loc), BoxType, *F,
                                         C.OpenedArchetypes, Var,
                                         hasDynamicLifetime));
   }

   AllocExistentialBoxInst *
   createAllocExistentialBox(PILLocation Loc, PILType ExistentialType,
                             CanType ConcreteType,
                             ArrayRef<InterfaceConformanceRef> Conformances) {
      return insert(AllocExistentialBoxInst::create(
         getPILDebugLocation(Loc), ExistentialType, ConcreteType, Conformances,
         F, C.OpenedArchetypes));
   }

   ApplyInst *createApply(
      PILLocation Loc, PILValue Fn, SubstitutionMap Subs,
      ArrayRef<PILValue> Args, bool isNonThrowing = false,
      const GenericSpecializationInformation *SpecializationInfo = nullptr) {
      return insert(ApplyInst::create(getPILDebugLocation(Loc), Fn, Subs, Args,
                                      isNonThrowing, C.silConv, *F,
                                      C.OpenedArchetypes, SpecializationInfo));
   }

   TryApplyInst *createTryApply(
      PILLocation Loc, PILValue fn, SubstitutionMap subs,
      ArrayRef<PILValue> args, PILBasicBlock *normalBB, PILBasicBlock *errorBB,
      const GenericSpecializationInformation *SpecializationInfo = nullptr) {
      return insertTerminator(TryApplyInst::create(
         getPILDebugLocation(Loc), fn, subs, args, normalBB, errorBB, *F,
         C.OpenedArchetypes, SpecializationInfo));
   }

   PartialApplyInst *createPartialApply(
      PILLocation Loc, PILValue Fn, SubstitutionMap Subs,
      ArrayRef<PILValue> Args, ParameterConvention CalleeConvention,
      PartialApplyInst::OnStackKind OnStack =
      PartialApplyInst::OnStackKind::NotOnStack,
      const GenericSpecializationInformation *SpecializationInfo = nullptr) {
      return insert(PartialApplyInst::create(
         getPILDebugLocation(Loc), Fn, Args, Subs, CalleeConvention, *F,
         C.OpenedArchetypes, SpecializationInfo, OnStack));
   }

   BeginApplyInst *createBeginApply(
      PILLocation Loc, PILValue Fn, SubstitutionMap Subs,
      ArrayRef<PILValue> Args, bool isNonThrowing = false,
      const GenericSpecializationInformation *SpecializationInfo = nullptr) {
      return insert(BeginApplyInst::create(
         getPILDebugLocation(Loc), Fn, Subs, Args, isNonThrowing, C.silConv, *F,
         C.OpenedArchetypes, SpecializationInfo));
   }

   AbortApplyInst *createAbortApply(PILLocation loc, PILValue beginApply) {
      return insert(new (getModule()) AbortApplyInst(getPILDebugLocation(loc),
                                                     beginApply));
   }

   EndApplyInst *createEndApply(PILLocation loc, PILValue beginApply) {
      return insert(new (getModule()) EndApplyInst(getPILDebugLocation(loc),
                                                   beginApply));
   }

   BuiltinInst *createBuiltin(PILLocation Loc, Identifier Name, PILType ResultTy,
                              SubstitutionMap Subs,
                              ArrayRef<PILValue> Args) {
      return insert(BuiltinInst::create(getPILDebugLocation(Loc), Name,
                                        ResultTy, Subs, Args, getModule()));
   }

   /// Create a binary function with the signature: OpdTy, OpdTy -> ResultTy.
   BuiltinInst *createBuiltinBinaryFunction(PILLocation Loc, StringRef Name,
                                            PILType OpdTy, PILType ResultTy,
                                            ArrayRef<PILValue> Args) {
      auto &C = getAstContext();

      llvm::SmallString<16> NameStr = Name;
      appendOperandTypeName(OpdTy, NameStr);
      auto Ident = C.getIdentifier(NameStr);
      return insert(BuiltinInst::create(getPILDebugLocation(Loc), Ident, ResultTy,
                                        {}, Args, getModule()));
   }

   // Create a binary function with the signature: OpdTy1, OpdTy2 -> ResultTy.
   BuiltinInst *createBuiltinBinaryFunctionWithTwoOpTypes(
      PILLocation Loc, StringRef Name, PILType OpdTy1, PILType OpdTy2,
      PILType ResultTy, ArrayRef<PILValue> Args) {
      auto &C = getAstContext();

      llvm::SmallString<16> NameStr = Name;
      appendOperandTypeName(OpdTy1, NameStr);
      appendOperandTypeName(OpdTy2, NameStr);
      auto Ident = C.getIdentifier(NameStr);
      return insert(BuiltinInst::create(getPILDebugLocation(Loc), Ident,
                                        ResultTy, {}, Args, getModule()));
   }

   /// Create a binary function with the signature:
   /// OpdTy, OpdTy, Int1 -> (OpdTy, Int1)
   BuiltinInst *
   createBuiltinBinaryFunctionWithOverflow(PILLocation Loc, StringRef Name,
                                           ArrayRef<PILValue> Args) {
      assert(Args.size() == 3 && "Need three arguments");
      assert(Args[0]->getType() == Args[1]->getType() &&
             "Binary operands must match");
      assert(Args[2]->getType().is<BuiltinIntegerType>() &&
             Args[2]->getType().getAstType()->isBuiltinIntegerType(1) &&
             "Must have a third Int1 operand");

      PILType OpdTy = Args[0]->getType();
      PILType Int1Ty = Args[2]->getType();

      TupleTypeElt ResultElts[] = {OpdTy.getAstType(), Int1Ty.getAstType()};
      Type ResultTy = TupleType::get(ResultElts, getAstContext());
      PILType PILResultTy =
         PILType::getPrimitiveObjectType(ResultTy->getCanonicalType());

      return createBuiltinBinaryFunction(Loc, Name, OpdTy, PILResultTy, Args);
   }

   // Creates a dynamic_function_ref or function_ref depending on whether f is
   // dynamically_replaceable.
   FunctionRefBaseInst *createFunctionRefFor(PILLocation Loc, PILFunction *f) {
      if (f->isDynamicallyReplaceable())
         return createDynamicFunctionRef(Loc, f);
      else
         return createFunctionRef(Loc, f);
   }

   FunctionRefBaseInst *createFunctionRef(PILLocation Loc, PILFunction *f,
                                          PILInstructionKind kind) {
      if (kind == PILInstructionKind::FunctionRefInst)
         return createFunctionRef(Loc, f);
      else if (kind == PILInstructionKind::DynamicFunctionRefInst)
         return createDynamicFunctionRef(Loc, f);
      else if (kind == PILInstructionKind::PreviousDynamicFunctionRefInst)
         return createPreviousDynamicFunctionRef(Loc, f);
      assert(false && "Should not get here");
      return nullptr;
   }

   FunctionRefInst *createFunctionRef(PILLocation Loc, PILFunction *f) {
      return insert(new (getModule()) FunctionRefInst(getPILDebugLocation(Loc), f,
                                                      getTypeExpansionContext()));
   }

   DynamicFunctionRefInst *
   createDynamicFunctionRef(PILLocation Loc, PILFunction *f) {
      return insert(new (getModule()) DynamicFunctionRefInst(
         getPILDebugLocation(Loc), f, getTypeExpansionContext()));
   }

   PreviousDynamicFunctionRefInst *
   createPreviousDynamicFunctionRef(PILLocation Loc, PILFunction *f) {
      return insert(new (getModule()) PreviousDynamicFunctionRefInst(
         getPILDebugLocation(Loc), f, getTypeExpansionContext()));
   }

   AllocGlobalInst *createAllocGlobal(PILLocation Loc, PILGlobalVariable *g) {
      return insert(new (getModule())
                       AllocGlobalInst(getPILDebugLocation(Loc), g));
   }
   GlobalAddrInst *createGlobalAddr(PILLocation Loc, PILGlobalVariable *g) {
      return insert(new (getModule()) GlobalAddrInst(getPILDebugLocation(Loc), g,
                                                     getTypeExpansionContext()));
   }
   GlobalAddrInst *createGlobalAddr(PILLocation Loc, PILType Ty) {
      return insert(new (F->getModule())
                       GlobalAddrInst(getPILDebugLocation(Loc), Ty));
   }
   GlobalValueInst *createGlobalValue(PILLocation Loc, PILGlobalVariable *g) {
      return insert(new (getModule()) GlobalValueInst(getPILDebugLocation(Loc), g,
                                                      getTypeExpansionContext()));
   }
   IntegerLiteralInst *createIntegerLiteral(IntegerLiteralExpr *E);

   IntegerLiteralInst *createIntegerLiteral(PILLocation Loc, PILType Ty,
                                            intmax_t Value) {
      return insert(
         IntegerLiteralInst::create(getPILDebugLocation(Loc), Ty, Value,
                                    getModule()));
   }
   IntegerLiteralInst *createIntegerLiteral(PILLocation Loc, PILType Ty,
                                            const APInt &Value) {
      return insert(
         IntegerLiteralInst::create(getPILDebugLocation(Loc), Ty, Value,
                                    getModule()));
   }

   FloatLiteralInst *createFloatLiteral(FloatLiteralExpr *E);

   FloatLiteralInst *createFloatLiteral(PILLocation Loc, PILType Ty,
                                        const APFloat &Value) {
      return insert(
         FloatLiteralInst::create(getPILDebugLocation(Loc), Ty, Value,
                                  getModule()));
   }

   StringLiteralInst *createStringLiteral(PILLocation Loc, StringRef text,
                                          StringLiteralInst::Encoding encoding) {
      return insert(StringLiteralInst::create(getPILDebugLocation(Loc), text,
                                              encoding, getModule()));
   }

   StringLiteralInst *createStringLiteral(PILLocation Loc, const Twine &text,
                                          StringLiteralInst::Encoding encoding) {
      SmallVector<char, 256> Out;
      return insert(StringLiteralInst::create(
         getPILDebugLocation(Loc), text.toStringRef(Out), encoding, getModule()));
   }

   /// If \p LV is non-trivial, return a \p Qualifier load of \p LV. If \p LV is
   /// trivial, use trivial instead.
   ///
   /// *NOTE* The SupportUnqualifiedPIL is an option to ease the bring up of
   /// Semantic PIL. It enables a pass that must be able to run on both Semantic
   /// PIL and non-Semantic PIL. It has a default argument of false, so if this
   /// is not necessary for your pass, just ignore the parameter.
   LoadInst *createTrivialLoadOr(PILLocation Loc, PILValue LV,
                                 LoadOwnershipQualifier Qualifier,
                                 bool SupportUnqualifiedPIL = false) {
      if (SupportUnqualifiedPIL && !hasOwnership()) {
         assert(
            Qualifier != LoadOwnershipQualifier::Copy &&
            "In unqualified PIL, a copy must be done separately form the load");
         return createLoad(Loc, LV, LoadOwnershipQualifier::Unqualified);
      }

      if (LV->getType().isTrivial(getFunction())) {
         return createLoad(Loc, LV, LoadOwnershipQualifier::Trivial);
      }
      return createLoad(Loc, LV, Qualifier);
   }

   LoadInst *createLoad(PILLocation Loc, PILValue LV,
                        LoadOwnershipQualifier Qualifier) {
      assert(((Qualifier != LoadOwnershipQualifier::Unqualified) ||
                   !hasOwnership()) && "Unqualified inst in qualified function");
      assert(((Qualifier == LoadOwnershipQualifier::Unqualified) ||
                   hasOwnership()) && "Qualified inst in unqualified function");
      assert(isLoadableOrOpaque(LV->getType()));
      return insert(new (getModule())
                       LoadInst(getPILDebugLocation(Loc), LV, Qualifier));
   }

   KeyPathInst *createKeyPath(PILLocation Loc,
                              KeyPathPattern *Pattern,
                              SubstitutionMap Subs,
                              ArrayRef<PILValue> Args,
                              PILType Ty) {
      return insert(KeyPathInst::create(getPILDebugLocation(Loc),
                                        Pattern, Subs, Args,
                                        Ty, getFunction()));
   }

   /// Convenience function for calling emitLoad on the type lowering for
   /// non-address values.
   PILValue emitLoadValueOperation(PILLocation Loc, PILValue LV,
                                   LoadOwnershipQualifier Qualifier) {
      assert(isLoadableOrOpaque(LV->getType()));
      const auto &lowering = getTypeLowering(LV->getType());
      return lowering.emitLoad(*this, Loc, LV, Qualifier);
   }

   LoadBorrowInst *createLoadBorrow(PILLocation Loc, PILValue LV) {
      assert(isLoadableOrOpaque(LV->getType()));
      return insert(new (getModule())
                       LoadBorrowInst(getPILDebugLocation(Loc), LV));
   }

   BeginBorrowInst *createBeginBorrow(PILLocation Loc, PILValue LV) {
      return insert(new (getModule())
                       BeginBorrowInst(getPILDebugLocation(Loc), LV));
   }

   PILValue emitLoadBorrowOperation(PILLocation loc, PILValue v) {
      if (!hasOwnership()) {
         return emitLoadValueOperation(loc, v,
                                       LoadOwnershipQualifier::Unqualified);
      }
      return createLoadBorrow(loc, v);
   }

   PILValue emitBeginBorrowOperation(PILLocation loc, PILValue v) {
      if (!hasOwnership() ||
          v.getOwnershipKind().isCompatibleWith(ValueOwnershipKind::Guaranteed))
         return v;
      return createBeginBorrow(loc, v);
   }

   void emitEndBorrowOperation(PILLocation loc, PILValue v) {
      if (!hasOwnership())
         return;
      createEndBorrow(loc, v);
   }

   // Pass in an address or value, perform a begin_borrow/load_borrow and pass
   // the value to the passed in closure. After the closure has finished
   // executing, automatically insert the end_borrow. The closure can assume that
   // it will receive a loaded loadable value.
   void emitScopedBorrowOperation(PILLocation loc, PILValue original,
                                  function_ref<void(PILValue)> &&fun);

   /// Utility function that returns a trivial store if the stored type is
   /// trivial and a \p Qualifier store if the stored type is non-trivial.
   ///
   /// *NOTE* The SupportUnqualifiedPIL is an option to ease the bring up of
   /// Semantic PIL. It enables a pass that must be able to run on both Semantic
   /// PIL and non-Semantic PIL. It has a default argument of false, so if this
   /// is not necessary for your pass, just ignore the parameter.
   StoreInst *createTrivialStoreOr(PILLocation Loc, PILValue Src,
                                   PILValue DestAddr,
                                   StoreOwnershipQualifier Qualifier,
                                   bool SupportUnqualifiedPIL = false) {
      if (SupportUnqualifiedPIL && !hasOwnership()) {
         assert(
            Qualifier != StoreOwnershipQualifier::Assign &&
            "In unqualified PIL, assigns must be represented via 2 instructions");
         return createStore(Loc, Src, DestAddr,
                            StoreOwnershipQualifier::Unqualified);
      }
      if (Src->getType().isTrivial(getFunction())) {
         return createStore(Loc, Src, DestAddr, StoreOwnershipQualifier::Trivial);
      }
      return createStore(Loc, Src, DestAddr, Qualifier);
   }

   StoreInst *createStore(PILLocation Loc, PILValue Src, PILValue DestAddr,
                          StoreOwnershipQualifier Qualifier) {
      assert(((Qualifier != StoreOwnershipQualifier::Unqualified) ||
                   !hasOwnership()) && "Unqualified inst in qualified function");
      assert(((Qualifier == StoreOwnershipQualifier::Unqualified) ||
                   hasOwnership()) && "Qualified inst in unqualified function");
      return insert(new (getModule()) StoreInst(getPILDebugLocation(Loc), Src,
                                                DestAddr, Qualifier));
   }

   /// Convenience function for calling emitStore on the type lowering for
   /// non-address values.
   void emitStoreValueOperation(PILLocation Loc, PILValue Src, PILValue DestAddr,
                                StoreOwnershipQualifier Qualifier) {
      assert(!Src->getType().isAddress());
      const auto &lowering = getTypeLowering(Src->getType());
      return lowering.emitStore(*this, Loc, Src, DestAddr, Qualifier);
   }

   EndBorrowInst *createEndBorrow(PILLocation loc, PILValue borrowedValue) {
      return insert(new (getModule())
                       EndBorrowInst(getPILDebugLocation(loc), borrowedValue));
   }

   EndBorrowInst *createEndBorrow(PILLocation Loc, PILValue BorrowedValue,
                                  PILValue OriginalValue) {
      return insert(new (getModule())
                       EndBorrowInst(getPILDebugLocation(Loc), BorrowedValue));
   }

   BeginAccessInst *createBeginAccess(PILLocation loc, PILValue address,
                                      PILAccessKind accessKind,
                                      PILAccessEnforcement enforcement,
                                      bool noNestedConflict,
                                      bool fromBuiltin) {
      return insert(new (getModule()) BeginAccessInst(
         getPILDebugLocation(loc), address, accessKind, enforcement,
         noNestedConflict, fromBuiltin));
   }

   EndAccessInst *createEndAccess(PILLocation loc, PILValue address,
                                  bool aborted) {
      return insert(new (getModule()) EndAccessInst(
         getPILDebugLocation(loc), address, aborted));
   }

   BeginUnpairedAccessInst *
   createBeginUnpairedAccess(PILLocation loc, PILValue address, PILValue buffer,
                             PILAccessKind accessKind,
                             PILAccessEnforcement enforcement,
                             bool noNestedConflict,
                             bool fromBuiltin) {
      return insert(new (getModule()) BeginUnpairedAccessInst(
         getPILDebugLocation(loc), address, buffer, accessKind, enforcement,
         noNestedConflict, fromBuiltin));
   }

   EndUnpairedAccessInst *
   createEndUnpairedAccess(PILLocation loc, PILValue buffer,
                           PILAccessEnforcement enforcement, bool aborted,
                           bool fromBuiltin) {
      return insert(new (getModule()) EndUnpairedAccessInst(
         getPILDebugLocation(loc), buffer, enforcement, aborted, fromBuiltin));
   }

   AssignInst *createAssign(PILLocation Loc, PILValue Src, PILValue DestAddr,
                            AssignOwnershipQualifier Qualifier) {
      return insert(new (getModule())
                       AssignInst(getPILDebugLocation(Loc), Src, DestAddr,
                                  Qualifier));
   }

   AssignByWrapperInst *createAssignByWrapper(PILLocation Loc,
                                              PILValue Src, PILValue Dest,
                                              PILValue Initializer,
                                              PILValue Setter,
                                              AssignOwnershipQualifier Qualifier) {
      return insert(new (getModule())
                       AssignByWrapperInst(getPILDebugLocation(Loc), Src, Dest,
                                           Initializer, Setter, Qualifier));
   }

   StoreBorrowInst *createStoreBorrow(PILLocation Loc, PILValue Src,
                                      PILValue DestAddr) {
      return insert(new (getModule())
                       StoreBorrowInst(getPILDebugLocation(Loc), Src, DestAddr));
   }

   MarkUninitializedInst *
   createMarkUninitialized(PILLocation Loc, PILValue src,
                           MarkUninitializedInst::Kind k) {
      return insert(new (getModule()) MarkUninitializedInst(
         getPILDebugLocation(Loc), src, k));
   }
   MarkUninitializedInst *createMarkUninitializedVar(PILLocation Loc,
                                                     PILValue src) {
      return createMarkUninitialized(Loc, src, MarkUninitializedInst::Var);
   }
   MarkUninitializedInst *createMarkUninitializedRootSelf(PILLocation Loc,
                                                          PILValue src) {
      return createMarkUninitialized(Loc, src, MarkUninitializedInst::RootSelf);
   }

   MarkFunctionEscapeInst *createMarkFunctionEscape(PILLocation Loc,
                                                    ArrayRef<PILValue> vars) {
      return insert(
         MarkFunctionEscapeInst::create(getPILDebugLocation(Loc), vars, getFunction()));
   }

   DebugValueInst *createDebugValue(PILLocation Loc, PILValue src,
                                    PILDebugVariable Var);
   DebugValueAddrInst *createDebugValueAddr(PILLocation Loc, PILValue src,
                                            PILDebugVariable Var);

#define NEVER_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
  Load##Name##Inst *createLoad##Name(PILLocation Loc, \
                                     PILValue src, \
                                     IsTake_t isTake) { \
    return insert(new (getModule()) \
      Load##Name##Inst(getPILDebugLocation(Loc), src, isTake)); \
  } \
  Store##Name##Inst *createStore##Name(PILLocation Loc, \
                                       PILValue value, \
                                       PILValue dest, \
                                       IsInitialization_t isInit) { \
    return insert(new (getModule()) \
      Store##Name##Inst(getPILDebugLocation(Loc), value, dest, isInit)); \
  }
#define LOADABLE_REF_STORAGE_HELPER(Name)                                      \
  Name##ToRefInst *create##Name##ToRef(PILLocation Loc, PILValue op,           \
                                       PILType ty) {                           \
    return insert(new (getModule())                                            \
                      Name##ToRefInst(getPILDebugLocation(Loc), op, ty));      \
  }                                                                            \
  RefTo##Name##Inst *createRefTo##Name(PILLocation Loc, PILValue op,           \
                                       PILType ty) {                           \
    return insert(new (getModule())                                            \
                      RefTo##Name##Inst(getPILDebugLocation(Loc), op, ty));    \
  }                                                                            \
  StrongCopy##Name##ValueInst *createStrongCopy##Name##Value(                  \
      PILLocation Loc, PILValue operand) {                                     \
    auto type = getFunction().getLoweredType(                                  \
        operand->getType().getAstType().getReferenceStorageReferent());        \
    return insert(new (getModule()) StrongCopy##Name##ValueInst(               \
        getPILDebugLocation(Loc), operand, type));                             \
  }

#define ALWAYS_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
  LOADABLE_REF_STORAGE_HELPER(Name) \
  StrongRetain##Name##Inst *createStrongRetain##Name(PILLocation Loc, \
                                                     PILValue Operand, \
                                                     Atomicity atomicity) { \
    return insert(new (getModule()) \
      StrongRetain##Name##Inst(getPILDebugLocation(Loc), Operand, atomicity)); \
  } \
  Name##RetainInst *create##Name##Retain(PILLocation Loc, PILValue Operand, \
                                         Atomicity atomicity) { \
    return insert(new (getModule()) \
      Name##RetainInst(getPILDebugLocation(Loc), Operand, atomicity)); \
  } \
  Name##ReleaseInst *create##Name##Release(PILLocation Loc, \
                                           PILValue Operand, \
                                           Atomicity atomicity) { \
    return insert(new (getModule()) \
      Name##ReleaseInst(getPILDebugLocation(Loc), Operand, atomicity)); \
  }
#define SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
  NEVER_LOADABLE_CHECKED_REF_STORAGE(Name, "...") \
  ALWAYS_LOADABLE_CHECKED_REF_STORAGE(Name, "...")
#define UNCHECKED_REF_STORAGE(Name, ...) \
  LOADABLE_REF_STORAGE_HELPER(Name)
#include "polarphp/ast/ReferenceStorageDef.h"
#undef LOADABLE_REF_STORAGE_HELPER

   CopyAddrInst *createCopyAddr(PILLocation Loc, PILValue srcAddr,
                                PILValue destAddr, IsTake_t isTake,
                                IsInitialization_t isInitialize) {
      assert(srcAddr->getType() == destAddr->getType());
      return insert(new (getModule()) CopyAddrInst(
         getPILDebugLocation(Loc), srcAddr, destAddr, isTake, isInitialize));
   }

   BindMemoryInst *createBindMemory(PILLocation Loc, PILValue base,
                                    PILValue index, PILType boundType) {
      return insert(BindMemoryInst::create(getPILDebugLocation(Loc), base, index,
                                           boundType, getFunction(),
                                           C.OpenedArchetypes));
   }

   ConvertFunctionInst *createConvertFunction(PILLocation Loc, PILValue Op,
                                              PILType Ty,
                                              bool WithoutActuallyEscaping) {
      return insert(ConvertFunctionInst::create(getPILDebugLocation(Loc), Op, Ty,
                                                getFunction(), C.OpenedArchetypes,
                                                WithoutActuallyEscaping));
   }

   ConvertEscapeToNoEscapeInst *
   createConvertEscapeToNoEscape(PILLocation Loc, PILValue Op, PILType Ty,
                                 bool lifetimeGuaranteed) {
      return insert(ConvertEscapeToNoEscapeInst::create(
         getPILDebugLocation(Loc), Op, Ty, getFunction(), C.OpenedArchetypes,
         lifetimeGuaranteed));
   }

   ThinFunctionToPointerInst *
   createThinFunctionToPointer(PILLocation Loc, PILValue Op, PILType Ty) {
      return insert(new (getModule()) ThinFunctionToPointerInst(
         getPILDebugLocation(Loc), Op, Ty));
   }

   PointerToThinFunctionInst *
   createPointerToThinFunction(PILLocation Loc, PILValue Op, PILType Ty) {
      return insert(PointerToThinFunctionInst::create(
         getPILDebugLocation(Loc), Op, Ty, getFunction(), C.OpenedArchetypes));
   }

   UpcastInst *createUpcast(PILLocation Loc, PILValue Op, PILType Ty) {
      return insert(UpcastInst::create(getPILDebugLocation(Loc), Op, Ty,
                                       getFunction(), C.OpenedArchetypes));
   }

   AddressToPointerInst *createAddressToPointer(PILLocation Loc, PILValue Op,
                                                PILType Ty) {
      return insert(new (getModule()) AddressToPointerInst(
         getPILDebugLocation(Loc), Op, Ty));
   }

   PointerToAddressInst *createPointerToAddress(PILLocation Loc, PILValue Op,
                                                PILType Ty,
                                                bool isStrict,
                                                bool isInvariant = false){
      return insert(new (getModule()) PointerToAddressInst(
         getPILDebugLocation(Loc), Op, Ty, isStrict, isInvariant));
   }

   UncheckedRefCastInst *createUncheckedRefCast(PILLocation Loc, PILValue Op,
                                                PILType Ty) {
      return insert(UncheckedRefCastInst::create(
         getPILDebugLocation(Loc), Op, Ty, getFunction(), C.OpenedArchetypes));
   }

   UncheckedRefCastAddrInst *
   createUncheckedRefCastAddr(PILLocation Loc,
                              PILValue src, CanType sourceFormalType,
                              PILValue dest, CanType targetFormalType) {
      return insert(new (getModule()) UncheckedRefCastAddrInst(
         getPILDebugLocation(Loc), src, sourceFormalType,
         dest, targetFormalType));
   }

   UncheckedAddrCastInst *createUncheckedAddrCast(PILLocation Loc, PILValue Op,
                                                  PILType Ty) {
      return insert(UncheckedAddrCastInst::create(
         getPILDebugLocation(Loc), Op, Ty, getFunction(), C.OpenedArchetypes));
   }

   UncheckedTrivialBitCastInst *
   createUncheckedTrivialBitCast(PILLocation Loc, PILValue Op, PILType Ty) {
      return insert(UncheckedTrivialBitCastInst::create(
         getPILDebugLocation(Loc), Op, Ty, getFunction(), C.OpenedArchetypes));
   }

   UncheckedBitwiseCastInst *
   createUncheckedBitwiseCast(PILLocation Loc, PILValue Op, PILType Ty) {
      return insert(UncheckedBitwiseCastInst::create(
         getPILDebugLocation(Loc), Op, Ty, getFunction(), C.OpenedArchetypes));
   }

   RefToBridgeObjectInst *createRefToBridgeObject(PILLocation Loc, PILValue Ref,
                                                  PILValue Bits) {
      auto Ty = PILType::getBridgeObjectType(getAstContext());
      return insert(new (getModule()) RefToBridgeObjectInst(
         getPILDebugLocation(Loc), Ref, Bits, Ty));
   }

   BridgeObjectToRefInst *createBridgeObjectToRef(PILLocation Loc, PILValue Op,
                                                  PILType Ty) {
      return insert(new (getModule()) BridgeObjectToRefInst(
         getPILDebugLocation(Loc), Op, Ty));
   }

   ValueToBridgeObjectInst *createValueToBridgeObject(PILLocation Loc,
                                                      PILValue value) {
      auto Ty = PILType::getBridgeObjectType(getAstContext());
      return insert(new (getModule()) ValueToBridgeObjectInst(
         getPILDebugLocation(Loc), value, Ty));
   }

   BridgeObjectToWordInst *createBridgeObjectToWord(PILLocation Loc,
                                                    PILValue Op) {
      auto Ty = PILType::getBuiltinWordType(getAstContext());
      return createBridgeObjectToWord(Loc, Op, Ty);
   }

   BridgeObjectToWordInst *createBridgeObjectToWord(PILLocation Loc, PILValue Op,
                                                    PILType Ty) {
      return insert(new (getModule()) BridgeObjectToWordInst(
         getPILDebugLocation(Loc), Op, Ty));
   }

   RefToRawPointerInst *createRefToRawPointer(PILLocation Loc, PILValue Op,
                                              PILType Ty) {
      return insert(new (getModule())
                       RefToRawPointerInst(getPILDebugLocation(Loc), Op, Ty));
   }

   RawPointerToRefInst *createRawPointerToRef(PILLocation Loc, PILValue Op,
                                              PILType Ty) {
      return insert(new (getModule())
                       RawPointerToRefInst(getPILDebugLocation(Loc), Op, Ty));
   }

   ThinToThickFunctionInst *createThinToThickFunction(PILLocation Loc,
                                                      PILValue Op, PILType Ty) {
      return insert(ThinToThickFunctionInst::create(
         getPILDebugLocation(Loc), Op, Ty, getFunction(), C.OpenedArchetypes));
   }

   // @todo
//   ThickToObjCMetatypeInst *createThickToObjCMetatype(PILLocation Loc,
//                                                      PILValue Op, PILType Ty) {
//      return insert(new (getModule()) ThickToObjCMetatypeInst(
//         getPILDebugLocation(Loc), Op, Ty));
//   }
//
//   ObjCToThickMetatypeInst *createObjCToThickMetatype(PILLocation Loc,
//                                                      PILValue Op, PILType Ty) {
//      return insert(new (getModule()) ObjCToThickMetatypeInst(
//         getPILDebugLocation(Loc), Op, Ty));
//   }
//
//   ObjCProtocolInst *createObjCInterface(PILLocation Loc, InterfaceDecl *P,
//                                        PILType Ty) {
//      return insert(new (getModule())
//                       ObjCProtocolInst(getPILDebugLocation(Loc), P, Ty));
//   }

   CopyValueInst *createCopyValue(PILLocation Loc, PILValue operand) {
      assert(!operand->getType().isTrivial(getFunction()) &&
             "Should not be passing trivial values to this api. Use instead "
             "emitCopyValueOperation");
      return insert(new (getModule())
                       CopyValueInst(getPILDebugLocation(Loc), operand));
   }

   DestroyValueInst *createDestroyValue(PILLocation Loc, PILValue operand) {
      assert(isLoadableOrOpaque(operand->getType()));
      assert(!operand->getType().isTrivial(getFunction()) &&
             "Should not be passing trivial values to this api. Use instead "
             "emitDestroyValueOperation");
      return insert(new (getModule())
                       DestroyValueInst(getPILDebugLocation(Loc), operand));
   }

   UnconditionalCheckedCastInst *
   createUnconditionalCheckedCast(PILLocation Loc, PILValue op,
                                  PILType destLoweredTy,
                                  CanType destFormalTy) {
      return insert(UnconditionalCheckedCastInst::create(
         getPILDebugLocation(Loc), op, destLoweredTy, destFormalTy,
         getFunction(), C.OpenedArchetypes));
   }

   UnconditionalCheckedCastAddrInst *
   createUnconditionalCheckedCastAddr(PILLocation Loc,
                                      PILValue src, CanType sourceFormalType,
                                      PILValue dest, CanType targetFormalType) {
      return insert(new (getModule()) UnconditionalCheckedCastAddrInst(
         getPILDebugLocation(Loc), src, sourceFormalType,
         dest, targetFormalType));
   }

   UnconditionalCheckedCastValueInst *
   createUnconditionalCheckedCastValue(PILLocation Loc,
                                       PILValue op, CanType srcFormalTy,
                                       PILType destLoweredTy,
                                       CanType destFormalTy) {
      return insert(UnconditionalCheckedCastValueInst::create(
         getPILDebugLocation(Loc), op, srcFormalTy,
         destLoweredTy, destFormalTy,
         getFunction(), C.OpenedArchetypes));
   }

   RetainValueInst *createRetainValue(PILLocation Loc, PILValue operand,
                                      Atomicity atomicity) {
      assert(!hasOwnership());
      assert(isLoadableOrOpaque(operand->getType()));
      return insert(new (getModule()) RetainValueInst(getPILDebugLocation(Loc),
                                                      operand, atomicity));
   }

   RetainValueAddrInst *createRetainValueAddr(PILLocation Loc, PILValue operand,
                                              Atomicity atomicity) {
      assert(!hasOwnership());
      return insert(new (getModule()) RetainValueAddrInst(
         getPILDebugLocation(Loc), operand, atomicity));
   }

   ReleaseValueInst *createReleaseValue(PILLocation Loc, PILValue operand,
                                        Atomicity atomicity) {
      assert(!hasOwnership());
      assert(isLoadableOrOpaque(operand->getType()));
      return insert(new (getModule()) ReleaseValueInst(getPILDebugLocation(Loc),
                                                       operand, atomicity));
   }

   ReleaseValueAddrInst *createReleaseValueAddr(PILLocation Loc,
                                                PILValue operand,
                                                Atomicity atomicity) {
      assert(!hasOwnership());
      return insert(new (getModule()) ReleaseValueAddrInst(
         getPILDebugLocation(Loc), operand, atomicity));
   }

   UnmanagedRetainValueInst *createUnmanagedRetainValue(PILLocation Loc,
                                                        PILValue operand,
                                                        Atomicity atomicity) {
      assert(hasOwnership());
      assert(isLoadableOrOpaque(operand->getType()));
      return insert(new (getModule()) UnmanagedRetainValueInst(
         getPILDebugLocation(Loc), operand, atomicity));
   }

   UnmanagedReleaseValueInst *createUnmanagedReleaseValue(PILLocation Loc,
                                                          PILValue operand,
                                                          Atomicity atomicity) {
      assert(hasOwnership());
      assert(isLoadableOrOpaque(operand->getType()));
      return insert(new (getModule()) UnmanagedReleaseValueInst(
         getPILDebugLocation(Loc), operand, atomicity));
   }

   AutoreleaseValueInst *createAutoreleaseValue(PILLocation Loc,
                                                PILValue operand,
                                                Atomicity atomicity) {
      return insert(new (getModule()) AutoreleaseValueInst(
         getPILDebugLocation(Loc), operand, atomicity));
   }

   UnmanagedAutoreleaseValueInst *
   createUnmanagedAutoreleaseValue(PILLocation Loc, PILValue operand,
                                   Atomicity atomicity) {
      return insert(new (getModule()) UnmanagedAutoreleaseValueInst(
         getPILDebugLocation(Loc), operand, atomicity));
   }

   SetDeallocatingInst *createSetDeallocating(PILLocation Loc,
                                              PILValue operand,
                                              Atomicity atomicity) {
      return insert(new (getModule()) SetDeallocatingInst(
         getPILDebugLocation(Loc), operand, atomicity));
   }

   ObjectInst *createObject(PILLocation Loc, PILType Ty,
                            ArrayRef<PILValue> Elements,
                            unsigned NumBaseElements) {
      return insert(ObjectInst::create(getPILDebugLocation(Loc), Ty, Elements,
                                       NumBaseElements, getModule(),
                                       hasOwnership()));
   }

   StructInst *createStruct(PILLocation Loc, PILType Ty,
                            ArrayRef<PILValue> Elements) {
      assert(isLoadableOrOpaque(Ty));
      return insert(StructInst::create(getPILDebugLocation(Loc), Ty, Elements,
                                       getModule(), hasOwnership()));
   }

   TupleInst *createTuple(PILLocation Loc, PILType Ty,
                          ArrayRef<PILValue> Elements) {
      assert(isLoadableOrOpaque(Ty));
      return insert(TupleInst::create(getPILDebugLocation(Loc), Ty, Elements,
                                      getModule(), hasOwnership()));
   }

   TupleInst *createTuple(PILLocation loc, ArrayRef<PILValue> elts);

   EnumInst *createEnum(PILLocation Loc, PILValue Operand,
                        EnumElementDecl *Element, PILType Ty) {
      assert(isLoadableOrOpaque(Ty));
      return insert(new (getModule()) EnumInst(getPILDebugLocation(Loc),
                                               Operand, Element, Ty));
   }

   /// Inject a loadable value into the corresponding optional type.
   EnumInst *createOptionalSome(PILLocation Loc, PILValue operand, PILType ty) {
      assert(isLoadableOrOpaque(ty));
      auto someDecl = getModule().getAstContext().getOptionalSomeDecl();
      return createEnum(Loc, operand, someDecl, ty);
   }

   /// Create the nil value of a loadable optional type.
   EnumInst *createOptionalNone(PILLocation Loc, PILType ty) {
      assert(isLoadableOrOpaque(ty));
      auto noneDecl = getModule().getAstContext().getOptionalNoneDecl();
      return createEnum(Loc, nullptr, noneDecl, ty);
   }

   InitEnumDataAddrInst *createInitEnumDataAddr(PILLocation Loc,
                                                PILValue Operand,
                                                EnumElementDecl *Element,
                                                PILType Ty) {
      return insert(new (getModule()) InitEnumDataAddrInst(
         getPILDebugLocation(Loc), Operand, Element, Ty));
   }

   UncheckedEnumDataInst *createUncheckedEnumData(PILLocation Loc,
                                                  PILValue Operand,
                                                  EnumElementDecl *Element,
                                                  PILType Ty) {
      assert(isLoadableOrOpaque(Ty));
      return insert(new (getModule()) UncheckedEnumDataInst(
         getPILDebugLocation(Loc), Operand, Element, Ty));
   }

   UncheckedEnumDataInst *createUncheckedEnumData(PILLocation Loc,
                                                  PILValue Operand,
                                                  EnumElementDecl *Element) {
      PILType EltType = Operand->getType().getEnumElementType(
         Element, getModule(), getTypeExpansionContext());
      return createUncheckedEnumData(Loc, Operand, Element, EltType);
   }

   /// Return unchecked_enum_data %Operand, #Optional<T>.some.
   PILValue emitExtractOptionalPayloadOperation(PILLocation Loc,
                                                PILValue Operand) {
      auto *Decl = F->getAstContext().getOptionalSomeDecl();
      return createUncheckedEnumData(Loc, Operand, Decl);
   }

   UncheckedTakeEnumDataAddrInst *
   createUncheckedTakeEnumDataAddr(PILLocation Loc, PILValue Operand,
                                   EnumElementDecl *Element, PILType Ty) {
      return insert(new (getModule()) UncheckedTakeEnumDataAddrInst(
         getPILDebugLocation(Loc), Operand, Element, Ty));
   }

   UncheckedTakeEnumDataAddrInst *
   createUncheckedTakeEnumDataAddr(PILLocation Loc, PILValue Operand,
                                   EnumElementDecl *Element) {
      PILType EltType = Operand->getType().getEnumElementType(
         Element, getModule(), getTypeExpansionContext());
      return createUncheckedTakeEnumDataAddr(Loc, Operand, Element, EltType);
   }

   InjectEnumAddrInst *createInjectEnumAddr(PILLocation Loc, PILValue Operand,
                                            EnumElementDecl *Element) {
      return insert(new (getModule()) InjectEnumAddrInst(
         getPILDebugLocation(Loc), Operand, Element));
   }

   SelectEnumInst *
   createSelectEnum(PILLocation Loc, PILValue Operand, PILType Ty,
                    PILValue DefaultValue,
                    ArrayRef<std::pair<EnumElementDecl *, PILValue>> CaseValues,
                    Optional<ArrayRef<ProfileCounter>> CaseCounts = None,
                    ProfileCounter DefaultCount = ProfileCounter()) {
      assert(isLoadableOrOpaque(Ty));
      return insert(SelectEnumInst::create(
         getPILDebugLocation(Loc), Operand, Ty, DefaultValue, CaseValues,
         getModule(), CaseCounts, DefaultCount, hasOwnership()));
   }

   SelectEnumAddrInst *createSelectEnumAddr(
      PILLocation Loc, PILValue Operand, PILType Ty, PILValue DefaultValue,
      ArrayRef<std::pair<EnumElementDecl *, PILValue>> CaseValues,
      Optional<ArrayRef<ProfileCounter>> CaseCounts = None,
      ProfileCounter DefaultCount = ProfileCounter()) {
      return insert(SelectEnumAddrInst::create(
         getPILDebugLocation(Loc), Operand, Ty, DefaultValue, CaseValues,
         getModule(), CaseCounts, DefaultCount));
   }

   SelectValueInst *createSelectValue(
      PILLocation Loc, PILValue Operand, PILType Ty, PILValue DefaultResult,
      ArrayRef<std::pair<PILValue, PILValue>> CaseValuesAndResults) {
      return insert(SelectValueInst::create(getPILDebugLocation(Loc), Operand, Ty,
                                            DefaultResult, CaseValuesAndResults,
                                            getModule(), hasOwnership()));
   }

   TupleExtractInst *createTupleExtract(PILLocation Loc, PILValue Operand,
                                        unsigned FieldNo, PILType ResultTy) {
      return insert(new (getModule()) TupleExtractInst(
         getPILDebugLocation(Loc), Operand, FieldNo, ResultTy));
   }

   TupleExtractInst *createTupleExtract(PILLocation Loc, PILValue Operand,
                                        unsigned FieldNo) {
      auto type = Operand->getType().getTupleElementType(FieldNo);
      return createTupleExtract(Loc, Operand, FieldNo, type);
   }

   TupleElementAddrInst *createTupleElementAddr(PILLocation Loc,
                                                PILValue Operand,
                                                unsigned FieldNo,
                                                PILType ResultTy) {
      return insert(new (getModule()) TupleElementAddrInst(
         getPILDebugLocation(Loc), Operand, FieldNo, ResultTy));
   }

   TupleElementAddrInst *
   createTupleElementAddr(PILLocation Loc, PILValue Operand, unsigned FieldNo) {
      return insert(new (getModule()) TupleElementAddrInst(
         getPILDebugLocation(Loc), Operand, FieldNo,
         Operand->getType().getTupleElementType(FieldNo)));
   }

   StructExtractInst *createStructExtract(PILLocation Loc, PILValue Operand,
                                          VarDecl *Field, PILType ResultTy) {
      return insert(new (getModule()) StructExtractInst(
         getPILDebugLocation(Loc), Operand, Field, ResultTy));
   }

   StructExtractInst *createStructExtract(PILLocation Loc, PILValue Operand,
                                          VarDecl *Field) {
      auto type = Operand->getType().getFieldType(Field, getModule(),
                                                  getTypeExpansionContext());
      return createStructExtract(Loc, Operand, Field, type);
   }

   StructElementAddrInst *createStructElementAddr(PILLocation Loc,
                                                  PILValue Operand,
                                                  VarDecl *Field,
                                                  PILType ResultTy) {
      return insert(new (getModule()) StructElementAddrInst(
         getPILDebugLocation(Loc), Operand, Field, ResultTy));
   }

   StructElementAddrInst *
   createStructElementAddr(PILLocation Loc, PILValue Operand, VarDecl *Field) {
      auto ResultTy = Operand->getType().getFieldType(Field, getModule(),
                                                      getTypeExpansionContext());
      return createStructElementAddr(Loc, Operand, Field, ResultTy);
   }

   RefElementAddrInst *createRefElementAddr(PILLocation Loc, PILValue Operand,
                                            VarDecl *Field, PILType ResultTy) {
      return insert(new (getModule()) RefElementAddrInst(
         getPILDebugLocation(Loc), Operand, Field, ResultTy));
   }
   RefElementAddrInst *createRefElementAddr(PILLocation Loc, PILValue Operand,
                                            VarDecl *Field) {
      auto ResultTy = Operand->getType().getFieldType(Field, getModule(),
                                                      getTypeExpansionContext());
      return createRefElementAddr(Loc, Operand, Field, ResultTy);
   }

   RefTailAddrInst *createRefTailAddr(PILLocation Loc, PILValue Ref,
                                      PILType ResultTy) {
      return insert(new (getModule()) RefTailAddrInst(getPILDebugLocation(Loc),
                                                      Ref, ResultTy));
   }

   DestructureStructInst *createDestructureStruct(PILLocation Loc,
                                                  PILValue Operand) {
      return insert(DestructureStructInst::create(
         getFunction(), getPILDebugLocation(Loc), Operand));
   }

   DestructureTupleInst *createDestructureTuple(PILLocation Loc,
                                                PILValue Operand) {
      return insert(DestructureTupleInst::create(
         getFunction(), getPILDebugLocation(Loc), Operand));
   }

   MultipleValueInstruction *emitDestructureValueOperation(PILLocation loc,
                                                           PILValue operand) {
      // If you hit this assert, you are using the wrong method. Use instead:
      //
      // emitDestructureValueOperation(PILLocation, PILValue,
      //                               SmallVectorImpl<PILValue> &);
      assert(hasOwnership() && "Expected to be called in ownership code only.");
      PILType opTy = operand->getType();
      if (opTy.is<TupleType>())
         return createDestructureTuple(loc, operand);
      if (opTy.getStructOrBoundGenericStruct())
         return createDestructureStruct(loc, operand);
      llvm_unreachable("Can not emit a destructure for this type of operand.");
   }

   void
   emitDestructureValueOperation(PILLocation loc, PILValue operand,
                                 function_ref<void(unsigned, PILValue)> func);

   void emitDestructureValueOperation(PILLocation loc, PILValue operand,
                                      SmallVectorImpl<PILValue> &result);

   void emitDestructureAddressOperation(PILLocation loc, PILValue operand,
                                        SmallVectorImpl<PILValue> &result);

   ClassMethodInst *createClassMethod(PILLocation Loc, PILValue Operand,
                                      PILDeclRef Member, PILType MethodTy) {
      return insert(new (getModule()) ClassMethodInst(
         getPILDebugLocation(Loc), Operand, Member, MethodTy));
   }

   SuperMethodInst *createSuperMethod(PILLocation Loc, PILValue Operand,
                                      PILDeclRef Member, PILType MethodTy) {
      return insert(new (getModule()) SuperMethodInst(
         getPILDebugLocation(Loc), Operand, Member, MethodTy));
   }

   ObjCMethodInst *createObjCMethod(PILLocation Loc, PILValue Operand,
                                    PILDeclRef Member, PILType MethodTy) {
      return insert(ObjCMethodInst::create(getPILDebugLocation(Loc), Operand,
                                           Member, MethodTy, &getFunction(),
                                           C.OpenedArchetypes));
   }

   ObjCSuperMethodInst *createObjCSuperMethod(PILLocation Loc, PILValue Operand,
                                              PILDeclRef Member, PILType MethodTy) {
      return insert(new (getModule()) ObjCSuperMethodInst(
         getPILDebugLocation(Loc), Operand, Member, MethodTy));
   }

   WitnessMethodInst *createWitnessMethod(PILLocation Loc, CanType LookupTy,
                                          InterfaceConformanceRef Conformance,
                                          PILDeclRef Member, PILType MethodTy) {
      return insert(WitnessMethodInst::create(
         getPILDebugLocation(Loc), LookupTy, Conformance, Member, MethodTy,
         &getFunction(), C.OpenedArchetypes));
   }

   OpenExistentialAddrInst *
   createOpenExistentialAddr(PILLocation Loc, PILValue Operand, PILType SelfTy,
                             OpenedExistentialAccess ForAccess) {
      auto *I = insert(new (getModule()) OpenExistentialAddrInst(
         getPILDebugLocation(Loc), Operand, SelfTy, ForAccess));
      if (C.OpenedArchetypesTracker)
         C.OpenedArchetypesTracker->registerOpenedArchetypes(I);
      return I;
   }

   OpenExistentialValueInst *createOpenExistentialValue(PILLocation Loc,
                                                        PILValue Operand,
                                                        PILType SelfTy) {
      auto *I = insert(new (getModule()) OpenExistentialValueInst(
         getPILDebugLocation(Loc), Operand, SelfTy));
      if (C.OpenedArchetypesTracker)
         C.OpenedArchetypesTracker->registerOpenedArchetypes(I);
      return I;
   }

   OpenExistentialMetatypeInst *createOpenExistentialMetatype(PILLocation Loc,
                                                              PILValue operand,
                                                              PILType selfTy) {
      auto *I = insert(new (getModule()) OpenExistentialMetatypeInst(
         getPILDebugLocation(Loc), operand, selfTy));
      if (C.OpenedArchetypesTracker)
         C.OpenedArchetypesTracker->registerOpenedArchetypes(I);
      return I;
   }

   OpenExistentialRefInst *
   createOpenExistentialRef(PILLocation Loc, PILValue Operand, PILType Ty) {
      auto *I = insert(new (getModule()) OpenExistentialRefInst(
         getPILDebugLocation(Loc), Operand, Ty, hasOwnership()));
      if (C.OpenedArchetypesTracker)
         C.OpenedArchetypesTracker->registerOpenedArchetypes(I);
      return I;
   }

   OpenExistentialBoxInst *
   createOpenExistentialBox(PILLocation Loc, PILValue Operand, PILType Ty) {
      auto *I = insert(new (getModule()) OpenExistentialBoxInst(
         getPILDebugLocation(Loc), Operand, Ty));
      if (C.OpenedArchetypesTracker)
         C.OpenedArchetypesTracker->registerOpenedArchetypes(I);
      return I;
   }

   OpenExistentialBoxValueInst *
   createOpenExistentialBoxValue(PILLocation Loc, PILValue Operand, PILType Ty) {
      auto *I = insert(new (getModule()) OpenExistentialBoxValueInst(
         getPILDebugLocation(Loc), Operand, Ty));
      if (C.OpenedArchetypesTracker)
         C.OpenedArchetypesTracker->registerOpenedArchetypes(I);
      return I;
   }

   InitExistentialAddrInst *
   createInitExistentialAddr(PILLocation Loc, PILValue Existential,
                             CanType FormalConcreteType,
                             PILType LoweredConcreteType,
                             ArrayRef<InterfaceConformanceRef> Conformances) {
      return insert(InitExistentialAddrInst::create(
         getPILDebugLocation(Loc), Existential, FormalConcreteType,
         LoweredConcreteType, Conformances, &getFunction(), C.OpenedArchetypes));
   }

   InitExistentialValueInst *
   createInitExistentialValue(PILLocation Loc, PILType ExistentialType,
                              CanType FormalConcreteType, PILValue Concrete,
                              ArrayRef<InterfaceConformanceRef> Conformances) {
      return insert(InitExistentialValueInst::create(
         getPILDebugLocation(Loc), ExistentialType, FormalConcreteType, Concrete,
         Conformances, &getFunction(), C.OpenedArchetypes));
   }

   InitExistentialMetatypeInst *
   createInitExistentialMetatype(PILLocation Loc, PILValue metatype,
                                 PILType existentialType,
                                 ArrayRef<InterfaceConformanceRef> conformances) {
      return insert(InitExistentialMetatypeInst::create(
         getPILDebugLocation(Loc), existentialType, metatype, conformances,
         &getFunction(), C.OpenedArchetypes));
   }

   InitExistentialRefInst *
   createInitExistentialRef(PILLocation Loc, PILType ExistentialType,
                            CanType FormalConcreteType, PILValue Concrete,
                            ArrayRef<InterfaceConformanceRef> Conformances) {
      return insert(InitExistentialRefInst::create(
         getPILDebugLocation(Loc), ExistentialType, FormalConcreteType, Concrete,
         Conformances, &getFunction(), C.OpenedArchetypes));
   }

   DeinitExistentialAddrInst *createDeinitExistentialAddr(PILLocation Loc,
                                                          PILValue Existential) {
      return insert(new (getModule()) DeinitExistentialAddrInst(
         getPILDebugLocation(Loc), Existential));
   }

   DeinitExistentialValueInst *
   createDeinitExistentialValue(PILLocation Loc, PILValue Existential) {
      return insert(new (getModule()) DeinitExistentialValueInst(
         getPILDebugLocation(Loc), Existential));
   }

   ProjectBlockStorageInst *createProjectBlockStorage(PILLocation Loc,
                                                      PILValue Storage) {
      auto CaptureTy = Storage->getType()
         .castTo<PILBlockStorageType>()
         ->getCaptureAddressType();
      return createProjectBlockStorage(Loc, Storage, CaptureTy);
   }
   ProjectBlockStorageInst *createProjectBlockStorage(PILLocation Loc,
                                                      PILValue Storage,
                                                      PILType CaptureTy) {
      return insert(new (getModule()) ProjectBlockStorageInst(
         getPILDebugLocation(Loc), Storage, CaptureTy));
   }

   InitBlockStorageHeaderInst *
   createInitBlockStorageHeader(PILLocation Loc, PILValue BlockStorage,
                                PILValue InvokeFunction, PILType BlockType,
                                SubstitutionMap Subs) {
      return insert(InitBlockStorageHeaderInst::create(getFunction(),
                                                       getPILDebugLocation(Loc), BlockStorage, InvokeFunction, BlockType, Subs));
   }

   MetatypeInst *createMetatype(PILLocation Loc, PILType Metatype) {
      return insert(MetatypeInst::create(getPILDebugLocation(Loc), Metatype,
                                         &getFunction(), C.OpenedArchetypes));
   }

   // @todo
//   ObjCMetatypeToObjectInst *
//   createObjCMetatypeToObject(PILLocation Loc, PILValue Op, PILType Ty) {
//      return insert(new (getModule()) ObjCMetatypeToObjectInst(
//         getPILDebugLocation(Loc), Op, Ty));
//   }
//
//   ObjCExistentialMetatypeToObjectInst *
//   createObjCExistentialMetatypeToObject(PILLocation Loc, PILValue Op,
//                                         PILType Ty) {
//      return insert(new (getModule()) ObjCExistentialMetatypeToObjectInst(
//         getPILDebugLocation(Loc), Op, Ty));
//   }

   ValueMetatypeInst *createValueMetatype(PILLocation Loc, PILType Metatype,
                                          PILValue Base);

   ExistentialMetatypeInst *
   createExistentialMetatype(PILLocation Loc, PILType Metatype, PILValue Base) {
      return insert(new (getModule()) ExistentialMetatypeInst(
         getPILDebugLocation(Loc), Metatype, Base));
   }

   CopyBlockInst *createCopyBlock(PILLocation Loc, PILValue Operand) {
      return insert(new (getModule())
                       CopyBlockInst(getPILDebugLocation(Loc), Operand));
   }

   CopyBlockWithoutEscapingInst *
   createCopyBlockWithoutEscaping(PILLocation Loc, PILValue Block,
                                  PILValue Closure) {
      return insert(new (getModule()) CopyBlockWithoutEscapingInst(
         getPILDebugLocation(Loc), Block, Closure));
   }

   StrongRetainInst *createStrongRetain(PILLocation Loc, PILValue Operand,
                                        Atomicity atomicity) {
      assert(!hasOwnership());
      return insert(new (getModule()) StrongRetainInst(getPILDebugLocation(Loc),
                                                       Operand, atomicity));
   }
   StrongReleaseInst *createStrongRelease(PILLocation Loc, PILValue Operand,
                                          Atomicity atomicity) {
      assert(!hasOwnership());
      return insert(new (getModule()) StrongReleaseInst(
         getPILDebugLocation(Loc), Operand, atomicity));
   }

   EndLifetimeInst *createEndLifetime(PILLocation Loc, PILValue Operand) {
      return insert(new (getModule())
                       EndLifetimeInst(getPILDebugLocation(Loc), Operand));
   }

   UncheckedOwnershipConversionInst *
   createUncheckedOwnershipConversion(PILLocation Loc, PILValue Operand,
                                      ValueOwnershipKind Kind) {
      return insert(new (getModule()) UncheckedOwnershipConversionInst(
         getPILDebugLocation(Loc), Operand, Kind));
   }

   FixLifetimeInst *createFixLifetime(PILLocation Loc, PILValue Operand) {
      return insert(new (getModule())
                       FixLifetimeInst(getPILDebugLocation(Loc), Operand));
   }
   void emitFixLifetime(PILLocation Loc, PILValue Operand) {
      if (getTypeLowering(Operand->getType()).isTrivial())
         return;
      createFixLifetime(Loc, Operand);
   }
   ClassifyBridgeObjectInst *createClassifyBridgeObject(PILLocation Loc,
                                                        PILValue value);
   MarkDependenceInst *createMarkDependence(PILLocation Loc, PILValue value,
                                            PILValue base) {
      return insert(new (getModule()) MarkDependenceInst(
         getPILDebugLocation(Loc), value, base));
   }
   IsUniqueInst *createIsUnique(PILLocation Loc, PILValue operand) {
      auto Int1Ty = PILType::getBuiltinIntegerType(1, getAstContext());
      return insert(new (getModule()) IsUniqueInst(getPILDebugLocation(Loc),
                                                   operand, Int1Ty));
   }
   IsEscapingClosureInst *createIsEscapingClosure(PILLocation Loc,
                                                  PILValue operand,
                                                  unsigned VerificationType) {
      auto Int1Ty = PILType::getBuiltinIntegerType(1, getAstContext());
      return insert(new (getModule()) IsEscapingClosureInst(
         getPILDebugLocation(Loc), operand, Int1Ty, VerificationType));
   }

   DeallocStackInst *createDeallocStack(PILLocation Loc, PILValue operand) {
      return insert(new (getModule())
                       DeallocStackInst(getPILDebugLocation(Loc), operand));
   }
   DeallocRefInst *createDeallocRef(PILLocation Loc, PILValue operand,
                                    bool canBeOnStack) {
      return insert(new (getModule()) DeallocRefInst(
         getPILDebugLocation(Loc), operand, canBeOnStack));
   }
   DeallocPartialRefInst *createDeallocPartialRef(PILLocation Loc,
                                                  PILValue operand,
                                                  PILValue metatype) {
      return insert(new (getModule()) DeallocPartialRefInst(
         getPILDebugLocation(Loc), operand, metatype));
   }
   DeallocBoxInst *createDeallocBox(PILLocation Loc,
                                    PILValue operand) {
      return insert(new (getModule()) DeallocBoxInst(
         getPILDebugLocation(Loc), operand));
   }
   DeallocExistentialBoxInst *createDeallocExistentialBox(PILLocation Loc,
                                                          CanType concreteType,
                                                          PILValue operand) {
      return insert(new (getModule()) DeallocExistentialBoxInst(
         getPILDebugLocation(Loc), concreteType, operand));
   }
   DeallocValueBufferInst *createDeallocValueBuffer(PILLocation Loc,
                                                    PILType valueType,
                                                    PILValue operand) {
      return insert(new (getModule()) DeallocValueBufferInst(
         getPILDebugLocation(Loc), valueType, operand));
   }
   DestroyAddrInst *createDestroyAddr(PILLocation Loc, PILValue Operand) {
      return insert(new (getModule())
                       DestroyAddrInst(getPILDebugLocation(Loc), Operand));
   }

   ProjectValueBufferInst *createProjectValueBuffer(PILLocation Loc,
                                                    PILType valueType,
                                                    PILValue operand) {
      return insert(new (getModule()) ProjectValueBufferInst(
         getPILDebugLocation(Loc), valueType, operand));
   }
   ProjectBoxInst *createProjectBox(PILLocation Loc, PILValue boxOperand,
                                    unsigned index);
   ProjectExistentialBoxInst *createProjectExistentialBox(PILLocation Loc,
                                                          PILType valueTy,
                                                          PILValue boxOperand) {
      return insert(new (getModule()) ProjectExistentialBoxInst(
         getPILDebugLocation(Loc), valueTy, boxOperand));
   }

   //===--------------------------------------------------------------------===//
   // Unchecked cast helpers
   //===--------------------------------------------------------------------===//

   // Create an UncheckedRefCast if the source and dest types are legal,
   // otherwise return null.
   // Unwrap or wrap optional types as needed.
   SingleValueInstruction *tryCreateUncheckedRefCast(PILLocation Loc, PILValue Op,
                                                     PILType ResultTy);

   // Create the appropriate cast instruction based on result type.
   SingleValueInstruction *createUncheckedBitCast(PILLocation Loc, PILValue Op,
                                                  PILType Ty);

   //===--------------------------------------------------------------------===//
   // Runtime failure
   //===--------------------------------------------------------------------===//

   CondFailInst *createCondFail(PILLocation Loc, PILValue Operand,
                                StringRef Message, bool Inverted = false) {
      if (Inverted) {
         PILType Ty = Operand->getType();
         PILValue True(createIntegerLiteral(Loc, Ty, 1));
         Operand =
            createBuiltinBinaryFunction(Loc, "xor", Ty, Ty, {Operand, True});
      }
      return insert(CondFailInst::create(getPILDebugLocation(Loc), Operand,
                                         Message, getModule()));
   }

   BuiltinInst *createBuiltinTrap(PILLocation Loc) {
      AstContext &AST = getAstContext();
      auto Id_trap = AST.getIdentifier("int_trap");
      return createBuiltin(Loc, Id_trap, getModule().Types.getEmptyTupleType(),
                           {}, {});
   }

   //===--------------------------------------------------------------------===//
   // Array indexing instructions
   //===--------------------------------------------------------------------===//

   IndexAddrInst *createIndexAddr(PILLocation Loc, PILValue Operand,
                                  PILValue Index) {
      return insert(new (getModule()) IndexAddrInst(getPILDebugLocation(Loc),
                                                    Operand, Index));
   }

   TailAddrInst *createTailAddr(PILLocation Loc, PILValue Operand,
                                PILValue Count, PILType ResultTy) {
      return insert(new (getModule()) TailAddrInst(getPILDebugLocation(Loc),
                                                   Operand, Count, ResultTy));
   }

   IndexRawPointerInst *createIndexRawPointer(PILLocation Loc, PILValue Operand,
                                              PILValue Index) {
      return insert(new (getModule()) IndexRawPointerInst(
         getPILDebugLocation(Loc), Operand, Index));
   }

   //===--------------------------------------------------------------------===//
   // Terminator PILInstruction Creation Methods
   //===--------------------------------------------------------------------===//

   UnreachableInst *createUnreachable(PILLocation Loc) {
      return insertTerminator(new (getModule())
                                 UnreachableInst(getPILDebugLocation(Loc)));
   }

   ReturnInst *createReturn(PILLocation Loc, PILValue ReturnValue) {
      return insertTerminator(new (getModule()) ReturnInst(
         getPILDebugLocation(Loc), ReturnValue));
   }

   ThrowInst *createThrow(PILLocation Loc, PILValue errorValue) {
      return insertTerminator(
         new (getModule()) ThrowInst(getPILDebugLocation(Loc), errorValue));
   }

   UnwindInst *createUnwind(PILLocation loc) {
      return insertTerminator(
         new (getModule()) UnwindInst(getPILDebugLocation(loc)));
   }

   YieldInst *createYield(PILLocation loc, ArrayRef<PILValue> yieldedValues,
                          PILBasicBlock *resumeBB, PILBasicBlock *unwindBB) {
      return insertTerminator(
         YieldInst::create(getPILDebugLocation(loc), yieldedValues,
                           resumeBB, unwindBB, getFunction()));
   }

   CondBranchInst *
   createCondBranch(PILLocation Loc, PILValue Cond, PILBasicBlock *Target1,
                    PILBasicBlock *Target2,
                    ProfileCounter Target1Count = ProfileCounter(),
                    ProfileCounter Target2Count = ProfileCounter()) {
      return insertTerminator(
         CondBranchInst::create(getPILDebugLocation(Loc), Cond, Target1, Target2,
                                Target1Count, Target2Count, getFunction()));
   }

   CondBranchInst *
   createCondBranch(PILLocation Loc, PILValue Cond, PILBasicBlock *Target1,
                    ArrayRef<PILValue> Args1, PILBasicBlock *Target2,
                    ArrayRef<PILValue> Args2,
                    ProfileCounter Target1Count = ProfileCounter(),
                    ProfileCounter Target2Count = ProfileCounter()) {
      return insertTerminator(
         CondBranchInst::create(getPILDebugLocation(Loc), Cond, Target1, Args1,
                                Target2, Args2, Target1Count, Target2Count, getFunction()));
   }

   CondBranchInst *
   createCondBranch(PILLocation Loc, PILValue Cond, PILBasicBlock *Target1,
                    OperandValueArrayRef Args1, PILBasicBlock *Target2,
                    OperandValueArrayRef Args2,
                    ProfileCounter Target1Count = ProfileCounter(),
                    ProfileCounter Target2Count = ProfileCounter()) {
      SmallVector<PILValue, 6> ArgsCopy1;
      SmallVector<PILValue, 6> ArgsCopy2;

      ArgsCopy1.reserve(Args1.size());
      ArgsCopy2.reserve(Args2.size());

      for (auto I = Args1.begin(), E = Args1.end(); I != E; ++I)
         ArgsCopy1.push_back(*I);
      for (auto I = Args2.begin(), E = Args2.end(); I != E; ++I)
         ArgsCopy2.push_back(*I);

      return insertTerminator(CondBranchInst::create(
         getPILDebugLocation(Loc), Cond, Target1, ArgsCopy1, Target2, ArgsCopy2,
         Target1Count, Target2Count, getFunction()));
   }

   BranchInst *createBranch(PILLocation Loc, PILBasicBlock *TargetBlock) {
      return insertTerminator(
         BranchInst::create(getPILDebugLocation(Loc), TargetBlock, getFunction()));
   }

   BranchInst *createBranch(PILLocation Loc, PILBasicBlock *TargetBlock,
                            ArrayRef<PILValue> Args) {
      return insertTerminator(
         BranchInst::create(getPILDebugLocation(Loc), TargetBlock, Args,
                            getFunction()));
   }

   BranchInst *createBranch(PILLocation Loc, PILBasicBlock *TargetBlock,
                            OperandValueArrayRef Args);

   SwitchValueInst *
   createSwitchValue(PILLocation Loc, PILValue Operand, PILBasicBlock *DefaultBB,
                     ArrayRef<std::pair<PILValue, PILBasicBlock *>> CaseBBs) {
      return insertTerminator(SwitchValueInst::create(
         getPILDebugLocation(Loc), Operand, DefaultBB, CaseBBs, getFunction()));
   }

   SwitchEnumInst *createSwitchEnum(
      PILLocation Loc, PILValue Operand, PILBasicBlock *DefaultBB,
      ArrayRef<std::pair<EnumElementDecl *, PILBasicBlock *>> CaseBBs,
      Optional<ArrayRef<ProfileCounter>> CaseCounts = None,
      ProfileCounter DefaultCount = ProfileCounter()) {
      return insertTerminator(SwitchEnumInst::create(
         getPILDebugLocation(Loc), Operand, DefaultBB, CaseBBs, getFunction(),
         CaseCounts, DefaultCount));
   }

   SwitchEnumAddrInst *createSwitchEnumAddr(
      PILLocation Loc, PILValue Operand, PILBasicBlock *DefaultBB,
      ArrayRef<std::pair<EnumElementDecl *, PILBasicBlock *>> CaseBBs,
      Optional<ArrayRef<ProfileCounter>> CaseCounts = None,
      ProfileCounter DefaultCount = ProfileCounter()) {
      return insertTerminator(SwitchEnumAddrInst::create(
         getPILDebugLocation(Loc), Operand, DefaultBB, CaseBBs, getFunction(),
         CaseCounts, DefaultCount));
   }

   DynamicMethodBranchInst *
   createDynamicMethodBranch(PILLocation Loc, PILValue Operand,
                             PILDeclRef Member, PILBasicBlock *HasMethodBB,
                             PILBasicBlock *NoMethodBB) {
      return insertTerminator(
         DynamicMethodBranchInst::create(getPILDebugLocation(Loc), Operand,
                                         Member, HasMethodBB, NoMethodBB, getFunction()));
   }

   CheckedCastBranchInst *
   createCheckedCastBranch(PILLocation Loc, bool isExact, PILValue op,
                           PILType destLoweredTy, CanType destFormalTy,
                           PILBasicBlock *successBB,
                           PILBasicBlock *failureBB,
                           ProfileCounter Target1Count = ProfileCounter(),
                           ProfileCounter Target2Count = ProfileCounter());

   CheckedCastValueBranchInst *
   createCheckedCastValueBranch(PILLocation Loc,
                                PILValue op, CanType srcFormalTy,
                                PILType destLoweredTy,
                                CanType destFormalTy,
                                PILBasicBlock *successBB,
                                PILBasicBlock *failureBB) {
      return insertTerminator(CheckedCastValueBranchInst::create(
         getPILDebugLocation(Loc), op, srcFormalTy,
         destLoweredTy, destFormalTy,
         successBB, failureBB, getFunction(), C.OpenedArchetypes));
   }

   CheckedCastAddrBranchInst *
   createCheckedCastAddrBranch(PILLocation Loc, CastConsumptionKind consumption,
                               PILValue src, CanType sourceFormalType,
                               PILValue dest, CanType targetFormalType,
                               PILBasicBlock *successBB,
                               PILBasicBlock *failureBB,
                               ProfileCounter Target1Count = ProfileCounter(),
                               ProfileCounter Target2Count = ProfileCounter()) {
      return insertTerminator(new (getModule()) CheckedCastAddrBranchInst(
         getPILDebugLocation(Loc), consumption, src, sourceFormalType, dest,
         targetFormalType, successBB, failureBB, Target1Count, Target2Count));
   }

   //===--------------------------------------------------------------------===//
   // Memory management helpers
   //===--------------------------------------------------------------------===//

   /// Returns the default atomicity of the module.
   Atomicity getDefaultAtomicity() {
      return getModule().isDefaultAtomic() ? Atomicity::Atomic : Atomicity::NonAtomic;
   }

   /// Try to fold a destroy_addr operation into the previous instructions, or
   /// generate an explicit one if that fails.  If this inserts a new
   /// instruction, it returns it, otherwise it returns null.
   DestroyAddrInst *emitDestroyAddrAndFold(PILLocation Loc, PILValue Operand) {
      auto U = emitDestroyAddr(Loc, Operand);
      if (U.isNull() || !U.is<DestroyAddrInst *>())
         return nullptr;
      return U.get<DestroyAddrInst *>();
   }

   /// Perform a strong_release instruction at the current location, attempting
   /// to fold it locally into nearby retain instructions or emitting an explicit
   /// strong release if necessary.  If this inserts a new instruction, it
   /// returns it, otherwise it returns null.
   StrongReleaseInst *emitStrongReleaseAndFold(PILLocation Loc,
                                               PILValue Operand) {
      auto U = emitStrongRelease(Loc, Operand);
      if (U.isNull())
         return nullptr;
      if (auto *SRI = U.dyn_cast<StrongReleaseInst *>())
         return SRI;
      U.get<StrongRetainInst *>()->eraseFromParent();
      return nullptr;
   }

   /// Emit a release_value instruction at the current location, attempting to
   /// fold it locally into another nearby retain_value instruction.  This
   /// returns the new instruction if it inserts one, otherwise it returns null.
   ///
   /// This instruction doesn't handle strength reduction of release_value into
   /// a noop / strong_release / unowned_release.  For that, use the
   /// emitReleaseValueOperation method below or use the TypeLowering API.
   ReleaseValueInst *emitReleaseValueAndFold(PILLocation Loc, PILValue Operand) {
      auto U = emitReleaseValue(Loc, Operand);
      if (U.isNull())
         return nullptr;
      if (auto *RVI = U.dyn_cast<ReleaseValueInst *>())
         return RVI;
      U.get<RetainValueInst *>()->eraseFromParent();
      return nullptr;
   }

   /// Emit a release_value instruction at the current location, attempting to
   /// fold it locally into another nearby retain_value instruction.  This
   /// returns the new instruction if it inserts one, otherwise it returns null.
   ///
   /// This instruction doesn't handle strength reduction of release_value into
   /// a noop / strong_release / unowned_release.  For that, use the
   /// emitReleaseValueOperation method below or use the TypeLowering API.
   DestroyValueInst *emitDestroyValueAndFold(PILLocation Loc, PILValue Operand) {
      auto U = emitDestroyValue(Loc, Operand);
      if (U.isNull())
         return nullptr;
      if (auto *DVI = U.dyn_cast<DestroyValueInst *>())
         return DVI;
      auto *CVI = U.get<CopyValueInst *>();
      CVI->replaceAllUsesWith(CVI->getOperand());
      CVI->eraseFromParent();
      return nullptr;
   }

   /// Emit a release_value instruction at the current location, attempting to
   /// fold it locally into another nearby retain_value instruction. Returns a
   /// pointer union initialized with a release value inst if it inserts one,
   /// otherwise returns the retain. It is expected that the caller will remove
   /// the retain_value. This allows for the caller to update any state before
   /// the retain_value is destroyed.
   PointerUnion<RetainValueInst *, ReleaseValueInst *>
   emitReleaseValue(PILLocation Loc, PILValue Operand);

   /// Emit a strong_release instruction at the current location, attempting to
   /// fold it locally into another nearby strong_retain instruction. Returns a
   /// pointer union initialized with a strong_release inst if it inserts one,
   /// otherwise returns the pointer union initialized with the strong_retain. It
   /// is expected that the caller will remove the returned strong_retain. This
   /// allows for the caller to update any state before the release value is
   /// destroyed.
   PointerUnion<StrongRetainInst *, StrongReleaseInst *>
   emitStrongRelease(PILLocation Loc, PILValue Operand);

   /// Emit a destroy_addr instruction at \p Loc attempting to fold the
   /// destroy_addr locally into a copy_addr instruction. Returns a pointer union
   /// initialized with the folded copy_addr if the destroy_addr was folded into
   /// a copy_addr. Otherwise, returns the newly inserted destroy_addr.
   PointerUnion<CopyAddrInst *, DestroyAddrInst *>
   emitDestroyAddr(PILLocation Loc, PILValue Operand);

   /// Emit a destroy_value instruction at the current location, attempting to
   /// fold it locally into another nearby copy_value instruction. Returns a
   /// pointer union initialized with a destroy_value inst if it inserts one,
   /// otherwise returns the copy_value. It is expected that the caller will
   /// remove the copy_value. This allows for the caller to update any state
   /// before the copy_value is destroyed.
   PointerUnion<CopyValueInst *, DestroyValueInst *>
   emitDestroyValue(PILLocation Loc, PILValue Operand);

   /// Convenience function for calling emitCopy on the type lowering
   /// for the non-address value.
   PILValue emitCopyValueOperation(PILLocation Loc, PILValue v) {
      assert(!v->getType().isAddress());
      auto &lowering = getTypeLowering(v->getType());
      return lowering.emitCopyValue(*this, Loc, v);
   }

   /// Convenience function for calling TypeLowering.emitDestroy on the type
   /// lowering for the non-address value.
   void emitDestroyValueOperation(PILLocation Loc, PILValue v) {
      assert(!v->getType().isAddress());
      if (F->hasOwnership() && v.getOwnershipKind() == ValueOwnershipKind::None)
         return;
      auto &lowering = getTypeLowering(v->getType());
      lowering.emitDestroyValue(*this, Loc, v);
   }

   /// Convenience function for destroying objects and addresses.
   ///
   /// Objects are destroyed using emitDestroyValueOperation and addresses by
   /// emitting destroy_addr.
   void emitDestroyOperation(PILLocation loc, PILValue v) {
      if (v->getType().isObject())
         return emitDestroyValueOperation(loc, v);
      createDestroyAddr(loc, v);
   }

   PILValue emitTupleExtract(PILLocation Loc, PILValue Operand, unsigned FieldNo,
                             PILType ResultTy) {
      // Fold tuple_extract(tuple(x,y,z),2)
      if (auto *TI = dyn_cast<TupleInst>(Operand))
         return TI->getOperand(FieldNo);

      return createTupleExtract(Loc, Operand, FieldNo, ResultTy);
   }

   PILValue emitTupleExtract(PILLocation Loc, PILValue Operand,
                             unsigned FieldNo) {
      return emitTupleExtract(Loc, Operand, FieldNo,
                              Operand->getType().getTupleElementType(FieldNo));
   }

   PILValue emitStructExtract(PILLocation Loc, PILValue Operand, VarDecl *Field,
                              PILType ResultTy) {
      if (auto *SI = dyn_cast<StructInst>(Operand))
         return SI->getFieldValue(Field);

      return createStructExtract(Loc, Operand, Field, ResultTy);
   }

   PILValue emitStructExtract(PILLocation Loc, PILValue Operand,
                              VarDecl *Field) {
      auto type = Operand->getType().getFieldType(Field, getModule(),
                                                  getTypeExpansionContext());
      return emitStructExtract(Loc, Operand, Field, type);
   }

   PILValue emitThickToObjCMetatype(PILLocation Loc, PILValue Op, PILType Ty);
   PILValue emitObjCToThickMetatype(PILLocation Loc, PILValue Op, PILType Ty);

   //===--------------------------------------------------------------------===//
   // Private Helper Methods
   //===--------------------------------------------------------------------===//

private:
   /// insert - This is a template to avoid losing type info on the result.
   template <typename T> T *insert(T *TheInst) {
      insertImpl(TheInst);
      return TheInst;
   }

   /// insertTerminator - This is the same as insert, but clears the insertion
   /// point after doing the insertion.  This is used by terminators, since it
   /// isn't valid to insert something after a terminator.
   template <typename T> T *insertTerminator(T *TheInst) {
      insertImpl(TheInst);
      clearInsertionPoint();
      return TheInst;
   }

   void insertImpl(PILInstruction *TheInst) {
      if (BB == 0)
         return;

      BB->insert(InsertPt, TheInst);

      C.notifyInserted(TheInst);

// TODO: We really shouldn't be creating instructions unless we are going to
// insert them into a block... This failed in SimplifyCFG.
#ifndef NDEBUG
      TheInst->verifyOperandOwnership();
#endif
   }

   bool isLoadableOrOpaque(PILType Ty) {
      auto &M = C.Module;

      if (!PILModuleConventions(M).useLoweredAddresses())
         return true;

      return getTypeLowering(Ty).isLoadable();
   }

   void appendOperandTypeName(PILType OpdTy, llvm::SmallString<16> &Name) {
      if (auto BuiltinIntTy =
         dyn_cast<BuiltinIntegerType>(OpdTy.getAstType())) {
         if (BuiltinIntTy == BuiltinIntegerType::getWordType(getAstContext())) {
            Name += "_Word";
         } else {
            unsigned NumBits = BuiltinIntTy->getWidth().getFixedWidth();
            Name += "_Int" + llvm::utostr(NumBits);
         }
      } else if (auto BuiltinFloatTy =
         dyn_cast<BuiltinFloatType>(OpdTy.getAstType())) {
         Name += "_FP";
         switch (BuiltinFloatTy->getFPKind()) {
            case BuiltinFloatType::IEEE16: Name += "IEEE16"; break;
            case BuiltinFloatType::IEEE32: Name += "IEEE32"; break;
            case BuiltinFloatType::IEEE64: Name += "IEEE64"; break;
            case BuiltinFloatType::IEEE80: Name += "IEEE80"; break;
            case BuiltinFloatType::IEEE128: Name += "IEEE128"; break;
            case BuiltinFloatType::PPC128: Name += "PPC128"; break;
         }
      } else {
         assert(OpdTy.getAstType() == getAstContext().TheRawPointerType);
         Name += "_RawPointer";
      }
   }
};

/// An wrapper on top of PILBuilder's constructor that automatically sets the
/// current PILDebugScope based on the specified insertion point. This is useful
/// for situations where a single PIL instruction is lowered into a sequence of
/// PIL instructions.
class PILBuilderWithScope : public PILBuilder {
   void inheritScopeFrom(PILInstruction *I) {
      assert(I->getDebugScope() && "instruction has no debug scope");
      setCurrentDebugScope(I->getDebugScope());
   }

public:
   /// Build instructions before the given insertion point, inheriting the debug
   /// location.
   ///
   /// Clients should prefer this constructor.
   PILBuilderWithScope(PILInstruction *I, PILBuilderContext &C)
      : PILBuilder(I, I->getDebugScope(), C)
   {}

   /// Build instructions before the given insertion point, inheriting the debug
   /// location and using the context from the passed in builder.
   ///
   /// Clients should prefer this constructor.
   PILBuilderWithScope(PILInstruction *I, PILBuilder &B)
      : PILBuilder(I, I->getDebugScope(), B.getBuilderContext()) {}

   explicit PILBuilderWithScope(
      PILInstruction *I,
      SmallVectorImpl<PILInstruction *> *InsertedInstrs = nullptr)
      : PILBuilder(I, InsertedInstrs) {
      assert(I->getDebugScope() && "instruction has no debug scope");
      setCurrentDebugScope(I->getDebugScope());
   }

   explicit PILBuilderWithScope(PILBasicBlock::iterator I)
      : PILBuilderWithScope(&*I) {}

   explicit PILBuilderWithScope(PILBasicBlock::iterator I, PILBuilder &B)
      : PILBuilder(&*I, &*I->getDebugScope(), B.getBuilderContext()) {}

   explicit PILBuilderWithScope(PILInstruction *I,
                                PILInstruction *InheritScopeFrom)
      : PILBuilderWithScope(I) {
      inheritScopeFrom(InheritScopeFrom);
   }

   explicit PILBuilderWithScope(PILBasicBlock::iterator I,
                                PILInstruction *InheritScopeFrom)
      : PILBuilderWithScope(&*I) {
      inheritScopeFrom(InheritScopeFrom);
   }

   explicit PILBuilderWithScope(PILBasicBlock *BB,
                                PILInstruction *InheritScopeFrom)
      : PILBuilder(BB) {
      inheritScopeFrom(InheritScopeFrom);
   }

   explicit PILBuilderWithScope(PILBasicBlock *BB, PILBuilder &B,
                                PILInstruction *InheritScopeFrom)
      : PILBuilder(BB, InheritScopeFrom->getDebugScope(),
                   B.getBuilderContext()) {}

   /// Creates a new PILBuilder with an insertion point at the
   /// beginning of BB and the debug scope from the first
   /// non-metainstruction in the BB.
   explicit PILBuilderWithScope(PILBasicBlock *BB) : PILBuilder(BB->begin()) {
      const PILDebugScope *DS = BB->getScopeOfFirstNonMetaInstruction();
      assert(DS && "Instruction without debug scope associated!");
      setCurrentDebugScope(DS);
   }
};

class SavedInsertionPointRAII {
   PILBuilder &builder;
   PointerUnion<PILInstruction *, PILBasicBlock *> savedInsertionPoint;

public:
   /// Constructor that saves a Builder's insertion point without changing the
   /// builder's underlying insertion point.
   SavedInsertionPointRAII(PILBuilder &B) : builder(B), savedInsertionPoint() {
      // If our builder does not have a valid insertion point, just put nullptr
      // into SavedIP.
      if (!builder.hasValidInsertionPoint()) {
         savedInsertionPoint = static_cast<PILBasicBlock *>(nullptr);
         return;
      }

      // If we are inserting into the end of the block, stash the insertion block.
      if (builder.insertingAtEndOfBlock()) {
         savedInsertionPoint = builder.getInsertionBB();
         return;
      }

      // Otherwise, stash the instruction.
      PILInstruction *i = &*builder.getInsertionPoint();
      savedInsertionPoint = i;
   }

   SavedInsertionPointRAII(PILBuilder &b, PILInstruction *insertionPoint)
      : SavedInsertionPointRAII(b) {
      builder.setInsertionPoint(insertionPoint);
   }

   SavedInsertionPointRAII(PILBuilder &b, PILBasicBlock *block,
                           PILBasicBlock::iterator iter)
      : SavedInsertionPointRAII(b) {
      builder.setInsertionPoint(block, iter);
   }

   SavedInsertionPointRAII(PILBuilder &b, PILBasicBlock *insertionBlock)
      : SavedInsertionPointRAII(b) {
      builder.setInsertionPoint(insertionBlock);
   }

   SavedInsertionPointRAII(const SavedInsertionPointRAII &) = delete;
   SavedInsertionPointRAII &operator=(const SavedInsertionPointRAII &) = delete;
   SavedInsertionPointRAII(SavedInsertionPointRAII &&) = delete;
   SavedInsertionPointRAII &operator=(SavedInsertionPointRAII &&) = delete;

   ~SavedInsertionPointRAII() {
      if (savedInsertionPoint.isNull()) {
         builder.clearInsertionPoint();
      } else if (savedInsertionPoint.is<PILInstruction *>()) {
         builder.setInsertionPoint(savedInsertionPoint.get<PILInstruction *>());
      } else {
         builder.setInsertionPoint(savedInsertionPoint.get<PILBasicBlock *>());
      }
   }
};

/// Apply a debug location override for the duration of the current scope.
class DebugLocOverrideRAII {
   PILBuilder &Builder;
   Optional<PILLocation> oldOverride;
#ifndef NDEBUG
   Optional<PILLocation> installedOverride;
#endif

public:
   DebugLocOverrideRAII(PILBuilder &B, Optional<PILLocation> Loc) : Builder(B) {
      oldOverride = B.getCurrentDebugLocOverride();
      Builder.applyDebugLocOverride(Loc);
#ifndef NDEBUG
      installedOverride = Loc;
#endif
   }

   ~DebugLocOverrideRAII() {
      assert(Builder.getCurrentDebugLocOverride() == installedOverride &&
             "Restoring debug location override to an unexpected state");
      Builder.applyDebugLocOverride(oldOverride);
   }

   DebugLocOverrideRAII(const DebugLocOverrideRAII &) = delete;
   DebugLocOverrideRAII &operator=(const DebugLocOverrideRAII &) = delete;
   DebugLocOverrideRAII(DebugLocOverrideRAII &&) = delete;
   DebugLocOverrideRAII &operator=(DebugLocOverrideRAII &&) = delete;
};

} // end polar namespace

#endif
