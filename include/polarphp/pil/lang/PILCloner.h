//===--- PILCloner.h - Defines the PILCloner class --------------*- C++ -*-===//
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
// This file defines the PILCloner class, used for cloning PIL instructions.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILCLONER_H
#define POLARPHP_PIL_PILCLONER_H

#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/pil/lang/PILOpenedArchetypesTracker.h"
#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILDebugScope.h"
#include "polarphp/pil/lang/PILVisitor.h"

namespace polar {

/// PILCloner - Abstract PIL visitor which knows how to clone instructions and
/// whose behavior can be customized by subclasses via the CRTP. This is meant
/// to be subclassed to implement inlining, function specialization, and other
/// operations requiring cloning (while possibly modifying, at the same time)
/// instruction sequences.
///
/// By default, this visitor will not do anything useful when called on a
/// basic block, or function; subclasses that want to handle those should
/// implement the appropriate visit functions and/or provide other entry points.
template<typename ImplClass>
class PILCloner : protected PILInstructionVisitor<ImplClass> {
   friend class PILVisitorBase<ImplClass>;
   friend class PILInstructionVisitor<ImplClass>;


protected:
   /// MARK: Context shared with CRTP extensions.

   PILBuilder Builder;
   TypeSubstitutionMap OpenedExistentialSubs;
   PILOpenedArchetypesTracker OpenedArchetypesTracker;

private:
   /// MARK: Private state hidden from CRTP extensions.

   // The old-to-new value map.
   llvm::DenseMap<PILValue, PILValue> ValueMap;

   /// The old-to-new block map. Some entries may be premapped with original
   /// blocks.
   llvm::DenseMap<PILBasicBlock*, PILBasicBlock*> BBMap;

   // The original blocks in DFS preorder. All blocks in this list are mapped.
   // After cloning, this represents the entire cloned CFG.
   //
   // This could always be rediscovered by the client, but caching it is a
   // convenient way to iterate over the cloned region.
   SmallVector<PILBasicBlock *, 8> preorderBlocks;

   /// Set of basic blocks where unreachable was inserted.
   SmallPtrSet<PILBasicBlock *, 32> BlocksWithUnreachables;

   // Keep track of the last cloned block in function order. For single block
   // regions, this will be the start block.
   PILBasicBlock *lastClonedBB = nullptr;

public:
   using PILInstructionVisitor<ImplClass>::asImpl;

   explicit PILCloner(PILFunction &F,
                      PILOpenedArchetypesTracker &OpenedArchetypesTracker)
      : Builder(F), OpenedArchetypesTracker(OpenedArchetypesTracker) {
      Builder.setOpenedArchetypesTracker(&OpenedArchetypesTracker);
   }

   explicit PILCloner(PILFunction &F) : Builder(F), OpenedArchetypesTracker(&F) {
      Builder.setOpenedArchetypesTracker(&OpenedArchetypesTracker);
   }

   explicit PILCloner(PILGlobalVariable *GlobVar)
      : Builder(GlobVar), OpenedArchetypesTracker(nullptr) {}

   void clearClonerState() {
      ValueMap.clear();
      BBMap.clear();
      preorderBlocks.clear();
      BlocksWithUnreachables.clear();
   }

   /// Clients of PILCloner who want to know about any newly created
   /// instructions can install a SmallVector into the builder to collect them.
   void setTrackingList(SmallVectorImpl<PILInstruction*> *II) {
      getBuilder().setTrackingList(II);
   }

   SmallVectorImpl<PILInstruction*> *getTrackingList() {
      return getBuilder().getTrackingList();
   }

   PILBuilder &getBuilder() { return Builder; }

   // After cloning, returns a non-null pointer to the last cloned block in
   // function order. For single block regions, this will be the start block.
   PILBasicBlock *getLastClonedBB() { return lastClonedBB; }

   /// Visit all blocks reachable from the given `StartBB` and all instructions
   /// in those blocks.
   ///
   /// This is used to clone a region within a function and mutates the original
   /// function. `StartBB` cannot be the function entry block.
   ///
   /// The entire CFG is discovered in DFS preorder while cloning non-terminator
   /// instructions. `visitTerminator` is called in the same order, but only
   /// after mapping all blocks.
   void cloneReachableBlocks(PILBasicBlock *startBB,
                             ArrayRef<PILBasicBlock *> exitBlocks,
                             PILBasicBlock *insertAfterBB = nullptr,
                             bool havePrepopulatedFunctionArgs = false);

   /// Clone all blocks in this function and all instructions in those
   /// blocks.
   ///
   /// This is used to clone an entire function and should not mutate the
   /// original function except if \p replaceOriginalFunctionInPlace is true.
   ///
   /// entryArgs must have a PILValue from the cloned function corresponding to
   /// each argument in the original function `F`.
   ///
   /// Cloned instructions are inserted starting at the end of clonedEntryBB.
   void cloneFunctionBody(PILFunction *F, PILBasicBlock *clonedEntryBB,
                          ArrayRef<PILValue> entryArgs,
                          bool replaceOriginalFunctionInPlace = false);

   /// MARK: Callback utilities used from CRTP extensions during cloning.
   /// These should only be called from within an instruction cloning visitor.

   /// Visitor callback that registers a cloned instruction. All the original
   /// instruction's results are mapped onto the cloned instruction's results for
   /// use within the cloned region.
   ///
   /// CRTP extensions can
   /// override the implementation via `postProcess`.
   void recordClonedInstruction(PILInstruction *Orig, PILInstruction *Cloned) {
      asImpl().postProcess(Orig, Cloned);
      assert((Orig->getDebugScope() ? Cloned->getDebugScope() != nullptr : true)
             && "cloned instruction dropped debug scope");
   }

   /// Visitor callback that maps an original value to an existing value when the
   /// original instruction will not be cloned. This is used when the instruction
   /// visitor can fold away the cloned instruction, and it skips the usual
   /// `postProcess()` callback. recordClonedInstruction() and
   /// recordFoldedValue() are the only two ways for a visitor to map an original
   /// value to another value for use within the cloned region.
   void recordFoldedValue(PILValue origValue, PILValue mappedValue) {
      asImpl().mapValue(origValue, mappedValue);
   }

   /// Mark a block containing an unreachable instruction for use in the `fixUp`
   /// callback.
   void addBlockWithUnreachable(PILBasicBlock *BB) {
      BlocksWithUnreachables.insert(BB);
   }

   /// Register a re-mapping for opened existentials.
   void registerOpenedExistentialRemapping(ArchetypeType *From,
                                           ArchetypeType *To) {
      auto result = OpenedExistentialSubs.insert(
         std::make_pair(CanArchetypeType(From), CanType(To)));
      assert(result.second);
      (void)result;
   }

   /// MARK: Public access to the cloned state, during and after cloning.

   /// After cloning, provides a list of all cloned blocks in DFS preorder.
   ArrayRef<PILBasicBlock *> originalPreorderBlocks() const {
      return preorderBlocks;
   }

   PILLocation getOpLocation(PILLocation Loc) {
      return asImpl().remapLocation(Loc);
   }

   const PILDebugScope *getOpScope(const PILDebugScope *DS) {
      return asImpl().remapScope(DS);
   }

   SubstitutionMap getOpSubstitutionMap(SubstitutionMap Subs) {
      // If we have open existentials to substitute, check whether that's
      // relevant to this this particular substitution.
      if (!OpenedExistentialSubs.empty()) {
         for (auto ty : Subs.getReplacementTypes()) {
            // If we found a type containing an opened existential, substitute
            // open existentials throughout the substitution map.
            if (ty->hasOpenedExistential()) {
               Subs = Subs.subst(QueryTypeSubstitutionMapOrIdentity{
                                    OpenedExistentialSubs},
                                 MakeAbstractConformanceForGenericType());
               break;
            }
         }
      }

      return asImpl().remapSubstitutionMap(Subs).getCanonical();
   }

   PILType getTypeInClonedContext(PILType Ty) {
      auto objectTy = Ty.getAstType();
      // Do not substitute opened existential types, if we do not have any.
      if (!objectTy->hasOpenedExistential())
         return Ty;
      // Do not substitute opened existential types, if it is not required.
      // This is often the case when cloning basic blocks inside the same
      // function.
      if (OpenedExistentialSubs.empty())
         return Ty;

      // Substitute opened existential types, if we have any.
      return Ty.subst(
         Builder.getModule(),
         QueryTypeSubstitutionMapOrIdentity{OpenedExistentialSubs},
         MakeAbstractConformanceForGenericType());
   }
   PILType getOpType(PILType Ty) {
      Ty = getTypeInClonedContext(Ty);
      return asImpl().remapType(Ty);
   }

   CanType getAstTypeInClonedContext(Type ty) {
      // Do not substitute opened existential types, if we do not have any.
      if (!ty->hasOpenedExistential())
         return ty->getCanonicalType();
      // Do not substitute opened existential types, if it is not required.
      // This is often the case when cloning basic blocks inside the same
      // function.
      if (OpenedExistentialSubs.empty())
         return ty->getCanonicalType();

      return ty.subst(
         QueryTypeSubstitutionMapOrIdentity{OpenedExistentialSubs},
         MakeAbstractConformanceForGenericType()
      )->getCanonicalType();
   }

   CanType getOpASTType(CanType ty) {
      ty = getAstTypeInClonedContext(ty);
      return asImpl().remapASTType(ty);
   }

   void remapOpenedType(CanOpenedArchetypeType archetypeTy) {
      auto existentialTy = archetypeTy->getOpenedExistentialType()->getCanonicalType();
      auto replacementTy = OpenedArchetypeType::get(getOpASTType(existentialTy));
      registerOpenedExistentialRemapping(archetypeTy, replacementTy);
   }

   InterfaceConformanceRef getOpConformance(Type ty,
                                           InterfaceConformanceRef conformance) {
      // If we have open existentials to substitute, do so now.
      if (ty->hasOpenedExistential() && !OpenedExistentialSubs.empty()) {
         conformance =
            conformance.subst(ty,
                              QueryTypeSubstitutionMapOrIdentity{
                                 OpenedExistentialSubs},
                              MakeAbstractConformanceForGenericType());
      }

      return asImpl().remapConformance(getAstTypeInClonedContext(ty),
                                       conformance);
   }

   ArrayRef<InterfaceConformanceRef>
   getOpConformances(Type ty,
                     ArrayRef<InterfaceConformanceRef> conformances) {
      SmallVector<InterfaceConformanceRef, 4> newConformances;
      for (auto conformance : conformances)
         newConformances.push_back(getOpConformance(ty, conformance));
      return ty->getAstContext().AllocateCopy(newConformances);
   }

   bool isValueCloned(PILValue OrigValue) const {
      return ValueMap.count(OrigValue);
   }

   /// Return the possibly new value representing the given value within the
   /// cloned region.
   ///
   /// Assumes that `isValueCloned` is true.
   PILValue getOpValue(PILValue Value) {
      return asImpl().getMappedValue(Value);
   }
   template <size_t N, typename ArrayRefType>
   SmallVector<PILValue, N> getOpValueArray(ArrayRefType Values) {
      SmallVector<PILValue, N> Ret(Values.size());
      for (unsigned i = 0, e = Values.size(); i != e; ++i)
         Ret[i] = asImpl().getMappedValue(Values[i]);
      return Ret;
   }

   PILFunction *getOpFunction(PILFunction *Func) {
      return asImpl().remapFunction(Func);
   }

   bool isBlockCloned(PILBasicBlock *OrigBB) const {
      auto bbIter = BBMap.find(OrigBB);
      if (bbIter == BBMap.end())
         return false;

      // Exit blocks are mapped to themselves during region cloning.
      return bbIter->second != OrigBB;
   }

   /// Return the new block within the cloned region analagous to the given
   /// original block.
   ///
   /// Assumes that `isBlockCloned` is true.
   PILBasicBlock *getOpBasicBlock(PILBasicBlock *BB) {
      return asImpl().remapBasicBlock(BB);
   }

protected:
   /// MARK: CRTP visitors and other CRTP overrides.

#define INST(CLASS, PARENT) void visit##CLASS(CLASS *I);
#include "polarphp/pil/lang/PILNodesDef.h"

   // Visit the instructions in a single basic block, not including the block
   // terminator.
   void visitInstructionsInBlock(PILBasicBlock *BB);

   // Visit a block's terminator. This is called with each block in DFS preorder
   // after visiting and mapping all basic blocks and after visiting all
   // non-terminator instructions in the block.
   void visitTerminator(PILBasicBlock *BB) {
      asImpl().visit(BB->getTerminator());
   }

   // CFG cloning requires cloneFunction() or cloneReachableBlocks().
   void visitPILBasicBlock(PILFunction *F) = delete;

   // Function cloning requires cloneFunction().
   void visitPILFunction(PILFunction *F) = delete;

   // MARK: PILCloner subclasses use the CRTP to customize the following callback
   // implementations. Remap functions are called before cloning to modify
   // constructor arguments. The postProcess function is called afterwards on
   // the result.

   PILLocation remapLocation(PILLocation Loc) { return Loc; }
   const PILDebugScope *remapScope(const PILDebugScope *DS) { return DS; }
   PILType remapType(PILType Ty) {
      return Ty;
   }

   CanType remapASTType(CanType Ty) {
      return Ty;
   }

   InterfaceConformanceRef remapConformance(Type Ty, InterfaceConformanceRef C) {
      return C;
   }
   /// Get the value that takes the place of the given `Value` within the cloned
   /// region. The given value must already have been mapped by this cloner.
   PILValue getMappedValue(PILValue Value);
   void mapValue(PILValue origValue, PILValue mappedValue);

   PILFunction *remapFunction(PILFunction *Func) { return Func; }
   PILBasicBlock *remapBasicBlock(PILBasicBlock *BB);
   void postProcess(PILInstruction *Orig, PILInstruction *Cloned);

   SubstitutionMap remapSubstitutionMap(SubstitutionMap Subs) { return Subs; }

   /// This is called by either of the top-level visitors, cloneReachableBlocks
   /// or clonePILFunction, after all other visitors are have been called.
   ///
   /// After fixUp, the PIL must be valid and semantically equivalent to the PIL
   /// before cloning.
   ///
   /// Common fix-ups are handled first in `doFixUp` and may not be overridden.
   void fixUp(PILFunction *F) {}
private:
   /// MARK: PILCloner implementation details hidden from CRTP extensions.

   /// PILVisitor CRTP callback. Preprocess any instruction before cloning.
   void beforeVisit(PILInstruction *Orig) {
      // Update the set of available opened archetypes with the opened
      // archetypes used by the current instruction.
      auto TypeDependentOperands = Orig->getTypeDependentOperands();
      Builder.getOpenedArchetypes().addOpenedArchetypeOperands(
         TypeDependentOperands);
   }

   void clonePhiArgs(PILBasicBlock *oldBB);

   void visitBlocksDepthFirst(PILBasicBlock *StartBB);

   /// Also perform fundamental cleanup first, then call the CRTP extension,
   /// `fixUp`.
   void doFixUp(PILFunction *F);
};

/// A PILBuilder that automatically invokes postprocess on each
/// inserted instruction.
template<class SomePILCloner, unsigned N = 4>
class PILBuilderWithPostProcess : public PILBuilder {
   SomePILCloner &SC;
   PILInstruction *Orig;
   SmallVector<PILInstruction*, N> InsertedInstrs;

public:
   PILBuilderWithPostProcess(SomePILCloner *sc, PILInstruction *Orig)
      : PILBuilder(sc->getBuilder().getInsertionBB(), &InsertedInstrs),
        SC(*sc), Orig(Orig)
   {
      setInsertionPoint(SC.getBuilder().getInsertionBB(),
                        SC.getBuilder().getInsertionPoint());
      setOpenedArchetypesTracker(SC.getBuilder().getOpenedArchetypesTracker());
   }

   ~PILBuilderWithPostProcess() {
      for (auto *I : InsertedInstrs) {
         SC.recordClonedInstruction(Orig, I);
      }
   }
};


/// PILClonerWithScopes - a PILCloner that automatically clones
/// PILDebugScopes. In contrast to inline scopes, this generates a
/// deep copy of the scope tree.
template<typename ImplClass>
class PILClonerWithScopes : public PILCloner<ImplClass> {
   friend class PILCloner<ImplClass>;
public:
   PILClonerWithScopes(PILFunction &To,
                       PILOpenedArchetypesTracker &OpenedArchetypesTracker,
                       bool Disable = false)
      : PILCloner<ImplClass>(To, OpenedArchetypesTracker) {

      // We only want to do this when we generate cloned functions, not
      // when we inline.

      // FIXME: This is due to having TypeSubstCloner inherit from
      //        PILClonerWithScopes, and having TypeSubstCloner be used
      //        both by passes that clone whole functions and ones that
      //        inline functions.
      if (Disable)
         return;

      scopeCloner.reset(new ScopeCloner(To));
   }

   PILClonerWithScopes(PILFunction &To,
                       bool Disable = false)
      : PILCloner<ImplClass>(To) {

      // We only want to do this when we generate cloned functions, not
      // when we inline.

      // FIXME: This is due to having TypeSubstCloner inherit from
      //        PILClonerWithScopes, and having TypeSubstCloner be used
      //        both by passes that clone whole functions and ones that
      //        inline functions.
      if (Disable)
         return;

      scopeCloner.reset(new ScopeCloner(To));
   }


private:
   std::unique_ptr<ScopeCloner> scopeCloner;
protected:
   /// Clone the PILDebugScope for the cloned function.
   void postProcess(PILInstruction *Orig, PILInstruction *Cloned) {
      PILCloner<ImplClass>::postProcess(Orig, Cloned);
   }

   const PILDebugScope *remapScope(const PILDebugScope *DS) {
      return scopeCloner ? scopeCloner->getOrCreateClonedScope(DS) : DS;
   }
};

/// Clone a function without transforming it.
class PILFunctionCloner : public PILClonerWithScopes<PILFunctionCloner> {
   using SuperTy = PILClonerWithScopes<PILFunctionCloner>;
   friend class PILCloner<PILFunctionCloner>;

public:
   PILFunctionCloner(PILFunction *newF) : PILClonerWithScopes(*newF) {}

   /// Clone all blocks in this function and all instructions in those
   /// blocks.
   ///
   /// This is used to clone an entire function without mutating the original
   /// function.
   ///
   /// The new function is expected to be completely empty. Clone the entry
   /// blocks arguments here. The cloned arguments become the inputs to the
   /// general PILCloner, which expects the new entry block to be ready to emit
   /// instructions into.
   void cloneFunction(PILFunction *origF) {
      PILFunction *newF = &Builder.getFunction();

      auto *newEntryBB = newF->createBasicBlock();
      newEntryBB->cloneArgumentList(origF->getEntryBlock());

      // Copy the new entry block arguments into a separate vector purely to
      // resolve the type mismatch between PILArgument* and PILValue.
      SmallVector<PILValue, 8> entryArgs;
      entryArgs.reserve(newF->getArguments().size());
      llvm::transform(newF->getArguments(), std::back_inserter(entryArgs),
                      [](PILArgument *arg) -> PILValue { return arg; });

      SuperTy::cloneFunctionBody(origF, newEntryBB, entryArgs);
   }
};

template<typename ImplClass>
PILValue
PILCloner<ImplClass>::getMappedValue(PILValue Value) {
   auto VI = ValueMap.find(Value);
   if (VI != ValueMap.end())
      return VI->second;

   // If we have undef, just remap the type.
   if (auto *U = dyn_cast<PILUndef>(Value)) {
      auto type = getOpType(U->getType());
      ValueBase *undef =
         (type == U->getType() ? U : PILUndef::get(type, Builder.getFunction()));
      return PILValue(undef);
   }

   llvm_unreachable("Unmapped value while cloning?");
}

template <typename ImplClass>
void PILCloner<ImplClass>::mapValue(PILValue origValue, PILValue mappedValue) {
   auto iterAndInserted = ValueMap.insert({origValue, mappedValue});
   (void)iterAndInserted;
   assert(iterAndInserted.second && "Original value already mapped.");
}

template<typename ImplClass>
PILBasicBlock*
PILCloner<ImplClass>::remapBasicBlock(PILBasicBlock *BB) {
   PILBasicBlock *MappedBB = BBMap[BB];
   assert(MappedBB && "Unmapped basic block while cloning?");
   return MappedBB;
}

template<typename ImplClass>
void
PILCloner<ImplClass>::postProcess(PILInstruction *orig,
                                  PILInstruction *cloned) {
   assert((orig->getDebugScope() ? cloned->getDebugScope()!=nullptr : true) &&
          "cloned function dropped debug scope");

   // It sometimes happens that an instruction with no results gets mapped
   // to an instruction with results, e.g. when specializing a cast.
   // Just ignore this.
   auto origResults = orig->getResults();
   if (origResults.empty()) return;

   // Otherwise, map the results over one-by-one.
   auto clonedResults = cloned->getResults();
   assert(origResults.size() == clonedResults.size());
   for (auto i : indices(origResults))
      asImpl().mapValue(origResults[i], clonedResults[i]);
}

template<typename ImplClass>
void PILCloner<ImplClass>::visitInstructionsInBlock(PILBasicBlock* BB) {
   // Iterate over and visit all instructions other than the terminator to clone.
   for (auto I = BB->begin(), E = --BB->end(); I != E; ++I) {
      asImpl().visit(&*I);
   }
}

template <typename ImplClass>
void PILCloner<ImplClass>::cloneReachableBlocks(
   PILBasicBlock *startBB, ArrayRef<PILBasicBlock *> exitBlocks,
   PILBasicBlock *insertAfterBB,
   bool havePrepopulatedFunctionArgs) {

   PILFunction *F = startBB->getParent();
   assert(F == &Builder.getFunction()
          && "cannot clone region across functions.");
   assert(BBMap.empty() && "This API does not allow clients to map blocks.");
   assert((havePrepopulatedFunctionArgs || ValueMap.empty()) &&
          "Stale ValueMap.");

   auto *clonedStartBB = insertAfterBB ? F->createBasicBlockAfter(insertAfterBB)
                                       : F->createBasicBlock();

   BBMap.insert(std::make_pair(startBB, clonedStartBB));
   getBuilder().setInsertionPoint(clonedStartBB);
   clonePhiArgs(startBB);

   // Premap exit blocks to terminate so that visitBlocksDepthFirst terminates
   // after discovering the cloned region. Mapping an exit block to itself
   // provides the correct destination block during visitTerminator.
   for (auto *exitBB : exitBlocks)
      BBMap[exitBB] = exitBB;

   // Discover and map the region to be cloned.
   visitBlocksDepthFirst(startBB);

   doFixUp(F);
}

template <typename ImplClass>
void PILCloner<ImplClass>::cloneFunctionBody(PILFunction *F,
                                             PILBasicBlock *clonedEntryBB,
                                             ArrayRef<PILValue> entryArgs,
                                             bool replaceOriginalFunctionInPlace) {

   assert((replaceOriginalFunctionInPlace || F != clonedEntryBB->getParent()) &&
          "Must clone into a new function.");
   assert(BBMap.empty() && "This API does not allow clients to map blocks.");
   assert(ValueMap.empty() && "Stale ValueMap.");

   assert(entryArgs.size() == F->getArguments().size());
   for (unsigned i = 0, e = entryArgs.size(); i != e; ++i)
      ValueMap[F->getArgument(i)] = entryArgs[i];

   BBMap.insert(std::make_pair(&*F->begin(), clonedEntryBB));

   Builder.setInsertionPoint(clonedEntryBB);

   // This will layout all newly cloned blocks immediate after clonedEntryBB.
   visitBlocksDepthFirst(&*F->begin());

   doFixUp(F);
}

template<typename ImplClass>
void PILCloner<ImplClass>::clonePhiArgs(PILBasicBlock *oldBB) {
   auto *mappedBB = BBMap[oldBB];

   // Create new arguments for each of the original block's arguments.
   for (auto *Arg : oldBB->getPILPhiArguments()) {
      PILValue mappedArg = mappedBB->createPhiArgument(
         getOpType(Arg->getType()), Arg->getOwnershipKind());

      asImpl().mapValue(Arg, mappedArg);
   }
}

// This private helper visits BBs in depth-first preorder (only processing
// blocks on the first visit), mapping newly visited BBs to new BBs and cloning
// all instructions into the caller.
template <typename ImplClass>
void PILCloner<ImplClass>::visitBlocksDepthFirst(PILBasicBlock *startBB) {
   // The caller clones startBB because it may be a function header, which
   // requires special handling.
   assert(BBMap.count(startBB) && "The caller must map the first BB.");

   assert(preorderBlocks.empty());

   // First clone the CFG region.
   //
   // FIXME: Add reverse iteration to PILSuccessor, then convert this to an RPOT
   // traversal. We would prefer to keep CFG regions in RPO order, and this would
   // not create as large a worklist for functions with many large switches.
   SmallVector<PILBasicBlock *, 8> dfsWorklist(1, startBB);
   // Keep a reference to the last cloned BB so blocks can be laid out in the
   // order they are created, which differs from the order they are
   // cloned. Blocks are created in BFS order but cloned in DFS preorder (when no
   // critical edges are present).
   lastClonedBB = BBMap[startBB];
   while (!dfsWorklist.empty()) {
      auto *BB = dfsWorklist.pop_back_val();
      preorderBlocks.push_back(BB);

      // Phis are cloned during the first preorder walk so that successor phis
      // exist before predecessor terminators are generated.
      if (BB != startBB)
         clonePhiArgs(BB);

      // Non-terminating instructions are cloned in the first preorder walk so
      // that all opened existentials are registered with OpenedArchetypesTracker
      // before phi argument type substitution in successors.
      getBuilder().setInsertionPoint(BBMap[BB]);
      asImpl().visitInstructionsInBlock(BB);

      unsigned dfsSuccStartIdx = dfsWorklist.size();
      for (auto &succ : BB->getSuccessors()) {
         // Only visit a successor that has not already been visited and was not
         // premapped by the client.
         if (BBMap.count(succ))
            continue;

         // Map the successor to a new BB. Layout the cloned blocks in the order
         // they are visited and cloned.
         lastClonedBB =
            getBuilder().getFunction().createBasicBlockAfter(lastClonedBB);

         BBMap.insert(std::make_pair(succ.getBB(), lastClonedBB));

         dfsWorklist.push_back(succ);
      }
      // Reverse the worklist to pop the successors in forward order. This
      // precisely yields DFS preorder when no critical edges are present.
      std::reverse(dfsWorklist.begin() + dfsSuccStartIdx, dfsWorklist.end());
   }
   // Visit terminators only after the CFG is valid so all branch targets exist.
   //
   // Visiting in pre-order provides a nice property for the individual
   // instruction visitors. It allows those visitors to make use of dominance
   // relationships, particularly the fact that operand values will be mapped.
   for (auto *origBB : preorderBlocks) {
      // Set the insertion point to the new mapped BB
      getBuilder().setInsertionPoint(BBMap[origBB]);
      asImpl().visitTerminator(origBB);
   }
}

/// Clean-up after cloning.
template<typename ImplClass>
void
PILCloner<ImplClass>::doFixUp(PILFunction *F) {
   // If our source function is in ossa form, but the function into which we are
   // cloning is not in ossa, after we clone, eliminate default arguments.
   if (!getBuilder().hasOwnership() && F->hasOwnership()) {
      for (auto &Block : getBuilder().getFunction()) {
         auto *Term = Block.getTerminator();
         if (auto *CCBI = dyn_cast<CheckedCastBranchInst>(Term)) {
            // Check if we have a default argument.
            auto *FailureBlock = CCBI->getFailureBB();
            assert(FailureBlock->getNumArguments() <= 1 &&
                   "We should either have no args or a single default arg");
            if (0 == FailureBlock->getNumArguments())
               continue;
            FailureBlock->getArgument(0)->replaceAllUsesWith(CCBI->getOperand());
            FailureBlock->eraseArgument(0);
            continue;
         }

         if (auto *SEI = dyn_cast<SwitchEnumInst>(Term)) {
            if (auto DefaultBlock = SEI->getDefaultBBOrNull()) {
               assert(DefaultBlock.get()->getNumArguments() <= 1 &&
                      "We should either have no args or a single default arg");
               if (0 == DefaultBlock.get()->getNumArguments())
                  continue;
               DefaultBlock.get()->getArgument(0)->replaceAllUsesWith(
                  SEI->getOperand());
               DefaultBlock.get()->eraseArgument(0);
               continue;
            }
         }
      }
   }

   // Remove any code after unreachable instructions.

   // NOTE: It is unfortunate that it essentially duplicates the code from
   // sil-combine, but doing so allows for avoiding any cross-layer invocations
   // between PIL and PILOptimizer layers.

   for (auto *BB : BlocksWithUnreachables) {
      for (auto &I : *BB) {
         if (!isa<UnreachableInst>(&I))
            continue;

         // Collect together all the instructions after this point
         llvm::SmallVector<PILInstruction *, 32> ToRemove;
         for (auto Inst = BB->rbegin(); &*Inst != &I; ++Inst)
            ToRemove.push_back(&*Inst);

         for (auto *Inst : ToRemove) {
            // Replace any non-dead results with PILUndef values
            Inst->replaceAllUsesOfAllResultsWithUndef();
            Inst->eraseFromParent();
         }
      }
   }

   BlocksWithUnreachables.clear();

   // Call any cleanup specific to the CRTP extensions.
   asImpl().fixUp(F);
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitAllocStackInst(AllocStackInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   // Drop the debug info from mandatory-inlined instructions. It's the law!
   PILLocation Loc = getOpLocation(Inst->getLoc());
   Optional<PILDebugVariable> VarInfo = Inst->getVarInfo();
   if (Loc.getKind() == PILLocation::MandatoryInlinedKind) {
      Loc = MandatoryInlinedLocation::getAutoGeneratedLocation();
      VarInfo = None;
   }
   recordClonedInstruction(Inst,
                           getBuilder().createAllocStack(
                              Loc, getOpType(Inst->getElementType()), VarInfo));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitAllocRefInst(AllocRefInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   auto CountArgs = getOpValueArray<8>(OperandValueArrayRef(Inst->
      getTailAllocatedCounts()));
   SmallVector<PILType, 4> ElemTypes;
   for (PILType OrigElemType : Inst->getTailAllocatedTypes()) {
      ElemTypes.push_back(getOpType(OrigElemType));
   }
   auto *NewInst = getBuilder().createAllocRef(getOpLocation(Inst->getLoc()),
                                               getOpType(Inst->getType()),
                                               Inst->isObjC(), Inst->canAllocOnStack(),
                                               ElemTypes, CountArgs);
   recordClonedInstruction(Inst, NewInst);
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitAllocRefDynamicInst(AllocRefDynamicInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   auto CountArgs = getOpValueArray<8>(OperandValueArrayRef(Inst->
      getTailAllocatedCounts()));
   SmallVector<PILType, 4> ElemTypes;
   for (PILType OrigElemType : Inst->getTailAllocatedTypes()) {
      ElemTypes.push_back(getOpType(OrigElemType));
   }
   auto *NewInst = getBuilder().createAllocRefDynamic(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getMetatypeOperand()),
      getOpType(Inst->getType()),
      Inst->isObjC(),
      ElemTypes, CountArgs);
   recordClonedInstruction(Inst, NewInst);
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitAllocBoxInst(AllocBoxInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   // Drop the debug info from mandatory-inlined instructions.
   PILLocation Loc = getOpLocation(Inst->getLoc());
   Optional<PILDebugVariable> VarInfo = Inst->getVarInfo();
   if (Loc.getKind() == PILLocation::MandatoryInlinedKind) {
      Loc = MandatoryInlinedLocation::getAutoGeneratedLocation();
      VarInfo = None;
   }

   recordClonedInstruction(
      Inst,
      getBuilder().createAllocBox(
         Loc, this->getOpType(Inst->getType()).template castTo<PILBoxType>(),
         VarInfo));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitAllocExistentialBoxInst(
   AllocExistentialBoxInst *Inst) {
   auto origExistentialType = Inst->getExistentialType();
   auto origFormalType = Inst->getFormalConcreteType();

   auto conformances = getOpConformances(origFormalType,
                                         Inst->getConformances());

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createAllocExistentialBox(
         getOpLocation(Inst->getLoc()), getOpType(origExistentialType),
         getOpASTType(origFormalType), conformances));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitAllocValueBufferInst(AllocValueBufferInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createAllocValueBuffer(
      getOpLocation(Inst->getLoc()),
      getOpType(Inst->getValueType()),
      getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitBuiltinInst(BuiltinInst *Inst) {
   auto Args = getOpValueArray<8>(Inst->getArguments());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createBuiltin(
         getOpLocation(Inst->getLoc()), Inst->getName(),
         getOpType(Inst->getType()),
         getOpSubstitutionMap(Inst->getSubstitutions()), Args));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitApplyInst(ApplyInst *Inst) {
   auto Args = getOpValueArray<8>(Inst->getArguments());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createApply(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getCallee()),
         getOpSubstitutionMap(Inst->getSubstitutionMap()), Args,
         Inst->isNonThrowing(),
         GenericSpecializationInformation::create(Inst, getBuilder())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitTryApplyInst(TryApplyInst *Inst) {
   auto Args = getOpValueArray<8>(Inst->getArguments());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createTryApply(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getCallee()),
         getOpSubstitutionMap(Inst->getSubstitutionMap()), Args,
         getOpBasicBlock(Inst->getNormalBB()),
         getOpBasicBlock(Inst->getErrorBB()),
         GenericSpecializationInformation::create(Inst, getBuilder())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitPartialApplyInst(PartialApplyInst *Inst) {
   auto Args = getOpValueArray<8>(Inst->getArguments());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createPartialApply(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getCallee()),
         getOpSubstitutionMap(Inst->getSubstitutionMap()), Args,
         Inst->getType().getAs<PILFunctionType>()->getCalleeConvention(),
         Inst->isOnStack(),
         GenericSpecializationInformation::create(Inst, getBuilder())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitBeginApplyInst(BeginApplyInst *Inst) {
   auto Args = getOpValueArray<8>(Inst->getArguments());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createBeginApply(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getCallee()),
         getOpSubstitutionMap(Inst->getSubstitutionMap()), Args,
         Inst->isNonThrowing(),
         GenericSpecializationInformation::create(Inst, getBuilder())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitAbortApplyInst(AbortApplyInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createAbortApply(getOpLocation(Inst->getLoc()),
                                          getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitEndApplyInst(EndApplyInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createEndApply(getOpLocation(Inst->getLoc()),
                                        getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitFunctionRefInst(FunctionRefInst *Inst) {
   PILFunction *OpFunction =
      getOpFunction(Inst->getInitiallyReferencedFunction());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createFunctionRef(
      getOpLocation(Inst->getLoc()), OpFunction));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitDynamicFunctionRefInst(
   DynamicFunctionRefInst *Inst) {
   PILFunction *OpFunction =
      getOpFunction(Inst->getInitiallyReferencedFunction());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createDynamicFunctionRef(
      getOpLocation(Inst->getLoc()), OpFunction));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitPreviousDynamicFunctionRefInst(
   PreviousDynamicFunctionRefInst *Inst) {
   PILFunction *OpFunction =
      getOpFunction(Inst->getInitiallyReferencedFunction());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createPreviousDynamicFunctionRef(
      getOpLocation(Inst->getLoc()), OpFunction));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitAllocGlobalInst(AllocGlobalInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createAllocGlobal(getOpLocation(Inst->getLoc()),
                                           Inst->getReferencedGlobal()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitGlobalAddrInst(GlobalAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createGlobalAddr(getOpLocation(Inst->getLoc()),
                                          Inst->getReferencedGlobal()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitGlobalValueInst(GlobalValueInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createGlobalValue(getOpLocation(Inst->getLoc()),
                                           Inst->getReferencedGlobal()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitIntegerLiteralInst(IntegerLiteralInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createIntegerLiteral(getOpLocation(Inst->getLoc()),
                                              getOpType(Inst->getType()),
                                              Inst->getValue()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitFloatLiteralInst(FloatLiteralInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createFloatLiteral(getOpLocation(Inst->getLoc()),
                                            getOpType(Inst->getType()),
                                            Inst->getValue()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitStringLiteralInst(StringLiteralInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createStringLiteral(
      getOpLocation(Inst->getLoc()),
      Inst->getValue(), Inst->getEncoding()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitLoadInst(LoadInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   if (!getBuilder().hasOwnership()) {
      switch (Inst->getOwnershipQualifier()) {
         case LoadOwnershipQualifier::Copy: {
            auto *li = getBuilder().createLoad(getOpLocation(Inst->getLoc()),
                                               getOpValue(Inst->getOperand()),
                                               LoadOwnershipQualifier::Unqualified);
            // This will emit a retain_value/strong_retain as appropriate.
            getBuilder().emitCopyValueOperation(getOpLocation(Inst->getLoc()), li);
            return recordClonedInstruction(Inst, li);
         }
         case LoadOwnershipQualifier::Take:
         case LoadOwnershipQualifier::Trivial:
         case LoadOwnershipQualifier::Unqualified:
            break;
      }
      return recordClonedInstruction(
         Inst, getBuilder().createLoad(getOpLocation(Inst->getLoc()),
                                       getOpValue(Inst->getOperand()),
                                       LoadOwnershipQualifier::Unqualified));
   }

   return recordClonedInstruction(
      Inst, getBuilder().createLoad(getOpLocation(Inst->getLoc()),
                                    getOpValue(Inst->getOperand()),
                                    Inst->getOwnershipQualifier()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitLoadBorrowInst(LoadBorrowInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   // If we are not inlining into an ownership function, just use a load.
   if (!getBuilder().hasOwnership()) {
      return recordClonedInstruction(
         Inst, getBuilder().createLoad(getOpLocation(Inst->getLoc()),
                                       getOpValue(Inst->getOperand()),
                                       LoadOwnershipQualifier::Unqualified));
   }

   recordClonedInstruction(
      Inst, getBuilder().createLoadBorrow(getOpLocation(Inst->getLoc()),
                                          getOpValue(Inst->getOperand())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitBeginBorrowInst(BeginBorrowInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   if (!getBuilder().hasOwnership()) {
      return recordFoldedValue(Inst, getOpValue(Inst->getOperand()));
   }

   recordClonedInstruction(
      Inst, getBuilder().createBeginBorrow(getOpLocation(Inst->getLoc()),
                                           getOpValue(Inst->getOperand())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitStoreInst(StoreInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   if (!getBuilder().hasOwnership()) {
      switch (Inst->getOwnershipQualifier()) {
         case StoreOwnershipQualifier::Assign: {
            auto *li = getBuilder().createLoad(getOpLocation(Inst->getLoc()),
                                               getOpValue(Inst->getDest()),
                                               LoadOwnershipQualifier::Unqualified);
            auto *si = getBuilder().createStore(
               getOpLocation(Inst->getLoc()), getOpValue(Inst->getSrc()),
               getOpValue(Inst->getDest()), StoreOwnershipQualifier::Unqualified);
            getBuilder().emitDestroyValueOperation(getOpLocation(Inst->getLoc()), li);
            return recordClonedInstruction(Inst, si);
         }
         case StoreOwnershipQualifier::Init:
         case StoreOwnershipQualifier::Trivial:
         case StoreOwnershipQualifier::Unqualified:
            break;
      }

      return recordClonedInstruction(
         Inst, getBuilder().createStore(getOpLocation(Inst->getLoc()),
                                        getOpValue(Inst->getSrc()),
                                        getOpValue(Inst->getDest()),
                                        StoreOwnershipQualifier::Unqualified));
   }

   recordClonedInstruction(
      Inst, getBuilder().createStore(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getSrc()),
         getOpValue(Inst->getDest()), Inst->getOwnershipQualifier()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitStoreBorrowInst(StoreBorrowInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   if (!getBuilder().hasOwnership()) {
      // TODO: Eliminate store_borrow result so we can use
      // recordClonedInstruction. It is not "technically" necessary, but it is
      // better from an invariant perspective.
      getBuilder().createStore(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getSrc()),
         getOpValue(Inst->getDest()), StoreOwnershipQualifier::Unqualified);
      return;
   }

   recordClonedInstruction(
      Inst, getBuilder().createStoreBorrow(getOpLocation(Inst->getLoc()),
                                           getOpValue(Inst->getSrc()),
                                           getOpValue(Inst->getDest())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitEndBorrowInst(EndBorrowInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));

   // Do not clone any end_borrow.
   if (!getBuilder().hasOwnership())
      return;

   recordClonedInstruction(
      Inst,
      getBuilder().createEndBorrow(getOpLocation(Inst->getLoc()),
                                   getOpValue(Inst->getOperand()), PILValue()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitBeginAccessInst(BeginAccessInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createBeginAccess(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         Inst->getAccessKind(), Inst->getEnforcement(),
         Inst->hasNoNestedConflict(), Inst->isFromBuiltin()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitEndAccessInst(EndAccessInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createEndAccess(getOpLocation(Inst->getLoc()),
                                         getOpValue(Inst->getOperand()),
                                         Inst->isAborting()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitBeginUnpairedAccessInst(
   BeginUnpairedAccessInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createBeginUnpairedAccess(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getSource()),
         getOpValue(Inst->getBuffer()), Inst->getAccessKind(),
         Inst->getEnforcement(), Inst->hasNoNestedConflict(),
         Inst->isFromBuiltin()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitEndUnpairedAccessInst(
   EndUnpairedAccessInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createEndUnpairedAccess(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      Inst->getEnforcement(), Inst->isAborting(),
      Inst->isFromBuiltin()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitAssignInst(AssignInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createAssign(getOpLocation(Inst->getLoc()),
                                      getOpValue(Inst->getSrc()),
                                      getOpValue(Inst->getDest()),
                                      Inst->getOwnershipQualifier()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitAssignByWrapperInst(AssignByWrapperInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createAssignByWrapper(getOpLocation(Inst->getLoc()),
                                               getOpValue(Inst->getSrc()),
                                               getOpValue(Inst->getDest()),
                                               getOpValue(Inst->getInitializer()),
                                               getOpValue(Inst->getSetter()),
                                               Inst->getOwnershipQualifier()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitMarkUninitializedInst(MarkUninitializedInst *Inst) {
   PILValue OpValue = getOpValue(Inst->getOperand());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createMarkUninitialized(getOpLocation(Inst->getLoc()),
                                                 OpValue, Inst->getKind()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitMarkFunctionEscapeInst(MarkFunctionEscapeInst *Inst){
   auto OpElements = getOpValueArray<8>(Inst->getElements());
   auto OpLoc = getOpLocation(Inst->getLoc());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createMarkFunctionEscape(OpLoc, OpElements));
}
template<typename ImplClass>
void
PILCloner<ImplClass>::visitDebugValueInst(DebugValueInst *Inst) {
   // We cannot inline/clone debug intrinsics without a scope. If they
   // describe function arguments there is no way to determine which
   // function they belong to.
   if (!Inst->getDebugScope())
      return;

   // Since we want the debug info to survive, we do not remap the location here.
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createDebugValue(Inst->getLoc(),
                                          getOpValue(Inst->getOperand()),
                                          *Inst->getVarInfo()));
}
template<typename ImplClass>
void
PILCloner<ImplClass>::visitDebugValueAddrInst(DebugValueAddrInst *Inst) {
   // We cannot inline/clone debug intrinsics without a scope. If they
   // describe function arguments there is no way to determine which
   // function they belong to.
   if (!Inst->getDebugScope())
      return;

   // Do not remap the location for a debug Instruction.
   PILValue OpValue = getOpValue(Inst->getOperand());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createDebugValueAddr(Inst->getLoc(), OpValue,
                                              *Inst->getVarInfo()));
}

#define NEVER_LOADABLE_CHECKED_REF_STORAGE(Name, name, ...)                    \
  template <typename ImplClass>                                                \
  void PILCloner<ImplClass>::visitLoad##Name##Inst(Load##Name##Inst *Inst) {   \
    getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));      \
    recordClonedInstruction(                                                   \
        Inst, getBuilder().createLoad##Name(getOpLocation(Inst->getLoc()),     \
                                            getOpValue(Inst->getOperand()),    \
                                            Inst->isTake()));                  \
  }                                                                            \
  template <typename ImplClass>                                                \
  void PILCloner<ImplClass>::visitStore##Name##Inst(Store##Name##Inst *Inst) { \
    getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));      \
    recordClonedInstruction(                                                   \
        Inst, getBuilder().createStore##Name(getOpLocation(Inst->getLoc()),    \
                                             getOpValue(Inst->getSrc()),       \
                                             getOpValue(Inst->getDest()),      \
                                             Inst->isInitializationOfDest())); \
  }
#define LOADABLE_REF_STORAGE_HELPER(Name, name)                                \
  template <typename ImplClass>                                                \
  void PILCloner<ImplClass>::visitRefTo##Name##Inst(RefTo##Name##Inst *Inst) { \
    getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));      \
    recordClonedInstruction(                                                   \
        Inst, getBuilder().createRefTo##Name(getOpLocation(Inst->getLoc()),    \
                                             getOpValue(Inst->getOperand()),   \
                                             getOpType(Inst->getType())));     \
  }                                                                            \
  template <typename ImplClass>                                                \
  void PILCloner<ImplClass>::visit##Name##ToRefInst(Name##ToRefInst *Inst) {   \
    getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));      \
    recordClonedInstruction(                                                   \
        Inst, getBuilder().create##Name##ToRef(getOpLocation(Inst->getLoc()),  \
                                               getOpValue(Inst->getOperand()), \
                                               getOpType(Inst->getType())));   \
  }                                                                            \
  template <typename ImplClass>                                                \
  void PILCloner<ImplClass>::visitStrongCopy##Name##ValueInst(                 \
      StrongCopy##Name##ValueInst *Inst) {                                     \
    getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));      \
    recordClonedInstruction(Inst, getBuilder().createStrongCopy##Name##Value(  \
                                      getOpLocation(Inst->getLoc()),           \
                                      getOpValue(Inst->getOperand())));        \
  }
#define ALWAYS_LOADABLE_CHECKED_REF_STORAGE(Name, name, ...)                   \
  LOADABLE_REF_STORAGE_HELPER(Name, name)                                      \
  template <typename ImplClass>                                                \
  void PILCloner<ImplClass>::visitStrongRetain##Name##Inst(                    \
      StrongRetain##Name##Inst *Inst) {                                        \
    getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));      \
    recordClonedInstruction(Inst, getBuilder().createStrongRetain##Name(       \
                                      getOpLocation(Inst->getLoc()),           \
                                      getOpValue(Inst->getOperand()),          \
                                      Inst->getAtomicity()));                  \
  }                                                                            \
  template <typename ImplClass>                                                \
  void PILCloner<ImplClass>::visit##Name##RetainInst(Name##RetainInst *Inst) { \
    getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));      \
    recordClonedInstruction(Inst, getBuilder().create##Name##Retain(           \
                                      getOpLocation(Inst->getLoc()),           \
                                      getOpValue(Inst->getOperand()),          \
                                      Inst->getAtomicity()));                  \
  }                                                                            \
  template <typename ImplClass>                                                \
  void PILCloner<ImplClass>::visit##Name##ReleaseInst(                         \
      Name##ReleaseInst *Inst) {                                               \
    getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));      \
    recordClonedInstruction(Inst, getBuilder().create##Name##Release(          \
                                      getOpLocation(Inst->getLoc()),           \
                                      getOpValue(Inst->getOperand()),          \
                                      Inst->getAtomicity()));                  \
  }
#define SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, name, ...) \
  NEVER_LOADABLE_CHECKED_REF_STORAGE(Name, name, "...") \
  ALWAYS_LOADABLE_CHECKED_REF_STORAGE(Name, name, "...")
#define UNCHECKED_REF_STORAGE(Name, name, ...) \
  LOADABLE_REF_STORAGE_HELPER(Name, name)
#include "polarphp/ast/ReferenceStorageDef.h"
#undef LOADABLE_REF_STORAGE_HELPER

template<typename ImplClass>
void
PILCloner<ImplClass>::visitCopyAddrInst(CopyAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createCopyAddr(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getSrc()),
         getOpValue(Inst->getDest()), Inst->isTakeOfSrc(),
         Inst->isInitializationOfDest()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitBindMemoryInst(BindMemoryInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createBindMemory(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getBase()),
         getOpValue(Inst->getIndex()), getOpType(Inst->getBoundType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitConvertFunctionInst(ConvertFunctionInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createConvertFunction(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         getOpType(Inst->getType()), Inst->withoutActuallyEscaping()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitConvertEscapeToNoEscapeInst(
   ConvertEscapeToNoEscapeInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createConvertEscapeToNoEscape(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         getOpType(Inst->getType()), Inst->isLifetimeGuaranteed()));
}

template<typename ImplClass>
void PILCloner<ImplClass>::visitThinFunctionToPointerInst(
   ThinFunctionToPointerInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createThinFunctionToPointer(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      getOpType(Inst->getType())));
}

template<typename ImplClass>
void PILCloner<ImplClass>::visitPointerToThinFunctionInst(
   PointerToThinFunctionInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createPointerToThinFunction(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitUpcastInst(UpcastInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createUpcast(getOpLocation(Inst->getLoc()),
                                      getOpValue(Inst->getOperand()),
                                      getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitAddressToPointerInst(AddressToPointerInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createAddressToPointer(getOpLocation(Inst->getLoc()),
                                                getOpValue(Inst->getOperand()),
                                                getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitPointerToAddressInst(PointerToAddressInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createPointerToAddress(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      getOpType(Inst->getType()),
      Inst->isStrict(), Inst->isInvariant()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitUncheckedRefCastInst(UncheckedRefCastInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createUncheckedRefCast(getOpLocation(Inst->getLoc()),
                                                getOpValue(Inst->getOperand()),
                                                getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitUncheckedRefCastAddrInst(UncheckedRefCastAddrInst *Inst) {
   PILLocation OpLoc = getOpLocation(Inst->getLoc());
   PILValue SrcValue = getOpValue(Inst->getSrc());
   PILValue DestValue = getOpValue(Inst->getDest());
   CanType SrcType = getOpASTType(Inst->getSourceFormalType());
   CanType TargetType = getOpASTType(Inst->getTargetFormalType());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createUncheckedRefCastAddr(OpLoc, SrcValue, SrcType,
                                                    DestValue, TargetType));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitUncheckedAddrCastInst(UncheckedAddrCastInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createUncheckedAddrCast(getOpLocation(Inst->getLoc()),
                                                 getOpValue(Inst->getOperand()),
                                                 getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitUncheckedTrivialBitCastInst(UncheckedTrivialBitCastInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createUncheckedTrivialBitCast(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitUncheckedBitwiseCastInst(UncheckedBitwiseCastInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createUncheckedBitwiseCast(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitRefToBridgeObjectInst(RefToBridgeObjectInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createRefToBridgeObject(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getConverted()),
      getOpValue(Inst->getBitsOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitBridgeObjectToRefInst(BridgeObjectToRefInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createBridgeObjectToRef(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getConverted()),
      getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitBridgeObjectToWordInst(BridgeObjectToWordInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createBridgeObjectToWord(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getConverted()),
      getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitRefToRawPointerInst(RefToRawPointerInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createRefToRawPointer(getOpLocation(Inst->getLoc()),
                                               getOpValue(Inst->getOperand()),
                                               getOpType(Inst->getType())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitValueToBridgeObjectInst(
   ValueToBridgeObjectInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createValueToBridgeObject(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitRawPointerToRefInst(RawPointerToRefInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createRawPointerToRef(getOpLocation(Inst->getLoc()),
                                               getOpValue(Inst->getOperand()),
                                               getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitThinToThickFunctionInst(ThinToThickFunctionInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createThinToThickFunction(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      getOpType(Inst->getType())));
}

//template<typename ImplClass>
//void
//PILCloner<ImplClass>::
//visitThickToObjCMetatypeInst(ThickToObjCMetatypeInst *Inst) {
//   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
//   recordClonedInstruction(Inst, getBuilder().createThickToObjCMetatype(
//      getOpLocation(Inst->getLoc()),
//      getOpValue(Inst->getOperand()),
//      getOpType(Inst->getType())));
//}
//
//template<typename ImplClass>
//void
//PILCloner<ImplClass>::
//visitObjCToThickMetatypeInst(ObjCToThickMetatypeInst *Inst) {
//   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
//   recordClonedInstruction(Inst, getBuilder().createObjCToThickMetatype(
//      getOpLocation(Inst->getLoc()),
//      getOpValue(Inst->getOperand()),
//      getOpType(Inst->getType())));
//}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitUnconditionalCheckedCastInst(
   UnconditionalCheckedCastInst *Inst) {
   PILLocation OpLoc = getOpLocation(Inst->getLoc());
   PILValue OpValue = getOpValue(Inst->getOperand());
   PILType OpLoweredType = getOpType(Inst->getTargetLoweredType());
   CanType OpFormalType = getOpASTType(Inst->getTargetFormalType());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createUnconditionalCheckedCast(
      OpLoc, OpValue,
      OpLoweredType, OpFormalType));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitUnconditionalCheckedCastAddrInst(
   UnconditionalCheckedCastAddrInst *Inst) {
   PILLocation OpLoc = getOpLocation(Inst->getLoc());
   PILValue SrcValue = getOpValue(Inst->getSrc());
   PILValue DestValue = getOpValue(Inst->getDest());
   CanType SrcType = getOpASTType(Inst->getSourceFormalType());
   CanType TargetType = getOpASTType(Inst->getTargetFormalType());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst,
                           getBuilder().createUnconditionalCheckedCastAddr(
                              OpLoc, SrcValue, SrcType, DestValue, TargetType));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitUnconditionalCheckedCastValueInst(
   UnconditionalCheckedCastValueInst *Inst) {
   PILLocation OpLoc = getOpLocation(Inst->getLoc());
   PILValue OpValue = getOpValue(Inst->getOperand());
   CanType SrcFormalType = getOpASTType(Inst->getSourceFormalType());
   PILType OpLoweredType = getOpType(Inst->getTargetLoweredType());
   CanType OpFormalType = getOpASTType(Inst->getTargetFormalType());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst,
      getBuilder().createUnconditionalCheckedCastValue(OpLoc,
                                                       OpValue,
                                                       SrcFormalType,
                                                       OpLoweredType,
                                                       OpFormalType));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitRetainValueInst(RetainValueInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createRetainValue(getOpLocation(Inst->getLoc()),
                                           getOpValue(Inst->getOperand()),
                                           Inst->getAtomicity()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitRetainValueAddrInst(RetainValueAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createRetainValueAddr(getOpLocation(Inst->getLoc()),
                                               getOpValue(Inst->getOperand()),
                                               Inst->getAtomicity()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitUnmanagedRetainValueInst(
   UnmanagedRetainValueInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   if (!getBuilder().hasOwnership()) {
      return recordClonedInstruction(
         Inst, getBuilder().createRetainValue(getOpLocation(Inst->getLoc()),
                                              getOpValue(Inst->getOperand()),
                                              Inst->getAtomicity()));
   }

   recordClonedInstruction(Inst, getBuilder().createUnmanagedRetainValue(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      Inst->getAtomicity()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitCopyValueInst(CopyValueInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   if (!getBuilder().hasOwnership()) {
      PILValue newValue = getBuilder().emitCopyValueOperation(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()));
      return recordFoldedValue(Inst, newValue);
   }

   recordClonedInstruction(
      Inst, getBuilder().createCopyValue(getOpLocation(Inst->getLoc()),
                                         getOpValue(Inst->getOperand())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitReleaseValueInst(ReleaseValueInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createReleaseValue(getOpLocation(Inst->getLoc()),
                                            getOpValue(Inst->getOperand()),
                                            Inst->getAtomicity()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitReleaseValueAddrInst(
   ReleaseValueAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createReleaseValueAddr(getOpLocation(Inst->getLoc()),
                                                getOpValue(Inst->getOperand()),
                                                Inst->getAtomicity()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitUnmanagedReleaseValueInst(
   UnmanagedReleaseValueInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   if (!getBuilder().hasOwnership()) {
      return recordClonedInstruction(
         Inst, getBuilder().createReleaseValue(getOpLocation(Inst->getLoc()),
                                               getOpValue(Inst->getOperand()),
                                               Inst->getAtomicity()));
   }
   recordClonedInstruction(Inst, getBuilder().createUnmanagedReleaseValue(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      Inst->getAtomicity()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitDestroyValueInst(DestroyValueInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   if (!getBuilder().hasOwnership()) {
      return recordClonedInstruction(
         Inst, getBuilder().createReleaseValue(
            getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
            RefCountingInst::Atomicity::Atomic));
   }

   recordClonedInstruction(
      Inst, getBuilder().createDestroyValue(getOpLocation(Inst->getLoc()),
                                            getOpValue(Inst->getOperand())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitAutoreleaseValueInst(
   AutoreleaseValueInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createAutoreleaseValue(getOpLocation(Inst->getLoc()),
                                                getOpValue(Inst->getOperand()),
                                                Inst->getAtomicity()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitUnmanagedAutoreleaseValueInst(
   UnmanagedAutoreleaseValueInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   if (!getBuilder().hasOwnership()) {
      return recordClonedInstruction(Inst, getBuilder().createAutoreleaseValue(
         getOpLocation(Inst->getLoc()),
         getOpValue(Inst->getOperand()),
         Inst->getAtomicity()));
   }

   recordClonedInstruction(Inst, getBuilder().createUnmanagedAutoreleaseValue(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      Inst->getAtomicity()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitSetDeallocatingInst(SetDeallocatingInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createSetDeallocating(getOpLocation(Inst->getLoc()),
                                               getOpValue(Inst->getOperand()),
                                               Inst->getAtomicity()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitObjectInst(ObjectInst *Inst) {
   auto Elements = getOpValueArray<8>(Inst->getAllElements());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst,
      getBuilder().createObject(getOpLocation(Inst->getLoc()), Inst->getType(),
                                Elements, Inst->getBaseElements().size()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitStructInst(StructInst *Inst) {
   auto Elements = getOpValueArray<8>(Inst->getElements());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createStruct(getOpLocation(Inst->getLoc()),
                                      getOpType(Inst->getType()), Elements));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitTupleInst(TupleInst *Inst) {
   auto Elements = getOpValueArray<8>(Inst->getElements());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createTuple(getOpLocation(Inst->getLoc()),
                                     getOpType(Inst->getType()), Elements));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitEnumInst(EnumInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst,
      getBuilder().createEnum(
         getOpLocation(Inst->getLoc()),
         Inst->hasOperand() ? getOpValue(Inst->getOperand()) : PILValue(),
         Inst->getElement(), getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitInitEnumDataAddrInst(InitEnumDataAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createInitEnumDataAddr(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         Inst->getElement(), getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitUncheckedEnumDataInst(UncheckedEnumDataInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createUncheckedEnumData(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         Inst->getElement(), getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitUncheckedTakeEnumDataAddrInst(UncheckedTakeEnumDataAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createUncheckedTakeEnumDataAddr(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         Inst->getElement(), getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitInjectEnumAddrInst(InjectEnumAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createInjectEnumAddr(getOpLocation(Inst->getLoc()),
                                              getOpValue(Inst->getOperand()),
                                              Inst->getElement()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitMetatypeInst(MetatypeInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createMetatype(getOpLocation(Inst->getLoc()),
                                        getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitValueMetatypeInst(ValueMetatypeInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createValueMetatype(getOpLocation(Inst->getLoc()),
                                             getOpType(Inst->getType()),
                                             getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitExistentialMetatypeInst(ExistentialMetatypeInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createExistentialMetatype(
      getOpLocation(Inst->getLoc()),
      getOpType(Inst->getType()),
      getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitTupleExtractInst(TupleExtractInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createTupleExtract(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         Inst->getFieldNo(), getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitTupleElementAddrInst(TupleElementAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createTupleElementAddr(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         Inst->getFieldNo(), getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitStructExtractInst(StructExtractInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createStructExtract(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         Inst->getField(), getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitStructElementAddrInst(StructElementAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createStructElementAddr(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         Inst->getField(), getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitRefElementAddrInst(RefElementAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createRefElementAddr(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         Inst->getField(), getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitRefTailAddrInst(RefTailAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createRefTailAddr(getOpLocation(Inst->getLoc()),
                                           getOpValue(Inst->getOperand()),
                                           getOpType(Inst->getType())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitDestructureStructInst(
   DestructureStructInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));

   if (!getBuilder().hasOwnership()) {
      getBuilder().emitDestructureValueOperation(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         [&](unsigned index, PILValue value) {
            recordFoldedValue(Inst->getResults()[index], value);
         });
      return;
   }

   recordClonedInstruction(
      Inst, getBuilder().createDestructureStruct(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitDestructureTupleInst(
   DestructureTupleInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   if (!getBuilder().hasOwnership()) {
      getBuilder().emitDestructureValueOperation(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         [&](unsigned index, PILValue value) {
            recordFoldedValue(Inst->getResults()[index], value);
         });
      return;
   }

   recordClonedInstruction(
      Inst, getBuilder().createDestructureTuple(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitClassMethodInst(ClassMethodInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createClassMethod(getOpLocation(Inst->getLoc()),
                                           getOpValue(Inst->getOperand()),
                                           Inst->getMember(), Inst->getType()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitSuperMethodInst(SuperMethodInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createSuperMethod(getOpLocation(Inst->getLoc()),
                                           getOpValue(Inst->getOperand()),
                                           Inst->getMember(), Inst->getType()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitObjCMethodInst(ObjCMethodInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createObjCMethod(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         Inst->getMember(), getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitObjCSuperMethodInst(ObjCSuperMethodInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createObjCSuperMethod(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      Inst->getMember(), Inst->getType()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitWitnessMethodInst(WitnessMethodInst *Inst) {
   auto lookupType = Inst->getLookupType();
   auto conformance = getOpConformance(lookupType, Inst->getConformance());
   auto newLookupType = getOpASTType(lookupType);

   if (conformance.isConcrete()) {
      CanType Ty = conformance.getConcrete()->getType()->getCanonicalType();

      if (Ty != newLookupType) {
         assert(
            (Ty->isExactSuperclassOf(newLookupType) ||
             getBuilder().getModule().Types.getLoweredRValueType(
                getBuilder().getTypeExpansionContext(), Ty) == newLookupType) &&
            "Should only create upcasts for sub class.");

         // We use the super class as the new look up type.
         newLookupType = Ty;
      }
   }

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst,
                           getBuilder().createWitnessMethod(
                              getOpLocation(Inst->getLoc()), newLookupType,
                              conformance, Inst->getMember(), Inst->getType()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitOpenExistentialAddrInst(OpenExistentialAddrInst *Inst) {
   // Create a new archetype for this opened existential type.
   remapOpenedType(Inst->getType().castTo<OpenedArchetypeType>());

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createOpenExistentialAddr(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         getOpType(Inst->getType()), Inst->getAccessKind()));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitOpenExistentialValueInst(
   OpenExistentialValueInst *Inst) {
   // Create a new archetype for this opened existential type.
   remapOpenedType(Inst->getType().castTo<OpenedArchetypeType>());

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createOpenExistentialValue(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitOpenExistentialMetatypeInst(OpenExistentialMetatypeInst *Inst) {
   // Create a new archetype for this opened existential type.
   auto openedType = Inst->getType().getAstType();
   auto exType = Inst->getOperand()->getType().getAstType();
   while (auto exMetatype = dyn_cast<ExistentialMetatypeType>(exType)) {
      exType = exMetatype.getInstanceType();
      openedType = cast<MetatypeType>(openedType).getInstanceType();
   }
   remapOpenedType(cast<OpenedArchetypeType>(openedType));

   if (!Inst->getOperand()->getType().canUseExistentialRepresentation(
      ExistentialRepresentation::Class)) {
      getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
      recordClonedInstruction(Inst, getBuilder().createOpenExistentialMetatype(
         getOpLocation(Inst->getLoc()),
         getOpValue(Inst->getOperand()),
         getOpType(Inst->getType())));
      return;
   }

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createOpenExistentialMetatype(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitOpenExistentialRefInst(OpenExistentialRefInst *Inst) {
   // Create a new archetype for this opened existential type.
   remapOpenedType(Inst->getType().castTo<OpenedArchetypeType>());

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createOpenExistentialRef(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitOpenExistentialBoxInst(OpenExistentialBoxInst *Inst) {
   // Create a new archetype for this opened existential type.
   remapOpenedType(Inst->getType().castTo<OpenedArchetypeType>());

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createOpenExistentialBox(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitOpenExistentialBoxValueInst(OpenExistentialBoxValueInst *Inst) {
   // Create a new archetype for this opened existential type.
   remapOpenedType(Inst->getType().castTo<OpenedArchetypeType>());

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createOpenExistentialBoxValue(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitInitExistentialAddrInst(InitExistentialAddrInst *Inst) {
   CanType origFormalType = Inst->getFormalConcreteType();

   auto conformances = getOpConformances(origFormalType,
                                         Inst->getConformances());

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createInitExistentialAddr(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         getOpASTType(origFormalType),
         getOpType(Inst->getLoweredConcreteType()), conformances));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitInitExistentialValueInst(
   InitExistentialValueInst *Inst) {
   CanType origFormalType = Inst->getFormalConcreteType();

   auto conformances = getOpConformances(origFormalType,
                                         Inst->getConformances());

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createInitExistentialValue(
         getOpLocation(Inst->getLoc()), getOpType(Inst->getType()),
         getOpASTType(origFormalType), getOpValue(Inst->getOperand()),
         conformances));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitInitExistentialMetatypeInst(InitExistentialMetatypeInst *Inst) {
   auto origFormalType = Inst->getFormalErasedObjectType();
   auto conformances = getOpConformances(origFormalType,
                                         Inst->getConformances());

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createInitExistentialMetatype(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      getOpType(Inst->getType()), conformances));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitInitExistentialRefInst(InitExistentialRefInst *Inst) {
   CanType origFormalType = Inst->getFormalConcreteType();
   auto conformances = getOpConformances(origFormalType,
                                         Inst->getConformances());

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createInitExistentialRef(
         getOpLocation(Inst->getLoc()), getOpType(Inst->getType()),
         getOpASTType(origFormalType), getOpValue(Inst->getOperand()),
         conformances));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitDeinitExistentialAddrInst(DeinitExistentialAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createDeinitExistentialAddr(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitDeinitExistentialValueInst(
   DeinitExistentialValueInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createDeinitExistentialValue(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitCopyBlockInst(CopyBlockInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, Builder.createCopyBlock(getOpLocation(Inst->getLoc()),
                                    getOpValue(Inst->getOperand())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitCopyBlockWithoutEscapingInst(
   CopyBlockWithoutEscapingInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, Builder.createCopyBlockWithoutEscaping(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getBlock()),
      getOpValue(Inst->getClosure())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitStrongRetainInst(StrongRetainInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createStrongRetain(getOpLocation(Inst->getLoc()),
                                            getOpValue(Inst->getOperand()),
                                            Inst->getAtomicity()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitClassifyBridgeObjectInst(
   ClassifyBridgeObjectInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createClassifyBridgeObject(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitFixLifetimeInst(FixLifetimeInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createFixLifetime(getOpLocation(Inst->getLoc()),
                                           getOpValue(Inst->getOperand())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitEndLifetimeInst(EndLifetimeInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));

   // These are only needed in OSSA.
   if (!getBuilder().hasOwnership())
      return;

   recordClonedInstruction(
      Inst, getBuilder().createEndLifetime(getOpLocation(Inst->getLoc()),
                                           getOpValue(Inst->getOperand())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitUncheckedOwnershipConversionInst(
   UncheckedOwnershipConversionInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));

   if (!getBuilder().hasOwnership()) {
      return recordFoldedValue(Inst, getOpValue(Inst->getOperand()));
   }

   ValueOwnershipKind Kind = PILValue(Inst).getOwnershipKind();
   if (getOpValue(Inst->getOperand()).getOwnershipKind() ==
       ValueOwnershipKind::None) {
      Kind = ValueOwnershipKind::None;
   }
   recordClonedInstruction(Inst, getBuilder().createUncheckedOwnershipConversion(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()), Kind));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitMarkDependenceInst(MarkDependenceInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createMarkDependence(getOpLocation(Inst->getLoc()),
                                              getOpValue(Inst->getValue()),
                                              getOpValue(Inst->getBase())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitStrongReleaseInst(StrongReleaseInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createStrongRelease(getOpLocation(Inst->getLoc()),
                                             getOpValue(Inst->getOperand()),
                                             Inst->getAtomicity()));
}

template<typename ImplClass>
void PILCloner<ImplClass>::visitIsUniqueInst(IsUniqueInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createIsUnique(getOpLocation(Inst->getLoc()),
                                        getOpValue(Inst->getOperand())));
}
template <typename ImplClass>
void PILCloner<ImplClass>::visitIsEscapingClosureInst(
   IsEscapingClosureInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createIsEscapingClosure(getOpLocation(Inst->getLoc()),
                                                 getOpValue(Inst->getOperand()),
                                                 Inst->getVerificationType()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitDeallocStackInst(DeallocStackInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createDeallocStack(getOpLocation(Inst->getLoc()),
                                            getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitDeallocRefInst(DeallocRefInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createDeallocRef(getOpLocation(Inst->getLoc()),
                                          getOpValue(Inst->getOperand()),
                                          Inst->canAllocOnStack()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitDeallocPartialRefInst(DeallocPartialRefInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createDeallocPartialRef(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getInstance()),
      getOpValue(Inst->getMetatype())));
}

template<typename ImplClass>
void PILCloner<ImplClass>::visitDeallocValueBufferInst(
   DeallocValueBufferInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createDeallocValueBuffer(
      getOpLocation(Inst->getLoc()),
      getOpType(Inst->getValueType()),
      getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitDeallocBoxInst(DeallocBoxInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createDeallocBox(getOpLocation(Inst->getLoc()),
                                          getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitDeallocExistentialBoxInst(
   DeallocExistentialBoxInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createDeallocExistentialBox(
      getOpLocation(Inst->getLoc()),
      getOpASTType(Inst->getConcreteType()),
      getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitDestroyAddrInst(DestroyAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createDestroyAddr(getOpLocation(Inst->getLoc()),
                                           getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void PILCloner<ImplClass>::visitProjectValueBufferInst(
   ProjectValueBufferInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createProjectValueBuffer(
      getOpLocation(Inst->getLoc()),
      getOpType(Inst->getValueType()),
      getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void PILCloner<ImplClass>::visitProjectBoxInst(ProjectBoxInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createProjectBox(getOpLocation(Inst->getLoc()),
                                          getOpValue(Inst->getOperand()),
                                          Inst->getFieldIndex()));
}

template<typename ImplClass>
void PILCloner<ImplClass>::visitProjectExistentialBoxInst(
   ProjectExistentialBoxInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createProjectExistentialBox(
      getOpLocation(Inst->getLoc()),
      getOpType(Inst->getType()),
      getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitCondFailInst(CondFailInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createCondFail(getOpLocation(Inst->getLoc()),
                                        getOpValue(Inst->getOperand()),
                                        Inst->getMessage()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitIndexAddrInst(IndexAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createIndexAddr(getOpLocation(Inst->getLoc()),
                                         getOpValue(Inst->getBase()),
                                         getOpValue(Inst->getIndex())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitTailAddrInst(TailAddrInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createTailAddr(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getBase()),
         getOpValue(Inst->getIndex()), getOpType(Inst->getType())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitIndexRawPointerInst(IndexRawPointerInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createIndexRawPointer(getOpLocation(Inst->getLoc()),
                                               getOpValue(Inst->getBase()),
                                               getOpValue(Inst->getIndex())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitUnreachableInst(UnreachableInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createUnreachable(getOpLocation(Inst->getLoc())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitReturnInst(ReturnInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createReturn(getOpLocation(Inst->getLoc()),
                                      getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitThrowInst(ThrowInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createThrow(getOpLocation(Inst->getLoc()),
                                     getOpValue(Inst->getOperand())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitUnwindInst(UnwindInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createUnwind(getOpLocation(Inst->getLoc())));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitYieldInst(YieldInst *Inst) {
   auto Values = getOpValueArray<8>(Inst->getYieldedValues());
   auto ResumeBB = getOpBasicBlock(Inst->getResumeBB());
   auto UnwindBB = getOpBasicBlock(Inst->getUnwindBB());

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createYield(getOpLocation(Inst->getLoc()), Values,
                                     ResumeBB, UnwindBB));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitBranchInst(BranchInst *Inst) {
   auto Args = getOpValueArray<8>(Inst->getArgs());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createBranch(
      getOpLocation(Inst->getLoc()),
      getOpBasicBlock(Inst->getDestBB()), Args));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitCondBranchInst(CondBranchInst *Inst) {
   auto TrueArgs = getOpValueArray<8>(Inst->getTrueArgs());
   auto FalseArgs = getOpValueArray<8>(Inst->getFalseArgs());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createCondBranch(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getCondition()),
         getOpBasicBlock(Inst->getTrueBB()), TrueArgs,
         getOpBasicBlock(Inst->getFalseBB()), FalseArgs,
         Inst->getTrueBBCount(), Inst->getFalseBBCount()));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitCheckedCastBranchInst(CheckedCastBranchInst *Inst) {
   PILBasicBlock *OpSuccBB = getOpBasicBlock(Inst->getSuccessBB());
   PILBasicBlock *OpFailBB = getOpBasicBlock(Inst->getFailureBB());
   auto TrueCount = Inst->getTrueBBCount();
   auto FalseCount = Inst->getFalseBBCount();
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createCheckedCastBranch(
         getOpLocation(Inst->getLoc()), Inst->isExact(),
         getOpValue(Inst->getOperand()),
         getOpType(Inst->getTargetLoweredType()),
         getOpASTType(Inst->getTargetFormalType()),
         OpSuccBB, OpFailBB, TrueCount, FalseCount));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitCheckedCastValueBranchInst(
   CheckedCastValueBranchInst *Inst) {
   PILBasicBlock *OpSuccBB = getOpBasicBlock(Inst->getSuccessBB());
   PILBasicBlock *OpFailBB = getOpBasicBlock(Inst->getFailureBB());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createCheckedCastValueBranch(
         getOpLocation(Inst->getLoc()),
         getOpValue(Inst->getOperand()),
         getOpASTType(Inst->getSourceFormalType()),
         getOpType(Inst->getTargetLoweredType()),
         getOpASTType(Inst->getTargetFormalType()),
         OpSuccBB, OpFailBB));
}

template<typename ImplClass>
void PILCloner<ImplClass>::visitCheckedCastAddrBranchInst(
   CheckedCastAddrBranchInst *Inst) {
   PILBasicBlock *OpSuccBB = getOpBasicBlock(Inst->getSuccessBB());
   PILBasicBlock *OpFailBB = getOpBasicBlock(Inst->getFailureBB());
   PILValue SrcValue = getOpValue(Inst->getSrc());
   PILValue DestValue = getOpValue(Inst->getDest());
   CanType SrcType = getOpASTType(Inst->getSourceFormalType());
   CanType TargetType = getOpASTType(Inst->getTargetFormalType());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   auto TrueCount = Inst->getTrueBBCount();
   auto FalseCount = Inst->getFalseBBCount();
   recordClonedInstruction(Inst, getBuilder().createCheckedCastAddrBranch(
      getOpLocation(Inst->getLoc()),
      Inst->getConsumptionKind(), SrcValue,
      SrcType, DestValue, TargetType, OpSuccBB,
      OpFailBB, TrueCount, FalseCount));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitSwitchValueInst(SwitchValueInst *Inst) {
   PILBasicBlock *DefaultBB = nullptr;
   if (Inst->hasDefault())
      DefaultBB = getOpBasicBlock(Inst->getDefaultBB());
   SmallVector<std::pair<PILValue, PILBasicBlock*>, 8> CaseBBs;
   for (int i = 0, e = Inst->getNumCases(); i != e; ++i)
      CaseBBs.push_back(std::make_pair(getOpValue(Inst->getCase(i).first),
                                       getOpBasicBlock(Inst->getCase(i).second)));
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createSwitchValue(getOpLocation(Inst->getLoc()),
                                           getOpValue(Inst->getOperand()),
                                           DefaultBB, CaseBBs));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitSwitchEnumInst(SwitchEnumInst *Inst) {
   PILBasicBlock *DefaultBB = nullptr;
   if (Inst->hasDefault())
      DefaultBB = getOpBasicBlock(Inst->getDefaultBB());
   SmallVector<std::pair<EnumElementDecl*, PILBasicBlock*>, 8> CaseBBs;
   for (unsigned i = 0, e = Inst->getNumCases(); i != e; ++i)
      CaseBBs.push_back(std::make_pair(Inst->getCase(i).first,
                                       getOpBasicBlock(Inst->getCase(i).second)));
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createSwitchEnum(getOpLocation(Inst->getLoc()),
                                          getOpValue(Inst->getOperand()),
                                          DefaultBB, CaseBBs));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::
visitSwitchEnumAddrInst(SwitchEnumAddrInst *Inst) {
   PILBasicBlock *DefaultBB = nullptr;
   if (Inst->hasDefault())
      DefaultBB = getOpBasicBlock(Inst->getDefaultBB());
   SmallVector<std::pair<EnumElementDecl*, PILBasicBlock*>, 8> CaseBBs;
   for (unsigned i = 0, e = Inst->getNumCases(); i != e; ++i)
      CaseBBs.push_back(std::make_pair(Inst->getCase(i).first,
                                       getOpBasicBlock(Inst->getCase(i).second)));
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createSwitchEnumAddr(getOpLocation(Inst->getLoc()),
                                              getOpValue(Inst->getOperand()),
                                              DefaultBB, CaseBBs));
}



template<typename ImplClass>
void
PILCloner<ImplClass>::visitSelectEnumInst(SelectEnumInst *Inst) {
   PILValue DefaultResult;
   if (Inst->hasDefault())
      DefaultResult = getOpValue(Inst->getDefaultResult());
   SmallVector<std::pair<EnumElementDecl*, PILValue>, 8> CaseResults;
   for (unsigned i = 0, e = Inst->getNumCases(); i != e; ++i)
      CaseResults.push_back(std::make_pair(Inst->getCase(i).first,
                                           getOpValue(Inst->getCase(i).second)));

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createSelectEnum(getOpLocation(Inst->getLoc()),
                                          getOpValue(Inst->getEnumOperand()),
                                          getOpType(Inst->getType()),
                                          DefaultResult, CaseResults));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitSelectEnumAddrInst(SelectEnumAddrInst *Inst) {
   PILValue DefaultResult;
   if (Inst->hasDefault())
      DefaultResult = getOpValue(Inst->getDefaultResult());
   SmallVector<std::pair<EnumElementDecl*, PILValue>, 8> CaseResults;
   for (unsigned i = 0, e = Inst->getNumCases(); i != e; ++i)
      CaseResults.push_back(std::make_pair(Inst->getCase(i).first,
                                           getOpValue(Inst->getCase(i).second)));

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createSelectEnumAddr(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getEnumOperand()),
      getOpType(Inst->getType()), DefaultResult,
      CaseResults));
}

template<typename ImplClass>
void
PILCloner<ImplClass>::visitSelectValueInst(SelectValueInst *Inst) {
   PILValue DefaultResult;
   if (Inst->hasDefault())
      DefaultResult = getOpValue(Inst->getDefaultResult());
   SmallVector<std::pair<PILValue, PILValue>, 8> CaseResults;
   for (unsigned i = 0, e = Inst->getNumCases(); i != e; ++i)
      CaseResults.push_back(std::make_pair(getOpValue(Inst->getCase(i).first),
                                           getOpValue(Inst->getCase(i).second)));

   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createSelectValue(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         getOpType(Inst->getType()), DefaultResult, CaseResults));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitDynamicMethodBranchInst(
   DynamicMethodBranchInst *Inst) {
   PILBasicBlock *OpHasMethodBB = getOpBasicBlock(Inst->getHasMethodBB());
   PILBasicBlock *OpHasNoMethodBB = getOpBasicBlock(Inst->getNoMethodBB());
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst, getBuilder().createDynamicMethodBranch(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
         Inst->getMember(), OpHasMethodBB, OpHasNoMethodBB));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitProjectBlockStorageInst(
   ProjectBlockStorageInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(Inst, getBuilder().createProjectBlockStorage(
      getOpLocation(Inst->getLoc()),
      getOpValue(Inst->getOperand()),
      getOpType(Inst->getType())));
}

template <typename ImplClass>
void PILCloner<ImplClass>::visitInitBlockStorageHeaderInst(
   InitBlockStorageHeaderInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   recordClonedInstruction(
      Inst,
      getBuilder().createInitBlockStorageHeader(
         getOpLocation(Inst->getLoc()), getOpValue(Inst->getBlockStorage()),
         getOpValue(Inst->getInvokeFunction()), getOpType(Inst->getType()),
         getOpSubstitutionMap(Inst->getSubstitutions())));
}
// @todo
//template <typename ImplClass>
//void PILCloner<ImplClass>::visitObjCMetatypeToObjectInst(
//   ObjCMetatypeToObjectInst *Inst) {
//   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
//   recordClonedInstruction(Inst, getBuilder().createObjCMetatypeToObject(
//      getOpLocation(Inst->getLoc()),
//      getOpValue(Inst->getOperand()),
//      getOpType(Inst->getType())));
//}
//
//template <typename ImplClass>
//void PILCloner<ImplClass>::visitObjCExistentialMetatypeToObjectInst(
//   ObjCExistentialMetatypeToObjectInst *Inst) {
//   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
//   recordClonedInstruction(
//      Inst, getBuilder().createObjCExistentialMetatypeToObject(
//         getOpLocation(Inst->getLoc()), getOpValue(Inst->getOperand()),
//         getOpType(Inst->getType())));
//}

//template <typename ImplClass>
//void PILCloner<ImplClass>::visitObjCInterfaceInst(ObjCInterfaceInst *Inst) {
//   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
//   recordClonedInstruction(
//      Inst, getBuilder().createObjCInterface(getOpLocation(Inst->getLoc()),
//                                            Inst->getInterface(),
//                                            getOpType(Inst->getType())));
//}

template <typename ImplClass>
void PILCloner<ImplClass>::visitKeyPathInst(KeyPathInst *Inst) {
   getBuilder().setCurrentDebugScope(getOpScope(Inst->getDebugScope()));
   SmallVector<PILValue, 4> opValues;
   for (auto &op : Inst->getAllOperands())
      opValues.push_back(getOpValue(op.get()));

   recordClonedInstruction(Inst,
                           getBuilder().createKeyPath(
                              getOpLocation(Inst->getLoc()), Inst->getPattern(),
                              getOpSubstitutionMap(Inst->getSubstitutions()),
                              opValues, getOpType(Inst->getType())));
}

} // end namespace polar

#endif
