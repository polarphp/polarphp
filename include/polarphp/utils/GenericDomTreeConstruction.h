// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

//===----------------------------------------------------------------------===//
/// \file
///
/// Generic dominator tree construction - This file provides routines to
/// construct immediate dominator information for a flow-graph based on the
/// m_semi-NCA algorithm described in this dissertation:
///
///   Linear-Time Algorithms for Dominators and Related Problems
///   Loukas Georgiadis, Princeton University, November 2005, pp. 21-23:
///   ftp://ftp.cs.princeton.edu/reports/2005/737.pdf
///
/// This implements the O(n*log(n)) versions of EVAL and LINK, because it turns
/// out that the theoretically slower O(n*log(n)) implementation is actually
/// faster than the almost-linear O(n*alpha(n)) version, even for large CFGs.
///
/// The file uses the Depth Based Search algorithm to perform incremental
/// updates (insertion and deletions). The implemented algorithm is based on
/// this publication:
///
///   An Experimental Study of Dynamic Dominators
///   Loukas Georgiadis, et al., April 12 2016, pp. 5-7, 9-10:
///   https://arxiv.org/pdf/1604.02711.pdf
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_GENERIC_DOMTREE_CONSTRUCTION_H
#define POLARPHP_UTILS_GENERIC_DOMTREE_CONSTRUCTION_H

#include <queue>
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/DenseSet.h"
#include "polarphp/basic/adt/DepthFirstIterator.h"
#include "polarphp/basic/adt/PointerIntPair.h"
#include "polarphp/basic/adt/SmallPtrSet.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/GenericDomTree.h"

#define DEBUG_TYPE "dom-tree-builder"

namespace polar {
namespace utils {

using polar::basic::SmallDenseSet;
using polar::basic::SmallDenseMap;
using polar::basic::reverse;
using polar::basic::children;
using polar::basic::inverse_children;
using polar::utils::debug_stream;
using polar::basic::find;

namespace domtreebuilder {

template <typename DomTreeT>
struct SemiNCAInfo
{
   using NodePtr = typename DomTreeT::NodePtr;
   using NodeT = typename DomTreeT::NodeType;
   using TreeNodePtr = DomTreeNodeBase<NodeT> *;
   using RootsT = decltype(DomTreeT::roots);
   static constexpr bool sm_isPostDom = DomTreeT::IsPostDominator;

   // Information record used by m_semi-NCA during tree construction.
   struct InfoRec
   {
      unsigned m_dfsNum = 0;
      unsigned m_parent = 0;
      unsigned m_semi = 0;
      NodePtr m_label = nullptr;
      NodePtr m_idom = nullptr;
      SmallVector<NodePtr, 2> m_reverseChildren;
   };

   // Number to node mapping is 1-based. Initialize the mapping to start with
   // a dummy element.
   std::vector<NodePtr> m_numToNode = {nullptr};
   DenseMap<NodePtr, InfoRec> m_nodeToInfo;

   using UpdateT = typename DomTreeT::UpdateType;
   using UpdateKind = typename DomTreeT::UpdateKind;
   struct BatchUpdateInfo
   {
      SmallVector<UpdateT, 4> m_updates;
      using NodePtrAndKind = PointerIntPair<NodePtr, 1, UpdateKind>;

      // In order to be able to walk a CFG that is out of sync with the CFG
      // DominatorTree last knew about, use the list of updates to reconstruct
      // previous CFG versions of the current CFG. For each node, we store a set
      // of its virtually added/deleted future successors and predecessors.
      // Note that these children are from the future relative to what the
      // DominatorTree knows about -- using them to gets us some snapshot of the
      // CFG from the past (relative to the state of the CFG).
      DenseMap<NodePtr, SmallDenseSet<NodePtrAndKind, 4>> m_futureSuccessors;
      DenseMap<NodePtr, SmallDenseSet<NodePtrAndKind, 4>> m_futurepredecessors;
      // Remembers if the whole tree was recalculated at some point during the
      // current batch update.
      bool m_isRecalculated = false;
   };

   BatchUpdateInfo *m_batchUpdates;
   using BatchUpdatePtr = BatchUpdateInfo *;

   // If batchUpdatePtr is a nullptr, then there's no batch update in progress.
   SemiNCAInfo(BatchUpdatePtr batchUpdatePtr)
      : m_batchUpdates(batchUpdatePtr)
   {}

   void clear()
   {
      m_numToNode = {nullptr}; // Restore to initial state with a dummy start node.
      m_nodeToInfo.clear();
      // Don't reset the pointer to BatchUpdateInfo here -- if there's an update
      // in progress, we need this information to continue it.
   }

   template <bool Inverse>
   struct ChildrenGetter
   {
      using ResultTy = SmallVector<NodePtr, 8>;

      static ResultTy get(NodePtr node, std::integral_constant<bool, false>)
      {
         auto rchildren = reverse(children<NodePtr>(node));
         return ResultTy(rchildren.begin(), rchildren.end());
      }

      static ResultTy get(NodePtr node, std::integral_constant<bool, true>)
      {
         auto ichildren = inverse_children<NodePtr>(node);
         return ResultTy(ichildren.begin(), ichildren.end());
      }

      using Tag = std::integral_constant<bool, Inverse>;

      // The function below is the core part of the batch updater. It allows the
      // Depth Based Search algorithm to perform incremental updates in lockstep
      // with updates to the CFG. We emulated lockstep CFG updates by getting its
      // next snapshots by reverse-applying future updates.
      static ResultTy get(NodePtr node, BatchUpdatePtr batchUpdatePtr) {
         ResultTy res = get(node, Tag());
         // If there's no batch update in progress, simply return node's children.
         if (!batchUpdatePtr) {
            return res;
         }
         // CFG children are actually its *most current* children, and we have to
         // reverse-apply the future updates to get the node's children at the
         // point in time the update was performed.
         auto &futureChildren = (Inverse != sm_isPostDom) ? batchUpdatePtr->m_futurepredecessors
                                                          : batchUpdatePtr->m_futureSuccessors;
         auto fcIter = futureChildren.find(node);
         if (fcIter == futureChildren.end()) {
            return res;
         }
         for (auto childAndKind : fcIter->second) {
            const NodePtr child = childAndKind.getPointer();
            const UpdateKind updateKind = childAndKind.getInt();

            // Reverse-apply the future update.
            if (updateKind == UpdateKind::Insert) {
               // If there's an insertion in the future, it means that the edge must
               // exist in the current CFG, but was not present in it before.
               assert(find(res, child) != res.end()
                     && "Expected child not found in the CFG");
               res.erase(std::remove(res.begin(), res.end(), child), res.end());
               POLAR_DEBUG(debug_stream() << "\tHiding edge " << BlockNamePrinter(node) << " -> "
                           << BlockNamePrinter(child) << "\n");
            } else {
               // If there's an deletion in the future, it means that the edge cannot
               // exist in the current CFG, but existed in it before.
               assert(find(res, child) == res.end() &&
                      "Unexpected child found in the CFG");
               POLAR_DEBUG(debug_stream() << "\tShowing virtual edge " << BlockNamePrinter(node)
                           << " -> " << BlockNamePrinter(child) << "\n");
               res.push_back(child);
            }
         }

         return res;
      }
   };

   NodePtr getIDom(NodePtr node) const
   {
      auto infoIter = m_nodeToInfo.find(node);
      if (infoIter == m_nodeToInfo.end()) {
         return nullptr;
      }
      return infoIter->second.m_idom;
   }

   TreeNodePtr getNodeForBlock(NodePtr node, DomTreeT &domTree)
   {
      if (TreeNodePtr treeNode = domTree.getNode(node)) {
         return treeNode;
      }
      // Haven't calculated this node yet?  get or calculate the node for the
      // immediate dominator.
      NodePtr idom = getIDom(node);
      assert(idom || domTree.m_domTreeNodes[nullptr]);
      TreeNodePtr idomNode = getNodeForBlock(idom, domTree);
      // Add a new tree node for this NodeT, and link it as a child of
      // idomNode
      return (domTree.m_domTreeNodes[node] = idomNode->addChild(
               std::make_unique<DomTreeNodeBase<NodeT>>(node, idomNode)))
            .get();
   }

   static bool alwaysDescend(NodePtr, NodePtr)
   {
      return true;
   }

   struct BlockNamePrinter
   {
      NodePtr m_node;

      BlockNamePrinter(NodePtr blockNode) : m_node(blockNode)
      {}

      BlockNamePrinter(TreeNodePtr treeNode) : m_node(treeNode ? treeNode->getBlock() : nullptr)
      {}

      friend RawOutStream &operator<<(RawOutStream &outstream, const BlockNamePrinter &printer)
      {
         if (!printer.m_node) {
            outstream << "nullptr";
         } else {
            printer.m_node->printAsOperand(outstream, false);
         }
         return outstream;
      }
   };

   // Custom DFS implementation which can skip nodes based on a provided
   // predicate. It also collects m_reverseChildren so that we don't have to spend
   // time getting predecessors in SemiNCA.
   //
   // If IsReverse is set to true, the DFS walk will be performed backwards
   // relative to sm_isPostDom -- using reverse edges for dominators and forward
   // edges for postdominators.
   template <bool IsReverse = false, typename DescendCondition>
   unsigned runDFS(NodePtr node, unsigned lastNum, DescendCondition condition,
                   unsigned attachToNum)
   {
      assert(node);
      SmallVector<NodePtr, 64> workList = {node};
      if (m_nodeToInfo.count(node) != 0) {
         m_nodeToInfo[node].m_parent = attachToNum;
      }
      while (!workList.empty()) {
         const NodePtr bb = workList.popBackValue();
         auto &bbInfo = m_nodeToInfo[bb];

         // m_visited nodes always have positive DFS numbers.
         if (bbInfo.m_dfsNum != 0) continue;
         bbInfo.m_dfsNum = bbInfo.m_semi = ++lastNum;
         bbInfo.m_label = bb;
         m_numToNode.push_back(bb);
         constexpr bool direction = IsReverse != sm_isPostDom;  // XOR.
         for (const NodePtr succ :
              ChildrenGetter<direction>::get(bb, m_batchUpdates)) {
            const auto siter = m_nodeToInfo.find(succ);
            // Don't visit nodes more than once but remember to collect
            // m_reverseChildren.
            if (siter != m_nodeToInfo.end() && siter->second.m_dfsNum != 0) {
               if (succ != bb) siter->second.m_reverseChildren.push_back(bb);
               continue;
            }

            if (!condition(bb, succ)) {
               continue;
            }
            // It's fine to add succ to the map, because we know that it will be
            // m_visited later.
            auto &succInfo = m_nodeToInfo[succ];
            workList.push_back(succ);
            succInfo.m_parent = lastNum;
            succInfo.m_reverseChildren.push_back(bb);
         }
      }

      return lastNum;
   }

   NodePtr eval(NodePtr vin, unsigned lastLinked)
   {
      auto &vinInfo = m_nodeToInfo[vin];
      if (vinInfo.m_dfsNum < lastLinked) {
         return vin;
      }

      SmallVector<NodePtr, 32> work;
      SmallPtrSet<NodePtr, 32> m_visited;

      if (vinInfo.m_parent >= lastLinked) {
         work.push_back(vin);
      }
      while (!work.empty()) {
         NodePtr backNode = work.back();
         auto &vInfo = m_nodeToInfo[backNode];
         NodePtr vAncestor = m_numToNode[vInfo.m_parent];

         // Process Ancestor first
         if (m_visited.insert(vAncestor).second && vInfo.m_parent >= lastLinked) {
            work.push_back(vAncestor);
            continue;
         }
         work.pop_back();

         // update vInfo based on Ancestor info
         if (vInfo.m_parent < lastLinked) {
            continue;
         }
         auto &vaInfo = m_nodeToInfo[vAncestor];
         NodePtr vAncestorLabel = vaInfo.m_label;
         NodePtr vLabel = vInfo.m_label;
         if (m_nodeToInfo[vAncestorLabel].m_semi < m_nodeToInfo[vLabel].m_semi) {
            vInfo.m_label = vAncestorLabel;
         }
         vInfo.m_parent = vaInfo.m_parent;
      }
      return vinInfo.m_label;
   }

   // This function requires DFS to be run before calling it.
   void runSemiNCA(DomTreeT &domTree, const unsigned minLevel = 0)
   {
      const unsigned nextDFSNum(m_numToNode.size());
      // Initialize IDoms to spanning tree parents.
      for (unsigned i = 1; i < nextDFSNum; ++i) {
         const NodePtr node = m_numToNode[i];
         auto &vInfo = m_nodeToInfo[node];
         vInfo.m_idom = m_numToNode[vInfo.m_parent];
      }

      // Step #1: calculate the semidominators of all vertices.
      for (unsigned i = nextDFSNum - 1; i >= 2; --i) {
         NodePtr node = m_numToNode[i];
         auto &wInfo = m_nodeToInfo[node];
         // Initialize the semi dominator to point to the parent node.
         wInfo.m_semi = wInfo.m_parent;
         for (const auto &node : wInfo.m_reverseChildren) {
            if (m_nodeToInfo.count(node) == 0)  {// Skip unreachable predecessors.
               continue;
            }
            const TreeNodePtr treeNode = domTree.getNode(node);
            // Skip predecessors whose level is above the subtree we are processing.
            if (treeNode && treeNode->getLevel() < minLevel) {
               continue;
            }

            unsigned semiU = m_nodeToInfo[eval(node, i + 1)].m_semi;
            if (semiU < wInfo.m_semi) wInfo.m_semi = semiU;
         }
      }

      // Step #2: Explicitly define the immediate dominator of each vertex.
      //          m_idom[i] = NCA(SDom[i], SpanningTreeParent(i)).
      // Note that the parents were stored in IDoms and later got invalidated
      // during path compression in Eval.
      for (unsigned i = 2; i < nextDFSNum; ++i) {
         const NodePtr node = m_numToNode[i];
         auto &wInfo = m_nodeToInfo[node];
         const unsigned sdomNum = m_nodeToInfo[m_numToNode[wInfo.m_semi]].m_dfsNum;
         NodePtr wiDomCandidate = wInfo.m_idom;
         while (m_nodeToInfo[wiDomCandidate].m_dfsNum > sdomNum) {
            wiDomCandidate = m_nodeToInfo[wiDomCandidate].m_idom;
         }
         wInfo.m_idom = wiDomCandidate;
      }
   }

   // PostDominatorTree always has a virtual root that represents a virtual CFG
   // node that serves as a single exit from the function. All the other exits
   // (CFG nodes with terminators and nodes in infinite loops are logically
   // connected to this virtual CFG exit node).
   // This functions maps a nullptr CFG node to the virtual root tree node.
   void addvirtualRoot()
   {
      assert(sm_isPostDom && "Only postdominators have a virtual root");
      assert(m_numToNode.size() == 1 && "SNCAInfo must be freshly constructed");

      auto &bbInfo = m_nodeToInfo[nullptr];
      bbInfo.m_dfsNum = bbInfo.m_semi = 1;
      bbInfo.m_label = nullptr;

      m_numToNode.push_back(nullptr);  // m_numToNode[1] = nullptr;
   }

   // For postdominators, nodes with no forward successors are trivial roots that
   // are always selected as tree roots. roots with forward successors correspond
   // to CFG nodes within infinite loops.
   static bool hasForwardSuccessors(const NodePtr node, BatchUpdatePtr batchUpdatePtr)
   {
      assert(node && "node must be a valid node");
      return !ChildrenGetter<false>::get(node, batchUpdatePtr).empty();
   }

   static NodePtr getEntryNode(const DomTreeT &domTree)
   {
      assert(domTree.m_parent && "m_parent not set");
      return GraphTraits<typename DomTreeT::ParentPtr>::getEntryNode(domTree.m_parent);
   }

   // Finds all roots without relaying on the set of roots already stored in the
   // tree.
   // We define roots to be some non-redundant set of the CFG nodes
   static RootsT findRoots(const DomTreeT &domTree, BatchUpdatePtr batchUpdatePtr)
   {
      assert(domTree.m_parent && "m_parent pointer is not set");
      RootsT roots;
      // For dominators, function entry CFG node is always a tree root node.
      if (!sm_isPostDom) {
         roots.push_back(getEntryNode(domTree));
         return roots;
      }

      SemiNCAInfo SNCA(batchUpdatePtr);

      // PostDominatorTree always has a virtual root.
      SNCA.addvirtualRoot();
      unsigned num = 1;

      POLAR_DEBUG(debug_stream() << "\t\tLooking for trivial roots\n");

      // Step #1: Find all the trivial roots that are going to will definitely
      // remain tree roots.
      unsigned total = 0;
      // It may happen that there are some new nodes in the CFG that are result of
      // the ongoing batch update, but we cannot really pretend that they don't
      // exist -- we won't see any outgoing or incoming edges to them, so it's
      // fine to discover them here, as they would end up appearing in the CFG at
      // some point anyway.
      for (const NodePtr node : polar::basic::nodes(domTree.m_parent)) {
         ++total;
         // If it has no *successors*, it is definitely a root.
         if (!hasForwardSuccessors(node, batchUpdatePtr)) {
            roots.push_back(node);
            // Run DFS not to walk this part of CFG later.
            num = SNCA.runDFS(node, num, alwaysDescend, 1);
            POLAR_DEBUG(debug_stream() << "Found a new trivial root: " << BlockNamePrinter(node)
                        << "\n");
            POLAR_DEBUG(debug_stream() << "Last m_visited node: "
                        << BlockNamePrinter(SNCA.m_numToNode[num]) << "\n");
         }
      }
      POLAR_DEBUG(debug_stream() << "\t\tLooking for non-trivial roots\n");

      // Step #2: Find all non-trivial root candidates. Those are CFG nodes that
      // are reverse-unreachable were not m_visited by previous DFS walks (i.e. CFG
      // nodes in infinite loops).
      bool hasNonTrivialRoots = false;
      // Accounting for the virtual exit, see if we had any reverse-unreachable
      // nodes.
      if (total + 1 != num) {
         hasNonTrivialRoots = true;
         // Make another DFS pass over all other nodes to find the
         // reverse-unreachable blocks, and find the furthest paths we'll be able
         // to make.
         // Note that this looks node^2, but it's really 2N worst case, if every node
         // is unreachable. This is because we are still going to only visit each
         // unreachable node once, we may just visit it in two directions,
         // depending on how lucky we get.
         SmallPtrSet<NodePtr, 4> connectToExitBlock;
         for (const NodePtr node : nodes(domTree.m_parent)) {
            if (SNCA.m_nodeToInfo.count(node) == 0) {
               POLAR_DEBUG(debug_stream() << "\t\t\tVisiting node " << BlockNamePrinter(node)
                           << "\n");
               // Find the furthest away we can get by following successors, then
               // follow them in reverse.  This gives us some reasonable answer about
               // the post-dom tree inside any infinite loop. In particular, it
               // guarantees we get to the farthest away point along *some*
               // path. This also matches the GCC's behavior.
               // If we really wanted a totally complete picture of dominance inside
               // this infinite loop, we could do it with SCC-like algorithms to find
               // the lowest and highest points in the infinite loop.  In theory, it
               // would be nice to give the canonical backedge for the loop, but it's
               // expensive and does not always lead to a minimal set of roots.
               POLAR_DEBUG(debug_stream() << "\t\t\tRunning forward DFS\n");

               const unsigned newNum = SNCA.runDFS<true>(node, num, alwaysDescend, num);
               const NodePtr FurthestAway = SNCA.m_numToNode[newNum];
               POLAR_DEBUG(debug_stream() << "\t\t\tFound a new furthest away node "
                           << "(non-trivial root): "
                           << BlockNamePrinter(FurthestAway) << "\n");
               connectToExitBlock.insert(FurthestAway);
               roots.push_back(FurthestAway);
               POLAR_DEBUG(debug_stream() << "\t\t\tPrev m_dfsNum: " << num << ", new m_dfsNum: "
                           << newNum << "\n\t\t\tRemoving DFS info\n");
               for (unsigned i = newNum; i > num; --i) {
                  const NodePtr node = SNCA.m_numToNode[i];
                  POLAR_DEBUG(debug_stream() << "\t\t\t\tRemoving DFS info for "
                              << BlockNamePrinter(node) << "\n");
                  SNCA.m_nodeToInfo.erase(node);
                  SNCA.m_numToNode.pop_back();
               }
               const unsigned prevNum = num;
               POLAR_DEBUG(debug_stream() << "\t\t\tRunning reverse DFS\n");
               num = SNCA.runDFS(FurthestAway, num, alwaysDescend, 1);
               for (unsigned i = prevNum + 1; i <= num; ++i)
                  POLAR_DEBUG(debug_stream() << "\t\t\t\tfound node "
                              << BlockNamePrinter(SNCA.m_numToNode[i]) << "\n");
            }
         }
      }

      POLAR_DEBUG(debug_stream() << "total: " << total << ", num: " << num << "\n");
      POLAR_DEBUG(debug_stream() << "Discovered CFG nodes:\n");
      POLAR_DEBUG(for (size_t i = 0; i <= num; ++i) debug_stream()
            << i << ": " << BlockNamePrinter(SNCA.m_numToNode[i]) << "\n");
      assert((total + 1 == num) && "Everything should have been m_visited");
      // Step #3: If we found some non-trivial roots, make them non-redundant.
      if (hasNonTrivialRoots) {
         removeRedundantRoots(domTree, batchUpdatePtr, roots);
      }
      POLAR_DEBUG(debug_stream() << "Found roots: ");
      POLAR_DEBUG(for (auto *root : roots) debug_stream() << BlockNamePrinter(root) << " ");
      POLAR_DEBUG(debug_stream() << "\n");
      return roots;
   }

   // This function only makes sense for postdominators.
   // We define roots to be some set of CFG nodes where (reverse) DFS walks have
   // to start in order to visit all the CFG nodes (including the
   // reverse-unreachable ones).
   // When the search for non-trivial roots is done it may happen that some of
   // the non-trivial roots are reverse-reachable from other non-trivial roots,
   // which makes them redundant. This function removes them from the set of
   // input roots.
   static void removeRedundantRoots(const DomTreeT &domTree, BatchUpdatePtr batchUpdatePtr,
                                    RootsT &roots)
   {
      assert(sm_isPostDom && "This function is for postdominators only");
      POLAR_DEBUG(debug_stream() << "Removing redundant roots\n");
      SemiNCAInfo SNCA(batchUpdatePtr);
      for (unsigned i = 0; i < roots.size(); ++i) {
         auto &root = roots[i];
         // Trivial roots are always non-redundant.
         if (!hasForwardSuccessors(root, batchUpdatePtr)) continue;
         POLAR_DEBUG(debug_stream() << "\tChecking if " << BlockNamePrinter(root)
                     << " remains a root\n");
         SNCA.clear();
         // Do a forward walk looking for the other roots.
         const unsigned num = SNCA.runDFS<true>(root, 0, alwaysDescend, 0);
         // Skip the start node and begin from the second one (note that DFS uses
         // 1-based indexing).
         for (unsigned x = 2; x <= num; ++x) {
            const NodePtr node = SNCA.m_numToNode[x];
            // If we wound another root in a (forward) DFS walk, remove the current
            // root from the set of roots, as it is reverse-reachable from the other
            // one.
            if (llvm::find(roots, node) != roots.end()) {
               POLAR_DEBUG(debug_stream() << "\tForward DFS walk found another root "
                           << BlockNamePrinter(node) << "\n\tRemoving root "
                           << BlockNamePrinter(root) << "\n");
               std::swap(root, roots.back());
               roots.pop_back();

               // root at the back takes the current root's place.
               // Start the next loop iteration with the same index.
               --i;
               break;
            }
         }
      }
   }

   template <typename DescendCondition>
   void doFullDFSWalk(const DomTreeT &domTree, DescendCondition dc)
   {
      if (!sm_isPostDom) {
         assert(domTree.m_roots.getSize() == 1 && "Dominators should have a singe root");
         runDFS(domTree.m_roots[0], 0, dc, 0);
         return;
      }

      addvirtualRoot();
      unsigned num = 1;
      for (const NodePtr root : domTree.m_roots) num = runDFS(root, num, dc, 0);
   }

   static void calculateFromScratch(DomTreeT &domTree, BatchUpdatePtr batchUpdatePtr) {
      auto *m_parent = domTree.m_parent;
      domTree.reset();
      domTree.m_parent = m_parent;
      SemiNCAInfo SNCA(nullptr);  // Since we are rebuilding the whole tree,
      // there's no point doing it incrementally.

      // Step #0: Number blocks in depth-first order and initialize variables used
      // in later stages of the algorithm.
      domTree.m_roots = findRoots(domTree, nullptr);
      SNCA.doFullDFSWalk(domTree, alwaysDescend);

      SNCA.runSemiNCA(domTree);
      if (batchUpdatePtr) {
         batchUpdatePtr->m_isRecalculated = true;
         POLAR_DEBUG(debug_stream() << "DomTree recalculated, skipping future batch updates\n");
      }

      if (domTree.m_roots.empty()) return;

      // Add a node for the root. If the tree is a PostDominatorTree it will be
      // the virtual exit (denoted by (BasicBlock *) nullptr) which postdominates
      // all real exits (including multiple exit blocks, infinite loops).
      NodePtr root = sm_isPostDom ? nullptr : domTree.m_roots[0];

      domTree.RootNode = (domTree.m_domTreeNodes[root] =
            llvm::make_unique<DomTreeNodeBase<NodeT>>(root, nullptr))
            .get();
      SNCA.attachNewSubtree(domTree, domTree.RootNode);
   }

   void attachNewSubtree(DomTreeT& domTree, const TreeNodePtr attachTo)
   {
      // Attach the first unreachable block to attachTo.
      m_nodeToInfo[m_numToNode[1]].m_idom = attachTo->getBlock();
      // Loop over all of the discovered blocks in the function...
      for (size_t i = 1, e = m_numToNode.size(); i != e; ++i) {
         NodePtr wnode = m_numToNode[i];
         POLAR_DEBUG(debug_stream() << "\tdiscovered a new reachable node "
                     << BlockNamePrinter(wnode) << "\n");

         // Don't replace this with 'count', the insertion side effect is important
         if (domTree.m_domTreeNodes[wnode]) continue;  // Haven't calculated this node yet?

         NodePtr ImmDom = getIDom(wnode);

         // get or calculate the node for the immediate dominator.
         TreeNodePtr idomNode = getNodeForBlock(ImmDom, domTree);

         // Add a new tree node for this BasicBlock, and link it as a child of
         // idomNode.
         domTree.m_domTreeNodes[wnode] = idomNode->addChild(
                  llvm::make_unique<DomTreeNodeBase<NodeT>>(wnode, idomNode));
      }
   }

   void reattachExistingSubtree(DomTreeT &domTree, const TreeNodePtr attachTo)
   {
      m_nodeToInfo[m_numToNode[1]].m_idom = attachTo->getBlock();
      for (size_t i = 1, e = m_numToNode.size(); i != e; ++i) {
         const NodePtr node = m_numToNode[i];
         const TreeNodePtr treeNode = domTree.getNode(node);
         assert(treeNode);
         const TreeNodePtr NewIDom = domTree.getNode(m_nodeToInfo[node].m_idom);
         treeNode->setIDom(NewIDom);
      }
   }

   // Helper struct used during edge insertions.
   struct InsertionInfo
   {
      using BucketElementTy = std::pair<unsigned, TreeNodePtr>;
      struct DecreasingLevel {
         bool operator()(const BucketElementTy &first,
                         const BucketElementTy &second) const
         {
            return first.first > second.first;
         }
      };

      std::priority_queue<BucketElementTy, SmallVector<BucketElementTy, 8>,
      DecreasingLevel>
      m_bucket;  // Queue of tree nodes sorted by level in descending order.
      SmallDenseSet<TreeNodePtr, 8> m_affected;
      SmallDenseMap<TreeNodePtr, unsigned, 8> m_visited;
      SmallVector<TreeNodePtr, 8> m_affectedQueue;
      SmallVector<TreeNodePtr, 8> m_visitedNotAffectedQueue;
   };

   static void insert_edge(DomTreeT &domTree, const BatchUpdatePtr batchUpdatePtr,
                           const NodePtr from, const NodePtr to) {
      assert((from || sm_isPostDom) &&
             "from has to be a valid CFG node or a virtual root");
      assert(to && "Cannot be a nullptr");
      POLAR_DEBUG(debug_stream() << "Inserting edge " << BlockNamePrinter(from) << " -> "
                  << BlockNamePrinter(to) << "\n");
      TreeNodePtr fromTreeNode = domTree.getNode(from);

      if (!fromTreeNode) {
         // Ignore edges from unreachable nodes for (forward) dominators.
         if (!sm_isPostDom) {
            return;
         }
         // The unreachable node becomes a new root -- a tree node for it.
         TreeNodePtr virtualRoot = domTree.getNode(nullptr);
         fromTreeNode =
               (domTree.m_domTreeNodes[from] = virtualRoot->addChild(
                  std::make_unique<DomTreeNodeBase<NodeT>>(from, virtualRoot)))
               .get();
         domTree.m_roots.push_back(from);
      }

      domTree.m_dfsInfoValid = false;

      const TreeNodePtr toTreeNode = domTree.getNode(to);
      if (!toTreeNode) {
         insertUnreachable(domTree, batchUpdatePtr, fromTreeNode, to);
      } else {
         insertReachable(domTree, batchUpdatePtr, fromTreeNode, toTreeNode);
      }
   }

   // Determines if some existing root becomes reverse-reachable after the
   // insertion. Rebuilds the whole tree if that situation happens.
   static bool updateRootsBeforeInsertion(DomTreeT &domTree, const BatchUpdatePtr batchUpdatePtr,
                                          const TreeNodePtr from,
                                          const TreeNodePtr to)
   {
      assert(sm_isPostDom && "This function is only for postdominators");
      // Destination node is not attached to the virtual root, so it cannot be a
      // root.
      if (!domTree.isvirtualRoot(to->getIDom())) return false;

      auto riter = find(domTree.m_roots, to->getBlock());
      if (riter == domTree.m_roots.end())
         return false;  // to is not a root, nothing to update.

      POLAR_DEBUG(debug_stream() << "\t\tAfter the insertion, " << BlockNamePrinter(to)
                  << " is no longer a root\n\t\tRebuilding the tree!!!\n");

      calculateFromScratch(domTree, batchUpdatePtr);
      return true;
   }

   // m_updates the set of roots after insertion or deletion. This ensures that
   // roots are the same when after a series of updates and when the tree would
   // be built from scratch.
   static void updateRootsAfterUpdate(DomTreeT &domTree, const BatchUpdatePtr batchUpdatePtr)
   {
      assert(sm_isPostDom && "This function is only for postdominators");

      // The tree has only trivial roots -- nothing to update.
      if (std::none_of(domTree.m_roots.begin(), domTree.m_roots.end(), [batchUpdatePtr](const NodePtr node) {
                       return hasForwardSuccessors(node, batchUpdatePtr);
   }))
         return;

      // Recalculate the set of roots.
      auto roots = findRoots(domTree, batchUpdatePtr);
      if (domTree.m_roots.size() != roots.size() ||
          !std::is_permutation(domTree.m_roots.begin(), domTree.m_roots.end(), roots.begin())) {
         // The roots chosen in the CFG have changed. This is because the
         // incremental algorithm does not really know or use the set of roots and
         // can make a different (implicit) decision about which node within an
         // infinite loop becomes a root.

         POLAR_DEBUG(debug_stream() << "roots are different in updated trees\n"
                     << "The entire tree needs to be rebuilt\n");
         // It may be possible to update the tree without recalculating it, but
         // we do not know yet how to do it, and it happens rarely in practise.
         calculateFromScratch(domTree, batchUpdatePtr);
         return;
      }
   }

   // Handles insertion to a node already in the dominator tree.
   static void insertReachable(DomTreeT &domTree, const BatchUpdatePtr batchUpdatePtr,
                               const TreeNodePtr from, const TreeNodePtr to) {
      POLAR_DEBUG(debug_stream() << "\tReachable " << BlockNamePrinter(from->getBlock())
                  << " -> " << BlockNamePrinter(to->getBlock()) << "\n");
      if (sm_isPostDom && updateRootsBeforeInsertion(domTree, batchUpdatePtr, from, to)) return;
      // domTree.findncd expects both pointers to be valid. When from is a virtual
      // root, then its CFG block pointer is a nullptr, so we have to 'compute'
      // the ncd manually.
      const NodePtr ncdBlock =
            (from->getBlock() && to->getBlock())
            ? domTree.findNearestCommonDominator(from->getBlock(), to->getBlock())
            : nullptr;
      assert(ncdBlock || domTree.isPostDominator());
      const TreeNodePtr ncd = domTree.getNode(ncdBlock);
      assert(ncd);

      POLAR_DEBUG(debug_stream() << "\t\tNCA == " << BlockNamePrinter(ncd) << "\n");
      const TreeNodePtr toIDom = to->getIDom();

      // Nothing affected -- NCA property holds.
      // (Based on the lemma 2.5 from the second paper.)
      if (ncd == to || ncd == toIDom) return;

      // Identify and collect affected nodes.
      InsertionInfo insertInfo;
      POLAR_DEBUG(debug_stream() << "Marking " << BlockNamePrinter(to) << " as affected\n");
      insertInfo.m_affected.insert(to);
      const unsigned ToLevel = to->getLevel();
      POLAR_DEBUG(debug_stream() << "Putting " << BlockNamePrinter(to) << " into a m_bucket\n");
      insertInfo.m_bucket.push({ToLevel, to});

      while (!insertInfo.m_bucket.empty()) {
         const TreeNodePtr currentNode = insertInfo.m_bucket.top().second;
         const unsigned  currentLevel = currentNode->getLevel();
         insertInfo.m_bucket.pop();
         POLAR_DEBUG(debug_stream() << "\tAdding to m_visited and m_affectedQueue: "
                     << BlockNamePrinter(currentNode) << "\n");

         insertInfo.m_visited.insert({currentNode, currentLevel});
         insertInfo.m_affectedQueue.push_back(currentNode);

         // Discover and collect affected successors of the current node.
         visitInsertion(domTree, batchUpdatePtr, currentNode, currentLevel, ncd, insertInfo);
      }

      // Finish by updating immediate dominators and levels.
      updateInsertion(domTree, batchUpdatePtr, ncd, insertInfo);
   }

   // Visits an affected node and collect its affected successors.
   static void visitInsertion(DomTreeT &domTree, const BatchUpdatePtr batchUpdatePtr,
                              const TreeNodePtr treeNode, const unsigned rootLevel,
                              const TreeNodePtr ncd, InsertionInfo &insertInfo)
   {
      const unsigned ncdLevel = ncd->getLevel();
      POLAR_DEBUG(debug_stream() << "Visiting " << BlockNamePrinter(treeNode) << ",  rootLevel "
                  << rootLevel << "\n");

      SmallVector<TreeNodePtr, 8> stack = {treeNode};
      assert(treeNode->getBlock() && insertInfo.m_visited.count(treeNode) && "Preconditions!");

      SmallPtrSet<TreeNodePtr, 8> processed;

      do {
         TreeNodePtr next = stack.popBackValue();
         POLAR_DEBUG(debug_stream() << " next: " << BlockNamePrinter(next) << "\n");

         for (const NodePtr succ :
              ChildrenGetter<sm_isPostDom>::get(next->getBlock(), batchUpdatePtr)) {
            const TreeNodePtr succTN = domTree.getNode(succ);
            assert(succTN && "Unreachable successor found at reachable insertion");
            const unsigned SuccLevel = succTN->getLevel();

            POLAR_DEBUG(debug_stream() << "\tSuccessor " << BlockNamePrinter(succ) << ", level = "
                        << SuccLevel << "\n");

            // Do not process the same node multiple times.
            if (processed.count(next) > 0) {
               continue;
            }

            // succ dominated by subtree from -- not affected.
            // (Based on the lemma 2.5 from the second paper.)
            if (SuccLevel > rootLevel) {
               POLAR_DEBUG(debug_stream() << "\t\tDominated by subtree from\n");
               if (insertInfo.m_visited.count(succTN) != 0) {
                  POLAR_DEBUG(debug_stream() << "\t\t\talready m_visited at level "
                              << insertInfo.m_visited[succTN] << "\n\t\t\tcurrent level "
                              << rootLevel << ")\n");

                  // A node can be necessary to visit again if we see it again at
                  // a lower level than before.
                  if (insertInfo.m_visited[succTN] >= rootLevel) {
                     continue;
                  }
               }
               POLAR_DEBUG(debug_stream() << "\t\tMarking m_visited not affected "
                           << BlockNamePrinter(succ) << "\n");
               insertInfo.m_visited.insert({succTN, rootLevel});
               insertInfo.m_visitedNotAffectedQueue.push_back(succTN);
               stack.push_back(succTN);
            } else if ((SuccLevel > ncdLevel + 1) &&
                       insertInfo.m_affected.count(succTN) == 0) {
               POLAR_DEBUG(debug_stream() << "\t\tMarking affected and adding "
                           << BlockNamePrinter(succ) << " to a m_bucket\n");
               insertInfo.m_affected.insert(succTN);
               insertInfo.m_bucket.push({SuccLevel, succTN});
            }
         }

         processed.insert(next);
      } while (!stack.empty());
   }

   // m_updates immediate dominators and levels after insertion.
   static void updateInsertion(DomTreeT &domTree, const BatchUpdatePtr batchUpdatePtr,
                               const TreeNodePtr ncd, InsertionInfo &insertInfo)
   {
      POLAR_DEBUG(debug_stream() << "Updating ncd = " << BlockNamePrinter(ncd) << "\n");
      for (const TreeNodePtr treeNode : insertInfo.m_affectedQueue) {
         POLAR_DEBUG(debug_stream() << "\tIDom(" << BlockNamePrinter(treeNode)
                     << ") = " << BlockNamePrinter(ncd) << "\n");
         treeNode->setIDom(ncd);
      }

      updateLevelsAfterInsertion(insertInfo);
      if (sm_isPostDom) {
         updateRootsAfterUpdate(domTree, batchUpdatePtr);
      }
   }

   static void updateLevelsAfterInsertion(InsertionInfo &insertInfo)
   {
      POLAR_DEBUG(debug_stream() << "Updating levels for m_visited but not affected nodes\n");

      for (const TreeNodePtr treeNode : insertInfo.m_visitedNotAffectedQueue) {
         POLAR_DEBUG(debug_stream() << "\tlevel(" << BlockNamePrinter(treeNode) << ") = ("
                     << BlockNamePrinter(treeNode->getIDom()) << ") "
                     << treeNode->getIDom()->getLevel() << " + 1\n");
         treeNode->updateLevel();
      }
   }

   // Handles insertion to previously unreachable nodes.
   static void insertUnreachable(DomTreeT &domTree, const BatchUpdatePtr batchUpdatePtr,
                                 const TreeNodePtr from, const NodePtr to)
   {
      POLAR_DEBUG(debug_stream() << "Inserting " << BlockNamePrinter(from)
                  << " -> (unreachable) " << BlockNamePrinter(to) << "\n");

      // Collect discovered edges to already reachable nodes.
      SmallVector<std::pair<NodePtr, TreeNodePtr>, 8> discoverededgesToReachable;
      // Discover and connect nodes that became reachable with the insertion.
      computeUnreachableDominators(domTree, batchUpdatePtr, to, from, discoverededgesToReachable);

      POLAR_DEBUG(debug_stream() << "Inserted " << BlockNamePrinter(from)
                  << " -> (prev unreachable) " << BlockNamePrinter(to) << "\n");

      // Used the discovered edges and inset discovered connecting (incoming)
      // edges.
      for (const auto &edge : discoverededgesToReachable) {
         POLAR_DEBUG(debug_stream() << "\tInserting discovered connecting edge "
                     << BlockNamePrinter(edge.first) << " -> "
                     << BlockNamePrinter(edge.second) << "\n");
         insertReachable(domTree, batchUpdatePtr, domTree.getNode(edge.first), edge.second);
      }
   }

   // Connects nodes that become reachable with an insertion.
   static void computeUnreachableDominators(
         DomTreeT &domTree, const BatchUpdatePtr batchUpdatePtr, const NodePtr root,
         const TreeNodePtr incoming,
         SmallVectorImpl<std::pair<NodePtr, TreeNodePtr>>
         &discoveredConnectingedges)
   {
      assert(!domTree.getNode(root) && "root must not be reachable");

      // Visit only previously unreachable nodes.
      auto UnreachableDescender = [&domTree, &discoveredConnectingedges](NodePtr from,
            NodePtr to) {
         const TreeNodePtr toTreeNode = domTree.getNode(to);
         if (!toTreeNode) {
            return true;
         }
         discoveredConnectingedges.push_back({from, toTreeNode});
         return false;
      };

      SemiNCAInfo SNCA(batchUpdatePtr);
      SNCA.runDFS(root, 0, UnreachableDescender, 0);
      SNCA.runSemiNCA(domTree);
      SNCA.attachNewSubtree(domTree, incoming);

      POLAR_DEBUG(debug_stream() << "After adding unreachable nodes\n");
   }

   static void delete_edge(DomTreeT &domTree, const BatchUpdatePtr batchUpdatePtr,
                           const NodePtr from, const NodePtr to)
   {
      assert(from && to && "Cannot disconnect nullptrs");
      POLAR_DEBUG(debug_stream() << "Deleting edge " << BlockNamePrinter(from) << " -> "
                  << BlockNamePrinter(to) << "\n");

#ifndef NDEBUG
      // Ensure that the edge was in fact deleted from the CFG before informing
      // the DomTree about it.
      // The check is O(node), so run it only in debug configuration.
      auto isSuccessor = [batchUpdatePtr](const NodePtr succCandidate, const NodePtr of) {
         auto Successors = ChildrenGetter<sm_isPostDom>::get(of, batchUpdatePtr);
         return llvm::find(Successors, succCandidate) != Successors.end();
      };
      (void)isSuccessor;
      assert(!isSuccessor(to, from) && "Deleted edge still exists in the CFG!");
#endif

      const TreeNodePtr fromTreeNode = domTree.getNode(from);
      // Deletion in an unreachable subtree -- nothing to do.
      if (!fromTreeNode) {
         return;
      }
      const TreeNodePtr toTreeNode = domTree.getNode(to);
      if (!toTreeNode) {
         POLAR_DEBUG(debug_stream() << "\tTo (" << BlockNamePrinter(to)
                     << ") already unreachable -- there is no edge to delete\n");
         return;
      }

      const NodePtr ncdBlock = domTree.findNearestCommonDominator(from, to);
      const TreeNodePtr ncd = domTree.getNode(ncdBlock);

      // If to dominates from -- nothing to do.
      if (toTreeNode != ncd) {
         domTree.m_dfsInfoValid = false;

         const TreeNodePtr toIDom = toTreeNode->getIDom();
         POLAR_DEBUG(debug_stream() << "\tncd " << BlockNamePrinter(ncd) << ", toIDom "
                     << BlockNamePrinter(toIDom) << "\n");

         // to remains reachable after deletion.
         // (Based on the caption under Figure 4. from the second paper.)
         if (fromTreeNode != toIDom || hasPropersupport(domTree, batchUpdatePtr, toTreeNode)) {
            deleteReachable(domTree, batchUpdatePtr, fromTreeNode, toTreeNode);
         } else {
            deleteUnreachable(domTree, batchUpdatePtr, toTreeNode);
         }
      }

      if (sm_isPostDom) {
         updateRootsAfterUpdate(domTree, batchUpdatePtr);
      }
   }

   // Handles deletions that leave destination nodes reachable.
   static void deleteReachable(DomTreeT &domTree, const BatchUpdatePtr batchUpdatePtr,
                               const TreeNodePtr fromTreeNode,
                               const TreeNodePtr toTreeNode)
   {
      POLAR_DEBUG(debug_stream() << "Deleting reachable " << BlockNamePrinter(fromTreeNode) << " -> "
                  << BlockNamePrinter(toTreeNode) << "\n");
      POLAR_DEBUG(debug_stream() << "\tRebuilding subtree\n");

      // Find the top of the subtree that needs to be rebuilt.
      // (Based on the lemma 2.6 from the second paper.)
      const NodePtr toIDom =
            domTree.findNearestCommonDominator(fromTreeNode->getBlock(), toTreeNode->getBlock());
      assert(toIDom || domTree.isPostDominator());
      const TreeNodePtr toIDomTN = domTree.getNode(toIDom);
      assert(toIDomTN);
      const TreeNodePtr prevIDomSubTree = toIDomTN->getIDom();
      // Top of the subtree to rebuild is the root node. Rebuild the tree from
      // scratch.
      if (!prevIDomSubTree) {
         POLAR_DEBUG(debug_stream() << "The entire tree needs to be rebuilt\n");
         calculateFromScratch(domTree, batchUpdatePtr);
         return;
      }

      // Only visit nodes in the subtree starting at to.
      const unsigned level = toIDomTN->getLevel();
      auto descendBelow = [level, &domTree](NodePtr, NodePtr to) {
         return domTree.getNode(to)->getLevel() > level;
      };

      POLAR_DEBUG(debug_stream() << "\tTop of subtree: " << BlockNamePrinter(toIDomTN) << "\n");

      SemiNCAInfo SNCA(batchUpdatePtr);
      SNCA.runDFS(toIDom, 0, descendBelow, 0);
      POLAR_DEBUG(debug_stream() << "\tRunning m_semi-NCA\n");
      SNCA.runSemiNCA(domTree, level);
      SNCA.reattachExistingSubtree(domTree, prevIDomSubTree);
   }

   // Checks if a node has proper support, as defined on the page 3 and later
   // explained on the page 7 of the second paper.
   static bool hasPropersupport(DomTreeT &domTree, const BatchUpdatePtr batchUpdatePtr,
                                const TreeNodePtr treeNode) {
      POLAR_DEBUG(debug_stream() << "IsReachableFromIDom " << BlockNamePrinter(treeNode) << "\n");
      for (const NodePtr pred :
           ChildrenGetter<!sm_isPostDom>::get(treeNode->getBlock(), batchUpdatePtr)) {
         POLAR_DEBUG(debug_stream() << "\tpred " << BlockNamePrinter(pred) << "\n");
         if (!domTree.getNode(pred)) continue;

         const NodePtr support =
               domTree.findNearestCommonDominator(treeNode->getBlock(), pred);
         POLAR_DEBUG(debug_stream() << "\tsupport " << BlockNamePrinter(support) << "\n");
         if (support != treeNode->getBlock()) {
            POLAR_DEBUG(debug_stream() << "\t" << BlockNamePrinter(treeNode)
                        << " is reachable from support "
                        << BlockNamePrinter(support) << "\n");
            return true;
         }
      }

      return false;
   }

   // Handle deletions that make destination node unreachable.
   // (Based on the lemma 2.7 from the second paper.)
   static void deleteUnreachable(DomTreeT &domTree, const BatchUpdatePtr batchUpdatePtr,
                                 const TreeNodePtr toTreeNode) {
      POLAR_DEBUG(debug_stream() << "Deleting unreachable subtree " << BlockNamePrinter(toTreeNode)
                  << "\n");
      assert(toTreeNode);
      assert(toTreeNode->getBlock());

      if (sm_isPostDom) {
         // Deletion makes a region reverse-unreachable and creates a new root.
         // Simulate that by inserting an edge from the virtual root to toTreeNode and
         // adding it as a new root.
         POLAR_DEBUG(debug_stream() << "\tDeletion made a region reverse-unreachable\n");
         POLAR_DEBUG(debug_stream() << "\tAdding new root " << BlockNamePrinter(toTreeNode) << "\n");
         domTree.m_roots.push_back(toTreeNode->getBlock());
         insertReachable(domTree, batchUpdatePtr, domTree.getNode(nullptr), toTreeNode);
         return;
      }

      SmallVector<NodePtr, 16> m_affectedQueue;
      const unsigned level = toTreeNode->getLevel();

      // Traverse destination node's descendants with greater level in the tree
      // and collect m_visited nodes.
      auto descendAndCollect = [level, &m_affectedQueue, &domTree](NodePtr, NodePtr to) {
         const TreeNodePtr treeNode = domTree.getNode(to);
         assert(treeNode);
         if (treeNode->getLevel() > level) return true;
         if (find(m_affectedQueue, to) == m_affectedQueue.end()) {
            m_affectedQueue.push_back(to);
         }
         return false;
      };

      SemiNCAInfo SNCA(batchUpdatePtr);
      unsigned lastDFSNum =
            SNCA.runDFS(toTreeNode->getBlock(), 0, descendAndCollect, 0);

      TreeNodePtr minNode = toTreeNode;

      // Identify the top of the subtree to rebuild by finding the ncd of all
      // the affected nodes.
      for (const NodePtr node : m_affectedQueue) {
         const TreeNodePtr treeNode = domTree.getNode(node);
         const NodePtr ncdBlock =
               domTree.findNearestCommonDominator(treeNode->getBlock(), toTreeNode->getBlock());
         assert(ncdBlock || domTree.isPostDominator());
         const TreeNodePtr ncd = domTree.getNode(ncdBlock);
         assert(ncd);

         POLAR_DEBUG(debug_stream() << "Processing affected node " << BlockNamePrinter(treeNode)
                     << " with ncd = " << BlockNamePrinter(ncd)
                     << ", minNode =" << BlockNamePrinter(minNode) << "\n");
         if (ncd != treeNode && ncd->getLevel() < minNode->getLevel()) {
            minNode = ncd;
         }
      }

      // root reached, rebuild the whole tree from scratch.
      if (!minNode->getIDom()) {
         POLAR_DEBUG(debug_stream() << "The entire tree needs to be rebuilt\n");
         calculateFromScratch(domTree, batchUpdatePtr);
         return;
      }

      // Erase the unreachable subtree in reverse preorder to process all children
      // before deleting their parent.
      for (unsigned i = lastDFSNum; i > 0; --i) {
         const NodePtr node = SNCA.m_numToNode[i];
         const TreeNodePtr treeNode = domTree.getNode(node);
         POLAR_DEBUG(debug_stream() << "Erasing node " << BlockNamePrinter(treeNode) << "\n");

         eraseNode(domTree, treeNode);
      }

      // The affected subtree start at the to node -- there's no extra work to do.
      if (minNode == toTreeNode) return;

      POLAR_DEBUG(debug_stream() << "deleteUnreachable: running DFS with minNode = "
                  << BlockNamePrinter(minNode) << "\n");
      const unsigned minLevel = minNode->getLevel();
      const TreeNodePtr prevIDom = minNode->getIDom();
      assert(prevIDom);
      SNCA.clear();

      // Identify nodes that remain in the affected subtree.
      auto descendBelow = [minLevel, &domTree](NodePtr, NodePtr to) {
         const TreeNodePtr toTreeNode = domTree.getNode(to);
         return toTreeNode && toTreeNode->getLevel() > minLevel;
      };
      SNCA.runDFS(minNode->getBlock(), 0, descendBelow, 0);

      POLAR_DEBUG(debug_stream() << "Previous m_idom(minNode) = " << BlockNamePrinter(prevIDom)
                  << "\nRunning m_semi-NCA\n");

      // Rebuild the remaining part of affected subtree.
      SNCA.runSemiNCA(domTree, minLevel);
      SNCA.reattachExistingSubtree(domTree, prevIDom);
   }

   // Removes leaf tree nodes from the dominator tree.
   static void eraseNode(DomTreeT &domTree, const TreeNodePtr treeNode)
   {
      assert(treeNode);
      assert(treeNode->getNumChildren() == 0 && "Not a tree leaf");

      const TreeNodePtr m_idom = treeNode->getIDom();
      assert(m_idom);

      auto chiter = llvm::find(m_idom->m_children, treeNode);
      assert(chiter != m_idom->m_children.end());
      std::swap(*chiter, m_idom->m_children.back());
      m_idom->m_children.pop_back();
      domTree.m_domTreeNodes.erase(treeNode->getBlock());
   }

   //~~
   //===--------------------- DomTree Batch Updater --------------------------===
   //~~

   static void applyUpdates(DomTreeT &domTree, ArrayRef<UpdateT> updates)
   {
      const size_t numUpdates = updates.getSize();
      if (numUpdates == 0) {
         return;
      }

      // Take the fast path for a single update and avoid running the batch update
      // machinery.
      if (numUpdates == 1) {
         const auto &update = m_updates.front();
         if (update.getKind() == UpdateKind::Insert) {
            domTree.insert_edge(update.getFrom(), update.getTo());
         } else {
            domTree.delete_edge(update.getFrom(), update.getTo());
         }
         return;
      }

      BatchUpdateInfo batchUpdatePtr;
      POLAR_DEBUG(debug_stream() << "Legalizing " << batchUpdatePtr.m_updates.size() << " updates\n");
      cfg::LegalizeUpdates<NodePtr>(updates, batchUpdatePtr.m_updates, sm_isPostDom);

      const size_t numLegalized = batchUpdatePtr.m_updates.getSize();
      batchUpdatePtr.m_futureSuccessors.reserve(numLegalized);
      batchUpdatePtr.m_futurepredecessors.reserve(numLegalized);

      // Use the legalized future updates to initialize future successors and
      // predecessors. Note that these sets will only decrease size over time, as
      // the next CFG snapshots slowly approach the actual (current) CFG.
      for (UpdateT &update : batchUpdatePtr.m_updates) {
         batchUpdatePtr.m_futureSuccessors[update.getFrom()].insert({update.getTo(), update.getKind()});
         batchUpdatePtr.m_futurepredecessors[update.getTo()].insert({update.getFrom(), update.getKind()});
      }

      POLAR_DEBUG(debug_stream() << "About to apply " << numLegalized << " updates\n");
      POLAR_DEBUG(if (numLegalized < 32) for (const auto &update
                                              : reverse(batchUpdatePtr.m_updates)) {
                     debug_stream() << "\t";
                     update.dump();
                     debug_stream() << "\n";
                  });
      POLAR_DEBUG(debug_stream() << "\n");

      // Recalculate the DominatorTree when the number of updates
      // exceeds a threshold, which usually makes direct updating slower than
      // recalculation. We select this threshold proportional to the
      // size of the DominatorTree. The constant is selected
      // by choosing the one with an acceptable performance on some real-world
      // inputs.
      // Make unittests of the incremental algorithm work
      if (domTree.m_domTreeNodes.size() <= 100) {
         if (numLegalized > domTree.m_domTreeNodes.size()) {
            calculateFromScratch(domTree, &batchUpdatePtr);
         }
      } else if (numLegalized > domTree.m_domTreeNodes.size() / 40) {
         calculateFromScratch(domTree, &batchUpdatePtr);
      }

      // If the DominatorTree was recalculated at some point, stop the batch
      // updates. Full recalculations ignore batch updates and look at the actual
      // CFG.
      for (size_t i = 0; i < numLegalized && !batchUpdatePtr.m_isRecalculated; ++i) {
         applynextUpdate(domTree, batchUpdatePtr);
      }
   }

   static void applynextUpdate(DomTreeT &domTree, BatchUpdateInfo &batchUpdatePtr)
   {
      assert(!batchUpdatePtr.m_updates.empty() && "No updates to apply!");
      UpdateT currentUpdate = batchUpdatePtr.m_updates.popBackValue();
      POLAR_DEBUG(debug_stream() << "Applying update: ");
      POLAR_DEBUG(currentUpdate.dump(); debug_stream() << "\n");

      // Move to the next snapshot of the CFG by removing the reverse-applied
      // current update.
      auto &fs = batchUpdatePtr.m_futureSuccessors[currentUpdate.getFrom()];
      fs.erase({currentUpdate.getTo(), currentUpdate.getKind()});
      if (fs.empty()) {
         batchUpdatePtr.m_futureSuccessors.erase(currentUpdate.getFrom());
      }
      auto &fp = batchUpdatePtr.m_futurepredecessors[currentUpdate.getTo()];
      fp.erase({currentUpdate.getFrom(), currentUpdate.getKind()});
      if (fp.empty()) {
         batchUpdatePtr.m_futurepredecessors.erase(currentUpdate.getTo());
      }
      if (currentUpdate.getKind() == UpdateKind::Insert) {
         insert_edge(domTree, &batchUpdatePtr, currentUpdate.getFrom(), currentUpdate.getTo());
      } else {
         delete_edge(domTree, &batchUpdatePtr, currentUpdate.getFrom(), currentUpdate.getTo());
      }
   }

   //~~
   //===--------------- DomTree correctness verification ---------------------===
   //~~

   // Check if the tree has correct roots. A DominatorTree always has a single
   // root which is the function's entry node. A PostDominatorTree can have
   // multiple roots - one for each node with no successors and for infinite
   // loops.
   bool verify_roots(const DomTreeT &domTree) {
      if (!domTree.m_parent && !domTree.m_roots.empty()) {
         debug_stream() << "Tree has no parent but has roots!\n";
         debug_stream().flush();
         return false;
      }

      if (!sm_isPostDom) {
         if (domTree.m_roots.empty()) {
            debug_stream() << "Tree doesn't have a root!\n";
            debug_stream().flush();
            return false;
         }

         if (domTree.getRoot() != getEntryNode(domTree)) {
            debug_stream() << "Tree's root is not its parent's entry node!\n";
            debug_stream().flush();
            return false;
         }
      }

      RootsT computedRoots = findRoots(domTree, nullptr);
      if (domTree.m_roots.size() != computedRoots.size() ||
          !std::is_permutation(domTree.m_roots.begin(), domTree.m_roots.end(),
                               computedRoots.begin())) {
         debug_stream() << "Tree has different roots than freshly computed ones!\n";
         debug_stream() << "\tPdomTree roots: ";
         for (const NodePtr node : domTree.m_roots) {
            debug_stream() << BlockNamePrinter(node) << ", ";
         }
         debug_stream() << "\n\tComputed roots: ";
         for (const NodePtr node : computedRoots) {
            debug_stream() << BlockNamePrinter(node) << ", ";
         }

         debug_stream() << "\n";
         debug_stream().flush();
         return false;
      }

      return true;
   }

   // Checks if the tree contains all reachable nodes in the input graph.
   bool verify_reachability(const DomTreeT &domTree) {
      clear();
      doFullDFSWalk(domTree, alwaysDescend);

      for (auto &nodeToTreeNode : domTree.m_domTreeNodes) {
         const TreeNodePtr treeNode = nodeToTreeNode.second.get();
         const NodePtr bb = treeNode->getBlock();

         // Virtual root has a corresponding virtual CFG node.
         if (domTree.isvirtualRoot(treeNode)) {
            continue;
         }

         if (m_nodeToInfo.count(bb) == 0) {
            debug_stream() << "DomTree node " << BlockNamePrinter(bb)
                           << " not found by DFS walk!\n";
            debug_stream().flush();

            return false;
         }
      }

      for (const NodePtr node : m_numToNode) {
         if (node && !domTree.getNode(node)) {
            debug_stream() << "CFG node " << BlockNamePrinter(node)
                           << " not found in the DomTree!\n";
            debug_stream().flush();

            return false;
         }
      }

      return true;
   }

   // Check if for every parent with a level L in the tree all of its children
   // have level L + 1.
   static bool verify_levels(const DomTreeT &domTree)
   {
      for (auto &nodeToTreeNode : domTree.m_domTreeNodes) {
         const TreeNodePtr treeNode = nodeToTreeNode.second.get();
         const NodePtr bb = treeNode->getBlock();
         if (!bb) {
            continue;
         }
         const TreeNodePtr idom = treeNode->getIDom();
         if (!idom && treeNode->getLevel() != 0) {
            debug_stream() << "node without an idom " << BlockNamePrinter(bb)
                           << " has a nonzero level " << treeNode->getLevel() << "!\n";
            debug_stream().flush();

            return false;
         }

         if (idom && treeNode->getLevel() != idom->getLevel() + 1) {
            debug_stream() << "node " << BlockNamePrinter(bb) << " has level "
                           << treeNode->getLevel() << " while its idom "
                           << BlockNamePrinter(idom->getBlock()) << " has level "
                           << idom->getLevel() << "!\n";
            debug_stream().flush();

            return false;
         }
      }

      return true;
   }

   // Check if the computed DFS numbers are correct. Note that DFS info may not
   // be valid, and when that is the case, we don't verify the numbers.
   static bool verify_dfs_numbers(const DomTreeT &domTree)
   {
      if (!domTree.m_dfsInfoValid || !domTree.m_parent) {
         return true;
      }
      const NodePtr rootBB = sm_isPostDom ? nullptr : domTree.getRoots()[0];
      const TreeNodePtr root = domTree.getNode(rootBB);

      auto printNodeAndDFSNums = [](const TreeNodePtr treeNode) {
         debug_stream() << BlockNamePrinter(treeNode) << " {" << treeNode->getDFSNumIn() << ", "
                        << treeNode->getDFSNumOut() << '}';
      };

      // verify the root's DFS In number. Although DFS numbering would also work
      // if we started from some other value, we assume 0-based numbering.
      if (root->getDFSNumIn() != 0) {
         debug_stream() << "DFSIn number for the tree root is not:\n\t";
         printNodeAndDFSNums(root);
         debug_stream() << '\n';
         debug_stream().flush();
         return false;
      }

      // For each tree node verify if children's DFS numbers cover their parent's
      // DFS numbers with no gaps.
      for (const auto &nodeToTreeNode : domTree.m_domTreeNodes) {
         const TreeNodePtr node = nodeToTreeNode.second.get();

         // Handle tree leaves.
         if (node->getChildren().empty()) {
            if (node->getDFSNumIn() + 1 != node->getDFSNumOut()) {
               debug_stream() << "Tree leaf should have DFSOut = DFSIn + 1:\n\t";
               printNodeAndDFSNums(node);
               debug_stream() << '\n';
               debug_stream().flush();
               return false;
            }

            continue;
         }

         // Make a copy and sort it such that it is possible to check if there are
         // no gaps between DFS numbers of adjacent children.
         SmallVector<TreeNodePtr, 8> children(node->begin(), node->end());
         polar::basic::sort(children, [](const TreeNodePtr child1, const TreeNodePtr child2) {
            return child1->getDFSNumIn() < child2->getDFSNumIn();
         });

         auto printChildrenError = [node, &children, printNodeAndDFSNums](
               const TreeNodePtr firstCh, const TreeNodePtr secondCh) {
            assert(firstCh);

            debug_stream() << "Incorrect DFS numbers for:\n\tParent ";
            printNodeAndDFSNums(node);

            debug_stream() << "\n\tChild ";
            printNodeAndDFSNums(firstCh);

            if (secondCh) {
               debug_stream() << "\n\tSecond child ";
               printNodeAndDFSNums(secondCh);
            }

            debug_stream() << "\nAll children: ";
            for (const TreeNodePtr child : children) {
               printNodeAndDFSNums(child);
               debug_stream() << ", ";
            }

            debug_stream() << '\n';
            debug_stream().flush();
         };

         if (children.front()->getDFSNumIn() != node->getDFSNumIn() + 1) {
            printChildrenError(children.front(), nullptr);
            return false;
         }

         if (children.back()->getDFSNumOut() + 1 != node->getDFSNumOut()) {
            printChildrenError(children.back(), nullptr);
            return false;
         }

         for (size_t i = 0, e = children.size() - 1; i != e; ++i) {
            if (children[i]->getDFSNumOut() + 1 != children[i + 1]->getDFSNumIn()) {
               printChildrenError(children[i], children[i + 1]);
               return false;
            }
         }
      }

      return true;
   }

   // The below routines verify the correctness of the dominator tree relative to
   // the CFG it's coming from.  A tree is a dominator tree iff it has two
   // properties, called the parent property and the sibling property.  Tarjan
   // and Lengauer prove (but don't explicitly name) the properties as part of
   // the proofs in their 1972 paper, but the proofs are mostly part of proving
   // things about semidominators and idoms, and some of them are simply asserted
   // based on even earlier papers (see, e.g., lemma 2).  Some papers refer to
   // these properties as "valid" and "co-valid".  See, e.g., "Dominators,
   // directed bipolar orders, and independent spanning trees" by Loukas
   // Georgiadis and Robert E. Tarjan, as well as "Dominator Tree Verification
   // and Vertex-Disjoint Paths " by the same authors.

   // A very simple and direct explanation of these properties can be found in
   // "An Experimental Study of Dynamic Dominators", found at
   // https://arxiv.org/abs/1604.02711

   // The easiest way to think of the parent property is that it's a requirement
   // of being a dominator.  Let's just take immediate dominators.  For PARENT to
   // be an immediate dominator of CHILD, all paths in the CFG must go through
   // PARENT before they hit CHILD.  This implies that if you were to cut PARENT
   // out of the CFG, there should be no paths to CHILD that are reachable.  If
   // there are, then you now have a path from PARENT to CHILD that goes around
   // PARENT and still reaches CHILD, which by definition, means PARENT can't be
   // a dominator of CHILD (let alone an immediate one).

   // The sibling property is similar.  It says that for each pair of sibling
   // nodes in the dominator tree (LEFT and RIGHT) , they must not dominate each
   // other.  If sibling LEFT dominated sibling RIGHT, it means there are no
   // paths in the CFG from sibling LEFT to sibling RIGHT that do not go through
   // LEFT, and thus, LEFT is really an ancestor (in the dominator tree) of
   // RIGHT, not a sibling.

   // It is possible to verify the parent and sibling properties in
   // linear time, but the algorithms are complex. Instead, we do it in a
   // straightforward node^2 and node^3 way below, using direct path reachability.


   // Checks if the tree has the parent property: if for all edges from V to wnode in
   // the input graph, such that V is reachable, the parent of wnode in the tree is
   // an ancestor of V in the tree.
   //
   // This means that if a node gets disconnected from the graph, then all of
   // the nodes it dominated previously will now become unreachable.
   bool verify_parent_property(const DomTreeT &domTree)
   {
      for (auto &nodeToTreeNode : domTree.m_domTreeNodes) {
         const TreeNodePtr treeNode = nodeToTreeNode.second.get();
         const NodePtr bb = treeNode->getBlock();
         if (!bb || treeNode->getChildren().empty()) continue;

         POLAR_DEBUG(debug_stream() << "Verifying parent property of node "
                     << BlockNamePrinter(treeNode) << "\n");
         clear();
         doFullDFSWalk(domTree, [bb](NodePtr from, NodePtr to) {
            return from != bb && to != bb;
         });

         for (TreeNodePtr child : treeNode->getChildren())
            if (m_nodeToInfo.count(child->getBlock()) != 0) {
               debug_stream() << "child " << BlockNamePrinter(child)
                              << " reachable after its parent " << BlockNamePrinter(bb)
                              << " is removed!\n";
               debug_stream().flush();

               return false;
            }
      }

      return true;
   }

   // Check if the tree has sibling property: if a node V does not dominate a
   // node wnode for all siblings V and wnode in the tree.
   //
   // This means that if a node gets disconnected from the graph, then all of its
   // siblings will now still be reachable.
   bool verify_sibling_property(const DomTreeT &domTree)
   {
      for (auto &nodeToTreeNode : domTree.m_domTreeNodes) {
         const TreeNodePtr treeNode = nodeToTreeNode.second.get();
         const NodePtr bb = treeNode->getBlock();
         if (!bb || treeNode->getChildren().empty()) {
            continue;
         }
         const auto &siblings = treeNode->getChildren();
         for (const TreeNodePtr node : siblings) {
            clear();
            NodePtr BBN = node->getBlock();
            doFullDFSWalk(domTree, [BBN](NodePtr from, NodePtr to) {
               return from != BBN && to != BBN;
            });

            for (const TreeNodePtr S : siblings) {
               if (S == node) continue;

               if (m_nodeToInfo.count(S->getBlock()) == 0) {
                  debug_stream() << "node " << BlockNamePrinter(S)
                                 << " not reachable when its sibling " << BlockNamePrinter(node)
                                 << " is removed!\n";
                  debug_stream().flush();

                  return false;
               }
            }
         }
      }

      return true;
   }
};

template <class DomTreeT>
void calculate(DomTreeT &domTree)
{
   SemiNCAInfo<DomTreeT>::calculateFromScratch(domTree, nullptr);
}


template <typename DomTreeT>
void calculate_with_updates(DomTreeT &domTree,
                          ArrayRef<typename DomTreeT::UpdateType> updates)
{
   // TODO: Move BUI creation in common method, reuse in ApplyUpdates.
   typename SemiNCAInfo<DomTreeT>::BatchUpdateInfo bui;
   POLAR_DEBUG(debug_stream() << "Legalizing " << bui.m_updates.size() << " updates\n");
   cfg::LegalizeUpdates<typename DomTreeT::NodePtr>(updates, bui.m_updates,
                                                    DomTreeT::sg_isPostDominator);
   const size_t numLegalized = bui.m_updates.size();
   bui.m_futureSuccessors.reserve(numLegalized);
   bui.m_futureSuccessors.reserve(numLegalized);
   for (auto &item : bui.updates) {
      bui.FutureSuccessors[item.getFrom()].push_back({item.getTo(), item.getKind()});
      bui.FuturePredecessors[item.getTo()].push_back({item.getFrom(), item.getKind()});
   }

   SemiNCAInfo<DomTreeT>::calculateFromScratch(domTree, &bui);
}

template <class DomTreeT>
void insert_edge(DomTreeT &domTree, typename DomTreeT::NodePtr from,
                 typename DomTreeT::NodePtr to)
{
   if (domTree.isPostDominator()) std::swap(from, to);
   SemiNCAInfo<DomTreeT>::insert_edge(domTree, nullptr, from, to);
}

template <class DomTreeT>
void delete_edge(DomTreeT &domTree, typename DomTreeT::NodePtr from,
                 typename DomTreeT::NodePtr to) {
   if (domTree.isPostDominator()) {
      std::swap(from, to);
   }
   SemiNCAInfo<DomTreeT>::delete_edge(domTree, nullptr, from, to);
}

template <class DomTreeT>
void apply_updates(DomTreeT &domTree,
                   ArrayRef<typename DomTreeT::UpdateType> m_updates)
{
   SemiNCAInfo<DomTreeT>::applyUpdates(domTree, m_updates);
}

template <class DomTreeT>
bool verify(const DomTreeT &domTree)
{
   SemiNCAInfo<DomTreeT> SNCA(nullptr);
   return SNCA.verify_roots(domTree) && SNCA.verify_reachability(domTree) &&
         SNCA.verify_levels(domTree) && SNCA.verify_parent_property(domTree) &&
         SNCA.verify_sibling_property(domTree) && SNCA.verify_dfs_numbers(domTree);
}

}  // namespace domtreebuilder

} // utils
} // polar

#endif // POLARPHP_UTILS_GENERIC_DOMTREE_CONSTRUCTION_H
