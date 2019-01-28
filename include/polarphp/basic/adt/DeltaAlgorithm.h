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

/// DAGdeltaAlgorithm - Implements a "delta debugging" algorithm for minimizing
/// directed acyclic graphs using a predicate function.
///
/// The result of the algorithm is a subset of the input change set which is
/// guaranteed to satisfy the predicate, assuming that the input set did. For
/// well formed predicates, the result set is guaranteed to be such that
/// removing any single element not required by the dependencies on the other
/// elements would falsify the predicate.
///
/// The DAG should be used to represent dependencies in the changes which are
/// likely to hold across the predicate function. That is, for a particular
/// changeset S and predicate P:
///
///   P(S) => P(S union pred(S))
///
/// The minization algorithm uses this dependency information to attempt to
/// eagerly prune large subsets of changes. As with \see DeltaAlgorithm, the DAG
/// is not required to satisfy this property, but the algorithm will run
/// substantially fewer tests with appropriate dependencies. \see DeltaAlgorithm
/// for more information on the properties which the predicate function itself
/// should satisfy.
///

#ifndef POLARPHP_BASIC_ADT_DELTAALGORITHM_H
#define POLARPHP_BASIC_ADT_DELTAALGORITHM_H

#include <set>
#include <vector>

namespace polar {
namespace basic {

/// DeltaAlgorithm - Implements the delta debugging algorithm (A. Zeller '99)
/// for minimizing arbitrary sets using a predicate function.
///
/// The result of the algorithm is a subset of the input change set which is
/// guaranteed to satisfy the predicate, assuming that the input set did. For
/// well formed predicates, the result set is guaranteed to be such that
/// removing any single element would falsify the predicate.
///
/// For best results the predicate function *should* (but need not) satisfy
/// certain properties, in particular:
///  (1) The predicate should return false on an empty set and true on the full
///  set.
///  (2) If the predicate returns true for a set of changes, it should return
///  true for all supersets of that set.
///
/// It is not an error to provide a predicate that does not satisfy these
/// requirements, and the algorithm will generally produce reasonable
/// results. However, it may run substantially more tests than with a good
/// predicate.
class DeltaAlgorithm
{
public:
   using ChangeType = unsigned;
   // FIXME: Use a decent data structure.
   using ChangeSetType = std::set<ChangeType>;
   using ChangeSetListType = std::vector<ChangeSetType>;

private:
   /// Cache of failed test results. Successful test results are never cached
   /// since we always reduce following a success.
   std::set<ChangeSetType> m_failedTestsCache;

   /// getTestResult - Get the test result for the \p Changes from the
   /// cache, executing the test if necessary.
   ///
   /// \param Changes - The change set to test.
   /// \return - The test result.
   bool getTestResult(const ChangeSetType &changes);

   /// split - Partition a set of changes \p S into one or two subsets.
   void split(const ChangeSetType &set, ChangeSetListType &res);

   /// delta - Minimize a set of \p Changes which has been partioned into
   /// smaller sets, by attempting to remove individual subsets.
   ChangeSetType delta(const ChangeSetType &changes,
                       const ChangeSetListType &sets);

   /// search - search for a subset (or subsets) in \p Sets which can be
   /// removed from \p Changes while still satisfying the predicate.
   ///
   /// \param Res - On success, a subset of Changes which satisfies the
   /// predicate.
   /// \return - True on success.
   bool search(const ChangeSetType &changes, const ChangeSetListType &sets,
               ChangeSetType &res);

protected:
   /// updatedSearchState - Callback used when the search state changes.
   virtual void updatedSearchState(const ChangeSetType &changes,
                                   const ChangeSetListType &sets)
   {}

   /// executeOneTest - Execute a single test predicate on the change set \p S.
   virtual bool executeOneTest(const ChangeSetType &changeSet) = 0;

   DeltaAlgorithm& operator=(const DeltaAlgorithm&) = default;

public:
   virtual ~DeltaAlgorithm();

   /// run - Minimize the set \p Changes by executing \see executeOneTest() on
   /// subsets of changes and returning the smallest set which still satisfies
   /// the test predicate.
   ChangeSetType run(const ChangeSetType &changes);
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_DELTAALGORITHM_H
