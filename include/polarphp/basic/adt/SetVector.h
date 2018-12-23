// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/26.

#ifndef POLARPHP_BASIC_SET_VECTOR_H
#define POLARPHP_BASIC_SET_VECTOR_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/DenseSet.h"
#include "polarphp/basic/adt/StlExtras.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <vector>

namespace polar {
namespace basic {

/// A vector that has set insertion semantics.
///
/// This adapter class provides a way to keep a set of things that also has the
/// property of a deterministic iteration order. The order of iteration is the
/// order of insertion.
template <typename T, typename Vector = std::vector<T>,
          typename Set = DenseSet<T>>
class SetVector
{
public:
   using value_type = T;
   using key_type = T;
   using reference = T&;
   using const_reference = const T&;
   using set_type = Set;
   using vector_type = Vector;
   using iterator = typename vector_type::const_iterator;
   using const_iterator = typename vector_type::const_iterator;
   using reverse_iterator = typename vector_type::const_reverse_iterator;
   using const_reverse_iterator = typename vector_type::const_reverse_iterator;
   using size_type = typename vector_type::size_type;

   /// Construct an empty SetVector
   SetVector() = default;

   /// Initialize a SetVector with a range of elements
   template<typename IterType>
   SetVector(IterType start, IterType end)
   {
      insert(start, end);
   }

   ArrayRef<T> getArrayRef() const
   {
      return m_vector;
   }

   /// Clear the SetVector and return the underlying vector.
   Vector takeVector()
   {
      m_set.clear();
      return std::move(m_vector);
   }

   /// Determine if the SetVector is empty or not.
   bool empty() const
   {
      return m_vector.empty();
   }

   /// Determine the number of elements in the SetVector.
   size_type size() const
   {
      return m_vector.size();
   }

   /// Get an iterator to the beginning of the SetVector.
   iterator begin()
   {
      return m_vector.begin();
   }

   /// Get a const_iterator to the beginning of the SetVector.
   const_iterator begin() const
   {
      return m_vector.begin();
   }

   /// Get an iterator to the end of the SetVector.
   iterator end()
   {
      return m_vector.end();
   }

   /// Get a const_iterator to the end of the SetVector.
   const_iterator end() const
   {
      return m_vector.end();
   }

   /// Get an reverse_iterator to the end of the SetVector.
   reverse_iterator rbegin()
   {
      return m_vector.rbegin();
   }

   /// Get a const_reverse_iterator to the end of the SetVector.
   const_reverse_iterator rbegin() const
   {
      return m_vector.rbegin();
   }

   /// Get a reverse_iterator to the beginning of the SetVector.
   reverse_iterator rend()
   {
      return m_vector.rend();
   }

   /// Get a const_reverse_iterator to the beginning of the SetVector.
   const_reverse_iterator rend() const
   {
      return m_vector.rend();
   }

   /// Return the first element of the SetVector.
   const T &front() const
   {
      assert(!empty() && "Cannot call front() on empty SetVector!");
      return m_vector.front();
   }

   /// Return the last element of the SetVector.
   const T &back() const
   {
      assert(!empty() && "Cannot call back() on empty SetVector!");
      return m_vector.back();
   }

   /// Index into the SetVector.
   const_reference operator[](size_type n) const
   {
      assert(n < m_vector.size() && "SetVector access out of range!");
      return m_vector[n];
   }

   /// Insert a new element into the SetVector.
   /// \returns true if the element was inserted into the SetVector.
   bool insert(const value_type &value)
   {
      bool result = m_set.insert(value).second;
      if (result) {
         m_vector.push_back(value);
      }
      return result;
   }

   /// Insert a range of elements into the SetVector.
   template<typename IterType>
   void insert(IterType start, IterType end)
   {
      for (; start != end; ++start) {
         if (m_set.insert(*start).second) {
            m_vector.push_back(*start);
         }
      }
   }

   /// Remove an item from the set vector.
   bool remove(const value_type &value)
   {
      if (m_set.erase(value)) {
         typename vector_type::iterator iter = find(m_vector, value);
         assert(iter != m_vector.end() && "Corrupted SetVector instances!");
         m_vector.erase(iter);
         return true;
      }
      return false;
   }

   /// Erase a single element from the set vector.
   /// \returns an iterator pointing to the next element that followed the
   /// element erased. This is the end of the SetVector if the last element is
   /// erased.
   iterator erase(iterator iter)
   {
      const key_type &value = *iter;
      assert(m_set.count(value) && "Corrupted SetVector instances!");
      m_set.erase(value);

      // FIXME: No need to use the non-const iterator when built with
      // std:vector.erase(const_iterator) as defined in C++11. This is for
      // compatibility with non-standard libstdc++ up to 4.8 (fixed in 4.9).
      auto niter = m_vector.begin();
      std::advance(niter, std::distance<iterator>(niter, iter));
      return m_vector.erase(niter);
   }

   /// Remove items from the set vector based on a predicate function.
   ///
   /// This is intended to be equivalent to the following code, if we could
   /// write it:
   ///
   /// \code
   ///   V.erase(removeIf(V, P), V.end());
   /// \endcode
   ///
   /// However, SetVector doesn't expose non-const iterators, making any
   /// algorithm like removeIf impossible to use.
   ///
   /// \returns true if any element is removed.
   template <typename UnaryPredicate>
   bool removeIf(UnaryPredicate pred)
   {
      typename vector_type::iterator iter =
            polar::basic::remove_if(m_vector, TestAndEraseFromSet<UnaryPredicate>(pred, m_set));
      if (iter == m_vector.end()) {
         return false;
      }
      m_vector.erase(iter, m_vector.end());
      return true;
   }

   /// Count the number of elements of a given key in the SetVector.
   /// \returns 0 if the element is not in the SetVector, 1 if it is.
   size_type count(const key_type &key) const
   {
      return m_set.count(key);
   }

   /// Completely clear the SetVector
   void clear()
   {
      m_set.clear();
      m_vector.clear();
   }

   /// Remove the last element of the SetVector.
   void pop_back()
   {
      assert(!empty() && "Cannot remove an element from an empty SetVector!");
      m_set.erase(back());
      m_vector.pop_back();
   }

   /// Remove the last element of the SetVector.
   void popBack()
   {
      pop_back();
   }

   POLAR_NODISCARD T pop_back_val()
   {
      T ret = back();
      pop_back();
      return ret;
   }

   POLAR_NODISCARD T popBackValue()
   {
      return pop_back_val();
   }

   bool operator==(const SetVector &other) const
   {
      return m_vector == other.m_vector;
   }

   bool operator!=(const SetVector &other) const
   {
      return m_vector != other.m_vector;
   }

   /// Compute This := This u S, return whether 'This' changed.
   /// TODO: We should be able to use setUnion from SetOperations.h, but
   ///       SetVector interface is inconsistent with DenseSet.
   template <class SType>
   bool setUnion(const SType &set)
   {
      bool changed = false;
      for (typename SType::const_iterator siter = set.begin(), send = set.end(); siter != send;
           ++siter) {
         if (insert(*siter)) {
            changed = true;
         }
      }
      return changed;
   }

   /// Compute This := This - B
   /// TODO: We should be able to use setSubtract from SetOperations.h, but
   ///       SetVector interface is inconsistent with DenseSet.
   template <class SType>
   void setSubtract(const SType &set)
   {
      for (typename SType::const_iterator siter = set.begin(), send = set.end(); siter != send;
           ++siter)
      {
         remove(*siter);
      }
   }

private:
   /// A wrapper predicate designed for use with std::removeIf.
   ///
   /// This predicate wraps a predicate suitable for use with std::removeIf to
   /// call m_set.erase(x) on each element which is slated for removal.
   template <typename UnaryPredicate>
   class TestAndEraseFromSet
   {
      UnaryPredicate m_pred;
      set_type &m_set;

   public:
      TestAndEraseFromSet(UnaryPredicate pred, set_type &set)
         : m_pred(std::move(pred)), m_set(set)
      {}

      template <typename ArgumentT>
      bool operator()(const ArgumentT &arg)
      {
         if (m_pred(arg)) {
            m_set.erase(arg);
            return true;
         }
         return false;
      }
   };

   set_type m_set;         ///< The set.
   vector_type m_vector;   ///< The vector.
};

/// A SetVector that performs no allocations if smaller than
/// a certain size.
template <typename T, unsigned N>
class SmallSetVector
      : public SetVector<T, SmallVector<T, N>, SmallDenseSet<T, N>>
{
public:
   SmallSetVector() = default;

   /// Initialize a SmallSetVector with a range of elements
   template<typename IterType>
   SmallSetVector(IterType start, IterType end)
   {
      this->insert(start, end);
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_SET_VECTOR_H
