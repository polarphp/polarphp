// This source file is part of the polarphp.org open source project
//
// copyright (c) 2017 - 2018 polarphp software foundation
// copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/29.

#include "polarphp/basic/adt/DeltaAlgorithm.h"
#include <algorithm>
#include <iterator>
#include <set>

namespace polar {
namespace basic {

DeltaAlgorithm::~DeltaAlgorithm()
{
}

bool DeltaAlgorithm::getTestResult(const ChangeSetType &changes)
{
   if (m_failedTestsCache.count(changes)) {
      return false;
   }
   bool result = executeOneTest(changes);
   if (!result) {
      m_failedTestsCache.insert(changes);
   }
   return result;
}

void DeltaAlgorithm::split(const ChangeSetType &set, ChangeSetListType &result)
{
   // FIXME: Allow clients to provide heuristics for improved splitting.

   // FIXME: This is really slow.
   ChangeSetType lhs, rhs;
   unsigned idx = 0, N = set.size() / 2;
   for (ChangeSetType::const_iterator iter = set.begin(),
        ie = set.end(); iter != ie; ++iter, ++idx) {
      ((idx < N) ? lhs : rhs).insert(*iter);
   }
   if (!lhs.empty()) {
      result.push_back(lhs);
   }
   if (!rhs.empty()) {
      result.push_back(rhs);
   }
}

DeltaAlgorithm::ChangeSetType
DeltaAlgorithm::delta(const ChangeSetType &changes,
                      const ChangeSetListType &sets)
{
   // Invariant: union(result) == changes
   updatedSearchState(changes, sets);

   // If there is nothing left we can remove, we are done.
   if (sets.size() <= 1) {
      return changes;
   }
   // Look for a passing subset.
   ChangeSetType result;
   if (search(changes, sets, result)) {
      return result;
   }
   // Otherwise, partition the sets if possible; if not we are done.
   ChangeSetListType splitSets;
   for (ChangeSetListType::const_iterator iter = sets.begin(),
        ie = sets.end(); iter != ie; ++iter) {
      split(*iter, splitSets);
   }
   if (splitSets.size() == sets.size()) {
      return changes;
   }
   return delta(changes, splitSets);
}

bool DeltaAlgorithm::search(const ChangeSetType &changes,
                            const ChangeSetListType &sets,
                            ChangeSetType &result)
{
   // FIXME: Parallelize.
   for (ChangeSetListType::const_iterator iter = sets.begin(),
        ie = sets.end(); iter != ie; ++iter)
   {
      // If the test passes on this subset alone, recurse.
      if (getTestResult(*iter)) {
         ChangeSetListType sets;
         split(*iter, sets);
         result = delta(*iter, sets);
         return true;
      }

      // Otherwise, if we have more than two sets, see if test passes on the
      // complement.
      if (sets.size() > 2) {
         // FIXME: This is really slow.
         ChangeSetType complement;
         std::set_difference(
                  changes.begin(), changes.end(), iter->begin(), iter->end(),
                  std::insert_iterator<ChangeSetType>(complement, complement.begin()));
         if (getTestResult(complement)) {
            ChangeSetListType complementSets;
            complementSets.insert(complementSets.end(), sets.begin(), iter);
            complementSets.insert(complementSets.end(), iter + 1, sets.end());
            result = delta(complement, complementSets);
            return true;
         }
      }
   }
   return false;
}

DeltaAlgorithm::ChangeSetType DeltaAlgorithm::run(const ChangeSetType &changes)
{
   // Check empty set first to quickly find poor test functions.
   if (getTestResult(ChangeSetType())) {
      return ChangeSetType();
   }
   // Otherwise run the real delta algorithm.
   ChangeSetListType sets;
   split(changes, sets);
   return delta(changes, sets);
}

} // utils
} // polar
