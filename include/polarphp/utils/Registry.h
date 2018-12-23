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

#ifndef POLARPHP_UTILS_REGISTRY_H
#define POLARPHP_UTILS_REGISTRY_H

#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include "polarphp/utils/DynamicLibrary.h"
#include <memory>

namespace polar {
namespace utils {

using polar::basic::IteratorRange;
using polar::basic::make_range;
using polar::basic::StringRef;

/// A simple registry EntryType which provides only a name, description, and
/// no-argument constructor.
template <typename T>
class SimpleRegistryEntry
{
   StringRef m_name;
   StringRef m_desc;
   std::unique_ptr<T> (*m_ctor)();

public:
   SimpleRegistryEntry(StringRef name, StringRef desc, std::unique_ptr<T> (*ctor)())
      : m_name(name),
        m_desc(desc),
        m_ctor(ctor)
   {}

   StringRef getName() const
   {
      return m_name;
   }

   StringRef getDesc() const
   {
      return m_desc;
   }

   std::unique_ptr<T> instantiate() const
   {
      return m_ctor();
   }
};

/// A global registry used in conjunction with static constructors to make
/// pluggable components (like targets or garbage collectors) "just work" when
/// linked with an executable.
template <typename T>
class Registry
{
public:
   typedef T type;
   typedef SimpleRegistryEntry<T> EntryType;

   class Node;
   class Iterator;

private:
   Registry() = delete;

   friend class Node;
   static Node *m_head, *m_tail;

public:
   /// Node in linked list of entries.
   ///
   class Node
   {
      friend class Iterator;
      friend class Registry<T>;

      Node *m_next;
      const EntryType& m_value;

   public:
      Node(const EntryType &value)
         : m_next(nullptr), m_value(value)
      {}
   };

   /// Add a Node to the Registry: this is the interface between the plugin and
   /// the executable.
   ///
   /// This function is exported by the executable and called by the plugin to
   /// add a Node to the executable's registry. Therefore it's not defined here
   /// to avoid it being instantiated in the plugin and is instead defined in
   /// the executable (see LLVM_INSTANTIATE_REGISTRY below).
   static void addNode(Node *node);

   /// Iterators for registry entries.
   ///
   class Iterator
   {
      const Node *m_cur;

   public:
      explicit Iterator(const Node *node) : m_cur(node)
      {}

      bool operator==(const Iterator &other) const
      {
         return m_cur == other.m_cur;
      }

      bool operator!=(const Iterator &other) const
      {
         return m_cur != other.m_cur;
      }

      Iterator &operator++()
      {
         m_cur = m_cur->m_next;
         return *this;
      }

      const EntryType &operator*() const
      {
         return m_cur->m_value;
      }

      const EntryType *operator->() const
      {
         return &m_cur->m_value;
      }
   };

   // begin is not defined here in order to avoid usage of an undefined static
   // data member, instead it's instantiated by LLVM_INSTANTIATE_REGISTRY.
   static Iterator begin();
   static Iterator end()
   {
      return Iterator(nullptr);
   }

   static IteratorRange<Iterator> entries()
   {
      return make_range(begin(), end());
   }

   /// A static registration template. Use like such:
   ///
   ///   Registry<Collector>::Add<FancyGC>
   ///   X("fancy-gc", "Newfangled garbage collector.");
   ///
   /// Use of this template requires that:
   ///
   ///  1. The registered subclass has a default constructor.
   template <typename V>
   class Add
   {
      EntryType m_entry;
      Node m_node;

      static std::unique_ptr<T> getCtorFn() { return std::make_unique<V>(); }

   public:
      Add(StringRef name, StringRef desc)
         : m_entry(name, desc, getCtorFn), m_node(m_entry)
      {
         addNode(&m_node);
      }
   };
};

} // utils
} // polar

/// Instantiate a registry class.
///
/// This provides template definitions of addNode, begin, and the m_head and m_tail
/// pointers, then explicitly instantiates them. We could explicitly specialize
/// them, instead of the two-step process of define then instantiate, but
/// strictly speaking that's not allowed by the C++ standard (we would need to
/// have explicit specialization declarations in all translation units where the
/// specialization is used) so we don't.
#define POLAR_INSTANTIATE_REGISTRY(REGISTRY_CLASS) \
   namespace polar { \
   namespace utils { \
   template<typename T> typename Registry<T>::Node *Registry<T>::m_head = nullptr;\
   template<typename T> typename Registry<T>::Node *Registry<T>::m_tail = nullptr;\
   template<typename T> \
   void Registry<T>::addNode(typename Registry<T>::Node *node) { \
   if (m_tail) \
   m_tail->m_next = node; \
   else \
   m_head = node; \
   m_tail = node; \
} \
   template<typename T> typename Registry<T>::Iterator Registry<T>::begin() { \
   return Iterator(m_head); \
} \
   template REGISTRY_CLASS::Node *Registry<REGISTRY_CLASS::type>::m_head; \
   template REGISTRY_CLASS::Node *Registry<REGISTRY_CLASS::type>::m_tail; \
   template \
   void Registry<REGISTRY_CLASS::type>::addNode(REGISTRY_CLASS::Node*); \
   template REGISTRY_CLASS::Iterator Registry<REGISTRY_CLASS::type>::begin(); \
}\
}

#endif // POLARPHP_UTILS_REGISTRY_H
