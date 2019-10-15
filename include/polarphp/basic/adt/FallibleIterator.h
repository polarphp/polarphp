//===--- FallibleIterator.h - Wrapper for fallible iterators ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
// Created by polarboy on 2019/09/23.

#ifndef POLARPHP_BASIC_ADT_FallibleIterator_H
#define POLARPHP_BASIC_ADT_FallibleIterator_H

#include "polarphp/basic/adt/PointerIntPair.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include "polarphp/utils/Error.h"

#include <type_traits>

namespace polar::basic {

using polar::utils::Error;

/// A wrapper class for fallible iterators.
///
///   The FallibleIterator template wraps an underlying iterator-like class
/// whose increment and decrement operations are replaced with fallible versions
/// like:
///
///   @code{.cpp}
///   Error inc();
///   Error dec();
///   @endcode
///
///   It produces an interface that is (mostly) compatible with a traditional
/// c++ iterator, including ++ and -- operators that do not fail.
///
///   Instances of the wrapper are constructed with an instance of the
/// underlying iterator and (for non-end iterators) a reference to an Error
/// instance. If the underlying increment/decrement operations fail, the Error
/// is returned via this reference, and the resulting iterator value set to an
/// end-of-range sentinel value. This enables the following loop idiom:
///
///   @code{.cpp}
///   class Archive { // E.g. Potentially malformed on-disk archive
///   public:
///     FallibleIterator<ArchiveChildItr> children_begin(Error &error);
///     FallibleIterator<ArchiveChildItr> children_end();
///     iterator_range<FallibleIterator<ArchiveChildItr>>
///     children(Error &error) {
///       return make_range(children_begin(error), children_end());
///     //...
///   };
///
///   void walk(Archive &A) {
///     Error error = Error::success();
///     for (auto &C : A.children(error)) {
///       // Loop body only entered when increment succeeds.
///     }
///     if (error) {
///       // handle error.
///     }
///   }
///   @endcode
///
///   The wrapper marks the referenced Error as unchecked after each increment
/// and/or decrement operation, and clears the unchecked flag when a non-end
/// value is compared against end (since, by the increment invariant, not being
/// an end value proves that there was no error, and is equivalent to checking
/// that the Error is success). This allows early exits from the loop body
/// without requiring redundant error checks.
template <typename Underlying>
class FallibleIterator
{
private:
   template <typename T>
   using enable_if_struct_deref_supported = std::enable_if<
   !std::is_void<decltype(std::declval<T>().operator->())>::value,
   decltype(std::declval<T>().operator->())>;

public:
   /// Construct a fallible iterator that *cannot* be used as an end-of-range
   /// value.
   ///
   /// A value created by this method can be dereferenced, incremented,
   /// decremented and compared, providing the underlying type supports it.
   ///
   /// The error that is passed in will be initially marked as checked, so if the
   /// iterator is not used at all the Error need not be checked.
   static FallibleIterator makeIterator(Underlying iter, Error &error)
   {
      (void)!!error;
      return FallibleIterator(std::move(iter), &error);
   }

   /// Construct a fallible iteratro that can be used as an end-of-range value.
   ///
   /// A value created by this method can be dereferenced (if the underlying
   /// value points at a valid value) and compared, but not incremented or
   /// decremented.
   static FallibleIterator end(Underlying iter)
   {
      return FallibleIterator(std::move(iter), nullptr);
   }

   /// Forward dereference to the underlying iterator.
   auto operator*() -> decltype(*std::declval<Underlying>())
   {
      return *m_iter;
   }

   /// Forward const dereference to the underlying iterator.
   auto operator*() const -> decltype(*std::declval<const Underlying>())
   {
      return *m_iter;
   }

   /// Forward structure dereference to the underlying iterator (if the
   /// underlying iterator supports it).
   template <typename T = Underlying>
   typename enable_if_struct_deref_supported<T>::type operator->()
   {
      return m_iter.operator->();
   }

   /// Forward const structure dereference to the underlying iterator (if the
   /// underlying iterator supports it).
   template <typename T = Underlying>
   typename enable_if_struct_deref_supported<const T>::type operator->() const
   {
      return m_iter.operator->();
   }

   /// Increment the fallible iterator.
   ///
   /// If the underlying 'inc' operation fails, this will set the Error value
   /// and update this iterator value to point to end-of-range.
   ///
   /// The Error value is marked as needing checking, regardless of whether the
   /// 'inc' operation succeeds or fails.
   FallibleIterator &operator++()
   {
      assert(getErrPtr() && "Cannot increment end iterator");
      if (auto error = m_iter.inc()) {
         handleError(std::move(error));
      } else {
         resetCheckedFlag();
      }
      return *this;
   }

   /// Decrement the fallible iterator.
   ///
   /// If the underlying 'dec' operation fails, this will set the Error value
   /// and update this iterator value to point to end-of-range.
   ///
   /// The Error value is marked as needing checking, regardless of whether the
   /// 'dec' operation succeeds or fails.
   FallibleIterator &operator--()
   {
      assert(getErrPtr() && "Cannot decrement end iterator");
      if (auto error = m_iter.dec()) {
         handleError(std::move(error));
      } else {
         resetCheckedFlag();
      }
      return *this;
   }

   /// Compare fallible iterators for equality.
   ///
   /// Returns true if both lhs and rhs are end-of-range values, or if both are
   /// non-end-of-range values whose underlying iterator values compare equal.
   ///
   /// If this is a comparison between an end-of-range iterator and a
   /// non-end-of-range iterator, then the Error (referenced by the
   /// non-end-of-range value) is marked as checked: Since all
   /// increment/decrement operations result in an end-of-range value, comparing
   /// false against end-of-range is equivalent to checking that the Error value
   /// is success. This flag management enables early returns from loop bodies
   /// without redundant Error checks.
   friend bool operator==(const FallibleIterator &lhs,
                          const FallibleIterator &rhs)
   {
      // If both iterators are in the end state they compare
      // equal, regardless of whether either is valid.
      if (lhs.isEnd() && rhs.isEnd()) {
         return true;
      }
      assert(lhs.isValid() && rhs.isValid() &&
             "Invalid iterators can only be compared against end");
      bool equal = lhs.m_iter == rhs.m_iter;

      // If the iterators differ and this is a comparison against end then mark
      // the Error as checked.
      if (!equal) {
         if (lhs.isEnd()) {
            (void)!!*rhs.getErrPtr();
         } else {
            (void)!!*lhs.getErrPtr();
         }
      }
      return equal;
   }

   /// Compare fallible iterators for inequality.
   ///
   /// See notes for operator==.
   friend bool operator!=(const FallibleIterator &lhs,
                          const FallibleIterator &rhs)
   {
      return !(lhs == rhs);
   }

private:
   FallibleIterator(Underlying iter, Error *error)
      : m_iter(std::move(iter)),
        m_errorState(error, false)
   {}

   Error *getErrPtr() const
   {
      return m_errorState.getPointer();
   }

   bool isEnd() const
   {
      return getErrPtr() == nullptr;
   }

   bool isValid() const
   {
      return !m_errorState.getInt();
   }

   void handleError(Error error)
   {
      *getErrPtr() = std::move(error);
      m_errorState.setPointer(nullptr);
      m_errorState.setInt(true);
   }

   void resetCheckedFlag()
   {
      *getErrPtr() = Error::getSuccess();
   }

   Underlying m_iter;
   mutable PointerIntPair<Error *, 1> m_errorState;
};

/// Convenience wrapper to make a FallibleIterator value from an instance
/// of an underlying iterator and an Error reference.
template <typename Underlying>
FallibleIterator<Underlying> make_fallible_iter(Underlying iter, Error &error)
{
   return FallibleIterator<Underlying>::makeIterator(std::move(iter), error);
}

/// Convenience wrapper to make a FallibleIterator end value from an instance
/// of an underlying iterator.
template <typename Underlying>
FallibleIterator<Underlying> make_fallible_end(Underlying endMark)
{
   return FallibleIterator<Underlying>::end(std::move(endMark));
}

template <typename Underlying>
IteratorRange<FallibleIterator<Underlying>>
make_fallible_range(Underlying iter, Underlying endMark, Error &error)
{
   return make_range(make_fallible_iter(std::move(iter), error),
                     make_fallible_end(std::move(endMark)));
}

} // polar::basic

#endif // POLARPHP_BASIC_ADT_FallibleIterator_H
