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

#ifndef POLARPHP_BASIC_ADT_DAGDELTAALGORITHM_H
#define POLARPHP_BASIC_ADT_DAGDELTAALGORITHM_H

#include <set>
#include <utility>
#include <vector>

namespace polar {
namespace basic {

/// DAGDeltaAlgorithm - Implements a "delta debugging" algorithm for minimizing
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
class DAGDeltaAlgorithm
{
   virtual void anchor();

public:
   using ChangeType = unsigned;
   using EdgeType = std::pair<ChangeType, ChangeType>;

   // FIXME: Use a decent data structure.
   using ChangeSetType = std::set<ChangeType>;
   using ChangeSetListType = std::vector<ChangeSetType>;

public:
   virtual ~DAGDeltaAlgorithm() = default;

   /// run - Minimize the DAG formed by the \p Changes vertices and the
   /// \p Dependencies edges by executing \see executeOneTest() on subsets of
   /// changes and returning the smallest set which still satisfies the test
   /// predicate and the input \p Dependencies.
   ///
   /// \param Changes The list of changes.
   ///
   /// \param Dependencies The list of dependencies amongst changes. For each
   /// (x,y) in \p Dependencies, both x and y must be in \p Changes. The
   /// minimization algorithm guarantees that for each tested changed set S,
   /// \f$ x \in S \f$ implies \f$ y \in S \f$. It is an error to have cyclic
   /// dependencies.
   ChangeSetType run(const ChangeSetType &changes,
                     const std::vector<EdgeType> &dependencies);

   /// updatedSearchState - Callback used when the search state changes.
   virtual void updatedSearchState(const ChangeSetType &changes,
                                   const ChangeSetListType &sets,
                                   const ChangeSetType &required)
   {}

   /// executeOneTest - Execute a single test predicate on the change set \p S.
   virtual bool executeOneTest(const ChangeSetType &set) = 0;
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_DAGDELTAALGORITHM_H
