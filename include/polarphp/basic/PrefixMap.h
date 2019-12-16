//===--- PrefixMap.h - A ternary tree ---------------------------*- C++ -*-===//
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
//===----------------------------------------------------------------------===//
//
//  This file defines a data structure which stores a map whose keys are
//  sequences of comparable values.  Specifically, it implements the trie
//  variant known as a ternary search tree.  In performance, it is similar
//  to a binary tree; however, it has two properties specific to the use of
//  homogeneous sequences as keys:
//
//    - Individual entries do not necessarily store the entire key; instead,
//      the key data may be spread over a sequence of nodes.  This causes the
//      tree to be much more space-compact when keys share common prefixes.
//      This does require an extra pointer of storage in each node.
//
//      Unlike some traditional presentations of ternary trees, this
//      implementation allows more than one key element per node.
//
//    - It is efficient to find entries that share a common prefix with
//      a given key.
//
//  FIXME: The current implementation doesn't rebalance siblings.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_PREFIXMAP_H
#define POLARPHP_BASIC_PREFIXMAP_H

#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <iterator>

namespace polar {

void print_opaque_prefix_map(raw_ostream &out, void *root,
                             void (*printNode)(raw_ostream &, void*));
template <typename KeyElementType>
class PrefixMapKeyPrinter;

/// A map whose keys are sequences of comparable values, optimized for
/// finding a mapped value for the longest matching initial subsequence.
template <class KeyElementType, class ValueType,
          size_t InlineKeyCapacity
          = std::max<size_t>((sizeof(void*) - 1) / sizeof(KeyElementType), 1)>
class PrefixMap
{
public:
   using KeyType = ArrayRef<KeyElementType>;

   static_assert(std::is_trivially_copyable<KeyElementType>::value,
                 "key element type must be trivially copyable");
   static_assert(std::is_trivially_constructible<KeyElementType>::value,
                 "key element type must be trivially default-initializable");

private:
   template <typename T>
   union UninitializedStorage
   {
      alignas (alignof(T))
      char storage[sizeof(T)];

      template <typename... A>
      void emplace(A && ...value)
      {
         ::new(reinterpret_cast<void*>(&storage)) T(std::forward<A>(value)...);
      }

      T &get()
      {
         return *reinterpret_cast<T*>(storage);
      }

      const T &get() const
      {
         return *reinterpret_cast<T*>(storage);
      }
   };

   // We expect to see a lot of entries for keys like:
   //   ab
   //   abcdef
   //   abcdefg
   //   abcdefh
   // corresponding to closely-related paths and different prefixes
   // of the same path.  This basically demands something like a trie.
   //
   // The rules of our data structure are:
   //   - A node's complete key is the concatenation of its non-local prefix
   //     and its local key.  The non-local prefix is the concatenation of
   //     of the local keys of the further links followed from the root.
   //   - A node's local key contains no common prefix with the local keys of
   //     its left or right siblings.

   struct Node;
   struct NodeBase
   {
      // The initial layout of this struct is assumed in the out-of-line
      // printing code; you'll need to modify it.

      // left and right siblings: nodes which share the same non-local
      // prefix as this one, but which share no common local prefix with it.
      Node *left = nullptr;
      Node *right = nullptr;

      // further children: nodes whose non-local prefix is the concatenation
      // of the non-local prefix of this node and its local key.
      Node *further = nullptr;

      KeyElementType key[InlineKeyCapacity];
      unsigned char keyLength : 7;
      unsigned char hasValue : 1;
      static_assert(InlineKeyCapacity < (1 << 7),
                    "can't store inline key length in bit-field");

      NodeBase()
         : keyLength(0), hasValue(false)
      {}

      KeyType getLocalKey() const
      {
         return { key, keyLength };
      }
   };
   struct Node : NodeBase
   {
   private:
      UninitializedStorage<ValueType> Value;

   public:
      Node() { /* leave Value uninitialized */ }

      // We split NodeBase out so that we can just delegate to something that
      // copies all the other fields.
      Node(const Node &other) : NodeBase(other)
      {
         if (this->hasValue) {
            Value.emplace(other.Value.get());
         }
      }

      ValueType &get()
      {
         assert(this->hasValue);
         return Value.get();
      }

      const ValueType &get() const
      {
         assert(this->hasValue);
         return Value.get();
      }

      template <typename... A>
      void emplace(A && ...value)
      {
         assert(!this->hasValue);
         Value.emplace(std::forward<A>(value)...);
         this->hasValue = true;
      }
   };

   /// Splits a node in two.  The second part must always be non-empty.
   ///
   ///   ref -> cur 'abcdef' -> ...
   /// =>
   ///   ref -> split 'abc' -> cur 'def' -> ...
   ///
   /// Returns the node that stores the common prefix as its key.
   ///
   /// TODO: consider just reorganizing cur and its further node if
   /// cur doesn't have a value or left/right children, e.g.
   ///   ref -> cur 'abcdefg' -> further 'hij'
   /// =>
   ///   ref -> cur 'abc' -> further 'defghij'
   static Node *splitNode(Node **ref, size_t splitIndex)
   {
      auto cur = *ref;
      assert(splitIndex < cur->keyLength &&
             "split index would leave second node with empty key");

      auto split = new Node();
      split->further = cur;

      // Move the sibling links of cur onto split unless we're giving split
      // an empty local key, which is the only case where the siblings will
      // have split's key as a prefix.
      if (splitIndex != 0) {
         split->left = cur->left;
         split->right = cur->right;
         cur->left = nullptr;
         cur->right = nullptr;
      }

      // Initialize the key of the split node.
      split->keyLength = splitIndex;
      memcpy(split->key, cur->key, splitIndex * sizeof(KeyElementType));

      // Slide cur's key if the split point wasn't the start.
      if (splitIndex != 0) {
         cur->keyLength -= splitIndex;
         memmove(cur->key, cur->key + splitIndex,
                 cur->keyLength * sizeof(KeyElementType));
      }

      *ref = split;
      return split;
   }

   /// Try to find a node corresponding to a prefix of the given key.
   ///
   /// If 'remainingLookupKey' is non-null, the returned node will correspond
   /// to the longest prefix that had a value set, and 'remainingLookupKey'
   /// will be set to the part of the key that was not matched.  The tree
   /// will not be modified.
   ///
   /// If 'remainingLookupKey' is null, the returned node will correspond
   /// to the complete lookup key.  This node will be created if necessary.
   Node *getOrCreatePrefixNode(KeyType lookupKey,
                               KeyType *remainingLookupKey)
   {
      // Invariant: 'best' contains the last node we followed the further
      // link of that had a value.  This is used only when not creating
      // an exact match.
      Node *best = nullptr;

      Node **next = &m_root;
      while (Node * const cur = *next) {
         KeyType curKey = cur->getLocalKey();

         // Compare the lookup key with the stored key in the node.
         size_t len = std::min(curKey.size(), lookupKey.size());
         size_t i = 0;
         for (; i != len; ++i) {
            if (lookupKey[i] != curKey[i]) break;
         }

         // If we didn't reach the end of the common length, then we have two
         // basic cases:
         //    looking up 'def' in 'abc' or 'ghi'
         //    looking up 'abd' in 'abc' or 'abce'
         if (i != len) {
            assert(len != 0);
            bool goLeft = (lookupKey[i] < curKey[i]);

            // If there's no common prefix, just go to the appropriate side.
            if (i == 0) {
               next = (goLeft ? &cur->left : &cur->right);
               continue;
            }

            // Otherwise, there's a common prefix, but it's not cur's entire
            // key, so there's no node at that common prefix.  That means that
            // there can't be an exact match.  So if we don't need to return
            // one, we can just stop here.
            if (remainingLookupKey)
               return best;

            // Otherwise, we need to split cur at the appropriate place.
            Node *split = splitNode(next, i);

            // Continue from a sibling of its further link.
            lookupKey = lookupKey.slice(i);
            next = (goLeft ? &split->further->left : &split->further->right);
            assert(*next == nullptr);
            break;
         }

         // If we did reach the end of the common length, then we have three
         // cases:
         //   looking up 'abc' in 'abc'
         //   looking up 'abc' in 'abcdef'
         //   looking up 'abc' in 'ab'

         // We might have exhausted the node's local key.
         // (This could be empty.)
         if (len == curKey.size()) {
            lookupKey = lookupKey.slice(len);

            // Remember this as the best mapped match if it has a value.
            if (remainingLookupKey && cur->hasValue) {
               best = cur;
               *remainingLookupKey = lookupKey;
            }

            // If we've exhausted the lookup key, too, we have an exact match.
            if (lookupKey.empty()) {
               return (remainingLookupKey ? best : cur);
            }

            // Otherwise, we have a suffix match; continue along the further
            // path.
            next = &cur->further;
            continue;
         }

         // Otherwise, we matched a prefix of the node's key.
         assert(lookupKey.size() < curKey.size());

         // If we don't need to create an exact node, don't claim anything
         // more from the key and just use our deepest match.
         if (remainingLookupKey) {
            return best;
         }

         // Okay, we're supposed to return a node that exactly matches the key.
         // Split 'cur'.
         Node *split = splitNode(next, len);
         return split;
      }

      // Okay, we reached the end of our search.  If we don't need an exact
      // match, return our best match so far.
      if (remainingLookupKey) {
         return best;
      }

      // Otherwise, create nodes until we're out of lookup key.
      // TODO: balance the subtree when creating nodes to the left or right.
      do {
         best = new Node();
         *next = best;
         next = &best->further;

         auto len = std::min(InlineKeyCapacity, lookupKey.size());
         best->keyLength = len;
         memcpy(best->key, lookupKey.data(), len * sizeof(KeyElementType));
         lookupKey = lookupKey.slice(len);
      } while (!lookupKey.empty());

      return best;
   }

   /// Delete all the nodes in a subtree.
   static void deleteTree(Node *root) {
      if (!root) return;

      SmallVector<Node *, 8> stack;
      auto pushChildrenAndDelete = [&](Node *node) {
         if (node->left) stack.push_back(node->left);
         if (node->right) stack.push_back(node->right);
         if (node->further) stack.push_back(node->further);
         delete node;
      };
      pushChildrenAndDelete(root);
      while (!stack.empty()) {
         pushChildrenAndDelete(stack.pop_back_val());
      }
   }

   /// Copy all the nodes in a subtree.
   static Node *cloneTree(Node *root) {
      if (!root) return nullptr;

      SmallVector<Node **, 8> stack;
      auto copyAndPushChildren = [&](Node **ptr) {
         assert(*ptr);
         Node *copy = new Node(**ptr);
         *ptr = copy;
         if (copy->left) stack.push_back(&copy->left);
         if (copy->right) stack.push_back(&copy->right);
         if (copy->further) stack.push_back(&copy->further);
      };
      copyAndEnqueueChildren(&root);
      while (!stack.empty()) {
         copyAndEnqueueChildren(stack.pop_back_val());
      }
      return root;
   }

   Node *m_root = nullptr;

public:
   PrefixMap() {}

   PrefixMap(const PrefixMap &other)
      : m_root(cloneTree(other.m_root))
   {}

   PrefixMap(PrefixMap &&other)
      : m_root(other.m_root)
   {
      other.m_root = nullptr;
   }

   PrefixMap &operator=(const PrefixMap &other)
   {
      deleteTree(m_root);
      m_root = cloneTree(other.m_root);
      return *this;
   }

   PrefixMap &operator=(PrefixMap &&other)
   {
      deleteTree(m_root);
      m_root = other.m_root;
      other.m_root = nullptr;
      return *this;
   }

   ~PrefixMap()
   {
      deleteTree(m_root);
   }

   /// Are there any entries in this map?
   bool empty() const
   {
      // The only way to create nodes is to insert an entry, and we don't
      // yet support delete, so having any nodes means we're non-empty.
      return m_root == nullptr;
   }

   /// Return the number of entries in this map.
   size_t size() const
   {
      if (!m_root) {
         return 0;
      }
      size_t result = 0;
      SmallVector<Node *, 16> stack;
      auto handleNode = [&](Node *node) {
         if (node->hasValue) result++;
         if (node->left) stack.push_back(node->left);
         if (node->right) stack.push_back(node->right);
         if (node->further) stack.push_back(node->further);
      };
      handleNode(m_root);
      while (!stack.empty()) {
         handleNode(stack.pop_back_val());
      }
      return result;
   }

   /// An input iterator over the entries in the map.
   ///
   /// This iterator stores a stack of the access path to the entry and
   /// is therefore expensive to copy, and its operator* returns a proxy
   /// object with a reference to its internal storage.  These are both
   /// legal for C++ input iterators, but still, be careful.
   class const_iterator
   {
   protected:
      enum Position
      {
         /// We are visiting the node's left subtree.
         left,
         /// If the node is on the top of the stack, we are visiting its value.
         /// Otherwise, we are visiting its further subtree.
         further,
         // We remove the node from the stack when visiting its right subtree.
      };
      using NodeAndPosition = llvm::PointerIntPair<Node*, 1, Position>;
      SmallVector<NodeAndPosition, 8> m_stack;
   public:
      const_iterator() {}
      explicit const_iterator(Node *node)
      {
         enter(node);
      }

      /// A proxy object referencing a valid entry in the map.
      class ConstEntryProxy
      {
         ArrayRef<NodeAndPosition> m_path;
      public:
         explicit ConstEntryProxy(ArrayRef<NodeAndPosition> path)
            : m_path(path)
         {
            assert(!m_path.empty() && m_path.back().getPointer()->hasValue);
         }

         /// Return the value of the entry.  The returned reference is valid
         /// as long as the entry remains in the map.
         const ValueType &getValue() const
         {
            return m_path.back().getPointer()->get();
         }

         /// Read the value's key into the given buffer.
         KeyType getKey(SmallVectorImpl<KeyElementType> &buffer) const
         {
            buffer.clear();
            for (auto &entry : m_path.drop_back()) {
               if (entry.getInt() != further) continue;
               auto key = entry.getPointer()->getLocalKey();
               buffer.append(key.begin(), key.end());
            }
            auto key = m_path.back().getPointer()->getLocalKey();
            buffer.append(key.begin(), key.end());
            return buffer;
         }
      };

      /// Return a proxy value for the entry.  The returned proxy is invalidated
      /// by any change to the underlying iterator.
      ConstEntryProxy operator*() const
      {
         assert(!m_stack.empty());
         assert(m_stack.back().getPointer()->hasValue);
         return ConstEntryProxy(m_stack);
      }

      /// Advance the iterator.
      const_iterator &operator++()
      {
         assert(!m_stack.empty());

         while (true) {
            if (m_stack.back().getInt() == left) {
               m_stack.back().setInt(further);
               if (m_stack.back().getPointer()->hasValue) return *this;
            }
            if (enter(m_stack.back().getPointer()->further)) return *this;

            do {
               Node *top = m_stack.pop_back_val().getPointer();
               if (enter(top->right)) return *this;
               if (m_stack.empty()) return *this;
            } while (m_stack.back().getInt() == further);
         }
      }

      const_iterator operator++(int)
      {
         const_iterator copy = *this;
         operator++();
         return copy;
      }

      bool operator==(const const_iterator &other) const
      {
         return m_stack == other.m_stack;
      }

      bool operator!=(const const_iterator &other) const
      {
         return !operator==(other);
      }

      using difference_type = ptrdiff_t;
      using iterator_category = std::input_iterator_tag;
      using value_type = ConstEntryProxy;
      using pointer = ConstEntryProxy;
      using reference = ConstEntryProxy;

   private:
      bool enter(Node *node)
      {
         if (!node) {
            return false;
         }
         m_stack.push_back({node, left});
         if (enter(node->left)) {
            return true;
         }
         m_stack.back().setInt(further);
         if (node->hasValue) {
            return true;
         }
         if (enter(node->further)) {
            return true;
         }
         m_stack.pop_back();
         return enter(node->right);
      }
   };

   /// An input iterator over the entries in this map.
   ///
   /// This iterator stores a stack of the access path to the entry and
   /// is therefore expensive to copy, and its operator* returns a proxy
   /// object with a reference to its internal storage.  These are both
   /// legal for C++ input iterators, but still, be careful.
   struct iterator : const_iterator
   {
      iterator() : const_iterator() {}
      explicit iterator(Node *node) : const_iterator(node) {}

      struct EntryProxy : const_iterator::ConstEntryProxy
      {
         explicit EntryProxy(
               ArrayRef<typename const_iterator::NodeAndPosition> path)
            : const_iterator::ConstEntryProxy(path) {}
         ValueType &getValue() const
         {
            return const_cast<ValueType &>(
                     const_iterator::ConstEntryProxy::getValue());
         }
      };

      EntryProxy operator*() const
      {
         return EntryProxy(this->m_stack);
      }

      iterator &operator++()
      {
         return static_cast<iterator&>(const_iterator::operator++());
      }

      iterator operator++(int)
      {
         iterator copy = *this;
         operator++();
         return copy;
      }

      // Base implementations of operator== are fine.

      using value_type = EntryProxy;
      using pointer = EntryProxy;
      using reference = EntryProxy;
   };

   iterator begin()
   {
      return iterator(m_root);
   }

   iterator end()
   {
      return iterator();
   }

   const_iterator begin() const
   {
      return const_iterator(m_root);
   }

   const_iterator end() const
   {
      return const_iterator();
   }

   /// A handle to the mapping for a given key.  Only invalidated by
   /// changes that remove the mapping.
   class Handle
   {
      Node *m_ptr;
   public:
      Handle() : m_ptr(nullptr) {}
      explicit Handle(Node *ptr) : m_ptr(ptr) {}

      explicit operator bool() const
      {
         return m_ptr != nullptr;
      }

      ValueType &operator*() const
      {
         return m_ptr->get();
      }
   };

   using KeyIterator = typename KeyType::iterator;

   /// Find the longest prefix of the given encoded sequence which has
   /// an entry in this map, and return an iterator corresponding to the
   /// end of the prefix.
   std::pair<Handle, KeyIterator>
   findPrefix(KeyType key) const
   {
      KeyType remainingKey;
      auto node = const_cast<PrefixMap*>(this)
            ->getOrCreatePrefixNode(key, &remainingKey);
      assert(!node || node->hasValue);
      return { Handle(node), remainingKey.begin() };
   }

   /// Remove all entries in the map.
   void clear()
   {
      deleteTree(m_root);
      m_root = nullptr;
   }

   /// Get or create an entry in the map.
   ///
   /// \return a handle to the entry and a bool indicating (if true)
   ///   that the map was modified to insert the mapping.
   template <typename Fn>
   std::pair<Handle, bool>
   insertLazy(KeyType key, const Fn &create)
   {
      auto node = getOrCreatePrefixNode(key, nullptr);
      assert(node);
      if (node->hasValue) {
         return { Handle(node), false };
      } else {
         node->emplace(create());
         return { Handle(node), true };
      }
   }

   std::pair<Handle, bool>
   insert(KeyType key, ValueType &&value)
   {
      return insertLazy(key,
                        [&]() -> ValueType && { return std::move(value); });
   }

   std::pair<Handle, bool>
   insert(KeyType key, const ValueType &value)
   {
      return insertLazy(key,
                        [&]() -> const ValueType & { return value; });
   }

   /// Insert a new entry into the map, asserting that it doesn't
   /// already exist.
   template <typename Fn>
   Handle insertNewLazy(KeyType key, const Fn &create)
   {
      auto node = getOrCreatePrefixNode(key, nullptr);
      assert(node);
      node->emplace(create());
      return Handle(node);
   }

   Handle insertNew(KeyType key, ValueType &&value)
   {
      return insertNewLazy(key,
                           [&]() -> ValueType && { return std::move(value); });
   }

   Handle insertNew(KeyType key, const ValueType &value)
   {
      return insertNewLazy(key,
                           [&]() -> const ValueType & { return value; });
   }

   void dump() const
   {
      print(llvm::errs());
   }

   void print(raw_ostream &out) const
   {
      print_opaque_prefix_map(out, m_root,
                              [](raw_ostream &out, void *_node) {
         Node *node = reinterpret_cast<Node*>(_node);
         PrefixMapKeyPrinter<KeyElementType>::print(out, node->getLocalKey());
         if (node->hasValue) {
            out << " (" << node->get() << ')';
         }
      });
   }
};

/// The standard implementation of a key printer:
///   {0,1,2,3}
template <typename KeyElementType>
class PrefixMapKeyPrinter
{
public:
   static void print(raw_ostream &out, ArrayRef<KeyElementType> key) {
      out << '{';
      for (size_t i = 0; i != key.size(); ++i) {
         if (i != 0) out << ',';
         out << key[i];
      }
      out << '}';
   }
};

/// A key printer for char sequences that prints as a quoted string:
///   "hello, \"world\""
template <>
class PrefixMapKeyPrinter<char>
{
public:
   static void print(raw_ostream &out, ArrayRef<char> key);
};

/// A key printer for byte sequences that prints as a byte-string:
//   '0F346E'
template <>
class PrefixMapKeyPrinter<unsigned char>
{
public:
   static void print(raw_ostream &out, ArrayRef<unsigned char> key);
};

} //  polar

#endif // POLARPHP_BASIC_PREFIXMAP_H
