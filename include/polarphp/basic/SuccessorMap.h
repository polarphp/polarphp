//===--- SuccessorMap.h - Find the first mapped successor -------*- C++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/02.
//
// A data structure which maps from a discrete ordered domain (e.g.
// 'unsigned') to an arbitrary value type.  It provides two core
// operations:
//
//   - setting a value for an unmapped key
//   - find the value for the smallest mapped key that is larger than a
//     given unmapped key
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_SUCCESSORMAP_H
#define POLARPHP_BASIC_SUCCESSORMAP_H

#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

namespace polar {

/// Traits for a key type.  The default implementation is suitable for
/// a fundamental discrete type like 'unsigned'.
template <typename T>
struct SuccessorMapTraits
{
   static bool equals(const T &lhs, const T &rhs)
   {
      return lhs == rhs;
   }

   static bool precedes(const T &lhs, const T &rhs)
   {
      return lhs < rhs;
   }

   static T getSuccessor(const T &value)
   {
      return value + 1;
   }
};

/// A successor map.  Not a STL-style map.
template <typename K, typename V, typename Traits = SuccessorMapTraits<K>>
class SuccessorMap
{
   struct Node
   {
      Node(K &&key, V &&value)
         : begin(std::move(key)),
           end(Traits::getSuccessor(begin)),
           value(std::move(value))
      {}

      Node(const K &key, const V &value)
         : begin(key),
           end(Traits::getSuccessor(begin)),
           value(value)
      {}

      Node *left = nullptr;
      Node *right = nullptr;

      /// A half-open range.
      K begin;
      K end;
      V value;

      void dump() const
      {
         dumpNode(this);
      }
   };

   // The entire tree is uniquely owned by the map object.
   Node *m_root = nullptr;

public:
   SuccessorMap() {}

   SuccessorMap(SuccessorMap &&other)
      : m_root(other.m_root)
   {
      other.m_root = nullptr;
   }

   SuccessorMap &operator=(SuccessorMap &&other)
   {
      clear();
      m_root = other.m_root;
      other.m_root = nullptr;
   }

   SuccessorMap(const SuccessorMap &other)
      : m_root(copyTree(other.m_root))
   {}

   SuccessorMap &operator=(const SuccessorMap &other)
   {
      // TODO: this is clearly optimizable to re-use nodes.
      deleteTree(m_root);
      m_root = copyTree(other.m_root);
   }

   ~SuccessorMap()
   {
      deleteTree(m_root);
   }

   void clear()
   {
      deleteTree(m_root);
      m_root = nullptr;
   }

   template <typename KeyTy, typename valueTy>
   void insert(KeyTy &&key, valueTy &&value)
   {
      // Splay to find the greatest lower and least upper bounds.
      bool haveUpperBound = splay(key);
      Node *upperBound = (haveUpperBound ? m_root : nullptr);
      Node *lowerBound = (haveUpperBound ? m_root->left : m_root);
      assert(haveUpperBound == (upperBound != nullptr));
      assert(!lowerBound || !lowerBound->right);

      // Try to add this key to the end of the lower bound.
      assert(!upperBound || Traits::precedes(key, upperBound->begin));
      assert(!lowerBound || !Traits::precedes(key, lowerBound->end));

      // If the key is the end of the left child, append to it,
      // dropping the inserted value on the floor.
      if (lowerBound && Traits::equals(lowerBound->end, key)) {
         lowerBound->end = Traits::getSuccessor(lowerBound->end);

         // If the end of the lower bound is now the same as the
         // beginning of the upper bound, combine the nodes.
         if (upperBound && Traits::equals(lowerBound->end, upperBound->begin)) {
            lowerBound->end = std::move(upperBound->end);
            lowerBound->right = upperBound->right;
            assert(upperBound->left == lowerBound);
            m_root = lowerBound;
            delete upperBound;
         }
         return;
      }

      // Otherwise, if the key immediately precedes the beginning of the
      // upper bound, prepend to it.
      auto keySuccessor = Traits::getSuccessor(key);
      if (upperBound && Traits::equals(keySuccessor, upperBound->begin)) {
         upperBound->begin = std::move(keySuccessor);
         upperBound->value = std::forward<valueTy>(value);
         return;
      }

      // Otherwise, create a new node.
      m_root = new Node(std::forward<KeyTy>(key), std::forward<valueTy>(value));
      m_root->left = lowerBound;
      m_root->right = upperBound;
      if (upperBound) upperBound->left = nullptr;
   }

   /// Find the address of the stored value corresponding to the
   /// smallest key larger than the given one, or return a null pointer
   /// if the key is larger than anything in the map.
   V *findLeastUpperBound(const K &key)
   {
      if (splay(key)) {
         return &m_root->value;
      } else {
         return nullptr;
      }
   }

   /// Validate the well-formedness of this data structure.
   void validate() const
   {
#ifndef NDEBUG
      if (m_root) {
         validateNode(m_root, None, None);
      }
#endif
   }

   void dump() const
   {
      // We call dump() on the object instead of using dumpNode here so
      // that the former will be available in a debug build as long as
      // something in the program calls dump on the collection.
      if (m_root) m_root->dump();
      else llvm::errs() << "(empty)\n";
   }

private:
   /// Perform a top-down splay operation, attempting to set things up
   /// so that m_root is the least upper bound and its left child is the
   /// greatest lower bound.  The only time that's not satisfiable is
   /// if the key is larger than anything in the map.
   ///
   /// We assume that the key is not mapped.
   ///
   /// \return true if the root is now the least upper bound and its
   ///   left child (if present) is the greatest lower bound
   bool splay(const K &key)
   {
      if (!m_root) {
         return false;
      }

      // The root of the current subtree.
      Node *cur = m_root;

      // The root of the tree of nodes that are larger than the current
      // subtree, and the address of the empty slot on its far left arm.
      Node *upperTree = nullptr;
      Node **upperleftmost = &upperTree;

      // The root of the tree of nodes that are smaller than the current
      // subtrees, and the address of the empty slot on its far right arm.
      // As an invariant, this tree is always either empty or has no right
      // subtree.
      Node *lowerTree = nullptr;
      Node **lowerrightmost = &lowerTree;

      // Rotate a node in to become the new root of the lower tree.
      // Its right child must be clear.
      auto rotateAsLowerm_root = [&](Node *node) {
         assert(*lowerrightmost == nullptr);
         // The left child goes in the rightmost position of the old lower tree.
         // The right child gets dropped, and its position is the new rightmost
         // position.
         *lowerrightmost = node->left;
         lowerrightmost = &node->right;
         node->left = lowerTree;
         node->right = nullptr;
         lowerTree = node;
      };

      // Put a node in the leftmost position of the upper tree.
      auto placeInUpperleftmost = [&](Node *node) {
         assert(*upperleftmost == nullptr);
         assert(node->left == nullptr);
         *upperleftmost = node;
         upperleftmost = &node->left;
      };

      // A helper function to re-assemble the root node.  Tail-called on
      // all exit paths from splay().
      auto reassemble = [&](bool foundUpperBound) {
         assert(*lowerrightmost == nullptr);
         assert(*upperleftmost == nullptr);
         *lowerrightmost = cur->left;
         cur->left = lowerTree;
         *upperleftmost = cur->right;
         cur->right = upperTree;
         m_root = cur;
         assert(!foundUpperBound ||
                m_root->left == nullptr ||
                m_root->left->right == nullptr);
         return foundUpperBound;
      };

      // A helper to finish the operation, given that 'cur' is an upper bound.
      auto finishWithUpperBound = [&] {
         assert(cur->left == nullptr);
         return reassemble(true);
      };

      // A helper to finish the operation, given that there is no upper
      // bound in the 'cur' subtree.
      auto finishWithoutUpperBound = [&] {
         assert(cur->right == nullptr);

         // If the upper tree is empty, we really don't have an upper bound.
         if (!upperTree) return reassemble(false);

         // Otherwise, pull the leftmost leaf off the upper tree to
         // become the new root.
         Node **leafPosition = &upperTree;
         while ((*leafPosition)->left) {
            leafPosition = &(*leafPosition)->left;
         }
         Node *newRoot = *leafPosition;
         *leafPosition = newRoot->right;
         newRoot->right = nullptr;

         // Adjust upperleftmost.
         while (*leafPosition) leafPosition = &(*leafPosition)->left;
         upperleftmost = leafPosition;
         rotateAsLowerm_root(cur);
         cur = newRoot;
         return finishWithUpperBound();
      };

      while (true) {
         assert(cur);
         assert(lowerTree != nullptr || lowerrightmost == &lowerTree);
         assert(lowerTree == nullptr || lowerrightmost == &lowerTree->right);
         assert(*lowerrightmost == nullptr);
         assert(*upperleftmost == nullptr);

         // Check if we should recurse into the left subtree.
         if (Traits::precedes(key, cur->begin)) {
            // We should.  If the left subtree is empty, then 'cur' is our
            // least upper bound.
            auto left = cur->left;
            if (!left) return finishWithUpperBound();

            // Otherwise, check if we should recurse into the left-left subtree.
            if (Traits::precedes(key, left->begin)) {
               // We should.  If the left-left subtree is empty, then 'left'
               // is our least upper bound.  Zig left.
               auto leftleft = left->left;
               if (!leftleft) {
                  cur->left = nullptr;
                  placeInUpperleftmost(cur);
                  cur = left;
                  return finishWithUpperBound();
               }

               // Otherwise, zig-zig left.
               cur->left = left->right;
               left->right = cur;
               left->left = nullptr;
               placeInUpperleftmost(left);
               cur = leftleft;
               continue;
            }
            assert(!Traits::precedes(key, left->end) && "key already mapped!");

            // We should recurse into the left-right subtree.  In either
            // case, break off 'left' as the new root of the lower-bound tree.
            auto leftright = left->right;
            rotateAsLowerm_root(left);
            cur->left = nullptr;

            // If the left-right subtree is empty, then 'cur' is our least
            // upper bound.
            if (!leftright) return finishWithUpperBound();

            // Otherwise, complete the zig-zag left and continue.
            placeInUpperleftmost(cur);
            cur = leftright;
            continue;
         }
         assert(!Traits::precedes(key, cur->end) && "key already mapped!");

         // We should recurse into the right subtree. If that's empty,
         // we're done, and the subtree has no upper bound for the key.
         auto right = cur->right;
         if (!right) return finishWithoutUpperBound();

         // Check whether we should recurse into the right-left subtree.
         if (Traits::precedes(key, right->begin)) {
            // We should.  In either case, we need to rotate 'cur' to
            // become the new root of the lower tree.
            rotateAsLowerm_root(cur);

            // If the right-left subtree is empty, then 'right' is the
            // least upper bound.  Zig right.
            auto rightleft = right->left;
            if (!rightleft) {
               cur = right;
               return finishWithUpperBound();
            }

            // Otherwise, complete the zig-zag right and continue.
            right->left = nullptr;
            placeInUpperleftmost(right);
            cur = rightleft;
            continue;
         }
         assert(!Traits::precedes(key, right->end) && "key already mapped!");

         // We should recurse into the right-right subtree.  If that's
         // empty, we're done, and the subtree has no upper bound for the
         // key.  Zig right.
         auto rightright = right->right;
         if (!rightright) {
            rotateAsLowerm_root(cur);
            cur = right;
            return finishWithoutUpperBound();
         }

         // Otherwise, zig-zig right and continue.
         cur->right = right->left;
         right->left = cur;
         rotateAsLowerm_root(right);
         cur = rightright;
         continue;
      }
   }

#ifndef NDEBUG
   /// Validate that the node is well-formed and that all of its keys
   /// (and those of its children) fall (non-inclusively) between
   /// lowerBound and upperBound-1.
   static void validateNode(Node *node,
                            Optional<K> lowerBound,
                            Optional<K> upperBound)
   {
      // The node cannot have an empty key range.
      assert(Traits::precedes(node->begin, node->end));

      // The first key must be strictly higher than the lower bound.
      if (lowerBound.hasvalue()) {
         assert(Traits::precedes(lowerBound.getvalue(), node->begin));
      }
      // The last key (i.e. end-1) must be strictly lower than
      // upperBound-1, or in other words, end must precede upperBound.
      if (upperBound.hasvalue()) {
         assert(Traits::precedes(node->end, upperBound.getvalue()));
      }
      // The keys in the left sub-tree must all be strictly less than
      // begin-1, because if any key equals begin-1, that node should
      // have been merged into this one.
      if (node->left) {
         validateNode(node->left, lowerBound, node->begin);
      }

      // The keys in the right sub-tree must all be strictly greater
      // than end, because if any key equals end, that node should have
      // been merged into this one.
      if (node->right) {
         validateNode(node->right, node->end, upperBound);
      }
   }
#endif

   static void dumpNode(const Node *node)
   {
      dumpNode(node, 0);
   }

   static void dumpNode(const Node *node, unsigned indent)
   {
      llvm::errs().indent(indent);
      if (!node) {
         llvm::errs() << "(null)\n";
         return;
      }

      llvm::errs() << node->begin << ".." << node->end
                   << ": " << node->value << "\n";
      dumpNode(node->left, indent + 2);
      dumpNode(node->right, indent + 2);
   }

   /// Delete all the nodes in the given sub-tree.
   static void deleteTree(Node *root)
   {
      llvm::SmallVector<Node*, 16> queue; // actually a stack
      auto enqueue = [&](Node *n) {
         if (n) queue.push_back(n);
      };

      enqueue(root);
      while (!queue.empty()) {
         auto cur = queue.pop_back_val();
         enqueue(cur->left);
         enqueue(cur->right);
         delete cur;
      }
   }

   static Node *copyTree(Node *oldRoot)
   {
      // A list of nodes which have been cloned, but whose children
      // haven't yet been cloned.
      llvm::SmallVector<Node*, 16> worklist;

      auto cloneAtPosition = [&](Node *&position) {
         Node *oldNode = position;
         if (!oldNode) return;
         Node *newNode = new Node(*oldNode);
         position = newNode;
         worklist.push_back(newNode);
      };

      Node *newRoot = oldRoot;
      cloneAtPosition(newRoot);
      while (!worklist.empty()) {
         auto node = worklist.pop_back_val();
         cloneAtPosition(node->left);
         cloneAtPosition(node->right);
      }

      return newRoot;
   }
};

} // polar

#endif // POLARPHP_BASIC_SUCCESSORMAP_H
