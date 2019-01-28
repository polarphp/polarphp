// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/28.

#ifndef POLARPHP_BASIC_ADT_TINY_PTR_VECTOR_H
#define POLARPHP_BASIC_ADT_TINY_PTR_VECTOR_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/PointerUnion.h"
#include "polarphp/basic/adt/SmallVector.h"
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace polar {
namespace basic {

/// TinyPtrVector - This class is specialized for cases where there are
/// normally 0 or 1 element in a vector, but is general enough to go beyond that
/// when required.
///
/// NOTE: This container doesn't allow you to store a null pointer into it.
///
template <typename EltTy>
class TinyPtrVector
{
public:
   using VecTy = SmallVector<EltTy, 4>;
   using value_type = typename VecTy::value_type;
   using PtrUnion = PointerUnion<EltTy, VecTy *>;

private:
   PtrUnion m_value;

public:
   TinyPtrVector() = default;

   ~TinyPtrVector()
   {
      if (VecTy *vector = m_value.template dynamicCast<VecTy*>()) {
         delete vector;
      }
   }

   TinyPtrVector(const TinyPtrVector &other) : m_value(other.m_value)
   {
      if (VecTy *vector = m_value.template dynamicCast<VecTy*>()) {
         m_value = new VecTy(*vector);
      }
   }

   TinyPtrVector &operator=(const TinyPtrVector &other)
   {
      if (this == &other) {
         return *this;
      }
      if (other.empty()) {
         this->clear();
         return *this;
      }

      // Try to squeeze into the single slot. If it won't fit, allocate a copied
      // vector.
      if (m_value.template is<EltTy>()) {
         if (other.size() == 1) {
            m_value = other.front();
         } else {
            m_value = new VecTy(*other.m_value.template get<VecTy*>());
         }
         return *this;
      }

      // If we have a full vector allocated, try to re-use it.
      if (other.m_value.template is<EltTy>()) {
         m_value.template get<VecTy*>()->clear();
         m_value.template get<VecTy*>()->push_back(other.front());
      } else {
         *m_value.template get<VecTy*>() = *other.m_value.template get<VecTy*>();
      }
      return *this;
   }

   TinyPtrVector(TinyPtrVector &&other) : m_value(other.m_value)
   {
      other.m_value = (EltTy)nullptr;
   }

   TinyPtrVector &operator=(TinyPtrVector &&other)
   {
      if (this == &other) {
         return *this;
      }
      if (other.empty()) {
         this->clear();
         return *this;
      }

      // If this vector has been allocated on the heap, re-use it if cheap. If it
      // would require more copying, just delete it and we'll steal the other
      // side.
      if (VecTy *vector = m_value.template dynamicCast<VecTy*>()) {
         if (other.m_value.template is<EltTy>()) {
            vector->clear();
            vector->push_back(other.front());
            other.m_value = (EltTy)nullptr;
            return *this;
         }
         delete vector;
      }

      m_value = other.m_value;
      other.m_value = (EltTy)nullptr;
      return *this;
   }

   TinyPtrVector(std::initializer_list<EltTy> ilist)
      : m_value(ilist.size() == 0
                ? PtrUnion()
                : ilist.size() == 1 ? PtrUnion(*ilist.begin())
                                    : PtrUnion(new VecTy(ilist.begin(), ilist.end())))
   {}

   /// Constructor from an ArrayRef.
   ///
   /// This also is a constructor for individual array elements due to the single
   /// element constructor for ArrayRef.
   explicit TinyPtrVector(ArrayRef<EltTy> elems)
      : m_value(elems.empty()
                ? PtrUnion()
                : elems.getSize() == 1
                  ? PtrUnion(elems[0])
      : PtrUnion(new VecTy(elems.begin(), elems.end())))
   {}

   TinyPtrVector(size_t count, EltTy m_valueue)
      : m_value(count == 0 ? PtrUnion()
                           : count == 1 ? PtrUnion(m_valueue)
                                        : PtrUnion(new VecTy(count, m_valueue)))
   {}

   // implicit conversion operator to ArrayRef.
   operator ArrayRef<EltTy>() const
   {
      if (m_value.isNull()) {
         return std::nullopt;
      }
      if (m_value.template is<EltTy>()) {
         return *m_value.getAddrOfPtr1();
      }
      return *m_value.template get<VecTy*>();
   }

   // implicit conversion operator to MutableArrayRef.
   operator MutableArrayRef<EltTy>()
   {
      if (m_value.isNull()) {
         return std::nullopt;
      }
      if (m_value.template is<EltTy>()) {
         return *m_value.getAddrOfPtr1();
      }
      return *m_value.template get<VecTy*>();
   }

   // Implicit conversion to ArrayRef<U> if EltTy* implicitly converts to U*.
   template<typename U,
            typename std::enable_if<
               std::is_convertible<ArrayRef<EltTy>, ArrayRef<U>>::value,
               bool>::type = false>
   operator ArrayRef<U>() const
   {
      return operator ArrayRef<EltTy>();
   }

   bool empty() const
   {
      // This vector can be empty if it contains no element, or if it
      // contains a pointer to an empty vector.
      if (m_value.isNull()) return true;
      if (VecTy *vector = m_value.template dynamicCast<VecTy*>()) {
         return vector->empty();
      }
      return false;
   }

   unsigned size() const
   {
      if (empty()) {
         return 0;
      }
      if (m_value.template is<EltTy>()) {
         return 1;
      }
      return m_value.template get<VecTy*>()->size();
   }

   using iterator = EltTy *;
   using const_iterator = const EltTy *;
   using reverse_iterator = std::reverse_iterator<iterator>;
   using const_reverse_iterator = std::reverse_iterator<const_iterator>;

   iterator begin()
   {
      if (m_value.template is<EltTy>()) {
         return m_value.getAddrOfPtr1();
      }
      return m_value.template get<VecTy *>()->begin();
   }

   iterator end()
   {
      if (m_value.template is<EltTy>()) {
         return begin() + (m_value.isNull() ? 0 : 1);
      }
      return m_value.template get<VecTy *>()->end();
   }

   const_iterator begin() const
   {
      return (const_iterator)const_cast<TinyPtrVector*>(this)->begin();
   }

   const_iterator end() const
   {
      return (const_iterator)const_cast<TinyPtrVector*>(this)->end();
   }

   reverse_iterator rbegin()
   {
      return reverse_iterator(end());
   }

   reverse_iterator rend()
   {
      return reverse_iterator(begin());
   }

   const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(end());
   }

   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(begin());
   }

   EltTy operator[](unsigned i) const
   {
      assert(!m_value.isNull() && "can't index into an empty vector");
      if (EltTy elem = m_value.template dynamicCast<EltTy>()) {
         assert(i == 0 && "tinyvector index out of range");
         return elem;
      }

      assert(i < m_value.template get<VecTy*>()->size() &&
             "tinyvector index out of range");
      return (*m_value.template get<VecTy*>())[i];
   }

   EltTy front() const
   {
      assert(!empty() && "vector empty");
      if (EltTy elem = m_value.template dynamicCast<EltTy>()) {
         return elem;
      }
      return m_value.template get<VecTy*>()->front();
   }

   EltTy back() const
   {
      assert(!empty() && "vector empty");
      if (EltTy elem = m_value.template dynamicCast<EltTy>()) {
         return elem;
      }
      return m_value.template get<VecTy*>()->back();
   }

   void push_back(EltTy newValue)
   {
      assert(newValue && "Can't add a null value");

      // If we have nothing, add something.
      if (m_value.isNull()) {
         m_value = newValue;
         return;
      }

      // If we have a single value, convert to a vector.
      if (EltTy elem = m_value.template dynamicCast<EltTy>()) {
         m_value = new VecTy();
         m_value.template get<VecTy*>()->push_back(elem);
      }
      // Add the new value, we know we have a vector.
      m_value.template get<VecTy*>()->push_back(newValue);
   }

   void pushBack(EltTy newValue)
   {
      push_back(newValue);
   }

   void pop_back()
   {
      // If we have a single value, convert to empty.
      if (m_value.template is<EltTy>()) {
         m_value = (EltTy)nullptr;
      } else if (VecTy *vector = m_value.template get<VecTy*>()) {
         vector->pop_back();
      }
   }

   void clear()
   {
      // If we have a single value, convert to empty.
      if (m_value.template is<EltTy>()) {
         m_value = (EltTy)nullptr;
      } else if (VecTy *vector = m_value.template dynamicCast<VecTy*>()) {
         // If we have a vector form, just clear it.
         vector->clear();
      }
      // Otherwise, we're already empty.
   }

   iterator erase(iterator iter)
   {
      assert(iter >= begin() && "Iterator to erase is out of bounds.");
      assert(iter < end() && "Erasing at past-the-end iterator.");

      // If we have a single value, convert to empty.
      if (m_value.template is<EltTy>()) {
         if (iter == begin()) {
            m_value = (EltTy)nullptr;
         }
      } else if (VecTy *vector = m_value.template dynamicCast<VecTy*>()) {
         // multiple items in a vector; just do the erase, there is no
         // benefit to collapsing back to a pointer
         return vector->erase(iter);
      }
      return end();
   }

   iterator erase(iterator startIter, iterator endIter)
   {
      assert(startIter >= begin() && "Range to erase is out of bounds.");
      assert(startIter <= endIter && "Trying to erase invalid range.");
      assert(endIter <= end() && "Trying to erase past the end.");

      if (m_value.template is<EltTy>()) {
         if (startIter == begin() && startIter != endIter) {
            m_value = (EltTy)nullptr;
         }
      } else if (VecTy *vector = m_value.template dynamicCast<VecTy*>()) {
         return vector->erase(startIter, endIter);
      }
      return end();
   }

   iterator insert(iterator iter, const EltTy &element)
   {
      assert(iter >= this->begin() && "Insertion iterator is out of bounds.");
      assert(iter <= this->end() && "Inserting past the end of the vector.");
      if (iter == end()) {
         push_back(element);
         return std::prev(end());
      }
      assert(!m_value.isNull() && "Null value with non-end insert iterator.");
      if (EltTy vector = m_value.template dynamicCast<EltTy>()) {
         assert(iter == begin());
         m_value = element;
         push_back(vector);
         return begin();
      }
      return m_value.template get<VecTy*>()->insert(iter, element);
   }

   template<typename ItTy>
   iterator insert(iterator iter, ItTy from, ItTy to)
   {
      assert(iter >= this->begin() && "Insertion iterator is out of bounds.");
      assert(iter <= this->end() && "Inserting past the end of the vector.");
      if (from == to) {
         return iter;
      }
      // If we have a single value, convert to a vector.
      ptrdiff_t offset = iter - begin();
      if (m_value.isNull()) {
         if (std::next(from) == to) {
            m_value = *from;
            return begin();
         }
         m_value = new VecTy();
      } else if (EltTy V = m_value.template dynamicCast<EltTy>()) {
         m_value = new VecTy();
         m_value.template get<VecTy*>()->push_back(V);
      }
      return m_value.template get<VecTy*>()->insert(begin() + offset, from, to);
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_TINY_PTR_VECTOR_H
