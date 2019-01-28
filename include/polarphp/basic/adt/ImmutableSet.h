// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018  polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of  polarphp project authors
//
// Created by polarboy on 2018/06/23.

#ifndef  POLARPHP_BASIC_ADT_IMMUTABLE_SET_H
#define  POLARPHP_BASIC_ADT_IMMUTABLE_SET_H

#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/FoldingSet.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/Iterator.h"
#include "polarphp/utils/Allocator.h"
#include "polarphp/utils/ErrorHandling.h"
#include <cassert>
#include <cstdint>
#include <functional>
#include <iterator>
#include <new>
#include <vector>

namespace polar {
namespace basic {

//===----------------------------------------------------------------------===//
// Immutable AVL-Tree Definition.
//===----------------------------------------------------------------------===//

template <typename ImutInfo> class ImutAVLFactory;
template <typename ImutInfo> class ImutIntervalAVLFactory;
template <typename ImutInfo> class ImutAVLTreeInOrderIterator;
template <typename ImutInfo> class ImutAVLTreeGenericIterator;

template <typename ImutInfo>
class ImutAVLTree
{
public:
   using KeyTypeRef = typename ImutInfo::KeyTypeRef;
   using ValueType = typename ImutInfo::ValueType;
   using ValueTypeRef = typename ImutInfo::ValueTypeRef;
   using Factory = ImutAVLFactory<ImutInfo>;
   using Iterator = ImutAVLTreeInOrderIterator<ImutInfo>;

   friend class ImutAVLFactory<ImutInfo>;
   friend class ImutIntervalAVLFactory<ImutInfo>;
   friend class ImutAVLTreeGenericIterator<ImutInfo>;

   //===----------------------------------------------------===//
   // Public Interface.
   //===----------------------------------------------------===//

   /// Return a pointer to the left subtree.  This value
   ///  is NULL if there is no left subtree.
   ImutAVLTree *getLeft() const
   {
      return m_left;
   }

   /// Return a pointer to the right subtree.  This value is
   ///  NULL if there is no right subtree.
   ImutAVLTree *getRight() const
   {
      return m_right;
   }

   /// getHeight - Returns the height of the tree.  A tree with no subtrees
   ///  has a height of 1.
   unsigned getHeight() const
   {
      return m_height;
   }

   /// getValue - Returns the data value associated with the tree node.
   const ValueType& getValue() const
   {
      return m_value;
   }

   /// find - Finds the subtree associated with the specified key value.
   ///  This method returns NULL if no matching subtree is found.
   ImutAVLTree* find(KeyTypeRef key)
   {
      ImutAVLTree *temp = this;
      while (temp) {
         KeyTypeRef currentKey = ImutInfo::getKeyOfValue(temp->getValue());
         if (ImutInfo::isEqual(key, currentKey)) {
            return temp;
         } else if (ImutInfo::isLess(key, currentKey)) {
            temp = temp->getLeft();
         } else {
            temp = temp->getRight();
         }
      }
      return nullptr;
   }

   /// getMaxElement - Find the subtree associated with the highest ranged
   ///  key value.
   ImutAVLTree* getMaxElement()
   {
      ImutAVLTree *temp = this;
      ImutAVLTree *right = temp->getRight();
      while (right) {
         temp = right;
         right = temp->getRight();
      }
      return temp;
   }

   /// size - Returns the number of nodes in the tree, which includes
   ///  both leaves and non-leaf nodes.
   unsigned size() const
   {
      unsigned n = 1;
      if (const ImutAVLTree* lhs = getLeft()) {
         n += lhs->size();
      }
      if (const ImutAVLTree* rhs = getRight()) {
         n += rhs->size();
      }
      return n;
   }

   unsigned getSize() const
   {
      return size();
   }

   /// begin - Returns an Iterator that iterates over the nodes of the tree
   ///  in an inorder traversal.  The returned Iterator thus refers to the
   ///  the tree node with the minimum data element.
   Iterator begin() const
   {
      return Iterator(this);
   }

   /// end - Returns an Iterator for the tree that denotes the end of an
   ///  inorder traversal.
   Iterator end() const
   {
      return Iterator();
   }

   bool isElementEqual(ValueTypeRef value) const
   {
      // Compare the keys.
      if (!ImutInfo::isEqual(ImutInfo::getKeyOfValue(getValue()),
                             ImutInfo::getKeyOfValue(value))) {
         return false;
      }
      // Also compare the data values.
      if (!ImutInfo::isDataEqual(ImutInfo::getDataOfValue(getValue()),
                                 ImutInfo::getDataOfValue(value))) {
         return false;
      }
      return true;
   }

   bool isElementEqual(const ImutAVLTree *other) const
   {
      return isElementEqual(other->getValue());
   }

   /// isEqual - Compares two trees for structural equality and returns true
   ///   if they are equal.  This worst case performance of this operation is
   //    linear in the sizes of the trees.
   bool isEqual(const ImutAVLTree &other) const
   {
      if (&other == this) {
         return true;
      }
      Iterator liter = begin(), lend = end();
      Iterator riter = other.begin(), rend = other.end();

      while (liter != lend && riter != rend) {
         if (&*liter == &*riter) {
            liter.skipSubTree();
            riter.skipSubTree();
            continue;
         }
         if (!liter->isElementEqual(&*riter)) {
            return false;
         }
         ++liter;
         ++riter;
      }
      return liter == lend && riter == rend;
   }

   /// isNotEqual - Compares two trees for structural inequality.  Performance
   ///  is the same is isEqual.
   bool isNotEqual(const ImutAVLTree& other) const
   {
      return !isEqual(other);
   }

   /// contains - Returns true if this tree contains a subtree (node) that
   ///  has an data element that matches the specified key.  Complexity
   ///  is logarithmic in the size of the tree.
   bool contains(KeyTypeRef key)
   {
      return (bool) find(key);
   }

   /// foreach - A member template the accepts invokes operator() on a functor
   ///  object (specifed by Callback) for every node/subtree in the tree.
   ///  Nodes are visited using an inorder traversal.
   template <typename Callback>
   void foreach(Callback& callback)
   {
      if (ImutAVLTree* lhs = getLeft()) {
         lhs->foreach(callback);
      }
      callback(m_value);
      if (ImutAVLTree* rhs = getRight()) {
         rhs->foreach(callback);
      }
   }

   /// validateTree - A utility method that checks that the balancing and
   ///  ordering invariants of the tree are satisifed.  It is a recursive
   ///  method that returns the height of the tree, which is then consumed
   ///  by the enclosing validateTree call.  External callers should ignore the
   ///  return value.  An invalid tree will cause an assertion to fire in
   ///  a debug build.
   unsigned validateTree() const
   {
      unsigned heightLeft = getLeft() ? getLeft()->validateTree() : 0;
      unsigned heightRight = getRight() ? getRight()->validateTree() : 0;
      (void) heightLeft;
      (void) heightRight;

      assert(getHeight() == (heightLeft> heightRight ? heightLeft : heightRight ) + 1
             && "Height calculation wrong");

      assert((heightLeft> heightRight ? heightLeft - heightRight : heightRight - heightLeft) <= 2
             && "Balancing invariant violated");

      assert((!getLeft() ||
              ImutInfo::isLess(ImutInfo::getKeyOfValue(getLeft()->getValue()),
                               ImutInfo::getKeyOfValue(getValue()))) &&
             "Value in left child is not less that current value");


      assert(!(getRight() ||
               ImutInfo::isLess(ImutInfo::getKeyOfValue(getValue()),
                                ImutInfo::getKeyOfValue(getRight()->getValue()))) &&
             "Current value is not less that value of right child");

      return getHeight();
   }

   //===----------------------------------------------------===//
   // Internal values.
   //===----------------------------------------------------===//

private:
   Factory *m_factory;
   ImutAVLTree *m_left;
   ImutAVLTree *m_right;
   ImutAVLTree *m_prev = nullptr;
   ImutAVLTree *m_next = nullptr;

   unsigned m_height : 28;
   bool m_isMutable : 1;
   bool m_isDigestCached : 1;
   bool m_isCanonicalized : 1;

   ValueType m_value;
   uint32_t m_digest = 0;
   uint32_t m_refCount = 0;

   //===----------------------------------------------------===//
   // Internal methods (node manipulation; used by Factory).
   //===----------------------------------------------------===//

private:
   /// ImutAVLTree - Internal constructor that is only called by
   ///   ImutAVLFactory.
   ImutAVLTree(Factory *factory, ImutAVLTree* left, ImutAVLTree* right, ValueTypeRef value,
               unsigned height)
      : m_factory(factory), m_left(left), m_right(right), m_height(height), m_isMutable(true),
        m_isDigestCached(false), m_isCanonicalized(false), m_value(value)
   {
      if (m_left) {
         m_left->retain();
      }
      if (m_right) {
         m_right->retain();
      }
   }

   /// isMutable - Returns true if the left and right subtree references
   ///  (as well as height) can be changed.  If this method returns false,
   ///  the tree is truly immutable.  Trees returned from an ImutAVLFactory
   ///  object should always have this method return true.  Further, if this
   ///  method returns false for an instance of ImutAVLTree, all subtrees
   ///  will also have this method return false.  The converse is not true.
   bool isMutable() const
   {
      return m_isMutable;
   }

   /// hasCachedDigest - Returns true if the digest for this tree is cached.
   ///  This can only be true if the tree is immutable.
   bool hasCachedDigest() const
   {
      return m_isDigestCached;
   }

   //===----------------------------------------------------===//
   // Mutating operations.  A tree root can be manipulated as
   // long as its reference has not "escaped" from internal
   // methods of a factory object (see below).  When a tree
   // pointer is externally viewable by client code, the
   // internal "mutable bit" is cleared to mark the tree
   // immutable.  Note that a tree that still has its mutable
   // bit set may have children (subtrees) that are themselves
   // immutable.
   //===----------------------------------------------------===//

   /// markImmutable - Clears the mutable flag for a tree.  After this happens,
   ///   it is an error to call setLeft(), setRight(), and setHeight().
   void markImmutable()
   {
      assert(isMutable() && "Mutable flag already removed.");
      m_isMutable = false;
   }

   /// markedCachedDigest - Clears the NoCachedDigest flag for a tree.
   void markedCachedDigest()
   {
      assert(!hasCachedDigest() && "NoCachedDigest flag already removed.");
      m_isDigestCached = true;
   }

   /// setHeight - Changes the height of the tree.  Used internally by
   ///  ImutAVLFactory.
   void setHeight(unsigned height)
   {
      assert(isMutable() && "Only a mutable tree can have its height changed.");
      m_height = height;
   }

   static uint32_t computeDigest(ImutAVLTree *left, ImutAVLTree *right,
                                 ValueTypeRef value)
   {
      uint32_t digest = 0;

      if (left) {
         digest += left->computeDigest();
      }
      // Compute digest of stored data.
      FoldingSetNodeId id;
      ImutInfo::profile(id, value);
      digest += id.computeHash();

      if (right) {
         digest += right->computeDigest();
      }
      return digest;
   }

   uint32_t computeDigest()
   {
      // Check the lowest bit to determine if digest has actually been
      // pre-computed.
      if (hasCachedDigest()) {
         return m_digest;
      }
      uint32_t ret = computeDigest(getLeft(), getRight(), getValue());
      m_digest = ret;
      markedCachedDigest();
      return ret;
   }

   //===----------------------------------------------------===//
   // Reference count operations.
   //===----------------------------------------------------===//

public:
   void retain()
   {
      ++m_refCount;
   }

   void release()
   {
      assert(m_refCount> 0);
      if (--m_refCount == 0) {
         destroy();
      }
   }

   void destroy()
   {
      if (m_left) {
         m_left->release();
      }
      if (m_right) {
         m_right->release();
      }
      if (m_isCanonicalized) {
         if (m_next) {
            m_next->m_prev = m_prev;
         }
         if (m_prev) {
            m_prev->m_next = m_next;
         } else {
            m_factory->m_cache[m_factory->maskCacheIndex(computeDigest())] = m_next;
         }
      }

      // We need to clear the mutability bit in case we are
      // destroying the node as part of a sweep in ImutAVLFactory::recoverNodes().
      m_isMutable = false;
      m_factory->m_freeNodes.push_back(this);
   }
};

//===----------------------------------------------------------------------===//
// Immutable AVL-Tree Factory class.
//===----------------------------------------------------------------------===//

template <typename ImutInfo>
class ImutAVLFactory
{
   friend class ImutAVLTree<ImutInfo>;

   using TreeType = ImutAVLTree<ImutInfo>;
   using ValueTypeRef = typename TreeType::ValueTypeRef;
   using KeyTypeRef = typename TreeType::KeyTypeRef;
   using CacheTy = DenseMap<unsigned, TreeType*>;

   CacheTy m_cache;
   uintptr_t m_allocator;
   std::vector<TreeType*> m_createdNodes;
   std::vector<TreeType*> m_freeNodes;

   bool ownsAllocator() const
   {
      return (m_allocator & 0x1) == 0;
   }

   BumpPtrAllocator& getAllocator() const
   {
      return *reinterpret_cast<BumpPtrAllocator*>(m_allocator & ~0x1);
   }

   //===--------------------------------------------------===//
   // Public interface.
   //===--------------------------------------------------===//

public:
   ImutAVLFactory()
      : m_allocator(reinterpret_cast<uintptr_t>(new BumpPtrAllocator()))
   {}

   ImutAVLFactory(BumpPtrAllocator &alloc)
      : m_allocator(reinterpret_cast<uintptr_t>(&alloc) | 0x1)
   {}

   ~ImutAVLFactory()
   {
      if (ownsAllocator()){
         delete &getAllocator();
      }
   }

   TreeType* add(TreeType *tree, ValueTypeRef value)
   {
      tree = addInternal(value, tree);
      markImmutable(tree);
      recoverNodes();
      return tree;
   }

   TreeType* remove(TreeType *tree, KeyTypeRef value)
   {
      tree = removeInternal(value, tree);
      markImmutable(tree);
      recoverNodes();
      return tree;
   }

   TreeType* getEmptyTree() const
   {
      return nullptr;
   }

protected:
   //===--------------------------------------------------===//
   // A bunch of quick helper functions used for reasoning
   // about the properties of trees and their children.
   // These have succinct names so that the balancing code
   // is as terse (and readable) as possible.
   //===--------------------------------------------------===//

   bool isEmpty(TreeType *tree) const
   {
      return !tree;
   }

   unsigned getHeight(TreeType *tree) const
   {
      return tree ? tree->getHeight() : 0;
   }

   TreeType *getLeft(TreeType *tree) const
   {
      return tree->getLeft();
   }

   TreeType *getRight(TreeType *tree) const
   {
      return tree->getRight();
   }

   ValueTypeRef getValue(TreeType *tree) const
   {
      return tree->m_value;
   }

   // Make sure the index is not the Tombstone or Entry key of the DenseMap.
   static unsigned maskCacheIndex(unsigned value)
   {
      return (value & ~0x02);
   }

   unsigned incrementHeight(TreeType *left, TreeType *right) const
   {
      unsigned hl = getHeight(left);
      unsigned hr = getHeight(right);
      return (hl> hr ? hl : hr) + 1;
   }

   static bool compareTreeWithSection(TreeType *tree,
                                      typename TreeType::Iterator &treeIter,
                                      typename TreeType::Iterator &treeEnd)
   {
      typename TreeType::Iterator iter = tree->begin(), end = tree->end();
      for ( ; iter != end ; ++iter, ++treeIter) {
         if (treeIter == treeEnd || !iter->isElementEqual(&*treeIter)) {
            return false;
         }
      }
      return true;
   }

   //===--------------------------------------------------===//
   // "createNode" is used to generate new tree roots that link
   // to other trees.  The functon may also simply move links
   // in an existing root if that root is still marked mutable.
   // This is necessary because otherwise our balancing code
   // would leak memory as it would create nodes that are
   // then discarded later before the finished tree is
   // returned to the caller.
   //===--------------------------------------------------===//

   TreeType* createNode(TreeType *left, ValueTypeRef value, TreeType *right)
   {
      BumpPtrAllocator &allocator = getAllocator();
      TreeType *tree;
      if (!m_freeNodes.empty()) {
         tree = m_freeNodes.back();
         m_freeNodes.pop_back();
         assert(tree != left);
         assert(tree != right);
      } else {
         tree = (TreeType*) allocator.allocate<TreeType>();
      }
      new (tree) TreeType(this, left, right, value, incrementHeight(left, right));
      m_createdNodes.push_back(tree);
      return tree;
   }

   TreeType* createNode(TreeType *newLeft, TreeType *oldTree, TreeType *newRight)
   {
      return createNode(newLeft, getValue(oldTree), newRight);
   }

   void recoverNodes()
   {
      for (unsigned i = 0, n = m_createdNodes.size(); i < n; ++i) {
         TreeType *node = m_createdNodes[i];
         if (node->isMutable() && node->m_refCount == 0) {
            node->destroy();
         }
      }
      m_createdNodes.clear();
   }

   /// balanceTree - Used by addInternal and removeInternal to
   ///  balance a newly created tree.
   TreeType* balanceTree(TreeType *left, ValueTypeRef value, TreeType *right)
   {
      unsigned hl = getHeight(left);
      unsigned hr = getHeight(right);

      if (hl > hr + 2) {
         assert(!isEmpty(left) && "Left tree cannot be empty to have a height>= 2");

         TreeType *ll = getLeft(left);
         TreeType *lr = getRight(left);

         if (getHeight(ll)>= getHeight(lr)) {
            return createNode(ll, left, createNode(lr, value, right));
         }
         assert(!isEmpty(lr) && "LR cannot be empty because it has a height>= 1");

         TreeType *lrl = getLeft(lr);
         TreeType *lrr = getRight(lr);

         return createNode(createNode(ll, left, lrl), lr, createNode(lrr, value, right));
      }

      if (hr > hl + 2) {
         assert(!isEmpty(right) && "Right tree cannot be empty to have a height>= 2");

         TreeType *rl = getLeft(right);
         TreeType *rr = getRight(right);

         if (getHeight(rr)>= getHeight(rl)) {
            return createNode(createNode(left, value, rl), right, rr);
         }
         assert(!isEmpty(rl) && "RL cannot be empty because it has a height>= 1");

         TreeType *rll = getLeft(rl);
         TreeType *rlr = getRight(rl);

         return createNode(createNode(left, value, rll), rl, createNode(rlr, right, rr));
      }

      return createNode(left, value, right);
   }

   /// addInternal - Creates a new tree that includes the specified
   ///  data and the data from the original tree.  If the original tree
   ///  already contained the data item, the original tree is returned.
   TreeType* addInternal(ValueTypeRef value, TreeType *tree)
   {
      if (isEmpty(tree)) {
         return createNode(tree, value, tree);
      }
      assert(!tree->isMutable());

      KeyTypeRef key = ImutInfo::getKeyOfValue(value);
      KeyTypeRef kCurrent = ImutInfo::getKeyOfValue(getValue(tree));

      if (ImutInfo::isEqual(key, kCurrent)) {
         return createNode(getLeft(tree), value, getRight(tree));
      } else if (ImutInfo::isLess(key, kCurrent)) {
         return balanceTree(addInternal(value, getLeft(tree)), getValue(tree), getRight(tree));
      } else {
         return balanceTree(getLeft(tree), getValue(tree), addInternal(value, getRight(tree)));
      }
   }

   /// removeInternal - Creates a new tree that includes all the data
   ///  from the original tree except the specified data.  If the
   ///  specified data did not exist in the original tree, the original
   ///  tree is returned.
   TreeType* removeInternal(KeyTypeRef key, TreeType *tree)
   {
      if (isEmpty(tree)) {
         return tree;
      }
      assert(!tree->isMutable());
      KeyTypeRef kCurrent = ImutInfo::getKeyOfValue(getValue(tree));
      if (ImutInfo::isEqual(key, kCurrent)) {
         return combineTrees(getLeft(tree), getRight(tree));
      } else if (ImutInfo::isLess(key, kCurrent)) {
         return balanceTree(removeInternal(key, getLeft(tree)),
                            getValue(tree), getRight(tree));
      } else {
         return balanceTree(getLeft(tree), getValue(tree),
                            removeInternal(key, getRight(tree)));
      }
   }

   TreeType* combineTrees(TreeType *left, TreeType *right)
   {
      if (isEmpty(left)) {
         return right;
      }
      if (isEmpty(right)) {
         return left;
      }

      TreeType* oldNode;
      TreeType* newRight = removeMinBinding(right, oldNode);
      return balanceTree(left, getValue(oldNode), newRight);
   }

   TreeType* removeMinBinding(TreeType *tree, TreeType *&noderemoved)
   {
      assert(!isEmpty(tree));
      if (isEmpty(getLeft(tree))) {
         noderemoved = tree;
         return getRight(tree);
      }
      return balanceTree(removeMinBinding(getLeft(tree), noderemoved),
                         getValue(tree), getRight(tree));
   }

   /// markImmutable - Clears the mutable bits of a root and all of its
   ///  descendants.
   void markImmutable(TreeType *tree)
   {
      if (!tree || !tree->isMutable()) {
         return;
      }
      tree->markImmutable();
      markImmutable(getLeft(tree));
      markImmutable(getRight(tree));
   }

public:
   TreeType *getCanonicalTree(TreeType *treeNew)
   {
      if (!treeNew){
         return nullptr;
      }
      if (treeNew->m_isCanonicalized) {
         return treeNew;
      }
      // Search the hashtable for another tree with the same digest, and
      // if find a collision compare those trees by their contents.
      unsigned digest = treeNew->computeDigest();
      TreeType *&entry = m_cache[maskCacheIndex(digest)];
      do {
         if (!entry) {
            break;
         }
         for (TreeType *tree = entry ; tree != nullptr; tree = tree->m_next) {
            // Compare the Contents('T') with Contents('TNew')
            typename TreeType::Iterator titer = tree->begin(), treeEnd = tree->end();
            if (!compareTreeWithSection(treeNew, titer, treeEnd)) {
               continue;
            }
            if (titer != treeEnd) {
               continue; // T has more contents than TNew.
            }
            // Trees did match!  Return 'T'.
            if (treeNew->m_refCount == 0) {
               treeNew->destroy();
            }
            return tree;
         }
         entry->m_prev = treeNew;
         treeNew->m_next = entry;
      }
      while (false);

      entry = treeNew;
      treeNew->m_isCanonicalized = true;
      return treeNew;
   }
};

//===----------------------------------------------------------------------===//
// Immutable AVL-Tree Iterators.
//===----------------------------------------------------------------------===//

template <typename ImutInfo>
class ImutAVLTreeGenericIterator
      : public std::iterator<std::bidirectional_iterator_tag,
      ImutAVLTree<ImutInfo>>
{
   SmallVector<uintptr_t,20> m_stack;

public:
   enum VisitFlag { VisitedNone=0x0, VisitedLeft=0x1, VisitedRight=0x3,
                    Flags=0x3 };

   using TreeType = ImutAVLTree<ImutInfo>;

   ImutAVLTreeGenericIterator() = default;
   ImutAVLTreeGenericIterator(const TreeType *root)
   {
      if (root) {
         m_stack.push_back(reinterpret_cast<uintptr_t>(root));
      }
   }

   TreeType &operator*() const
   {
      assert(!m_stack.empty());
      return *reinterpret_cast<TreeType *>(m_stack.back() & ~Flags);
   }

   TreeType *operator->() const
   {
      return &*this;
   }

   uintptr_t getVisitState() const
   {
      assert(!m_stack.empty());
      return m_stack.back() & Flags;
   }

   bool atEnd() const
   {
      return m_stack.empty();
   }

   bool atBeginning() const
   {
      return m_stack.getSize() == 1 && getVisitState() == VisitedNone;
   }

   void skipToParent()
   {
      assert(!m_stack.empty());
      m_stack.pop_back();
      if (m_stack.empty()) {
         return;
      }
      switch (getVisitState()) {
      case VisitedNone:
         m_stack.back() |= VisitedLeft;
         break;
      case VisitedLeft:
         m_stack.back() |= VisitedRight;
         break;
      default:
         polar_unreachable("Unreachable.");
         break;
      }
   }

   bool operator==(const ImutAVLTreeGenericIterator &other) const
   {
      return m_stack == other.m_stack;
   }

   bool operator!=(const ImutAVLTreeGenericIterator &other) const
   {
      return !(*this == other);
   }

   ImutAVLTreeGenericIterator &operator++()
   {
      assert(!m_stack.empty());
      TreeType* current = reinterpret_cast<TreeType*>(m_stack.back() & ~Flags);
      assert(current);
      switch (getVisitState()) {
      case VisitedNone:
         if (TreeType *leftTree = current->getLeft()) {
            m_stack.push_back(reinterpret_cast<uintptr_t>(leftTree));
         } else {
            m_stack.back() |= VisitedLeft;
         }
         break;
      case VisitedLeft:
         if (TreeType* rightTree = current->getRight()) {
            m_stack.push_back(reinterpret_cast<uintptr_t>(rightTree));
         } else {
            m_stack.back() |= VisitedRight;
         }
         break;
      case VisitedRight:
         skipToParent();
         break;
      default:
         polar_unreachable("Unreachable.");
         break;
      }
      return *this;
   }

   ImutAVLTreeGenericIterator &operator--()
   {
      assert(!m_stack.empty());
      TreeType* current = reinterpret_cast<TreeType*>(m_stack.back() & ~Flags);
      assert(current);
      switch (getVisitState()) {
      case VisitedNone:
         m_stack.pop_back();
         break;
      case VisitedLeft:
         m_stack.back() &= ~Flags; // Set state to "VisitedNone."
         if (TreeType *leftTree = current->getLeft()) {
            m_stack.push_back(reinterpret_cast<uintptr_t>(leftTree) | VisitedRight);
         }
         break;
      case VisitedRight:
         m_stack.back() &= ~Flags;
         m_stack.back() |= VisitedLeft;
         if (TreeType *rightTree = current->getRight()) {
            m_stack.push_back(reinterpret_cast<uintptr_t>(rightTree) | VisitedRight);
         }
         break;
      default:
         polar_unreachable("Unreachable.");
         break;
      }
      return *this;
   }
};

template <typename ImutInfo>
class ImutAVLTreeInOrderIterator
      : public std::iterator<std::bidirectional_iterator_tag,
      ImutAVLTree<ImutInfo>>
{
   using InternalIteratorType = ImutAVLTreeGenericIterator<ImutInfo>;

   InternalIteratorType m_internalIter;

public:
   using TreeType = ImutAVLTree<ImutInfo>;

   ImutAVLTreeInOrderIterator(const TreeType* root) : m_internalIter(root)
   {
      if (root) {
         ++*this; // Advance to first element.
      }
   }

   ImutAVLTreeInOrderIterator() : m_internalIter()
   {}

   bool operator==(const ImutAVLTreeInOrderIterator &other) const
   {
      return m_internalIter == other.m_internalIter;
   }

   bool operator!=(const ImutAVLTreeInOrderIterator &other) const
   {
      return !(*this == other);
   }

   TreeType &operator*() const
   {
      return *m_internalIter;
   }

   TreeType *operator->() const
   {
      return &*m_internalIter;
   }

   ImutAVLTreeInOrderIterator &operator++()
   {
      do ++m_internalIter;
      while (!m_internalIter.atEnd() &&
             m_internalIter.getVisitState() != InternalIteratorType::VisitedLeft);

      return *this;
   }

   ImutAVLTreeInOrderIterator &operator--()
   {
      do --m_internalIter;
      while (!m_internalIter.atBeginning() &&
             m_internalIter.getVisitState() != InternalIteratorType::VisitedLeft);

      return *this;
   }

   void skipSubTree()
   {
      m_internalIter.skipToParent();

      while (!m_internalIter.atEnd() &&
             m_internalIter.getVisitState() != InternalIteratorType::VisitedLeft) {
         ++m_internalIter;
      }
   }
};

/// Generic Iterator that wraps a T::TreeType::Iterator and exposes
/// Iterator::getValue() on dereference.
template <typename T>
struct ImutAVLValueIterator
      : IteratorAdaptorBase<
      ImutAVLValueIterator<T>, typename T::TreeType::Iterator,
      typename std::iterator_traits<
      typename T::TreeType::Iterator>::iterator_category,
      const typename T::ValueType>
{
   ImutAVLValueIterator() = default;
   explicit ImutAVLValueIterator(typename T::TreeType *tree)
      : ImutAVLValueIterator::IteratorAdaptorBase(tree)
   {}

   typename ImutAVLValueIterator::reference operator*() const
   {
      return this->m_iter->getValue();
   }
};

//===----------------------------------------------------------------------===//
// Trait classes for profile information.
//===----------------------------------------------------------------------===//

/// Generic profile template.  The default behavior is to invoke the
/// profile method of an object.  Specializations for primitive integers
/// and generic handling of pointers is done below.
template <typename T>
struct ImutprofileInfo {
   using ValueType = const T;
   using ValueTypeRef = const T&;

   static void profile(FoldingSetNodeId &id, ValueTypeRef other)
   {
      FoldingSetTrait<T>::profile(other, id);
   }
};

/// profile traits for integers.
template <typename T>
struct ImutprofileInteger {
   using ValueType = const T;
   using ValueTypeRef = const T&;

   static void profile(FoldingSetNodeId &id, ValueTypeRef other)
   {
      id.addInteger(other);
   }
};

#define profile_INTEGER_INFO(X)\
   template<> struct ImutprofileInfo<X> : ImutprofileInteger<X> {};

profile_INTEGER_INFO(char)
profile_INTEGER_INFO(unsigned char)
profile_INTEGER_INFO(short)
profile_INTEGER_INFO(unsigned short)
profile_INTEGER_INFO(unsigned)
profile_INTEGER_INFO(signed)
profile_INTEGER_INFO(long)
profile_INTEGER_INFO(unsigned long)
profile_INTEGER_INFO(long long)
profile_INTEGER_INFO(unsigned long long)

#undef profile_INTEGER_INFO

/// profile traits for booleans.
template <>
struct ImutprofileInfo<bool>
{
   using ValueType = const bool;
   using ValueTypeRef = const bool&;

   static void profile(FoldingSetNodeId &id, ValueTypeRef other)
   {
      id.addBoolean(other);
   }
};

/// Generic profile trait for pointer types.  We treat pointers as
/// references to unique objects.
template <typename T>
struct ImutprofileInfo<T*>
{
   using ValueType = const T*;
   using ValueTypeRef = ValueType;

   static void profile(FoldingSetNodeId &id, ValueTypeRef value)
   {
      id.addPointer(value);
   }
};

//===----------------------------------------------------------------------===//
// Trait classes that contain element comparison operators and type
//  definitions used by ImutAVLTree, ImmutableSet, and ImmutableMap.  These
//  inherit from the profile traits (ImutprofileInfo) to include operations
//  for element profiling.
//===----------------------------------------------------------------------===//

/// ImutContainerInfo - Generic definition of comparison operations for
///   elements of immutable containers that defaults to using
///   std::equal_to<> and std::less<> to perform comparison of elements.
template <typename T>
struct ImutContainerInfo : public ImutprofileInfo<T>
{
   using ValueType = typename ImutprofileInfo<T>::ValueType;
   using ValueTypeRef = typename ImutprofileInfo<T>::ValueTypeRef;
   using KeyType = ValueType;
   using KeyTypeRef = ValueTypeRef;
   using DataType = bool;
   using DataTypeRef = bool;

   static KeyTypeRef getKeyOfValue(ValueTypeRef data)
   {
      return data;
   }

   static DataTypeRef getDataOfValue(ValueTypeRef)
   {
      return true;
   }

   static bool isEqual(KeyTypeRef lhs, KeyTypeRef rhs)
   {
      return std::equal_to<KeyType>()(lhs, rhs);
   }

   static bool isLess(KeyTypeRef lhs, KeyTypeRef rhs)
   {
      return std::less<KeyType>()(lhs, rhs);
   }

   static bool isDataEqual(DataTypeRef, DataTypeRef)
   {
      return true;
   }
};

/// ImutContainerInfo - Specialization for pointer values to treat pointers
///  as references to unique objects.  Pointers are thus compared by
///  their addresses.
template <typename T>
struct ImutContainerInfo<T*> : public ImutprofileInfo<T*>
{
   using ValueType = typename ImutprofileInfo<T*>::ValueType;
   using ValueTypeRef = typename ImutprofileInfo<T*>::ValueTypeRef;
   using KeyType = ValueType;
   using KeyTypeRef = ValueTypeRef;
   using DataType = bool;
   using DataTypeRef = bool;

   static KeyTypeRef getKeyOfValue(ValueTypeRef value)
   {
      return value;
   }

   static DataTypeRef getDataOfValue(ValueTypeRef)
   {
      return true;
   }

   static bool isEqual(KeyTypeRef lhs, KeyTypeRef rhs)
   {
      return lhs == rhs;
   }

   static bool isLess(KeyTypeRef lhs, KeyTypeRef rhs)
   {
      return lhs < rhs;
   }

   static bool isDataEqual(DataTypeRef, DataTypeRef)
   {
      return true;
   }
};

//===----------------------------------------------------------------------===//
// Immutable Set
//===----------------------------------------------------------------------===//

template <typename ValType, typename ValInfo = ImutContainerInfo<ValType>>
class ImmutableSet
{
public:
   using ValueType = typename ValInfo::ValueType;
   using ValueTypeRef = typename ValInfo::ValueTypeRef;
   using TreeType = ImutAVLTree<ValInfo>;

private:
   TreeType *m_root;

public:
   /// Constructs a set from a pointer to a tree root.  In general one
   /// should use a Factory object to create sets instead of directly
   /// invoking the constructor, but there are cases where make this
   /// constructor public is useful.
   explicit ImmutableSet(TreeType* root) : m_root(root)
   {
      if (m_root) {
         m_root->retain();
      }
   }

   ImmutableSet(const ImmutableSet &other) : m_root(other.m_root)
   {
      if (m_root) {
         m_root->retain();
      }
   }

   ~ImmutableSet()
   {
      if (m_root) {
         m_root->release();
      }
   }

   ImmutableSet &operator=(const ImmutableSet &other)
   {
      if (m_root != other.m_root) {
         if (other.m_root) {
            other.m_root->retain();
         }
         if (m_root) {
            m_root->release();
         }
         m_root = other.m_root;
      }
      return *this;
   }

   class Factory
   {
      typename TreeType::Factory m_FactoryTypepe;
      const bool m_canonicalize;

   public:
      Factory(bool canonicalize = true)
         : m_canonicalize(canonicalize)
      {}

      Factory(BumpPtrAllocator& alloc, bool canonicalize = true)
         : m_FactoryTypepe(alloc), m_canonicalize(canonicalize)
      {}

      Factory(const Factory &other) = delete;
      void operator=(const Factory &other) = delete;

      /// getEmptySet - Returns an immutable set that contains no elements.
      ImmutableSet getEmptySet()
      {
         return ImmutableSet(m_FactoryTypepe.getEmptyTree());
      }

      /// add - Creates a new immutable set that contains all of the values
      ///  of the original set with the addition of the specified value.  If
      ///  the original set already included the value, then the original set is
      ///  returned and no memory is allocated.  The time and space complexity
      ///  of this operation is logarithmic in the size of the original set.
      ///  The memory allocated to represent the set is released when the
      ///  factory object that created the set is destroyed.
      POLAR_NODISCARD ImmutableSet add(ImmutableSet old, ValueTypeRef value)
      {
         TreeType *newType = m_FactoryTypepe.add(old.m_root, value);
         return ImmutableSet(m_canonicalize ? m_FactoryTypepe.getCanonicalTree(newType) : newType);
      }

      /// remove - Creates a new immutable set that contains all of the values
      ///  of the original set with the exception of the specified value.  If
      ///  the original set did not contain the value, the original set is
      ///  returned and no memory is allocated.  The time and space complexity
      ///  of this operation is logarithmic in the size of the original set.
      ///  The memory allocated to represent the set is released when the
      ///  factory object that created the set is destroyed.
      POLAR_NODISCARD ImmutableSet remove(ImmutableSet old, ValueTypeRef value)
      {
         TreeType *newType = m_FactoryTypepe.remove(old.m_root, value);
         return ImmutableSet(m_canonicalize ? m_FactoryTypepe.getCanonicalTree(newType) : newType);
      }

      BumpPtrAllocator& getAllocator()
      {
         return m_FactoryTypepe.getAllocator();
      }

      typename TreeType::Factory *getTreeFactory() const
      {
         return const_cast<typename TreeType::Factory *>(&m_FactoryTypepe);
      }
   };

   friend class Factory;

   /// Returns true if the set contains the specified value.
   bool contains(ValueTypeRef value) const
   {
      return m_root ? m_root->contains(value) : false;
   }

   bool operator==(const ImmutableSet &other) const
   {
      return m_root && other.m_root ? m_root->isEqual(*other.m_root) : m_root == other.m_root;
   }

   bool operator!=(const ImmutableSet &other) const
   {
      return m_root && other.m_root ? m_root->isNotEqual(*other.m_root) : m_root != other.m_root;
   }

   TreeType *getRoot()
   {
      if (m_root) {
         m_root->retain();
      }
      return m_root;
   }

   TreeType *getRootWithoutRetain() const
   {
      return m_root;
   }

   /// isEmpty - Return true if the set contains no elements.
   bool isEmpty() const
   {
      return !m_root;
   }

   /// isSingleton - Return true if the set contains exactly one element.
   ///   This method runs in constant time.
   bool isSingleton() const
   {
      return getHeight() == 1;
   }

   template <typename Callback>
   void foreach(Callback& callback)
   {
      if (m_root) {
         m_root->foreach(callback);
      }
   }

   template <typename Callback>
   void foreach()
   {
      if (m_root) {
         Callback callback;
         m_root->foreach(callback);
      }
   }

   //===--------------------------------------------------===//
   // Iterators.
   //===--------------------------------------------------===//

   using Iterator = ImutAVLValueIterator<ImmutableSet>;

   Iterator begin() const
   {
      return Iterator(m_root);
   }

   Iterator end() const
   {
      return Iterator();
   }

   //===--------------------------------------------------===//
   // Utility methods.
   //===--------------------------------------------------===//

   unsigned getHeight() const
   {
      return m_root ? m_root->getHeight() : 0;
   }

   static void profile(FoldingSetNodeId &id, const ImmutableSet &set)
   {
      id.addPointer(set.m_root);
   }

   void profile(FoldingSetNodeId &id) const
   {
      return profile(id, *this);
   }

   //===--------------------------------------------------===//
   // For testing.
   //===--------------------------------------------------===//

   void validateTree() const
   {
      if (m_root) {
         m_root->validateTree();
      }
   }
};

// NOTE: This may some day replace the current ImmutableSet.
template <typename ValType, typename ValInfo = ImutContainerInfo<ValType>>
class ImmutableSetRef
{
public:
   using ValueType = typename ValInfo::ValueType;
   using ValueTypeRef = typename ValInfo::ValueTypeRef;
   using TreeType = ImutAVLTree<ValInfo>;
   using FactoryType = typename TreeType::Factory;

private:
   TreeType *m_root;
   FactoryType *m_factory;

public:
   /// Constructs a set from a pointer to a tree root.  In general one
   /// should use a Factory object to create sets instead of directly
   /// invoking the constructor, but there are cases where make this
   /// constructor public is useful.
   explicit ImmutableSetRef(TreeType* tree, FactoryType *factory)
      : m_root(tree),
        m_factory(factory)
   {
      if (m_root) {
         m_root->retain();
      }
   }

   ImmutableSetRef(const ImmutableSetRef &other)
      : m_root(other.m_root),
        m_factory(other.m_factory)
   {
      if (m_root) {
         m_root->retain();
      }
   }

   ~ImmutableSetRef()
   {
      if (m_root) {
         m_root->release();
      }
   }

   ImmutableSetRef &operator=(const ImmutableSetRef &other)
   {
      if (m_root != other.m_root) {
         if (other.m_root) {
            other.m_root->retain();
         }
         if (m_root) {
            m_root->release();
         }
         m_root = other.m_root;
         m_factory = other.m_factory;
      }
      return *this;
   }

   static ImmutableSetRef getEmptySet(FactoryType *factory)
   {
      return ImmutableSetRef(0, factory);
   }

   ImmutableSetRef add(ValueTypeRef value)
   {
      return ImmutableSetRef(m_factory->add(m_root, value), m_factory);
   }

   ImmutableSetRef remove(ValueTypeRef value)
   {
      return ImmutableSetRef(m_factory->remove(m_root, value), m_factory);
   }

   /// Returns true if the set contains the specified value.
   bool contains(ValueTypeRef value) const
   {
      return m_root ? m_root->contains(value) : false;
   }

   ImmutableSet<ValType> asImmutableSet(bool canonicalize = true) const
   {
      return ImmutableSet<ValType>(canonicalize ?
                                      m_factory->getCanonicalTree(m_root) : m_root);
   }

   TreeType *getRootWithoutRetain() const
   {
      return m_root;
   }

   bool operator==(const ImmutableSetRef &other) const
   {
      return m_root && other.m_root? m_root->isEqual(*other.m_root) : m_root == other.m_root;
   }

   bool operator!=(const ImmutableSetRef &other) const
   {
      return m_root && other.m_root ? m_root->isNotEqual(*other.m_root) : m_root != other.m_root;
   }

   /// isEmpty - Return true if the set contains no elements.
   bool isEmpty() const
   {
      return !m_root;
   }

   /// isSingleton - Return true if the set contains exactly one element.
   ///   This method runs in constant time.
   bool isSingleton() const
   {
      return getHeight() == 1;
   }

   //===--------------------------------------------------===//
   // Iterators.
   //===--------------------------------------------------===//

   using Iterator = ImutAVLValueIterator<ImmutableSetRef>;

   Iterator begin() const
   {
      return Iterator(m_root);
   }

   Iterator end() const
   {
      return Iterator();
   }

   //===--------------------------------------------------===//
   // Utility methods.
   //===--------------------------------------------------===//

   unsigned getHeight() const
   {
      return m_root ? m_root->getHeight() : 0;
   }

   static void profile(FoldingSetNodeId &id, const ImmutableSetRef &set)
   {
      id.addPointer(set.m_root);
   }

   void profile(FoldingSetNodeId &id) const
   {
      return profile(id, *this);
   }

   //===--------------------------------------------------===//
   // For testing.
   //===--------------------------------------------------===//

   void validateTree() const
   {
      if (m_root) {
         m_root->validateTree();
      }
   }
};

} // basic
} // polar

#endif //  POLARPHP_BASIC_ADT_IMMUTABLE_SET_H
