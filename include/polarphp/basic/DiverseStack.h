//===--- DiverseStack.h - Stack of variably-sized objects -------*- C++ -*-===//
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
///
/// \file
///
/// This file defines a m_data structure for representing a stack of
/// variably-sized objects.  It is a requirement that the object type
/// be trivially movable, meaning that iter has a trivial move
/// constructor and a trivial destructor.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_DIVERSE_STACK_H
#define POLARPHP_BASIC_DIVERSE_STACK_H

#include "polarphp/basic/Malloc.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/PointerLikeTypeTraits.h"
#include <cassert>
#include <cstring>
#include <utility>

namespace polar::basic {

template <typename T>
class DiverseStackImpl;

/// DiverseStack - A stack of heterogeneously-typed objects.
///
/// \tparam T - A common base class of the objects on the stack; must
///   provide an allocated_size() const method.
/// \tparam InlineCapacity - the amount of inline storage to provide, in bytes.
template <typename T, unsigned InlineCapacity>
class DiverseStack : public DiverseStackImpl<T>
{
public:
   DiverseStack()
      : DiverseStackImpl<T>(m_inlineStorage + InlineCapacity)
   {}

   DiverseStack(const DiverseStack &other)
      : DiverseStackImpl<T>(other, m_inlineStorage + InlineCapacity)
   {}

   DiverseStack(const DiverseStackImpl<T> &other)
      : DiverseStackImpl<T>(other, m_inlineStorage + InlineCapacity)
   {}

   DiverseStack(DiverseStack<T, InlineCapacity> &&other)
      : DiverseStackImpl<T>(std::move(other), m_inlineStorage + InlineCapacity)
   {}

   DiverseStack(DiverseStackImpl<T> &&other)
      : DiverseStackImpl<T>(std::move(other), m_inlineStorage + InlineCapacity)
   {}
private:
   char m_inlineStorage[InlineCapacity];
};

/// A base class for DiverseStackImpl.
class DiverseStackBase
{
public:
   /// The top of the stack.
   char *begin;

   /// The bottom of the stack, i.e. the end of the allocation.
   char *end;

   /// The beginning of the allocation.
   char *allocated;

   bool isAllocatedInline() const
   {
      return (allocated == reinterpret_cast<const char *>(this + 1));
   }

   void checkValid() const
   {
      assert(allocated <= begin);
      assert(begin <= end);
   }

   void initialize(char *end)
   {
      this->begin = this->end = end;
      allocated = reinterpret_cast<char*>(this + 1);
   }

   void copyFrom(const DiverseStackBase &other)
   {
      // Ensure that we're large enough to store all the m_data.
      std::size_t size = static_cast<std::size_t>(other.end - other.begin);
      pushNewStorage(size);
      std::memcpy(begin, other.begin, size);
   }

   void pushNewStorage(std::size_t needed)
   {
      checkValid();
      if (std::size_t(begin - allocated) >= needed) {
         begin -= needed;
      } else {
         pushNewStorageSlow(needed);
      }
   }

   void pushNewStorageSlow(std::size_t needed);

   /// A stable iterator is the equivalent of an index into the stack.
   /// It's an iterator that stays stable across modification of the
   /// stack.
   class stable_iterator
   {
      std::size_t m_depth;
      friend class DiverseStackBase;
      template <typename T> friend class DiverseStackImpl;
      stable_iterator(std::size_t depth)
         : m_depth(depth) {}
   public:
      stable_iterator() = default;
      friend bool operator==(stable_iterator lhs, stable_iterator rhs)
      {
         return lhs.m_depth == rhs.m_depth;
      }

      friend bool operator!=(stable_iterator lhs, stable_iterator rhs)
      {
         return !operator==(lhs, rhs);
      }

      static stable_iterator invalid()
      {
         return stable_iterator(static_cast<std::size_t>(-1));
      }

      bool isValid() const
      {
         return m_depth != static_cast<std::size_t>(-1);
      }

      std::size_t getDepth() const
      {
         return m_depth;
      }

      /// A helper class that wraps a stable_iterator as something that
      /// pretends to be a non-null pointer.
      ///
      /// This allows stable_iterators to be placed in TinyPtrVector.
      ///
      /// A wrapper is needed because we don't want to give stable_iterator
      /// a null inhabitant, an operator bool, conversions from nullptr_t, or
      /// any similar features that TinyPtrVector reasonably requires of its
      /// element types.
      class AsPointer
      {
         void *m_encodedValue;
         explicit AsPointer(void *encodedValue)
            : m_encodedValue(encodedValue)
         {}
      public:
         enum { NumLowBitsAvailable = 3 };

         /// Allow a null AsPointer to be created with either 'nullptr' or
         /// 'AsPointer()'.
         /*implicit*/ AsPointer(std::nullptr_t = nullptr)
            : m_encodedValue(nullptr)
         {}

         /// Allow an AsPointer to be tested as a boolean value.
         explicit operator bool() const
         {
            return m_encodedValue != nullptr;
         }

         /// Allow an AsPointer to be compared for equality with a void*.
         friend bool operator==(AsPointer lhs, void *rhs)
         {
            return lhs.m_encodedValue == rhs;
         }

         /// Allow an implicit conversion from stable_iterator.
         /*implicit*/ AsPointer(stable_iterator iter)
         {
            assert(iter.isValid() && "can't encode invalid stable_iterator");
            auto encodedDepth = (iter.m_depth + 1) << NumLowBitsAvailable;
            m_encodedValue = reinterpret_cast<void*>(encodedDepth);
            assert(m_encodedValue && "encoded pointer was null");
         }

         /// Allow an implicit conversion to stable_iterator.
         operator stable_iterator() const
         {
            assert(m_encodedValue && "can't decode null pointer");
            auto encodedDepth = reinterpret_cast<std::size_t>(m_encodedValue);
            auto depth = (encodedDepth >> NumLowBitsAvailable) - 1;
            auto iter = stable_iterator(depth);
            assert(iter.isValid() && "decoded stable_iterator was invalid");
            return iter;
         }

         void *getAsVoidPointer() const
         {
            return m_encodedValue;
         }

         static AsPointer getFromVoidPointer(void *ptr)
         {
            return AsPointer(ptr);
         }
      };
      AsPointer asPointer() const
      {
         return *this;
      }
   };

   stable_iterator stable_begin() const
   {
      return stable_iterator(end - begin);
   }

   static stable_iterator stable_end()
   {
      return stable_iterator(0);
   }

   void checkIterator(stable_iterator iter) const
   {
      assert(iter.isValid() && "checking an invalid iterator");
      checkValid();
      assert(iter.m_depth <= size_t(end - begin));
   }
};

template <class T>
class DiverseStackImpl : private DiverseStackBase
{
   DiverseStackImpl(const DiverseStackImpl<T> &other) = delete;
   DiverseStackImpl(DiverseStackImpl<T> &&other) = delete;

protected:
   DiverseStackImpl(char *endMark)
   {
      initialize(endMark);
   }

   DiverseStackImpl(const DiverseStackImpl<T> &other, char *endMark)
   {
      initialize(endMark);
      copyFrom(other);
   }

   DiverseStackImpl(DiverseStackImpl<T> &&other, char *endMark)
   {
      // If the other is allocated inline, just initialize and copy.
      if (other.isAllocatedInline()) {
         initialize(endMark);
         copyFrom(other);
         return;
      }
      // Otherwise, steal its allocations.
      this->begin = other.begin;
      this->end = other.end;
      allocated = other.allocated;
      other.begin = other.end = other.allocated = reinterpret_cast<char*>((&other + 1));
      assert(other.isAllocatedInline());
   }

public:
   ~DiverseStackImpl()
   {
      checkValid();
      if (!isAllocatedInline()) {
         delete[] allocated;
      }
   }

   /// Query whether the stack is empty.
   bool empty() const
   {
      checkValid();
      return this->begin == this->end;
   }

   /// Return a reference to the top element on the stack.
   T &top()
   {
      assert(!empty());
      return *reinterpret_cast<T*>(begin);
   }

   /// Return a reference to the top element on the stack.
   const T &top() const
   {
      assert(!empty());
      return *reinterpret_cast<const T*>(begin);
   }

   using DiverseStackBase::stable_iterator;
   using DiverseStackBase::stable_begin;
   using DiverseStackBase::stable_end;

   class const_iterator;
   class iterator
   {
      char *m_ptr;
      friend class DiverseStackImpl;
      friend class const_iterator;
      iterator(char *ptr) : m_ptr(ptr)
      {}

   public:
      iterator() = default;

      T &operator*() const
      {
         return *reinterpret_cast<T*>(m_ptr);
      }

      T *operator->() const
      {
         return reinterpret_cast<T*>(m_ptr);
      }

      iterator &operator++()
      {
         m_ptr += (*this)->allocated_size();
         return *this;
      }

      iterator operator++(int)
      {
         auto copy = *this;
         operator++();
         return copy;
      }

      /// advancePast - Like operator++, but asserting that the current
      /// object has a known type.
      template <typename U>
      void advancePast()
      {
         assert((*this)->allocated_size() == sizeof(U));
         m_ptr += sizeof(U);
      }

      friend bool operator==(iterator lhs, iterator rhs)
      {
         return lhs.m_ptr == rhs.m_ptr;
      }

      friend bool operator!=(iterator lhs, iterator rhs)
      {
         return !operator==(lhs, rhs);
      }
   };

   using DiverseStackBase::checkIterator;
   void checkIterator(iterator iter) const
   {
      checkValid();
      assert(this->begin <= iter.m_ptr && iter.m_ptr <= this->end);
   }

   iterator begin()
   {
      checkValid(); return iterator(begin);
   }

   iterator end()
   {
      checkValid();
      return iterator(end);
   }

   iterator find(stable_iterator iter)
   {
      checkIterator(iter);
      return iterator(this->end - iter.m_depth);
   }

   stable_iterator stabilize(iterator iter) const
   {
      checkIterator(iter);
      return stable_iterator(this->end - iter.m_ptr);
   }

   T &findAndAdvance(stable_iterator &i)
   {
      auto unstableIter = find(i);
      assert(unstableIter != end());
      T &value = *unstableIter;
      ++unstableIter;
      i = stabilize(unstableIter);
      return value;
   }

   class const_iterator
   {
      const char *m_ptr;
      friend class DiverseStackImpl;
      const_iterator(const char *ptr)
         : m_ptr(ptr) {}
   public:
      const_iterator() = default;
      const_iterator(iterator iter)
         : m_ptr(iter.m_ptr) {}

      const T &operator*() const
      {
         return *reinterpret_cast<const T*>(m_ptr);
      }

      const T *operator->() const
      {
         return reinterpret_cast<const T*>(m_ptr);
      }

      const_iterator &operator++()
      {
         m_ptr += (*this)->allocated_size();
         return *this;
      }

      const_iterator operator++(int)
      {
         auto copy = *this;
         operator++();
         return copy;
      }

      /// advancePast - Like operator++, but asserting that the current
      /// object has a known type.
      template <typename U> void advancePast()
      {
         assert((*this)->allocated_size() == sizeof(U));
         m_ptr += sizeof(U);
      }

      friend bool operator==(const_iterator a, const_iterator b)
      {
         return a.m_ptr == b.m_ptr;
      }

      friend bool operator!=(const_iterator a, const_iterator b)
      {
         return !operator==(a, b);
      }
   };
   const_iterator begin() const
   {
      checkValid();
      return const_iterator(begin);
   }

   const_iterator end() const
   {
      checkValid();
      return const_iterator(end);
   }

   void checkIterator(const_iterator iter) const
   {
      checkValid();
      assert(this->begin <= iter.m_ptr && iter.m_ptr <= this->end);
   }

   const_iterator find(stable_iterator iter) const
   {
      checkIterator(iter);
      return const_iterator(this->end - iter.m_depth);
   }

   stable_iterator stabilize(const_iterator iter) const
   {
      checkIterator(iter);
      return stable_iterator(this->end - iter.m_ptr);
   }

   /// Push a new object onto the stack.
   template <typename U, typename... A> U &push(A && ...args)
   {
      pushNewStorage(sizeof(U));
      return *::new (begin) U(::std::forward<A>(args)...);
   }

   /// Pop an object off the stack.
   void pop() {
      assert(!empty());
      this->begin += top().allocated_size();
   }

   /// Pop an object of known type off the stack.
   template <typename U>
   void pop()
   {
      assert(!empty());
      assert(sizeof(U) == top().allocated_size());
      this->begin += sizeof(U);
   }

   /// Pop objects off of the stack until \p the object pointed to by stable_iter
   /// is the top element of the stack.
   void pop(stable_iterator stable_iter)
   {
      iterator iter = find(stable_iter);
      checkIterator(iter);
#ifndef NDEBUG
      while (this->begin != iter.m_ptr) {
         pop();
         checkIterator(iter);
      }
#else
      this->begin = iter.m_ptr;
#endif
   }
};

/// A helper class for copying value off a DiverseStack.
template <typename T>
class DiverseValueBuffer
{
   llvm::SmallVector<char, sizeof(T) + 10 * sizeof(void*)> m_data;

public:
   DiverseValueBuffer(const T &value)
   {
      size_t size = value.allocated_size();
      m_data.reserve(size);
      m_data.set_size(size);
      memcpy(m_data.m_data(), reinterpret_cast<const void *>(&value), size);
   }

   T &getCopy()
   {
      return *reinterpret_cast<T *>(m_data.data());
   }
};

} // end namespace polar::basic

/// Allow stable_iterators to be put in things like TinyPtrVectors.
namespace llvm {
template <>
struct PointerLikeTypeTraits<
      polar::basic::DiverseStackBase::stable_iterator::AsPointer>
{
   using AsPointer = polar::basic::DiverseStackBase::stable_iterator::AsPointer;
public:
   static inline void *getAsVoidPointer(AsPointer ptr)
   {
      return ptr.getAsVoidPointer();
   }

   static inline AsPointer getFromVoidPointer(void *ptr)
   {
      return AsPointer::getFromVoidPointer(ptr);
   }

   enum {
      NumLowBitsAvailable = AsPointer::NumLowBitsAvailable
   };
};
}

#endif // POLARPHP_BASIC_DIVERSE_STACK_H
