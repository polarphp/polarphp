// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/03.
//===----------------------------------------------------------------------===//
//
// The algorithm we use attempts to exploit the dependency information by
// minimizing top-down. We start by constructing an initial root set R, and
// then iteratively:
//
//   1. Minimize the set R using the test predicate:
//       P'(set) = P(set union pred*(set))
//
//   2. Extend R to R' = R union pred(R).
//
// until a fixed point is reached.
//
// The idea is that we want to quickly prune entire portions of the graph, so we
// try to find high-level nodes that can be eliminated with all of their
// dependents.
//
// FIXME: The current algorithm doesn't actually provide a strong guarantee
// about the minimality of the result. The problem is that after adding nodes to
// the required set, we no longer consider them for elimination. For strictly
// well formed predicates, this doesn't happen, but iter commonly occurs in
// practice when there are unmodelled dependencies. I believe we can resolve
// this by allowing the required set to be minimized as well, but need more test
// cases first.
//
//===----------------------------------------------------------------------===//

#include "polarphp/basic/adt/DagDeltaAlgorithm.h"
#include "polarphp/basic/adt/DeltaAlgorithm.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/Format.h"
#include "polarphp/utils/RawOutStream.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <map>

namespace polar {
namespace basic {

#define DEBUG_TYPE "dag-delta"

using polar::debug_stream;

namespace {

class DAGDeltaAlgorithmImpl
{
   friend class DeltaActiveSetHelper;

public:
   typedef DAGDeltaAlgorithm::ChangeType ChangeType;
   typedef DAGDeltaAlgorithm::ChangeSetType ChangeSetType;
   typedef DAGDeltaAlgorithm::ChangeSetListType ChangeSetListType;
   typedef DAGDeltaAlgorithm::EdgeType EdgeType;

private:
   typedef std::vector<ChangeType>::iterator PredIteratorType;
   typedef std::vector<ChangeType>::iterator SuccIteratorType;
   typedef std::set<ChangeType>::iterator m_predClosureIteratorType;
   typedef std::set<ChangeType>::iterator m_succClosureIteratorType;

   DAGDeltaAlgorithm &m_dda;

   std::vector<ChangeType> m_roots;

   /// Cache of failed test results. Successful test results are never cached
   /// since we always reduce following a success. We maintain an independent
   /// cache from that used by the individual delta passes because we may get
   /// hits across multiple individual delta invocations.
   mutable std::set<ChangeSetType> m_failedTestsCache;

   // FIXME: Gross.
   std::map<ChangeType, std::vector<ChangeType>> m_predecessors;
   std::map<ChangeType, std::vector<ChangeType>> m_successors;

   std::map<ChangeType, std::set<ChangeType> > m_predClosure;
   std::map<ChangeType, std::set<ChangeType> > m_succClosure;

private:
   PredIteratorType predBegin(ChangeType node)
   {
      assert(m_predecessors.count(node) && "Invalid node!");
      return m_predecessors[node].begin();
   }

   PredIteratorType predEnd(ChangeType node)
   {
      assert(m_predecessors.count(node) && "Invalid node!");
      return m_predecessors[node].end();
   }

   m_predClosureIteratorType predClosureBegin(ChangeType node)
   {
      assert(m_predClosure.count(node) && "Invalid node!");
      return m_predClosure[node].begin();
   }

   m_predClosureIteratorType predClosureEnd(ChangeType node)
   {
      assert(m_predClosure.count(node) && "Invalid node!");
      return m_predClosure[node].end();
   }

   SuccIteratorType succBegin(ChangeType node)
   {
      assert(m_successors.count(node) && "Invalid node!");
      return m_successors[node].begin();
   }

   SuccIteratorType succEnd(ChangeType node)
   {
      assert(m_successors.count(node) && "Invalid node!");
      return m_successors[node].end();
   }

   m_succClosureIteratorType succClosureBegin(ChangeType node)
   {
      assert(m_succClosure.count(node) && "Invalid node!");
      return m_succClosure[node].begin();
   }

   m_succClosureIteratorType succClosureEnd(ChangeType node)
   {
      assert(m_succClosure.count(node) && "Invalid node!");
      return m_succClosure[node].end();
   }

   void updatedSearchState(const ChangeSetType &changes,
                           const ChangeSetListType &sets,
                           const ChangeSetType &required)
   {
      m_dda.updatedSearchState(changes, sets, required);
   }

   /// executeOneTest - Execute a single test predicate on the change set \p set.
   bool executeOneTest(const ChangeSetType &set)
   {
      // Check dependencies invariant.
      POLAR_DEBUG({
                     for (ChangeSetType::const_iterator iter = set.begin(),
                     ie = set.end(); iter != ie; ++iter) {
                        for (SuccIteratorType it2 = succBegin(*iter),
                        ie2 = succEnd(*iter); it2 != ie2; ++it2) {
                           assert(set.count(*it2) && "Attempt to run invalid changeset!");
                        }

                     }
                  });
      return m_dda.executeOneTest(set);
   }

public:
   DAGDeltaAlgorithmImpl(DAGDeltaAlgorithm &dda, const ChangeSetType &changes,
                         const std::vector<EdgeType> &dependencies);

   ChangeSetType run();

   /// getTestResult - Get the test result for the active set \p changes with
   /// \p required changes from the cache, executing the test if necessary.
   ///
   /// \param changes - The set of active changes being minimized, which should
   /// have their pred closure included in the test.
   /// \param required - The set of changes which have previously been
   /// established to be required.
   /// \return - The test result.
   bool getTestResult(const ChangeSetType &changes, const ChangeSetType &required);
};

/// helper object for minimizing an active set of changes.
class DeltaActiveSetHelper : public DeltaAlgorithm
{
   DAGDeltaAlgorithmImpl &m_ddai;

   const ChangeSetType &m_required;

protected:
   /// updatedSearchState - Callback used when the search state changes.
   void updatedSearchState(const ChangeSetType &changes,
                           const ChangeSetListType &sets) override
   {
      m_ddai.updatedSearchState(changes, sets, m_required);
   }

   bool executeOneTest(const ChangeSetType &set) override
   {
      return m_ddai.getTestResult(set, m_required);
   }

public:
   DeltaActiveSetHelper(DAGDeltaAlgorithmImpl &m_ddai,
                        const ChangeSetType &required)
      : m_ddai(m_ddai), m_required(required)
   {}
};

} // anonymous namespace

DAGDeltaAlgorithmImpl::DAGDeltaAlgorithmImpl(
      DAGDeltaAlgorithm &dda, const ChangeSetType &changes,
      const std::vector<EdgeType> &dependencies)
   : m_dda(dda)
{
   for (ChangeSetType::const_iterator iter = changes.begin(),
        ie = changes.end(); iter != ie; ++iter) {
      m_predecessors.insert(std::make_pair(*iter, std::vector<ChangeType>()));
      m_successors.insert(std::make_pair(*iter, std::vector<ChangeType>()));
   }
   for (std::vector<EdgeType>::const_iterator iter = dependencies.begin(),
        ie = dependencies.end(); iter != ie; ++iter) {
      m_predecessors[iter->second].push_back(iter->first);
      m_successors[iter->first].push_back(iter->second);
   }

   // Compute the roots.
   for (ChangeSetType::const_iterator iter = changes.begin(),
        ie = changes.end(); iter != ie; ++iter) {
      if (succBegin(*iter) == succEnd(*iter)) {
         m_roots.push_back(*iter);
      }
   }
   // Pre-compute the closure of the successor relation.
   std::vector<ChangeType> Worklist(m_roots.begin(), m_roots.end());
   while (!Worklist.empty()) {
      ChangeType Change = Worklist.back();
      Worklist.pop_back();

      std::set<ChangeType> &ChangeSuccs = m_succClosure[Change];
      for (PredIteratorType iter = predBegin(Change),
           ie = predEnd(Change); iter != ie; ++iter) {
         m_succClosure[*iter].insert(Change);
         m_succClosure[*iter].insert(ChangeSuccs.begin(), ChangeSuccs.end());
         Worklist.push_back(*iter);
      }
   }

   // Invert to form the predecessor closure map.
   for (ChangeSetType::const_iterator iter = changes.begin(),
        ie = changes.end(); iter != ie; ++iter) {
      m_predClosure.insert(std::make_pair(*iter, std::set<ChangeType>()));
   }

   for (ChangeSetType::const_iterator iter = changes.begin(),
        ie = changes.end(); iter != ie; ++iter) {
      for (m_succClosureIteratorType it2 = succClosureBegin(*iter),
           ie2 = succClosureEnd(*iter); it2 != ie2; ++it2)
         m_predClosure[*it2].insert(*iter);
   }
   // Dump useful debug info.
   POLAR_DEBUG(
   {
               debug_stream() << "-- DAGDeltaAlgorithmImpl --\n";
               debug_stream() << "changes: [";
               for (ChangeSetType::const_iterator iter = changes.begin(),
               ie = changes.end(); iter != ie; ++iter) {
                  if (iter != changes.begin()) {
                     debug_stream() << ", ";
                  }
                  debug_stream() << *iter;
                  if (succBegin(*iter) != succEnd(*iter)) {
                     debug_stream() << "(";
                     for (SuccIteratorType it2 = succBegin(*iter),
                     ie2 = succEnd(*iter); it2 != ie2; ++it2) {
                        if (it2 != succBegin(*iter)) {
                           debug_stream() << ", ";
                        }
                        debug_stream() << "->" << *it2;
                     }
                     debug_stream() << ")";
                  }
               }
               debug_stream() << "]\n";

               debug_stream() << "m_roots: [";
               for (std::vector<ChangeType>::const_iterator iter = m_roots.begin(),
               ie = m_roots.end(); iter != ie; ++iter) {
                  if (iter != m_roots.begin()) {
                     debug_stream() << ", ";
                  }
                  debug_stream() << *iter;
               }
               debug_stream() << "]\n";
               debug_stream() << "Predecessor Closure:\n";
               for (ChangeSetType::const_iterator iter = changes.begin(),
               ie = changes.end(); iter != ie; ++iter) {
                  debug_stream() << polar::utils::format("  %-4d: [", *iter);
                  for (m_predClosureIteratorType it2 = predClosureBegin(*iter),
                  ie2 = predClosureEnd(*iter); it2 != ie2; ++it2) {
                     if (it2 != predClosureBegin(*iter)) {
                        debug_stream() << ", ";
                     }
                     debug_stream() << *it2;
                  }
                  debug_stream() << "]\n";
               }

               debug_stream() << "Successor Closure:\n";
               for (ChangeSetType::const_iterator iter = changes.begin(),
               ie = changes.end(); iter != ie; ++iter) {
                  debug_stream() << polar::utils::format("  %-4d: [", *iter);
                  for (m_succClosureIteratorType it2 = succClosureBegin(*iter),
                  ie2 = succClosureEnd(*iter); it2 != ie2; ++it2) {
                     if (it2 != succClosureBegin(*iter)) {
                        debug_stream() << ", ";
                     }
                     debug_stream() << *it2;
                  }
                  debug_stream() << "]\n";
               }
               debug_stream() << "\n\n";
            }
            );
}

bool DAGDeltaAlgorithmImpl::getTestResult(const ChangeSetType &changes,
                                          const ChangeSetType &required)
{
   ChangeSetType extended(required);
   extended.insert(changes.begin(), changes.end());
   for (ChangeSetType::const_iterator iter = changes.begin(),
        ie = changes.end(); iter != ie; ++iter) {
      extended.insert(predClosureBegin(*iter), predClosureEnd(*iter));
   }
   if (m_failedTestsCache.count(extended)) {
      return false;
   }
   bool result = executeOneTest(extended);
   if (!result) {
      m_failedTestsCache.insert(extended);
   }
   return result;
}

DAGDeltaAlgorithm::ChangeSetType
DAGDeltaAlgorithmImpl::run()
{
   // The current set of changes we are minimizing, starting at the roots.
   ChangeSetType currentSet(m_roots.begin(), m_roots.end());

   // The set of m_required changes.
   ChangeSetType m_required;

   // Iterate until the active set of changes is empty. Convergence is guaranteed
   // assuming input was a DAG.
   //
   // Invariant:  currentSet intersect m_required == {}
   // Invariant:  m_required == (m_required union succ*(m_required))
   while (!currentSet.empty()) {
      POLAR_DEBUG({
                     debug_stream() << "DAG_DD - " << currentSet.size() << " active changes, "
                     << m_required.size() << " m_required changes\n";
                  });

      // Minimize the current set of changes.
      DeltaActiveSetHelper helper(*this, m_required);
      ChangeSetType currentMinSet = helper.run(currentSet);

      // Update the set of m_required changes. Since
      //   currentMinSet subset currentSet
      // and after the last iteration,
      //   succ(currentSet) subset m_required
      // then
      //   succ(currentMinSet) subset m_required
      // and our invariant on m_required is maintained.
      m_required.insert(currentMinSet.begin(), currentMinSet.end());

      // Replace the current set with the predecssors of the minimized set of
      // active changes.
      currentSet.clear();
      for (ChangeSetType::const_iterator iter = currentMinSet.begin(),
           ie = currentMinSet.end(); iter != ie; ++iter) {
         currentSet.insert(predBegin(*iter), predEnd(*iter));
      }
      // FIXME: We could enforce currentSet intersect m_required == {} here if we
      // wanted to protect against cyclic graphs.
   }

   return m_required;
}

void DAGDeltaAlgorithm::anchor()
{
}

DAGDeltaAlgorithm::ChangeSetType
DAGDeltaAlgorithm::run(const ChangeSetType &changes,
                       const std::vector<EdgeType> &dependencies)
{
   return DAGDeltaAlgorithmImpl(*this, changes, dependencies).run();
}

} // basic
} // polar
