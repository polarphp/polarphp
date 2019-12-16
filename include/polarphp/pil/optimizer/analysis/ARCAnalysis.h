//===--- ARCAnalysis.h - PIL ARC Analysis -----------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_ARCANALYSIS_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_ARCANALYSIS_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/analysis/AliasAnalysis.h"
#include "polarphp/pil/optimizer/analysis/PostOrderAnalysis.h"
#include "polarphp/pil/optimizer/analysis/RCIdentityAnalysis.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/TinyPtrVector.h"

namespace polar {

class PILInstruction;
class AliasAnalysis;
class PostOrderAnalysis;
class RCIdentityAnalysis;
class RCIdentityFunctionInfo;
class LoopRegionFunctionInfo;
class PILLoopInfo;
class PILFunction;

/// Return true if this is a retain instruction.
bool isRetainInstruction(PILInstruction *II);

/// Return true if this is a release instruction.
bool isReleaseInstruction(PILInstruction *II);

/// \returns True if the user \p User decrements the ref count of \p Ptr.
bool mayDecrementRefCount(PILInstruction *User, PILValue Ptr,
                          AliasAnalysis *AA);

/// \returns True if the \p User might use the pointer \p Ptr in a manner that
/// requires \p Ptr to be alive before Inst or the release of Ptr may use memory
/// accessed by \p User.
bool mayHaveSymmetricInterference(PILInstruction *User, PILValue Ptr,
                                  AliasAnalysis *AA);

/// \returns True if the \p User must use the pointer \p Ptr in a manner that
/// requires \p Ptr to be alive before Inst.
bool mustUseValue(PILInstruction *User, PILValue Ptr, AliasAnalysis *AA);

/// Returns true if User must use Ptr in a guaranteed way.
///
/// This means that assuming that everything is conservative, we can ignore the
/// ref count effects of User on Ptr since we will only remove things over
/// guaranteed parameters if we are known safe in both directions.
bool mustGuaranteedUseValue(PILInstruction *User, PILValue Ptr,
                            AliasAnalysis *AA);

/// Returns true if \p Inst can never conservatively decrement reference counts.
bool canNeverDecrementRefCounts(PILInstruction *Inst);

/// Returns true if \p Inst may access any indirect object either via an address
/// or reference.
///
/// If false is returned and \p Inst has an address or reference type operand,
/// then \p Inst only operates on the value of the address itself, not the
/// memory. i.e. it does not dereference the address.
bool canUseObject(PILInstruction *Inst);

/// \returns true if the user \p User may use \p Ptr in a manner that requires
/// Ptr's life to be guaranteed to exist at this point.
///
/// TODO: Better name.
bool mayGuaranteedUseValue(PILInstruction *User, PILValue Ptr,
                           AliasAnalysis *AA);

/// If \p Op has arc uses in the instruction range [Start, End), return the
/// first such instruction. Otherwise return None. We assume that
/// Start and End are both in the same basic block.
Optional<PILBasicBlock::iterator>
valueHasARCUsesInInstructionRange(PILValue Op,
                                  PILBasicBlock::iterator Start,
                                  PILBasicBlock::iterator End,
                                  AliasAnalysis *AA);

/// If \p Op has arc uses in the instruction range [Start, End), return the last
/// use of such instruction. Otherwise return None. We assume that Start and End
/// are both in the same basic block.
Optional<PILBasicBlock::iterator> valueHasARCUsesInReverseInstructionRange(
   PILValue Op, PILBasicBlock::iterator Start, PILBasicBlock::iterator End,
   AliasAnalysis *AA);

/// If \p Op has instructions in the instruction range (Start, End] which may
/// decrement it, return the first such instruction. Returns None
/// if no such instruction exists. We assume that Start and End are both in the
/// same basic block.
Optional<PILBasicBlock::iterator>
valueHasARCDecrementOrCheckInInstructionRange(PILValue Op,
                                              PILBasicBlock::iterator Start,
                                              PILBasicBlock::iterator End,
                                              AliasAnalysis *AA);

/// A class that attempts to match owned return value and corresponding
/// epilogue retains for a specific function.
///
/// If we can not find the retain in the return block, we will try to find
/// in the predecessors.
///
/// The search stop when we encounter an instruction that may decrement
/// the return'ed value, as we do not want to create a lifetime gap once the
/// retain is moved.
class ConsumedResultToEpilogueRetainMatcher {
public:
   /// The state on how retains are found in a basic block.
   enum class FindRetainKind {
      None,      ///< Did not find a retain.
      Found,     ///< Found a retain.
      Recursion, ///< Found a retain and its due to self-recursion.
      Blocked    ///< Found a blocking instructions, i.e. MayDecrement.
   };

   using RetainKindValue = std::pair<FindRetainKind, PILInstruction *>;

private:
   PILFunction *F;
   RCIdentityFunctionInfo *RCFI;
   AliasAnalysis *AA;

   // We use a list of instructions for now so that we can keep the same interface
   // and handle exploded retain_value later.
   TinyPtrVector<PILInstruction *> EpilogueRetainInsts;

public:
   /// Finds matching releases in the return block of the function \p F.
   ConsumedResultToEpilogueRetainMatcher(RCIdentityFunctionInfo *RCFI,
                                         AliasAnalysis *AA,
                                         PILFunction *F);

   /// Finds matching releases in the provided block \p BB.
   void findMatchingRetains(PILBasicBlock *BB);

   ArrayRef<PILInstruction *> getEpilogueRetains() const {
      return EpilogueRetainInsts;
   }

   /// Recompute the mapping from argument to consumed arg.
   void recompute();

   using iterator = decltype(EpilogueRetainInsts)::iterator;
   using const_iterator = decltype(EpilogueRetainInsts)::const_iterator;
   iterator begin() { return EpilogueRetainInsts.begin(); }
   iterator end() { return EpilogueRetainInsts.end(); }
   const_iterator begin() const { return EpilogueRetainInsts.begin(); }
   const_iterator end() const { return EpilogueRetainInsts.end(); }

   using reverse_iterator = decltype(EpilogueRetainInsts)::reverse_iterator;
   using const_reverse_iterator = decltype(EpilogueRetainInsts)::const_reverse_iterator;
   reverse_iterator rbegin() { return EpilogueRetainInsts.rbegin(); }
   reverse_iterator rend() { return EpilogueRetainInsts.rend(); }
   const_reverse_iterator rbegin() const { return EpilogueRetainInsts.rbegin(); }
   const_reverse_iterator rend() const { return EpilogueRetainInsts.rend(); }

   unsigned size() const { return EpilogueRetainInsts.size(); }

   iterator_range<iterator> getRange() { return polar::make_range(begin(), end()); }

private:
   /// Return true if all the successors of the EpilogueRetainInsts do not have
   /// a retain.
   bool
   isTransitiveSuccessorsRetainFree(const llvm::DenseSet<PILBasicBlock *> &BBs);

   /// Finds matching releases in the provided block \p BB.
   RetainKindValue findMatchingRetainsInBasicBlock(PILBasicBlock *BB,
                                                   PILValue V);
};

/// A class that attempts to match owned arguments and corresponding epilogue
/// releases for a specific function.
///
/// Only try to find the epilogue release in the return block.
class ConsumedArgToEpilogueReleaseMatcher {
public:
   enum class ExitKind { Return, Throw };

private:
   PILFunction *F;
   RCIdentityFunctionInfo *RCFI;
   ExitKind Kind;
   ArrayRef<PILArgumentConvention> ArgumentConventions;

   class ArgumentState {
      /// The list of releases associated with this argument.
      TinyPtrVector<PILInstruction *> releases;

      /// If this is set to true, then we know that we were able to find
      /// a set of releases.
      bool jointPostDominatingReleaseSet;

   public:
      ArgumentState(ArrayRef<PILInstruction *> releases)
         : releases(releases), jointPostDominatingReleaseSet(false) {}

      void addRelease(PILInstruction *release) { releases.push_back(release); }
      void setHasJointPostDominatingReleaseSet() {
         jointPostDominatingReleaseSet = true;
      }

      bool foundSomeButNotAllReleases() const {
         return releases.size() && !jointPostDominatingReleaseSet;
      }

      /// If we were able to find a set of releases for this argument that joint
      /// post-dominate the argument, return our release set.
      Optional<ArrayRef<PILInstruction *>> getFullyPostDomReleases() const {
         if (releases.empty() || foundSomeButNotAllReleases())
            return None;
         return ArrayRef<PILInstruction *>(releases);
      }

      /// If we were able to find a set of releases for this argument, but those
      /// releases do not joint post-dominate the argument, return our release
      /// set.
      ///
      /// *NOTE* This returns none if we did not find any releases.
      Optional<ArrayRef<PILInstruction *>> getPartiallyPostDomReleases() const {
         if (releases.empty() || !foundSomeButNotAllReleases())
            return None;
         return ArrayRef<PILInstruction *>(releases);
      }
   };
   llvm::SmallMapVector<PILArgument *, ArgumentState, 8> ArgInstMap;

   /// Eventually this will be used in place of HasBlock.
   PILBasicBlock *ProcessedBlock;

public:
   /// Finds matching releases in the return block of the function \p F.
   ConsumedArgToEpilogueReleaseMatcher(
      RCIdentityFunctionInfo *RCFI,
      PILFunction *F,
      ArrayRef<PILArgumentConvention> ArgumentConventions =
         {PILArgumentConvention::Direct_Owned},
      ExitKind Kind = ExitKind::Return);

   /// Finds matching releases in the provided block \p BB.
   void findMatchingReleases(PILBasicBlock *BB);

   bool hasBlock() const { return ProcessedBlock != nullptr; }

   bool isEpilogueRelease(PILInstruction *i) const {
      // This is not a release instruction in the epilogue block.
      if (i->getParent() != ProcessedBlock)
         return false;

      using PairTy = const std::pair<PILArgument *, ArgumentState>;
      return llvm::any_of(ArgInstMap, [&i](PairTy &p) {
         auto completeList = p.second.getFullyPostDomReleases();
         // If we did not have a complete post dominating release set, then we do
         // not want to treat any releases from p as epilogue releases.
         if (!completeList)
            return false;

         // Then make sure that we either found an epilogue release or found
         // exploded epilogue releases. We rely on our callers to split up exploded
         // parameters.
         return completeList->size() == 1 && *completeList->begin() == i;
      });
   }

   /// Return true if we've found some epilogue releases for the argument
   /// but not all.
   bool hasSomeReleasesForArgument(PILArgument *arg) const {
      auto iter = ArgInstMap.find(arg);
      if (iter == ArgInstMap.end())
         return false;
      return iter->second.foundSomeButNotAllReleases();
   }

   bool isSingleRelease(PILArgument *arg) const {
      auto iter = ArgInstMap.find(arg);
      assert(iter != ArgInstMap.end() &&
             "Failed to get release list for argument");

      // If we do not have a fully post dominating release set bail.
      auto completeList = iter->second.getFullyPostDomReleases();
      if (!completeList)
         return false;

      return completeList->size() == 1;
   }

   PILInstruction *getSingleReleaseForArgument(PILArgument *arg) const {
      auto iter = ArgInstMap.find(arg);
      if (iter == ArgInstMap.end())
         return nullptr;
      if (!isSingleRelease(arg))
         return nullptr;
      auto completeList = iter->second.getFullyPostDomReleases();
      if (!completeList)
         return nullptr;
      return *completeList->begin();
   }

   PILInstruction *getSingleReleaseForArgument(PILValue value) const {
      auto *arg = dyn_cast<PILArgument>(value);
      if (!arg)
         return nullptr;
      return getSingleReleaseForArgument(arg);
   }

   ArrayRef<PILInstruction *> getReleasesForArgument(PILArgument *arg) const {
      auto iter = ArgInstMap.find(arg);
      if (iter == ArgInstMap.end())
         return {};
      auto completeList = iter->second.getFullyPostDomReleases();
      if (!completeList)
         return {};
      return completeList.getValue();
   }

   Optional<ArrayRef<PILInstruction *>>
   getPartiallyPostDomReleaseSet(PILArgument *arg) const {
      auto iter = ArgInstMap.find(arg);
      if (iter == ArgInstMap.end())
         return None;
      auto partialList = iter->second.getPartiallyPostDomReleases();
      if (!partialList)
         return None;
      return partialList;
   }

   ArrayRef<PILInstruction *> getReleasesForArgument(PILValue value) const {
      auto *arg = dyn_cast<PILArgument>(value);
      if (!arg)
         return {};
      return getReleasesForArgument(arg);
   }

   /// Recompute the mapping from argument to consumed arg.
   void recompute();

   bool isSingleReleaseMatchedToArgument(PILInstruction *inst) {
      using PairTy = const std::pair<PILArgument *, ArgumentState>;
      return count_if(ArgInstMap, [&inst](PairTy &p) {
         auto completeList = p.second.getFullyPostDomReleases();
         if (!completeList || completeList->size() > 1)
            return false;
         return *completeList->begin() == inst;
      });
   }

private:
   /// Return true if we have seen releases to part or all of \p Derived in
   /// \p Insts.
   ///
   /// NOTE: This function relies on projections to analyze the relation
   /// between the releases values in \p Insts and \p Derived, it also bails
   /// out and return true if projection path can not be formed between Base
   /// and any one the released values.
   bool isRedundantRelease(ArrayRef<PILInstruction *> Insts, PILValue Base,
                           PILValue Derived);

   /// Return true if we have a release instruction for all the reference
   /// semantics part of \p Argument.
   bool releaseArgument(ArrayRef<PILInstruction *> Insts, PILValue Argument);

   /// Walk the basic block and find all the releases that match to function
   /// arguments.
   void collectMatchingReleases(PILBasicBlock *BB);

   /// Walk the function and find all the destroy_addr instructions that match
   /// to function arguments.
   void collectMatchingDestroyAddresses(PILBasicBlock *BB);

   /// For every argument in the function, check to see whether all epilogue
   /// releases are found. Clear all releases for the argument if not all
   /// epilogue releases are found.
   void processMatchingReleases();
};

class ReleaseTracker {
   llvm::SmallSetVector<PILInstruction *, 4> TrackedUsers;
   llvm::SmallSetVector<PILInstruction *, 4> FinalReleases;
   std::function<bool(PILInstruction *)> AcceptableUserQuery;
   std::function<bool(PILInstruction *)> TransitiveUserQuery;

public:
   ReleaseTracker(std::function<bool(PILInstruction *)> AcceptableUserQuery,
                  std::function<bool(PILInstruction *)> TransitiveUserQuery)
      : TrackedUsers(), FinalReleases(),
        AcceptableUserQuery(AcceptableUserQuery),
        TransitiveUserQuery(TransitiveUserQuery) {}

   void trackLastRelease(PILInstruction *Inst) { FinalReleases.insert(Inst); }

   bool isUserAcceptable(PILInstruction *User) const {
      return AcceptableUserQuery(User);
   }
   bool isUserTransitive(PILInstruction *User) const {
      return TransitiveUserQuery(User);
   }

   bool isUser(PILInstruction *User) { return TrackedUsers.count(User); }

   void trackUser(PILInstruction *User) { TrackedUsers.insert(User); }

   using range = iterator_range<llvm::SmallSetVector<PILInstruction *, 4>::iterator>;

   // An ordered list of users, with "casts" before their transitive uses.
   range getTrackedUsers() { return {TrackedUsers.begin(), TrackedUsers.end()}; }

   range getFinalReleases() {
      return {FinalReleases.begin(), FinalReleases.end()};
   }
};

/// Return true if we can find a set of post-dominating final releases. Returns
/// false otherwise. The FinalRelease set is placed in the out parameter
/// FinalRelease.
bool getFinalReleasesForValue(PILValue Value, ReleaseTracker &Tracker);

/// Match a call to a trap BB with no ARC relevant side effects.
bool isARCInertTrapBB(const PILBasicBlock *BB);

/// Get the two result values of the builtin "unsafeGuaranteed" instruction.
///
/// Gets the (GuaranteedValue, Token) tuple from a call to "unsafeGuaranteed"
/// if the tuple elements are identified by a single tuple_extract use.
/// Otherwise, returns a (nullptr, nullptr) tuple.
std::pair<SingleValueInstruction *, SingleValueInstruction *>
getSingleUnsafeGuaranteedValueResult(BuiltinInst *UnsafeGuaranteedInst);

/// Get the single builtin "unsafeGuaranteedEnd" user of a builtin
/// "unsafeGuaranteed"'s token.
BuiltinInst *getUnsafeGuaranteedEndUser(PILValue UnsafeGuaranteedToken);

/// Walk forwards from an unsafeGuaranteedEnd builtin instruction looking for a
/// release on the reference returned by the matching unsafeGuaranteed builtin
/// ignoring releases on the way.
/// Return nullptr if no release is found.
///
///    %4 = builtin "unsafeGuaranteed"<Foo>(%0 : $Foo) : $(Foo, Builtin.Int8)
///    %5 = tuple_extract %ArraySemantic.h : $(Foo, Builtin.Int8), 0
///    %6 = tuple_extract %4 : $(Foo, Builtin.Int8), 1
///    %12 = builtin "unsafeGuaranteedEnd"(%6 : $Builtin.Int8) : $()
///    strong_release %5 : $Foo // <-- Matching release.
///
/// Alternatively, look for the release before the unsafeGuaranteedEnd.
PILInstruction *findReleaseToMatchUnsafeGuaranteedValue(
   PILInstruction *UnsafeGuaranteedEndI, PILInstruction *UnsafeGuaranteedI,
   PILValue UnsafeGuaranteedValue, PILBasicBlock &BB,
   RCIdentityFunctionInfo &RCFI);

} // end namespace polar

#endif
