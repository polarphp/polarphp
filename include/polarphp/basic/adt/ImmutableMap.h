// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018  polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of  polarPHP project authors
//
// Created by polarboy on 2018/06/23.

#ifndef  POLARPHP_BASIC_ADT_IMMUTABLE_MAP_H
#define  POLARPHP_BASIC_ADT_IMMUTABLE_MAP_H

#include "polarphp/basic/adt/FoldingSet.h"
#include "polarphp/basic/adt/ImmutableSet.h"
#include "polarphp/utils/Allocator.h"
#include <utility>

namespace polar {
namespace basic {

/// ImutKeyValueInfo -Traits class used by ImmutableMap.  While both the first
/// and second elements in a pair are used to generate profile information,
/// only the first element (the key) is used by isEqual and isLess.
template <typename T, typename S>
struct ImutKeyValueInfo
{
   using ValueType = const std::pair<T,S>;
   using ValueTypeRef = const ValueType&;
   using KeyType = const T;
   using KeyTypeRef = const T&;
   using DataType = const S;
   using DataTypeRef = const S&;

   static inline KeyTypeRef getKeyOfValue(ValueTypeRef value)
   {
      return value.first;
   }

   static inline DataTypeRef getDataOfValue(ValueTypeRef value)
   {
      return value.second;
   }

   static inline bool isEqual(KeyTypeRef lhs, KeyTypeRef rhs)
   {
      return ImutContainerInfo<T>::isEqual(lhs, rhs);
   }

   static inline bool isLess(KeyTypeRef lhs, KeyTypeRef rhs)
   {
      return ImutContainerInfo<T>::isLess(lhs, rhs);
   }

   static inline bool isDataEqual(DataTypeRef lhs, DataTypeRef rhs)
   {
      return ImutContainerInfo<S>::isEqual(lhs, rhs);
   }

   static inline void profile(FoldingSetNodeId &id, ValueTypeRef value)
   {
      ImutContainerInfo<T>::profile(id, value.first);
      ImutContainerInfo<S>::profile(id, value.second);
   }
};

template <typename KeyT, typename ValueT,
          typename ValInfo = ImutKeyValueInfo<KeyT, ValueT>>
class ImmutableMap
{
public:
   using ValueType = typename ValInfo::ValueType;
   using ValueTypeRef = typename ValInfo::ValueTypeRef;
   using KeyType = typename ValInfo::KeyType;
   using KeyTypeRef = typename ValInfo::KeyTypeRef;
   using DataType = typename ValInfo::DataType;
   using DataTypeRef = typename ValInfo::DataTypeRef;
   using TreeType = ImutAVLTree<ValInfo>;

protected:
   TreeType* m_root;

public:
   /// Constructs a map from a pointer to a tree root.  In general one
   /// should use a Factory object to create maps instead of directly
   /// invoking the constructor, but there are cases where make this
   /// constructor public is useful.
   explicit ImmutableMap(const TreeType *root) : m_root(const_cast<TreeType*>(root))
   {
      if (m_root) {
         m_root->retain();
      }
   }

   ImmutableMap(const ImmutableMap &other) : m_root(other.m_root)
   {
      if (m_root) {
         m_root->retain();
      }
   }

   ~ImmutableMap()
   {
      if (m_root) {
         m_root->release();
      }
   }

   ImmutableMap &operator=(const ImmutableMap &other)
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
      typename TreeType::Factory m_factory;
      const bool m_canonicalize;

   public:
      Factory(bool canonicalize = true) : m_canonicalize(canonicalize)
      {}

      Factory(BumpPtrAllocator &alloc, bool canonicalize = true)
         : m_factory(alloc), m_canonicalize(canonicalize)
      {}

      Factory(const Factory &) = delete;
      Factory &operator=(const Factory &) = delete;

      ImmutableMap getEmptyMap()
      {
         return ImmutableMap(m_factory.getEmptyTree());
      }

      POLAR_NODISCARD ImmutableMap add(ImmutableMap old, KeyTypeRef key,
                                       DataTypeRef data)
      {
         TreeType *tree = m_factory.add(old.m_root, std::pair<KeyType,DataType>(key, data));
         return ImmutableMap(m_canonicalize ? m_factory.getCanonicalTree(tree): tree);
      }

      POLAR_NODISCARD ImmutableMap remove(ImmutableMap old, KeyTypeRef key)
      {
         TreeType *tree = m_factory.remove(old.m_root, key);
         return ImmutableMap(m_canonicalize ? m_factory.getCanonicalTree(tree): tree);
      }

      typename TreeType::Factory *getTreeFactory() const
      {
         return const_cast<typename TreeType::Factory *>(&m_factory);
      }
   };

   bool contains(KeyTypeRef key) const
   {
      return m_root ? m_root->contains(key) : false;
   }

   bool operator==(const ImmutableMap &other) const
   {
      return m_root && other.m_root ? m_root->isEqual(*other.m_root) : m_root == other.m_root;
   }

   bool operator!=(const ImmutableMap &other) const
   {
      return m_root && other.m_root ? m_root->isNotEqual(*other.m_root) : m_root != other.m_root;
   }

   TreeType *getRoot() const
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

   void manualRetain()
   {
      if (m_root) {
         m_root->retain();
      }
   }

   void manualRelease()
   {
      if (m_root) {
         m_root->release();
      }
   }

   bool isEmpty() const
   {
      return !m_root;
   }

   //===--------------------------------------------------===//
   // Foreach - A limited form of map iteration.
   //===--------------------------------------------------===//

private:
   template <typename Callback>
   struct CBWrapper
   {
      Callback m_callback;
      void operator()(ValueTypeRef value)
      {
         m_callback(value.first, value.second);
      }
   };

   template <typename Callback>
   struct CBWrapperRef {
      Callback &m_callback;

      CBWrapperRef(Callback& callback) : m_callback(callback)
      {}

      void operator()(ValueTypeRef value)
      {
         m_callback(value.first, value.second);
      }
   };

public:
   template <typename Callback>
   void foreach(Callback &callback)
   {
      if (m_root) {
         CBWrapperRef<Callback> callbackWrapper(callback);
         m_root->foreach(callbackWrapper);
      }
   }

   template <typename Callback>
   void foreach()
   {
      if (m_root) {
         CBWrapper<Callback> callbackWrapper;
         m_root->foreach(callbackWrapper);
      }
   }

   //===--------------------------------------------------===//
   // For testing.
   //===--------------------------------------------------===//

   void verify() const
   {
      if (m_root) {
         m_root->verify();
      }
   }

   //===--------------------------------------------------===//
   // Iterators.
   //===--------------------------------------------------===//

   class Iterator : public ImutAVLValueIterator<ImmutableMap>
   {
      friend class ImmutableMap;

      Iterator() = default;
      explicit Iterator(TreeType *tree) : Iterator::ImutAVLValueIterator(tree)
      {}

   public:
      KeyTypeRef getKey() const
      {
         return (*this)->first;
      }

      DataTypeRef getData() const
      {
         return (*this)->second;
      }
   };

   Iterator begin() const
   {
      return Iterator(m_root);
   }

   Iterator end() const
   {
      return Iterator();
   }

   DataType* lookup(KeyTypeRef key) const
   {
      if (m_root) {
         TreeType* temp = m_root->find(key);
         if (temp) {
            return &temp->getValue().second;
         }
      }
      return nullptr;
   }

   /// getMaxElement - Returns the <key,value> pair in the ImmutableMap for
   ///  which key is the highest in the ordering of keys in the map.  This
   ///  method returns NULL if the map is empty.
   ValueType* getMaxElement() const
   {
      return m_root ? &(m_root->getMaxElement()->getValue()) : nullptr;
   }

   //===--------------------------------------------------===//
   // Utility methods.
   //===--------------------------------------------------===//

   unsigned getHeight() const
   {
      return m_root ? m_root->getHeight() : 0;
   }

   static inline void profile(FoldingSetNodeId &id, const ImmutableMap &map)
   {
      id.addPointer(map.m_root);
   }

   inline void profile(FoldingSetNodeId& id) const
   {
      return profile(id, *this);
   }
};

// NOTE: This will possibly become the new implementation of ImmutableMap some day.
template <typename KeyT, typename ValueT,
          typename ValInfo = ImutKeyValueInfo<KeyT, ValueT>>
class ImmutableMapRef
{
public:
   using ValueType = typename ValInfo::ValueType;
   using ValueTypeRef = typename ValInfo::ValueTypeRef;
   using KeyType = typename ValInfo::KeyType;
   using KeyTypeRef = typename ValInfo::KeyTypeRef;
   using DataType = typename ValInfo::DataType;
   using DataTypeRef = typename ValInfo::DataTypeRef;
   using TreeType = ImutAVLTree<ValInfo>;
   using FactoryType = typename TreeType::Factory;

protected:
   TreeType *m_root;
   FactoryType *m_factory;

public:
   /// Constructs a map from a pointer to a tree root.  In general one
   /// should use a Factory object to create maps instead of directly
   /// invoking the constructor, but there are cases where make this
   /// constructor public is useful.
   explicit ImmutableMapRef(const TreeType *root, FactoryType *factory)
      : m_root(const_cast<TreeType *>(root)), m_factory(factory)
   {
      if (root) {
         root->retain();
      }
   }

   explicit ImmutableMapRef(const ImmutableMap<KeyT, ValueT> &other,
                            typename ImmutableMap<KeyT, ValueT>::Factory &factory)
      : m_root(other.getRootWithoutRetain()),
        m_factory(factory.getTreeFactory())
   {
      if (m_root) {
         m_root->retain();
      }
   }

   ImmutableMapRef(const ImmutableMapRef &other)
      : m_root(other.m_root), m_factory(other.m_factory)
   {
      if (m_root) {
         m_root->retain();
      }
   }

   ~ImmutableMapRef()
   {
      if (m_root) {
         m_root->release();
      }
   }

   ImmutableMapRef &operator=(const ImmutableMapRef &other)
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

   static inline ImmutableMapRef getEmptyMap(FactoryType *factory)
   {
      return ImmutableMapRef(0, factory);
   }

   void manualRetain()
   {
      if (m_root) {
         m_root->retain();
      }
   }

   void manualRelease()
   {
      if (m_root) {
         m_root->release();
      }
   }

   ImmutableMapRef add(KeyTypeRef key, DataTypeRef data) const
   {
      TreeType *newType = m_factory->add(m_root, std::pair<KeyType, DataType>(key, data));
      return ImmutableMapRef(newType, m_factory);
   }

   ImmutableMapRef remove(KeyTypeRef key) const
   {
      TreeType *newType = m_factory->remove(m_root, key);
      return ImmutableMapRef(newType, m_factory);
   }

   bool contains(KeyTypeRef key) const
   {
      return m_root ? m_root->contains(key) : false;
   }

   ImmutableMap<KeyT, ValueT> asImmutableMap() const
   {
      return ImmutableMap<KeyT, ValueT>(m_factory->getCanonicalTree(m_root));
   }

   bool operator==(const ImmutableMapRef &other) const
   {
      return m_root && other.m_root ? m_root->isEqual(*other.m_root) : m_root == other.m_root;
   }

   bool operator!=(const ImmutableMapRef &other) const
   {
      return m_root && other.m_root ? m_root->isNotEqual(*other.m_root) : m_root != other.m_root;
   }

   bool isEmpty() const
   {
      return !m_root;
   }

   //===--------------------------------------------------===//
   // For testing.
   //===--------------------------------------------------===//

   void verify() const
   {
      if (m_root) {
         m_root->verify();
      }
   }

   //===--------------------------------------------------===//
   // Iterators.
   //===--------------------------------------------------===//

   class Iterator : public ImutAVLValueIterator<ImmutableMapRef>
   {
      friend class ImmutableMapRef;

      Iterator() = default;
      explicit Iterator(TreeType *tree) : Iterator::ImutAVLValueIterator(tree)
      {}

   public:
      KeyTypeRef getKey() const
      {
         return (*this)->first;
      }

      DataTypeRef getData() const
      {
         return (*this)->second;
      }
   };

   Iterator begin() const
   {
      return Iterator(m_root);
   }

   Iterator end() const
   {
      return Iterator();
   }

   DataType *lookup(KeyTypeRef key) const
   {
      if (m_root) {
         TreeType *temp = m_root->find(key);
         if (temp){
            return &temp->getValue().second;
         }
      }

      return nullptr;
   }

   /// getMaxElement - Returns the <key,value> pair in the ImmutableMap for
   ///  which key is the highest in the ordering of keys in the map.  This
   ///  method returns NULL if the map is empty.
   ValueType* getMaxElement() const
   {
      return m_root ? &(m_root->getMaxElement()->getValue()) : 0;
   }

   //===--------------------------------------------------===//
   // Utility methods.
   //===--------------------------------------------------===//

   unsigned getHeight() const
   {
      return m_root ? m_root->getHeight() : 0;
   }

   static inline void profile(FoldingSetNodeId &id, const ImmutableMapRef &map)
   {
      id.addPointer(map.m_root);
   }

   inline void profile(FoldingSetNodeId &id) const
   {
      return profile(id, *this);
   }
};

} // basic
} // polar

#endif //  POLARPHP_BASIC_ADT_IMMUTABLE_MAP_H
