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
/// This file defines a set of templates that efficiently compute a dominator
/// tree over a generic graph. This is used typically in polarVM for fast
/// dominance queries on the CFG, but is fully generic w.r.t. the underlying
/// graph types.
///
/// Unlike AdomTree/* graph algorithms, generic dominator tree has more requirements
/// on the graph's NodeRef. The NodeRef should be a pointer and,
/// NodeRef->getParent() must return the parent node that is also a pointer.
///
/// FIXME: Maybe GenericDomTree needs a TreeTraits, instead of GraphTraits.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_GENERIC_DOMTREE_H
#define POLARPHP_UTILS_GENERIC_DOMTREE_H

#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/GraphTraits.h"
#include "polarphp/basic/adt/PointerIntPair.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallPtrSet.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/utils/CfgUpdate.h"
#include "polarphp/utils/RawOutStream.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace polar {
namespace utils {

using polar::basic::SmallPtrSet;
using polar::basic::SmallVector;
using polar::basic::ArrayRef;
using polar::basic::GraphTraits;
using polar::basic::DenseMap;
using polar::basic::Inverse;

template <typename NodeT, bool IsPostDom>
class DominatorTreeBase;

namespace domtreebuilder {
template <typename DomTreeT>
struct SemiNCAInfo;
}  // namespace domtreebuilder

/// \brief Base class for the actual dominator tree node.
template <class NodeT>
class DomTreeNodeBase
{
   friend struct PostDominatorTree;
   friend class DominatorTreeBase<NodeT, false>;
   friend class DominatorTreeBase<NodeT, true>;
   friend struct domtreebuilder::SemiNCAInfo<DominatorTreeBase<NodeT, false>>;
   friend struct domtreebuilder::SemiNCAInfo<DominatorTreeBase<NodeT, true>>;

   NodeT *m_theBB;
   DomTreeNodeBase *m_idom;
   unsigned m_level;
   std::vector<DomTreeNodeBase *> m_children;
   mutable unsigned m_dfsNumIn = ~0;
   mutable unsigned m_dfsNumOut = ~0;

public:
   DomTreeNodeBase(NodeT *bb, DomTreeNodeBase *idom)
      : m_theBB(bb),
        m_idom(idom),
        m_level(m_idom ? m_idom->m_level + 1 : 0)
   {}

   using iterator = typename std::vector<DomTreeNodeBase *>::iterator;
   using const_iterator =
   typename std::vector<DomTreeNodeBase *>::const_iterator;

   iterator begin()
   {
      return m_children.begin();
   }

   iterator end()
   {
      return m_children.end();
   }

   const_iterator begin() const
   {
      return m_children.begin();
   }

   const_iterator end() const
   {
      return m_children.end();
   }

   NodeT *getBlock() const
   {
      return m_theBB;
   }

   DomTreeNodeBase *getIdom() const
   {
      return m_idom;
   }

   unsigned getLevel() const
   {
      return m_level;
   }

   const std::vector<DomTreeNodeBase *> &getChildren() const
   {
      return m_children;
   }

   std::unique_ptr<DomTreeNodeBase> addChild(
         std::unique_ptr<DomTreeNodeBase> child)
   {
      m_children.push_back(child.get());
      return child;
   }

   size_t getNumChildren() const
   {
      return m_children.size();
   }

   void clearAllChildren()
   {
      m_children.clear();
   }

   bool compare(const DomTreeNodeBase *other) const
   {
      if (getNumChildren() != other->getNumChildren()) {
         return true;
      }
      if (m_level != other->m_level)  {
         return true;
      }
      SmallPtrSet<const NodeT *, 4> otherChildren;
      for (const DomTreeNodeBase *iter : *other) {
         const NodeT *nd = iter->getBlock();
         otherChildren.insert(nd);
      }

      for (const DomTreeNodeBase *iter : *this) {
         const NodeT *node = iter->getBlock();
         if (otherChildren.count(node) == 0) {
            return true;
         }
      }
      return false;
   }

   void setIdom(DomTreeNodeBase *newIdom)
   {
      assert(m_idom && "No immediate dominator?");
      if (m_idom == newIdom) {
         return;
      }
      auto iter = find(m_idom->m_children, this);
      assert(iter != m_idom->m_children.end() &&
            "Not in immediate dominator children set!");
      // I am no longer your child...
      m_idom->m_children.erase(iter);
      // Switch to new dominator
      m_idom = newIdom;
      m_idom->m_children.push_back(this);
      updateLevel();
   }

   /// getDfsNumIn/getDfsNumOut - These return the DFS visitation order for nodes
   /// in the dominator tree. They are only guaranteed valid if
   /// updateDFSNumbers() has been called.
   unsigned getDfsNumIn() const
   {
      return m_dfsNumIn;
   }

   unsigned getDfsNumOut() const
   {
      return m_dfsNumOut;
   }

private:
   // Return true if this node is dominated by other. Use this only if DFS info
   // is valid.
   bool DominatedBy(const DomTreeNodeBase *other) const
   {
      return this->m_dfsNumIn >= other->m_dfsNumIn &&
            this->m_dfsNumOut <= other->m_dfsNumOut;
   }

   void updateLevel()
   {
      assert(m_idom);
      if (m_level == m_idom->m_level + 1) {
         return;
      }
      SmallVector<DomTreeNodeBase *, 64> workStack = {this};

      while (!workStack.empty())
      {
         DomTreeNodeBase *current = workStack.popBackValue();
         current->m_level = current->m_idom->m_level + 1;
         for (DomTreeNodeBase *c : *current) {
            assert(c->m_idom);
            if (c->m_level != c->m_idom->m_level + 1) {
               workStack.push_back(c);
            }
         }
      }
   }
};

template <class NodeT>
RawOutStream &operator<<(RawOutStream &outstream, const DomTreeNodeBase<NodeT> *node)
{
   if (node->getBlock()) {
      node->getBlock()->printAsOperand(outstream, false);
   } else {
      outstream << " <<exit node>>";
   }
   outstream << " {" << node->getDfsNumIn() << "," << node->getDfsNumOut() << "} ["
             << node->getLevel() << "]\n";
   return outstream;
}

template <class NodeT>
void print_dom_tree(const DomTreeNodeBase<NodeT> *node, RawOutStream &outstream,
                    unsigned level)
{
   outstream.indent(2 * level) << "[" << level << "] " << node;
   for (typename DomTreeNodeBase<NodeT>::const_iterator iter = node->begin(),
        end = node->end();
        iter != end; ++iter) {
      print_dom_tree<NodeT>(*iter, outstream, level + 1);
   }
}

namespace domtreebuilder
{
// The routines below are provided in a separate header but referenced here.
template <typename DomTreeT>
void calculate(DomTreeT &domTree);

template <typename DomTreeT>
void calculate_with_updates(DomTreeT &dataTree,
                            ArrayRef<typename DomTreeT::UpdateType> updates);

template <typename DomTreeT>
void insert_edge(DomTreeT &domTree, typename DomTreeT::NodePtr from,
                 typename DomTreeT::NodePtr to);

template <typename DomTreeT>
void delete_edge(DomTreeT &domTree, typename DomTreeT::NodePtr from,
                 typename DomTreeT::NodePtr to);

template <typename DomTreeT>
void apply_updates(DomTreeT &domTree,
                   ArrayRef<typename DomTreeT::UpdateType> updates);

template <typename DomTreeT>
bool verify(const DomTreeT &domTree);

}  // namespace domtreebuilder

/// \brief Core dominator tree base class.
///
/// This class is a generic template over graph nodes. It is instantiated for
/// various graphs in the LLVM IR or in the code generator.
template <typename NodeT, bool IsPostDom>
class DominatorTreeBase
{
public:
   static_assert(std::is_pointer<typename GraphTraits<NodeT *>::NodeRef>::value,
                 "currently DominatorTreeBase supports only pointer nodes");
   using NodeType = NodeT;
   using NodePtr = NodeT *;
   using ParentPtr = decltype(std::declval<NodeT *>()->getParent());
   static_assert(std::is_pointer<ParentPtr>::value,
                 "currently NodeT's parent must be a pointer type");
   using ParentType = typename std::remove_pointer<ParentPtr>::type;
   static constexpr bool sm_isPostDominator = IsPostDom;

   using UpdateType = cfg::Update<NodePtr>;
   using UpdateKind = cfg::UpdateKind;
   static constexpr UpdateKind sm_insert = UpdateKind::Insert;
   static constexpr UpdateKind sm_delete = UpdateKind::Delete;

protected:
   // Dominators always have a single root, postdominators can have more.
   SmallVector<NodeT *, IsPostDom ? 4 : 1> m_roots;

   using DomTreeNodeMapType =
   DenseMap<NodeT *, std::unique_ptr<DomTreeNodeBase<NodeT>>>;
   DomTreeNodeMapType m_domTreeNodes;
   DomTreeNodeBase<NodeT> *m_rootNode;
   ParentPtr m_parent = nullptr;

   mutable bool m_dfsInfoValid = false;
   mutable unsigned int m_sowQueries = 0;

   friend struct domtreebuilder::SemiNCAInfo<DominatorTreeBase>;

public:
   DominatorTreeBase() {}

   DominatorTreeBase(DominatorTreeBase &&arg)
      : m_roots(std::move(arg.m_roots)),
        m_domTreeNodes(std::move(arg.m_domTreeNodes)),
        m_rootNode(arg.m_rootNode),
        m_parent(arg.m_parent),
        m_dfsInfoValid(arg.m_dfsInfoValid),
        m_sowQueries(arg.m_sowQueries)
   {
      arg.wipe();
   }

   DominatorTreeBase &operator=(DominatorTreeBase &&other)
   {
      m_roots = std::move(other.m_roots);
      m_domTreeNodes = std::move(other.m_domTreeNodes);
      m_rootNode = other.m_rootNode;
      m_parent = other.m_parent;
      m_dfsInfoValid = other.m_dfsInfoValid;
      m_sowQueries = other.m_sowQueries;
      other.wipe();
      return *this;
   }

   DominatorTreeBase(const DominatorTreeBase &) = delete;
   DominatorTreeBase &operator=(const DominatorTreeBase &) = delete;

   /// getRoots - Return the root blocks of the current CFG.  This may include
   /// multiple blocks if we are computing post dominators.  For forward
   /// dominators, this will always be a single block (the entry node).
   ///
   const SmallVectorImpl<NodeT *> &getRoots() const
   {
      return m_roots;
   }

   /// isPostDominator - Returns true if analysis based of postdoms
   ///
   bool isPostDominator() const
   {
      return sm_isPostDominator;
   }

   /// compare - Return false if the other dominator tree base matches this
   /// dominator tree base. otherwise return true.
   bool compare(const DominatorTreeBase &other) const
   {
      if (m_parent != other.m_parent) {
         return true;
      }
      const DomTreeNodeMapType &otherDomTreeNodes = other.m_domTreeNodes;
      if (m_domTreeNodes.size() != otherDomTreeNodes.size()) {
         return true;
      }

      for (const auto &DomTreeNode : m_domTreeNodes) {
         NodeT *BB = DomTreeNode.first;
         typename DomTreeNodeMapType::const_iterator oiter =
               otherDomTreeNodes.find(BB);
         if (oiter == otherDomTreeNodes.end()) {
            return true;
         }
         DomTreeNodeBase<NodeT> &myNd = *DomTreeNode.second;
         DomTreeNodeBase<NodeT> &otherNd = *OI->second;
         if (myNd.compare(&otherNd)) {
            return true;
         }
      }
      return false;
   }

   void releaseMemory()
   {
      reset();
   }

   /// getNode - return the (Post)DominatorTree node for the specified basic
   /// block.  This is the same as using operator[] on this class.  The result
   /// may (but is not required to) be null for a forward (backwards)
   /// statically unreachable block.
   DomTreeNodeBase<NodeT> *getNode(NodeT *node) const
   {
      auto iter = m_domTreeNodes.find(node);
      if (iter != m_domTreeNodes.end()) {
         return iter->second.get();
      }
      return nullptr;
   }

   /// See getNode.
   DomTreeNodeBase<NodeT> *operator[](NodeT *node) const
   {
      return getNode(node);
   }

   /// getRootNode - This returns the entry node for the CFG of the function.  If
   /// this tree represents the post-dominance relations for a function, however,
   /// this root may be a node with the block == NULL.  This is the case when
   /// there are multiple exit nodes from a particular function.  Consumers of
   /// post-dominance information must be capable of dealing with this
   /// possibility.
   ///
   DomTreeNodeBase<NodeT> *getRootNode()
   {
      return m_rootNode;
   }

   const DomTreeNodeBase<NodeT> *getRootNode() const
   {
      return m_rootNode;
   }

   /// Get all nodes dominated by R, including R itself.
   void getDescendants(NodeT *R, SmallVectorImpl<NodeT *> &result) const
   {
      result.clear();
      const DomTreeNodeBase<NodeT> *RN = getNode(R);
      if (!RN) {
         return; // If R is unreachable, it will not be present in the DOM tree.
      }
      SmallVector<const DomTreeNodeBase<NodeT> *, 8> WL;
      WL.push_back(RN);
      while (!WL.empty()) {
         const DomTreeNodeBase<NodeT> *node = WL.popBackValue();
         result.push_back(node->getBlock());
         WL.append(N->begin(), node->end());
      }
   }

   /// properlyDominates - Returns true iff lhs dominates rhs and lhs != rhs.
   /// Note that this is not a constant time operation!
   ///
   bool properlyDominates(const DomTreeNodeBase<NodeT> *lhs,
                          const DomTreeNodeBase<NodeT> *rhs) const
   {
      if (!lhs || !rhs) {
         return false;
      }
      if (lhs == rhs) {
         return false;
      }
      return dominates(lhs, rhs);
   }

   bool properlyDominates(const NodeT *lhs, const NodeT *rhs) const;

   /// isReachableFromEntry - Return true if lhs is dominated by the entry
   /// block of the function containing it.
   bool isReachableFromEntry(const NodeT *node) const {
      assert(!this->isPostDominator() &&
             "This is not implemented for post dominators");
      return isReachableFromEntry(getNode(const_cast<NodeT *>(node)));
   }

   bool isReachableFromEntry(const DomTreeNodeBase<NodeT> *node) const
   {
      return node;
   }

   /// dominates - Returns true iff lhs dominates rhs.  Note that this is not a
   /// constant time operation!
   ///
   bool dominates(const DomTreeNodeBase<NodeT> *lhs,
                  const DomTreeNodeBase<NodeT> *rhs) const
   {
      // lhs node trivially dominates itself.
      if (rhs == lhs) {
         return true;
      }

      // An unreachable node is dominated by anything.
      if (!isReachableFromEntry(rhs)) {
         return true;
      }

      // And dominates nothing.
      if (!isReachableFromEntry(lhs)) {
         return false;
      }

      if (rhs->getIdom() == lhs) {
         return true;
      }

      if (lhs->getIdom() == rhs) {
         return false;
      }
      // lhs can only dominate rhs if it is higher in the tree.
      if (lhs->getLevel() >= rhs->getLevel()) {
         return false;
      }
      // Compare the result of the tree walk and the dfs numbers, if expensive
      // checks are enabled.
#ifdef EXPENSIVE_CHECKS
      assert((!m_dfsInfoValid ||
              (dominatedBySlowTreeWalk(lhs, rhs) == rhs->DominatedBy(lhs))) &&
             "Tree walk disagrees with dfs numbers!");
#endif

      if (m_dfsInfoValid) {
         return rhs->DominatedBy(lhs);
      }

      // If we end up with too many slow queries, just update the
      // DFS numbers on the theory that we are going to keep querying.
      m_sowQueries++;
      if (m_sowQueries > 32) {
         updateDFSNumbers();
         return rhs->DominatedBy(lhs);
      }

      return dominatedBySlowTreeWalk(lhs, rhs);
   }

   bool dominates(const NodeT *lhs, const NodeT *rhs) const;

   NodeT *getRoot() const
   {
      assert(this->m_roots.getSize() == 1 && "Should always have entry node!");
      return this->m_roots[0];
   }

   /// findNearestCommonDominator - Find nearest common dominator basic block
   /// for basic block lhs and rhs. If there is no such block then return nullptr.
   NodeT *findNearestCommonDominator(NodeT *lhs, NodeT *rhs) const
   {
      assert(lhs && rhs && "Pointers are not valid");
      assert(lhs->getParent() == rhs->getParent() &&
             "Two blocks are not in same function");

      // If either lhs or rhs is a entry block then it is nearest common dominator
      // (for forward-dominators).
      if (!isPostDominator()) {
         NodeT &entry = lhs->getParent()->front();
         if (lhs == &entry || rhs == &entry) {
            return &entry;
         }
      }

      DomTreeNodeBase<NodeT> *nodeA = getNode(lhs);
      DomTreeNodeBase<NodeT> *nodeB = getNode(rhs);

      if (!nodeA || !nodeB) return nullptr;

      // Use level information to go up the tree until the levels match. Then
      // continue going up til we arrive at the same node.
      while (nodeA && nodeA != nodeB) {
         if (nodeA->getLevel() < nodeB->getLevel()) {
            std::swap(nodeA, nodeB);
         }
         nodeA = nodeA->m_idom;
      }
      return nodeA ? nodeA->getBlock() : nullptr;
   }

   const NodeT *findNearestCommonDominator(const NodeT *lhs,
                                           const NodeT *rhs) const
   {
      // Cast away the const qualifiers here. This is ok since
      // const is re-introduced on the return type.
      return findNearestCommonDominator(const_cast<NodeT *>(lhs),
                                        const_cast<NodeT *>(rhs));
   }

   bool isVirtualRoot(const DomTreeNodeBase<NodeT> *lhs) const
   {
      return isPostDominator() && !lhs->getBlock();
   }

   //===--------------------------------------------------------------------===//
   // API to update (Post)DominatorTree information based on modifications to
   // the CFG...

   /// Inform the dominator tree about a sequence of CFG edge insertions and
   /// deletions and perform a batch update on the tree.
   ///
   /// This function should be used when there were multiple CFG updates after
   /// the last dominator tree update. It takes care of performing the updates
   /// in sync with the CFG and optimizes away the redundant operations that
   /// cancel each other.
   /// The functions expects the sequence of updates to be balanced. Eg.:
   ///  - {{Insert, lhs, rhs}, {Delete, lhs, rhs}, {Insert, lhs, rhs}} is fine, because
   ///    logically it results in a single insertions.
   ///  - {{Insert, lhs, rhs}, {Insert, lhs, rhs}} is invalid, because it doesn't make
   ///    sense to insert the same edge twice.
   ///
   /// What's more, the functions assumes that it's safe to ask every node in the
   /// CFG about its children and inverse children. This implies that deletions
   /// of CFG edges must not delete the CFG nodes before calling this function.
   ///
   /// Batch updates should be generally faster when performing longer sequences
   /// of updates than calling insertEdge/deleteEdge manually multiple times, as
   /// it can reorder the updates and remove redundant ones internally.
   /// The batch updater is also able to detect sequences of zero and exactly one
   /// update -- it's optimized to do less work in these cases.
   ///
   /// Note that for postdominators it automatically takes care of applying
   /// updates on reverse edges internally (so there's no need to swap the
   /// m_from and m_to pointers when constructing DominatorTree::UpdateType).
   /// The type of updates is the same for DomTreeBase<T> and PostDomTreeBase<T>
   /// with the same template parameter T.
   ///
   /// \param updates An unordered sequence of updates to perform.
   ///
   void applyUpdates(ArrayRef<UpdateType> updates)
   {
      domtreebuilder::apply_updates(*this, updates);
   }

   /// Inform the dominator tree about a CFG edge insertion and update the tree.
   ///
   /// This function has to be called just before or just after making the update
   /// on the actual CFG. There cannot be any other updates that the dominator
   /// tree doesn't know about.
   ///
   /// Note that for postdominators it automatically takes care of inserting
   /// a reverse edge internally (so there's no need to swap the parameters).
   ///
   void insertEdge(NodeT *from, NodeT *to)
   {
      assert(from);
      assert(to);
      assert(from->getParent() == m_parent);
      assert(to->getParent() == m_parent);
      domtreebuilder::insert_edge(*this, from, to);
   }

   /// Inform the dominator tree about a CFG edge deletion and update the tree.
   ///
   /// This function has to be called just after making the update on the actual
   /// CFG. An internal functions checks if the edge doesn't exist in the CFG in
   /// DEBUG mode. There cannot be any other updates that the
   /// dominator tree doesn't know about.
   ///
   /// Note that for postdominators it automatically takes care of deleting
   /// a reverse edge internally (so there's no need to swap the parameters).
   ///
   void deleteEdge(NodeT *m_from, NodeT *m_to)
   {
      assert(from);
      assert(to);
      assert(from->getParent() == m_parent);
      assert(to->getParent() == m_parent);
      domtreebuilder::delete_edge(*this, from, to);
   }

   /// Add a new node to the dominator tree information.
   ///
   /// This creates a new node as a child of DomBB dominator node, linking it
   /// into the children list of the immediate dominator.
   ///
   /// \param BB New node in CFG.
   /// \param DomBB CFG node that is dominator for BB.
   /// \returns New dominator tree node that represents new CFG node.
   ///
   DomTreeNodeBase<NodeT> *addNewBlock(NodeT *bb, NodeT *domBB)
   {
      assert(getNode(bb) == nullptr && "block already in dominator tree!");
      DomTreeNodeBase<NodeT> *idomNode = getNode(domBB);
      assert(idomNode && "Not immediate dominator specified for block!");
      m_dfsInfoValid = false;
      return (m_domTreeNodes[bb] = idomNode->addChild(
               std::make_unique<DomTreeNodeBase<NodeT>>(bb, idomNode))).get();
   }

   /// Add a new node to the forward dominator tree and make it a new root.
   ///
   /// \param BB New node in CFG.
   /// \returns New dominator tree node that represents new CFG node.
   ///
   DomTreeNodeBase<NodeT> *setNewRoot(NodeT *bb)
   {
      assert(getNode(bb) == nullptr && "block already in dominator tree!");
      assert(!this->isPostDominator() &&
             "Cannot change root of post-dominator tree");
      m_dfsInfoValid = false;
      DomTreeNodeBase<NodeT> *newNode = (m_domTreeNodes[bb] =
            std::make_unique<DomTreeNodeBase<NodeT>>(bb, nullptr)).get();
      if (m_roots.empty()) {
         addRoot(bb);
      } else {
         assert(m_roots.size() == 1);
         NodeT *oldRoot = m_roots.front();
         auto &oldNode = m_domTreeNodes[oldRoot];
         oldNode = newNode->addChild(std::move(m_domTreeNodes[oldRoot]));
         oldNode->m_idom = newNode;
         oldNode->updateLevel();
         m_roots[0] = bb;
      }
      return m_rootNode = newNode;
   }

   /// changeImmediateDominator - This method is used to update the dominator
   /// tree information when a node's immediate dominator changes.
   ///
   void changeImmediateDominator(DomTreeNodeBase<NodeT> *node,
                                 DomTreeNodeBase<NodeT> *newIdom)
   {
      assert(node && newIdom && "Cannot change null node pointers!");
      m_dfsInfoValid = false;
      node->setIdom(newIdom);
   }

   void changeImmediateDominator(NodeT *bb, NodeT *newBB)
   {
      changeImmediateDominator(getNode(bb), getNode(newBB));
   }

   /// eraseNode - Removes a node from the dominator tree. block must not
   /// dominate any other blocks. Removes node from its immediate dominator's
   /// children list. Deletes dominator node associated with basic block BB.
   void eraseNode(NodeT *bb)
   {
      DomTreeNodeBase<NodeT> *node = getNode(bb);
      assert(node && "Removing node that isn't in dominator tree.");
      assert(node->getChildren().empty() && "node is not a leaf node.");

      m_dfsInfoValid = false;

      // Remove node from immediate dominator's children list.
      DomTreeNodeBase<NodeT> *m_idom = node->getIdom();
      if (m_idom) {
         const auto I = find(m_idom->m_children, node);
         assert(I != m_idom->m_children.end() &&
               "Not in immediate dominator children set!");
         // I am no longer your child...
         m_idom->m_children.erase(I);
      }

      m_domTreeNodes.erase(bb);

      if (!IsPostDom) {
         return;
      }
      // Remember to update PostDominatorTree roots.
      auto riter = polar::basic::find(m_roots, bb);
      if (riter != m_roots.end()) {
         std::swap(*riter, m_roots.back());
         m_roots.pop_back();
      }
   }

   /// splitBlock - BB is split and now it has one successor. Update dominator
   /// tree to reflect this change.
   void splitBlock(NodeT *newBB)
   {
      if (sm_isPostDominator) {
         split<Inverse<NodeT *>>(newBB);
      } else {
         split<NodeT *>(newBB);
      }
   }

   /// print - Convert to human readable form
   ///
   void print(RawOutStream &outstream) const
   {
      outstream << "=============================--------------------------------\n";
      if (sm_isPostDominator) {
         outstream << "Inorder PostDominator Tree: ";
      } else {
         outstream << "Inorder Dominator Tree: ";
      }
      if (!m_dfsInfoValid) {
         outstream << "DFSNumbers invalid: " << m_sowQueries << " slow queries.";
      }
      outstream << "\n";
      // The postdom tree can have a null root if there are no returns.
      if (getRootNode()) {
         print_dom_tree<NodeT>(getRootNode(), outstream, 1);
      }
      if (sm_isPostDominator) {
         outstream << "m_roots: ";
         for (const NodePtr block : m_roots) {
            block->printAsOperand(outstream, false);
            outstream << " ";
         }
         outstream << "\n";
      }
   }

public:
   /// updateDFSNumbers - Assign In and Out numbers to the nodes while walking
   /// dominator tree in dfs order.
   void updateDFSNumbers() const
   {
      if (m_dfsInfoValid) {
         m_sowQueries = 0;
         return;
      }

      SmallVector<std::pair<const DomTreeNodeBase<NodeT> *,
            typename DomTreeNodeBase<NodeT>::const_iterator>,
            32> workStack;

      const DomTreeNodeBase<NodeT> *thisRoot = getRootNode();
      assert((!m_parent || thisRoot) && "Empty constructed DomTree");
      if (!thisRoot)
         return;

      // Both dominators and postdominators have a single root node. In the case
      // case of PostDominatorTree, this node is a virtual root.
      workStack.push_back({thisRoot, thisRoot->begin()});

      unsigned DFSNum = 0;
      thisRoot->m_dfsNumIn = DFSNum++;

      while (!workStack.empty()) {
         const DomTreeNodeBase<NodeT> *node = workStack.back().first;
         const auto childIt = workStack.back().second;

         // If we visited all of the children of this node, "recurse" back up the
         // stack setting the DFOutNum.
         if (childIt == node->end()) {
            node->m_dfsNumOut = DFSNum++;
            workStack.pop_back();
         } else {
            // otherwise, recursively visit this child.
            const DomTreeNodeBase<NodeT> *child = *childIt;
            ++workStack.back().second;
            workStack.push_back({child, child->begin()});
            child->m_dfsNumIn = DFSNum++;
         }
      }

      m_sowQueries = 0;
      m_dfsInfoValid = true;
   }

   /// recalculate - compute a dominator tree for the given function
   void recalculate(ParentType &func)
   {
      m_parent = &func;
      domtreebuilder::calculate(*this);
   }

   void recalculate(ParentType &func, ArrayRef<UpdateType> updates)
   {
      m_parent = &func;
      domtreebuilder::calculate_with_updates(*this, updates);
   }

   /// verify - checks if the tree is correct. There are 3 level of verification:
   ///  - Full --  verifies if the tree is correct by making sure all the
   ///             properties (including the parent and the sibling property)
   ///             hold.
   ///             Takes O(N^3) time.
   ///
   ///  - Basic -- checks if the tree is correct, but compares it to a freshly
   ///             constructed tree instead of checking the sibling property.
   ///             Takes O(N^2) time.
   ///
   ///  - Fast  -- checks basic tree structure and compares it with a freshly
   ///             constructed tree.
   ///             Takes O(N^2) time worst case, but is faster in practise (same
   ///             as tree construction).
   bool verify() const
   {
      return domtreebuilder::verify(*this);
   }

protected:
   void addRoot(NodeT *bb)
   {
      this->m_roots.push_back(bb);
   }

   void reset()
   {
      m_domTreeNodes.clear();
      m_roots.clear();
      m_rootNode = nullptr;
      m_parent = nullptr;
      m_dfsInfoValid = false;
      m_sowQueries = 0;
   }

   // newBB is split and now it has one successor. Update dominator tree to
   // reflect this change.
   template <class N>
   void split(typename GraphTraits<N>::NodeRef newBB)
   {
      using GraphT = GraphTraits<N>;
      using NodeRef = typename GraphT::NodeRef;
      assert(std::distance(GraphT::childBegin(newBB),
                           GraphT::childEnd(newBB)) == 1 &&
             "newBB should have a single successor!");
      NodeRef newBBSucc = *GraphT::childBegin(newBB);

      std::vector<NodeRef> predBlocks;
      for (const auto &pred : children<Inverse<N>>(newBB))
         predBlocks.push_back(pred);

      assert(!predBlocks.empty() && "No predblocks?");

      bool newBBDominatesNewBBSucc = true;
      for (const auto &pred : children<Inverse<N>>(newBBSucc)) {
         if (pred != newBB && !dominates(newBBSucc, pred) &&
             isReachableFromEntry(pred)) {
            newBBDominatesNewBBSucc = false;
            break;
         }
      }

      // Find newBB's immediate dominator and create new dominator tree node for
      // newBB.
      NodeT *newBBIdom = nullptr;
      unsigned i = 0;
      for (i = 0; i < predBlocks.size(); ++i)
         if (isReachableFromEntry(predBlocks[i])) {
            newBBIdom = predBlocks[i];
            break;
         }

      // It's possible that none of the predecessors of newBB are reachable;
      // in that case, newBB itself is unreachable, so nothing needs to be
      // changed.
      if (!newBBIdom) return;

      for (i = i + 1; i < predBlocks.size(); ++i) {
         if (isReachableFromEntry(predBlocks[i]))
            newBBIdom = findNearestCommonDominator(newBBIdom, predBlocks[i]);
      }

      // Create the new dominator tree node... and set the idom of newBB.
      DomTreeNodeBase<NodeT> *newBBNode = addNewBlock(newBB, newBBIdom);

      // If newBB strictly dominates other blocks, then it is now the immediate
      // dominator of newBBSucc.  Update the dominator tree as appropriate.
      if (newBBDominatesNewBBSucc) {
         DomTreeNodeBase<NodeT> *NewBBSuccNode = getNode(newBBSucc);
         changeImmediateDominator(NewBBSuccNode, newBBNode);
      }
   }

private:
   bool dominatedBySlowTreeWalk(const DomTreeNodeBase<NodeT> *lhs,
                                const DomTreeNodeBase<NodeT> *rhs) const
   {
      assert(lhs != rhs);
      assert(isReachableFromEntry(rhs));
      assert(isReachableFromEntry(lhs));

      const DomTreeNodeBase<NodeT> *m_idom;
      while ((m_idom = rhs->getIdom()) != nullptr && m_idom != lhs && m_idom != rhs) {
         rhs = m_idom;  // Walk up the tree
      }
      return m_idom != nullptr;
   }

   /// \brief Wipe this tree's state without releasing any resources.
   ///
   /// This is essentially a post-move helper only. It leaves the object in an
   /// assignable and destroyable state, but otherwise invalid.
   void wipe()
   {
      m_domTreeNodes.clear();
      m_rootNode = nullptr;
      m_parent = nullptr;
   }
};

template <typename T>
using DomTreeBase = DominatorTreeBase<T, false>;

template <typename T>
using PostDomTreeBase = DominatorTreeBase<T, true>;

// These two functions are declared out of line as a workaround for building
// with old (< r147295) versions of clang because of pr11642.
template <typename NodeT, bool IsPostDom>
bool DominatorTreeBase<NodeT, IsPostDom>::dominates(const NodeT *lhs,
                                                    const NodeT *rhs) const
{
   if (lhs == rhs)
      return true;

   // Cast away the const qualifiers here. This is ok since
   // this function doesn't actually return the values returned
   // from getNode.
   return dominates(getNode(const_cast<NodeT *>(lhs)),
                    getNode(const_cast<NodeT *>(rhs)));
}
template <typename NodeT, bool IsPostDom>
bool DominatorTreeBase<NodeT, IsPostDom>::properlyDominates(
      const NodeT *lhs, const NodeT *rhs) const
{
   if (lhs == rhs) {
      return false;
   }
   // Cast away the const qualifiers here. This is ok since
   // this function doesn't actually return the values returned
   // from getNode.
   return dominates(getNode(const_cast<NodeT *>(lhs)),
                    getNode(const_cast<NodeT *>(rhs)));
}

} // utils
} // polar

#endif // POLARPHP_UTILS_GENERIC_DOMTREE_H
