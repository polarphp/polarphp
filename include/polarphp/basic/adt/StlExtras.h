// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/30.

#ifndef POLARPHP_BASIC_ADT_STL_EXTRAS_H
#define POLARPHP_BASIC_ADT_STL_EXTRAS_H

#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/Iterator.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/global/AbiBreaking.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <optional>

#ifdef EXPENSIVE_CHECKS
#include <random> // for std::mt19937
#endif

namespace polar {
namespace basic {

// Only used by compiler if both template types are the same.  Useful when
// using SFINAE to test for the existence of member functions.
template <typename T, T>
struct SameType;

namespace internal {

template <typename RangeT>
using IterOfRange = decltype(std::begin(std::declval<RangeT &>()));

template <typename RangeT>
using ValueOfRange = typename std::remove_reference<decltype(
*std::begin(std::declval<RangeT &>()))>::type;

} // end namespace internal

//===----------------------------------------------------------------------===//
//     Extra additions to <type_traits>
//===----------------------------------------------------------------------===//

template <typename T>
struct negation : std::integral_constant<bool, !bool(T::value)> {};

template <typename...> struct conjunction : std::true_type {};
template <typename B1> struct conjunction<B1> : B1 {};
template <typename B1, typename... Bn>
struct conjunction<B1, Bn...>
      : std::conditional<bool(B1::value), conjunction<Bn...>, B1>::type {};

//===----------------------------------------------------------------------===//
//     Extra additions to <functional>
//===----------------------------------------------------------------------===//

template <typename Ty>
struct Identity
{
   using ArgumentType = Ty;

   Ty &operator()(Ty &self) const
   {
      return self;
   }

   const Ty &operator()(const Ty &self) const
   {
      return self;
   }
};

template <typename Ty>
struct LessPtr
{
   bool operator()(const Ty* left, const Ty* right) const
   {
      return *left < *right;
   }
};

template <typename Ty>
struct GreaterPtr
{
   bool operator()(const Ty* left, const Ty* right) const
   {
      return *right < *left;
   }
};

/// An efficient, type-erasing, non-owning reference to a callable. This is
/// intended for use as the type of a function parameter that is not used
/// after the function in question returns.
///
/// This class does not own the callable, so it is not in general safe to store
/// a FunctionRef.
template<typename Fn> class FunctionRef;

template<typename Ret, typename ...Params>
class FunctionRef<Ret(Params...)>
{
   Ret (*callback)(intptr_t callable, Params ...params) = nullptr;
   intptr_t callable;

   template<typename Callable>
   static Ret callback_fn(intptr_t callable, Params ...params)
   {
      return (*reinterpret_cast<Callable*>(callable))(
               std::forward<Params>(params)...);
   }

   public:
   FunctionRef() = default;

   template <typename Callable>
   FunctionRef(Callable &&callable,
               typename std::enable_if<
               !std::is_same<typename std::remove_reference<Callable>::type,
               FunctionRef>::value>::type * = nullptr)
      : callback(callback_fn<typename std::remove_reference<Callable>::type>),
        callable(reinterpret_cast<intptr_t>(&callable))
   {}

   Ret operator()(Params ...params) const
   {
      return callback(callable, std::forward<Params>(params)...);
   }

   operator bool() const
   {
      return callback;
   }
};

// deleter - Very very very simple method that is used to invoke operator
// delete on something.  It is used like this:
//
//   for_each(V.begin(), B.end(), deleter<Interval>);
template <typename T>
inline void deleter(T *ptr)
{
   delete ptr;
}

//===----------------------------------------------------------------------===//
//     Extra additions to <iterator>
//===----------------------------------------------------------------------===//

namespace adlinternal {

using std::begin;

template <typename ContainerTy>
auto adl_begin(ContainerTy &&container)
-> decltype(begin(std::forward<ContainerTy>(container)))
{
   return begin(std::forward<ContainerTy>(container));
}

using std::end;

template <typename ContainerTy>
auto adl_end(ContainerTy &&container)
-> decltype(end(std::forward<ContainerTy>(container)))
{
   return end(std::forward<ContainerTy>(container));
}

using std::swap;

template <typename T>
void adl_swap(T &&lhs, T &&rhs) noexcept(noexcept(swap(std::declval<T>(),
                                                       std::declval<T>())))
{
   swap(std::forward<T>(lhs), std::forward<T>(rhs));
}

} // end namespace adlinternal

template <typename ContainerTy>
auto adl_begin(ContainerTy &&container)
-> decltype(adlinternal::adl_begin(std::forward<ContainerTy>(container)))
{
   return adlinternal::adl_begin(std::forward<ContainerTy>(container));
}

template <typename ContainerTy>
auto adl_end(ContainerTy &&container)
-> decltype(adlinternal::adl_end(std::forward<ContainerTy>(container)))
{
   return adlinternal::adl_end(std::forward<ContainerTy>(container));
}

template <typename T>
void adl_swap(T &&lhs, T &&rhs) noexcept(
      noexcept(adlinternal::adl_swap(std::declval<T>(), std::declval<T>())))
{
   adlinternal::adl_swap(std::forward<T>(lhs), std::forward<T>(rhs));
}

/// Test whether \p rangeOrContainer is empty. Similar to C++17 std::empty.
template <typename T>
constexpr bool empty(const T &rangeOrContainer)
{
   return adl_begin(rangeOrContainer) == adl_end(rangeOrContainer);
}

// MappedIterator - This is a simple iterator adapter that causes a function to
// be applied whenever operator* is invoked on the iterator.

template <typename ItTy, typename FuncTy,
          typename FuncReturnTy =
          decltype(std::declval<FuncTy>()(*std::declval<ItTy>()))>
class MappedIterator
      : public IteratorAdaptorBase<
      MappedIterator<ItTy, FuncTy>, ItTy,
      typename std::iterator_traits<ItTy>::iterator_category,
      typename std::remove_reference<FuncReturnTy>::type>
{
public:
   MappedIterator(ItTy iter, FuncTy func)
      : MappedIterator::IteratorAdaptorBase(std::move(iter)), m_func(std::move(func))
   {}

   ItTy getCurrent()
   {
      return this->m_iter;
   }

   FuncReturnTy operator*()
   {
      return m_func(*this->m_iter);
   }

private:
   FuncTy m_func;
};

// map_iterator - Provide a convenient way to create MappedIterators, just like
// make_pair is useful for creating pairs...
template <typename ItTy, typename FuncTy>
inline MappedIterator<ItTy, FuncTy> map_iterator(ItTy iter, FuncTy func)
{
   return MappedIterator<ItTy, FuncTy>(std::move(iter), std::move(func));
}

/// Helper to determine if type T has a member called rbegin().
template <typename Ty>
class HasRbeginImpl
{
   using yes = char[1];
   using no = char[2];

   template <typename Inner>
   static yes& test(Inner *iter, decltype(iter->rbegin()) * = nullptr);

   template <typename>
   static no& test(...);

public:
   static const bool value = sizeof(test<Ty>(nullptr)) == sizeof(yes);
};

/// Metafunction to determine if T& or T has a member called rbegin().
template <typename Ty>
struct has_rbegin : HasRbeginImpl<typename std::remove_reference<Ty>::type> {
};

// Returns an IteratorRange over the given container which iterates in reverse.
// Note that the container must have rbegin()/rend() methods for this to work.
template <typename ContainerTy>
auto reverse(ContainerTy &&container,
             typename std::enable_if<has_rbegin<ContainerTy>::value>::type * =
      nullptr) -> decltype(make_range(container.rbegin(), container.rend()))
{
   return make_range(container.rbegin(), container.rend());
}

// Returns a std::reverse_iterator wrapped around the given iterator.
template <typename IteratorTy>
std::reverse_iterator<IteratorTy> make_reverse_iterator(IteratorTy iter)
{
   return std::reverse_iterator<IteratorTy>(iter);
}

// Returns an IteratorRange over the given container which iterates in reverse.
// Note that the container must have begin()/end() methods which return
// bidirectional iterators for this to work.
template <typename ContainerTy>
auto reverse(
      ContainerTy &&container,
      typename std::enable_if<!has_rbegin<ContainerTy>::value>::type * = nullptr)
-> decltype(make_range(polar::basic::make_reverse_iterator(std::end(container)),
                       polar::basic::make_reverse_iterator(std::begin(container))))
{
   return make_range(polar::basic::make_reverse_iterator(std::end(container)),
                     polar::basic::make_reverse_iterator(std::begin(container)));
}

/// An iterator adaptor that filters the elements of given inner iterators.
///
/// The predicate parameter should be a callable object that accepts the wrapped
/// iterator's reference type and returns a bool. When incrementing or
/// decrementing the iterator, it will call the predicate on each element and
/// skip any where it returns false.
///
/// \code
///   int A[] = { 1, 2, 3, 4 };
///   auto R = make_filter_range(A, [](int N) { return N % 2 == 1; });
///   // R contains { 1, 3 }.
/// \endcode
///
/// Note: FilterIteratorBase implements support for forward iteration.
/// FilterIteratorImpl exists to provide support for bidirectional iteration,
/// conditional on whether the wrapped iterator supports it.
///
template <typename WrappedIteratorT, typename PredicateT, typename IterTag>
class FilterIteratorBase
      : public IteratorAdaptorBase<
      FilterIteratorBase<WrappedIteratorT, PredicateT, IterTag>,
      WrappedIteratorT,
      typename std::common_type<
      IterTag, typename std::iterator_traits<
      WrappedIteratorT>::iterator_category>::type>
{
   using BaseT = IteratorAdaptorBase<
   FilterIteratorBase<WrappedIteratorT, PredicateT, IterTag>,
   WrappedIteratorT,
   typename std::common_type<
   IterTag, typename std::iterator_traits<
   WrappedIteratorT>::iterator_category>::type>;

protected:
   WrappedIteratorT m_end;
   PredicateT m_pred;

   void findNextValid()
   {
      while (this->m_iter != m_end && !m_pred(*this->m_iter)) {
         BaseT::operator++();
      }
   }

   // Construct the iterator. The begin iterator needs to know where the end
   // is, so that it can properly stop when it gets there. The end iterator only
   // needs the predicate to support bidirectional iteration.
   FilterIteratorBase(WrappedIteratorT begin, WrappedIteratorT end,
                      PredicateT pred)
      : BaseT(begin), m_end(end), m_pred(pred)
   {
      findNextValid();
   }

public:
   using BaseT::operator++;

   FilterIteratorBase &operator++()
   {
      BaseT::operator++();
      findNextValid();
      return *this;
   }
};

/// Specialization of FilterIteratorBase for forward iteration only.
template <typename WrappedIteratorT, typename PredicateT,
          typename IterTag = std::forward_iterator_tag>
class FilterIteratorImpl
      : public FilterIteratorBase<WrappedIteratorT, PredicateT, IterTag>
{
   using BaseT = FilterIteratorBase<WrappedIteratorT, PredicateT, IterTag>;

public:
   FilterIteratorImpl(WrappedIteratorT begin, WrappedIteratorT end,
                      PredicateT pred)
      : BaseT(begin, end, pred) {}
};

/// Specialization of FilterIteratorBase for bidirectional iteration.
template <typename WrappedIteratorT, typename PredicateT>
class FilterIteratorImpl<WrappedIteratorT, PredicateT,
      std::bidirectional_iterator_tag>
      : public FilterIteratorBase<WrappedIteratorT, PredicateT,
      std::bidirectional_iterator_tag> {
   using BaseT = FilterIteratorBase<WrappedIteratorT, PredicateT,
   std::bidirectional_iterator_tag>;
   void findPrevValid()
   {
      while (!this->m_pred(*this->m_iter))
         BaseT::operator--();
   }

public:
   using BaseT::operator--;

   FilterIteratorImpl(WrappedIteratorT begin, WrappedIteratorT end,
                      PredicateT pred)
      : BaseT(begin, end, pred) {}

   FilterIteratorImpl &operator--()
   {
      BaseT::operator--();
      findPrevValid();
      return *this;
   }
};

namespace internal {

template <bool is_bidirectional>
struct fwd_or_bidi_tag_impl
{
   using type = std::forward_iterator_tag;
};

template <>
struct fwd_or_bidi_tag_impl<true>
{
   using type = std::bidirectional_iterator_tag;
};

/// Helper which sets its type member to forward_iterator_tag if the category
/// of \p IterT does not derive from bidirectional_iterator_tag, and to
/// bidirectional_iterator_tag otherwise.
template <typename IterT>
struct fwd_or_bidi_tag
{
   using type = typename fwd_or_bidi_tag_impl<std::is_base_of<
   std::bidirectional_iterator_tag,
   typename std::iterator_traits<IterT>::iterator_category>::value>::type;
};

} // namespace internal

/// Defines FilterIterator to a suitable specialization of
/// FilterIteratorImpl, based on the underlying iterator's category.
template <typename WrappedIteratorT, typename PredicateT>
using FilterIterator = FilterIteratorImpl<
WrappedIteratorT, PredicateT,
typename internal::fwd_or_bidi_tag<WrappedIteratorT>::type>;

/// Convenience function that takes a range of elements and a predicate,
/// and return a new FilterIterator range.
///
/// FIXME: Currently if RangeT && is a rvalue reference to a temporary, the
/// lifetime of that temporary is not kept by the returned range object, and the
/// temporary is going to be dropped on the floor after the make_IteratorRange
/// full expression that contains this function call.
template <typename RangeT, typename PredicateT>
IteratorRange<FilterIterator<internal::IterOfRange<RangeT>, PredicateT>>
make_filter_range(RangeT &&range, PredicateT pred)
{
   using FilterIteratorT =
   FilterIterator<internal::IterOfRange<RangeT>, PredicateT>;
   return make_range(FilterIteratorT(std::begin(std::forward<RangeT>(range)),
                                     std::end(std::forward<RangeT>(range)),
                                     pred),
                     FilterIteratorT(std::end(std::forward<RangeT>(range)),
                                     std::end(std::forward<RangeT>(range)),
                                     pred));
}


/// A pseudo-iterator adaptor that is designed to implement "early increment"
/// style loops.
///
/// This is *not a normal iterator* and should almost never be used directly. It
/// is intended primarily to be used with range based for loops and some range
/// algorithms.
///
/// The iterator isn't quite an `OutputIterator` or an `InputIterator` but
/// somewhere between them. The constraints of these iterators are:
///
/// - On construction or after being incremented, it is comparable and
///   dereferencable. It is *not* incrementable.
/// - After being dereferenced, it is neither comparable nor dereferencable, it
///   is only incrementable.
///
/// This means you can only dereference the iterator once, and you can only
/// increment it once between dereferences.
template <typename WrappedIteratorT>
class early_inc_iterator_impl
      : public IteratorAdaptorBase<early_inc_iterator_impl<WrappedIteratorT>,
      WrappedIteratorT, std::input_iterator_tag>
{
   using BaseT =
   IteratorAdaptorBase<early_inc_iterator_impl<WrappedIteratorT>,
   WrappedIteratorT, std::input_iterator_tag>;

   using PointerT = typename std::iterator_traits<WrappedIteratorT>::pointer;

protected:
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
   bool m_isEarlyIncremented = false;
#endif

public:
   early_inc_iterator_impl(WrappedIteratorT iter) : BaseT(iter) {}

   using BaseT::operator*;
   typename BaseT::reference operator*()
   {
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
      assert(!m_isEarlyIncremented && "Cannot dereference twice!");
      m_isEarlyIncremented = true;
#endif
      return *(this->m_iter)++;
   }

   using BaseT::operator++;
   early_inc_iterator_impl &operator++()
   {
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
      assert(m_isEarlyIncremented && "Cannot increment before dereferencing!");
      m_isEarlyIncremented = false;
#endif
      return *this;
   }

   using BaseT::operator==;
   bool operator==(const early_inc_iterator_impl &rhs) const
   {
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
      assert(!m_isEarlyIncremented && "Cannot compare after dereferencing!");
#endif
      return BaseT::operator==(rhs);
   }
};

/// Make a range that does early increment to allow mutation of the underlying
/// range without disrupting iteration.
///
/// The underlying iterator will be incremented immediately after it is
/// dereferenced, allowing deletion of the current node or insertion of nodes to
/// not disrupt iteration provided they do not invalidate the *next* iterator --
/// the current iterator can be invalidated.
///
/// This requires a very exact pattern of use that is only really suitable to
/// range based for loops and other range algorithms that explicitly guarantee
/// to dereference exactly once each element, and to increment exactly once each
/// element.
template <typename RangeT>
IteratorRange<early_inc_iterator_impl<internal::IterOfRange<RangeT>>>
make_early_inc_range(RangeT &&range) {
   using EarlyIncIteratorT =
   early_inc_iterator_impl<internal::IterOfRange<RangeT>>;
   return make_range(EarlyIncIteratorT(std::begin(std::forward<RangeT>(range))),
                     EarlyIncIteratorT(std::end(std::forward<RangeT>(range))));
}

// forward declarations required by ZipShortest/ZipFirst
template <typename R, typename UnaryPredicate>
bool all_of(R &&range, UnaryPredicate pred);

template <size_t... I>
struct index_sequence;

template <typename... Ts>
struct index_sequence_for;

namespace internal
{

using std::declval;

// We have to alias this since inlining the actual type at the usage site
// in the parameter list of IteratorFacadeBase<> below ICEs MSVC 2017.
template<typename... Iters>
struct ZipTupleType
{
   using type = std::tuple<decltype(*declval<Iters>())...>;
};

template <typename ZipType, typename... Iters>
using zip_traits = IteratorFacadeBase<
ZipType, typename std::common_type<std::bidirectional_iterator_tag,
typename std::iterator_traits<
Iters>::iterator_category...>::type,
// ^ TODO: Implement random access methods.
typename ZipTupleType<Iters...>::type,
typename std::iterator_traits<typename std::tuple_element<
0, std::tuple<Iters...>>::type>::difference_type,
// ^ FIXME: This follows boost::make_zip_iterator's assumption that all
// inner iterators have the same difference_type. It would fail if, for
// instance, the second field's difference_type were non-numeric while the
// first is.
typename ZipTupleType<Iters...>::type *,
typename ZipTupleType<Iters...>::type>;

template <typename ZipType, typename... Iters>
struct ZipCommon : public zip_traits<ZipType, Iters...> {
   using Base = zip_traits<ZipType, Iters...>;
   using value_type = typename Base::value_type;

   std::tuple<Iters...> m_iterators;

protected:
   template <size_t... Ns>
   value_type deref(index_sequence<Ns...>) const
   {
      return value_type(*std::get<Ns>(m_iterators)...);
   }

   template <size_t... Ns>
   decltype(m_iterators) tupInc(index_sequence<Ns...>) const
   {
      return std::tuple<Iters...>(std::next(std::get<Ns>(m_iterators))...);
   }

   template <size_t... Ns>
   decltype(m_iterators) tupDec(index_sequence<Ns...>) const
   {
      return std::tuple<Iters...>(std::prev(std::get<Ns>(m_iterators))...);
   }

public:
   ZipCommon(Iters &&... ts) : m_iterators(std::forward<Iters>(ts)...)
   {}

   value_type operator*()
   {
      return deref(index_sequence_for<Iters...>{});
   }

   const value_type operator*() const
   {
      return deref(index_sequence_for<Iters...>{});
   }

   ZipType &operator++()
   {
      m_iterators = tupInc(index_sequence_for<Iters...>{});
      return *reinterpret_cast<ZipType *>(this);
   }

   ZipType &operator--()
   {
      static_assert(Base::IsBidirectional,
                    "All inner iterators must be at least bidirectional.");
      m_iterators = tupDec(index_sequence_for<Iters...>{});
      return *reinterpret_cast<ZipType *>(this);
   }
};

template <typename... Iters>
struct ZipFirst : public ZipCommon<ZipFirst<Iters...>, Iters...>
{
   using Base = ZipCommon<ZipFirst<Iters...>, Iters...>;

   bool operator==(const ZipFirst<Iters...> &other) const
   {
      return std::get<0>(this->m_iterators) == std::get<0>(other.m_iterators);
   }

   ZipFirst(Iters &&... ts) : Base(std::forward<Iters>(ts)...) {}
};

template <typename... Iters>
class ZipShortest : public ZipCommon<ZipShortest<Iters...>, Iters...>
{
   template <size_t... Ns>
   bool test(const ZipShortest<Iters...> &other, index_sequence<Ns...>) const
   {
      return all_of(std::initializer_list<bool>{std::get<Ns>(this->m_iterators) !=
                                                std::get<Ns>(other.m_iterators)...},
                    Identity<bool>{});
   }

public:
   using Base = ZipCommon<ZipShortest<Iters...>, Iters...>;

   ZipShortest(Iters &&... ts) : Base(std::forward<Iters>(ts)...)
   {}

   bool operator==(const ZipShortest<Iters...> &other) const
   {
      return !test(other, index_sequence_for<Iters...>{});
   }
};

template <template <typename...> class ItType, typename... Args>
class Zippy
{
public:
   using iterator = ItType<decltype(std::begin(std::declval<Args>()))...>;
   using iterator_category = typename iterator::iterator_category;
   using value_type = typename iterator::value_type;
   using difference_type = typename iterator::difference_type;
   using pointer = typename iterator::pointer;
   using reference = typename iterator::reference;

private:
   std::tuple<Args...> ts;

   template <size_t... Ns>
   iterator beginImpl(index_sequence<Ns...>) const
   {
      return iterator(std::begin(std::get<Ns>(ts))...);
   }

   template <size_t... Ns>
   iterator endImpl(index_sequence<Ns...>) const
   {
      return iterator(std::end(std::get<Ns>(ts))...);
   }

public:
   Zippy(Args &&... ts_) : ts(std::forward<Args>(ts_)...)
   {}

   iterator begin() const
   {
      return beginImpl(index_sequence_for<Args...>{});
   }

   iterator end() const
   {
      return endImpl(index_sequence_for<Args...>{});
   }
};

} // end namespace internal

/// zip iterator for two or more iteratable types.
template <typename T, typename U, typename... Args>
internal::Zippy<internal::ZipShortest, T, U, Args...> zip(T &&t, U &&u,
                                                          Args &&... args)
{
   return internal::Zippy<internal::ZipShortest, T, U, Args...>(
            std::forward<T>(t), std::forward<U>(u), std::forward<Args>(args)...);
}

/// zip iterator that, for the sake of efficiency, assumes the first iteratee to
/// be the shortest.
template <typename T, typename U, typename... Args>
internal::Zippy<internal::ZipFirst, T, U, Args...>
zip_first(T &&t, U &&u, Args &&... args)
{
   return internal::Zippy<internal::ZipFirst, T, U, Args...>(
            std::forward<T>(t), std::forward<U>(u), std::forward<Args>(args)...);
}

/// Iterator wrapper that concatenates sequences together.
///
/// This can concatenate different iterators, even with different types, into
/// a single iterator provided the value types of all the concatenated
/// iterators expose `reference` and `pointer` types that can be converted to
/// `ValueT &` and `ValueT *` respectively. It doesn't support more
/// interesting/customized pointer or reference types.
///
/// Currently this only supports forward or higher iterator categories as
/// inputs and always exposes a forward iterator interface.
template <typename ValueT, typename... IterTs>
class ConcatIterator
      : public IteratorFacadeBase<ConcatIterator<ValueT, IterTs...>,
      std::forward_iterator_tag, ValueT>
{
   using BaseT = typename ConcatIterator::IteratorFacadeBase;

   /// We store both the current and end iterators for each concatenated
   /// sequence in a tuple of pairs.
   ///
   /// Note that something like IteratorRange seems nice at first here, but the
   /// range properties are of little benefit and end up getting in the way
   /// because we need to do mutation on the current iterators.
   std::tuple<IterTs...> m_begins;
   std::tuple<IterTs...> m_ends;

   /// Attempts to increment a specific iterator.
   ///
   /// Returns true if it was able to increment the iterator. Returns false if
   /// the iterator is already at the end iterator.
   template <size_t Index>
   bool incrementHelper()
   {
      auto &begin = std::get<Index>(m_begins);
      auto &end = std::get<Index>(m_ends);
      if (begin == end) {
         return false;
      }
      ++begin;
      return true;
   }

   /// Increments the first non-end iterator.
   ///
   /// It is an error to call this with all iterators at the end.
   template <size_t... Ns>
   void increment(index_sequence<Ns...>)
   {
      // Build a sequence of functions to increment each iterator if possible.
      bool (ConcatIterator::*IncrementHelperFns[])() = {
            &ConcatIterator::incrementHelper<Ns>...};

      // Loop over them, and stop as soon as we succeed at incrementing one.
      for (auto &IncrementHelperFn : IncrementHelperFns) {
         if ((this->*IncrementHelperFn)()) {
            return;
         }
      }
      polar_unreachable("Attempted to increment an end concat iterator!");
   }

   /// Returns null if the specified iterator is at the end. Otherwise,
   /// dereferences the iterator and returns the address of the resulting
   /// reference.
   template <size_t Index>
   ValueT *getHelper() const
   {
      auto &begin = std::get<Index>(m_begins);
      auto &end = std::get<Index>(m_ends);
      if (begin == end) {
         return nullptr;
      }
      return &*begin;
   }

   /// Finds the first non-end iterator, dereferences, and returns the resulting
   /// reference.
   ///
   /// It is an error to call this with all iterators at the end.
   template <size_t... Ns>
   ValueT &get(index_sequence<Ns...>) const
   {
      // Build a sequence of functions to get from iterator if possible.
      ValueT *(ConcatIterator::*GetHelperFns[])() const = {
            &ConcatIterator::getHelper<Ns>...};

      // Loop over them, and return the first result we find.
      for (auto &GetHelperFn : GetHelperFns) {
         if (ValueT *ptr = (this->*GetHelperFn)()) {
            return *ptr;
         }
      }
      polar_unreachable("Attempted to get a pointer from an end concat iterator!");
   }

public:
   /// Constructs an iterator from a squence of ranges.
   ///
   /// We need the full range to know how to switch between each of the
   /// iterators.
   template <typename... RangeTs>
   explicit ConcatIterator(RangeTs &&... ranges)
      : m_begins(std::begin(ranges)...),
        m_ends(std::end(ranges)...)
   {}

   using BaseT::operator++;

   ConcatIterator &operator++()
   {
      increment(index_sequence_for<IterTs...>());
      return *this;
   }

   ValueT &operator*() const
   {
      return get(index_sequence_for<IterTs...>());
   }

   bool operator==(const ConcatIterator &other) const
   {
      return m_begins == other.m_begins && m_ends == other.m_ends;
   }
};

namespace internal {

/// Helper to store a sequence of ranges being concatenated and access them.
///
/// This is designed to facilitate providing actual storage when temporaries
/// are passed into the constructor such that we can use it as part of range
/// based for loops.
template <typename ValueT, typename... RangeTs>
class ConcatRange
{
public:
   using iterator =
   ConcatIterator<ValueT,
   decltype(std::begin(std::declval<RangeTs &>()))...>;

private:
   std::tuple<RangeTs...> m_ranges;

   template <size_t... Ns> iterator beginImpl(index_sequence<Ns...>)
   {
      return iterator(std::get<Ns>(m_ranges)...);
   }
   template <size_t... Ns> iterator endImpl(index_sequence<Ns...>)
   {
      return iterator(make_range(std::end(std::get<Ns>(m_ranges)),
                                 std::end(std::get<Ns>(m_ranges)))...);
   }

public:
   ConcatRange(RangeTs &&... ranges)
      : m_ranges(std::forward<RangeTs>(ranges)...)
   {}

   iterator begin()
   {
      return beginImpl(index_sequence_for<RangeTs...>{});
   }

   iterator end()
   {
      return endImpl(index_sequence_for<RangeTs...>{});
   }
};

} // end namespace internal

/// Concatenated range across two or more ranges.
///
/// The desired value type must be explicitly specified.
template <typename ValueT, typename... RangeTs>
internal::ConcatRange<ValueT, RangeTs...> concat(RangeTs &&... ranges)
{
   static_assert(sizeof...(RangeTs) > 1,
                 "Need more than one range to concatenate!");
   return internal::ConcatRange<ValueT, RangeTs...>(
            std::forward<RangeTs>(ranges)...);
}

//===----------------------------------------------------------------------===//
//     Extra additions to <utility>
//===----------------------------------------------------------------------===//

///  Function object to check whether the first component of a std::pair
/// compares less than the first component of another std::pair.
struct LessFirst
{
   template <typename T> bool operator()(const T &lhs, const T &rhs) const
   {
      return lhs.first < rhs.first;
   }
};

///  Function object to check whether the second component of a std::pair
/// compares less than the second component of another std::pair.
struct LessSecond
{
   template <typename T> bool operator()(const T &lhs, const T &rhs) const
   {
      return lhs.second < rhs.second;
   }
};


/// \brief Function object to apply a binary function to the first component of
/// a std::pair.
template<typename FuncTy>
struct on_first
{
   FuncTy func;
   template <typename T>
   auto operator()(const T &lhs, const T &rhs) const
   -> decltype(func(lhs.first, rhs.first))
   {
      return func(lhs.first, rhs.first);
   }
};

// A subset of N3658. More stuff can be added as-needed.

///  Represents a compile-time sequence of integers.
template <typename T, T... I>
struct integer_sequence
{
   using value_type = T;
   static constexpr size_t getSize()
   {
      return sizeof...(I);
   }
};

///  Alias for the common case of a sequence of size_ts.
template <size_t... Index>
struct index_sequence : integer_sequence<std::size_t, Index...>
{};

template <std::size_t N, std::size_t... Index>
struct build_index_impl : build_index_impl<N - 1, N - 1, Index...>
{};

template <std::size_t... I>
struct build_index_impl<0, I...> : index_sequence<I...>
{};

///  Creates a compile-time integer sequence for a parameter pack.
template <typename... Ts>
struct index_sequence_for : build_index_impl<sizeof...(Ts)>
{};

/// Utility type to build an inheritance chain that makes it easy to rank
/// overload candidates.
template <int N>
struct rank : rank<N - 1>
{};

template <>
struct rank<0>
{};

///  traits class for checking whether type T is one of any of the given
/// types in the variadic list.
template <typename T, typename... Ts>
struct is_one_of
{
   static const bool value = false;
};

template <typename T, typename U, typename... Ts>
struct is_one_of<T, U, Ts...> {
   static const bool value =
         std::is_same<T, U>::value || is_one_of<T, Ts...>::value;
};

///  traits class for checking whether type T is a base class for all
///  the given types in the variadic list.
template <typename T, typename... Ts>
struct are_base_of
{
   static const bool value = true;
};

template <typename T, typename U, typename... Ts>
struct are_base_of<T, U, Ts...>
{
   static const bool value =
         std::is_base_of<T, U>::value && are_base_of<T, Ts...>::value;
};

//===----------------------------------------------------------------------===//
//     Extra additions for arrays
//===----------------------------------------------------------------------===//

/// Find the length of an array.
template <typename T, std::size_t N>
constexpr inline size_t array_lengthof(T (&)[N])
{
   return N;
}

/// Adapt std::less<T> for array_pod_sort.
template<typename T>
inline int array_pod_sort_comparator(const void *P1, const void *P2)
{
   if (std::less<T>()(*reinterpret_cast<const T*>(P1),
                      *reinterpret_cast<const T*>(P2)))
      return -1;
   if (std::less<T>()(*reinterpret_cast<const T*>(P2),
                      *reinterpret_cast<const T*>(P1)))
      return 1;
   return 0;
}

/// get_array_pod_sort_comparator - This is an internal helper function used to
/// get type deduction of T right.
template<typename T>
inline int (*get_array_pod_sort_comparator(const T &))
(const void*, const void*)
{
   return array_pod_sort_comparator<T>;
}

/// array_pod_sort - This sorts an array with the specified start and end
/// extent.  This is just like std::sort, except that it calls qsort instead of
/// using an inlined template.  qsort is slightly slower than std::sort, but
/// most sorts are not performance critical in LLVM and std::sort has to be
/// template instantiated for each type, leading to significant measured code
/// bloat.  This function should generally be used instead of std::sort where
/// possible.
///
/// This function assumes that you have simple POD-like types that can be
/// compared with std::less and can be moved with memcpy.  If this isn't true,
/// you should use std::sort.
///
/// NOTE: If qsort_r were portable, we could allow a custom comparator and
/// default to std::less.
template<typename IteratorTy>
inline void array_pod_sort(IteratorTy start, IteratorTy end)
{
   // Don't inefficiently call qsort with one element or trigger undefined
   // behavior with an empty sequence.
   auto nelts = end - start;
   if (nelts <= 1) {
      return;
   }
   qsort(&*start, nelts, sizeof(*start), get_array_pod_sort_comparator(*start));
}

template <typename IteratorTy>
inline void array_pod_sort(
      IteratorTy start, IteratorTy end,
      int (*Compare)(
         const typename std::iterator_traits<IteratorTy>::value_type *,
         const typename std::iterator_traits<IteratorTy>::value_type *))
{
   // Don't inefficiently call qsort with one element or trigger undefined
   // behavior with an empty sequence.
   auto nelts = end - start;
   if (nelts <= 1) {
      return;
   }
   qsort(&*start, nelts, sizeof(*start),
         reinterpret_cast<int (*)(const void *, const void *)>(Compare));
}

// Provide wrappers to std::sort which shuffle the elements before sorting
// to help uncover non-deterministic behavior (PR35135).
template <typename IteratorTy>
inline void sort(IteratorTy start, IteratorTy end)
{
#ifdef EXPENSIVE_CHECKS
   std::mt19937 generator(std::random_device{}());
   std::shuffle(start, end, Generator);
#endif
   std::sort(start, end);
}

template <typename Container>
inline void sort(Container &&container)
{
   polar::basic::sort(adl_begin(container), adl_end(container));
}

template <typename IteratorTy, typename Compare>
inline void sort(IteratorTy start, IteratorTy end, Compare comp)
{
#ifdef EXPENSIVE_CHECKS
   std::mt19937 generator(std::random_device{}());
   std::shuffle(start, end, generator);
#endif
   std::sort(start, end, comp);
}

template <typename Container, typename Compare>
inline void sort(Container &&container, Compare comp)
{
   polar::basic::sort(adl_begin(container), adl_end(container), comp);
}

//===----------------------------------------------------------------------===//
//     Extra additions to <algorithm>
//===----------------------------------------------------------------------===//

/// For a container of pointers, deletes the pointers and then clears the
/// container.
template<typename Container>
void delete_container_pointers(Container &container)
{
   for (auto item : container) {
      delete item;
   }
   container.clear();
}

/// In a container of pairs (usually a map) whose second element is a pointer,
/// deletes the second elements and then clears the container.
template<typename Container>
void delete_container_seconds(Container &container)
{
   for (auto &item : container) {
      delete item.second;
   }
   container.clear();
}

/// Provide wrappers to std::for_each which take ranges instead of having to
/// pass begin/end explicitly.
template <typename R, typename UnaryPredicate>
UnaryPredicate for_each(R &&range, UnaryPredicate pred)
{
   return std::for_each(adl_begin(range), adl_end(range), pred);
}

/// Provide wrappers to std::all_of which take ranges instead of having to pass
/// begin/end explicitly.
template <typename R, typename UnaryPredicate>
bool all_of(R &&range, UnaryPredicate pred)
{
   return std::all_of(adl_begin(range), adl_end(range), pred);
}

/// Provide wrappers to std::any_of which take ranges instead of having to pass
/// begin/end explicitly.
template <typename R, typename UnaryPredicate>
bool any_of(R &&range, UnaryPredicate pred)
{
   return std::any_of(adl_begin(range), adl_end(range), pred);
}

/// Provide wrappers to std::none_of which take ranges instead of having to pass
/// begin/end explicitly.
template <typename R, typename UnaryPredicate>
bool none_of(R &&range, UnaryPredicate pred)
{
   return std::none_of(adl_begin(range), adl_end(range), pred);
}

/// Provide wrappers to std::find which take ranges instead of having to pass
/// begin/end explicitly.
template <typename R, typename T>
auto find(R &&range, const T &value) -> decltype(adl_begin(range))
{
   return std::find(adl_begin(range), adl_end(range), value);
}

/// Provide wrappers to std::find_if which take ranges instead of having to pass
/// begin/end explicitly.
template <typename R, typename UnaryPredicate>
auto find_if(R &&range, UnaryPredicate pred) -> decltype(adl_begin(range))
{
   return std::find_if(adl_begin(range), adl_end(range), pred);
}

template <typename R, typename UnaryPredicate>
auto find_if_not(R &&range, UnaryPredicate pred) -> decltype(adl_begin(range))
{
   return std::find_if_not(adl_begin(range), adl_end(range), pred);
}

/// Provide wrappers to std::remove_if which take ranges instead of having to
/// pass begin/end explicitly.
template <typename R, typename UnaryPredicate>
auto remove_if(R &&range, UnaryPredicate pred) -> decltype(adl_begin(range))
{
   return std::remove_if(adl_begin(range), adl_end(range), pred);
}

/// Provide wrappers to std::copy_if which take ranges instead of having to
/// pass begin/end explicitly.
template <typename R, typename OutputIt, typename UnaryPredicate>
OutputIt copy_if(R &&range, OutputIt out, UnaryPredicate pred)
{
   return std::copy_if(adl_begin(range), adl_end(range), out, pred);
}

template <typename R, typename OutputIt>
OutputIt copy(R &&range, OutputIt out)
{
   return std::copy(adl_begin(range), adl_end(range), out);
}

/// Wrapper function around std::find to detect if an element exists
/// in a container.
template <typename R, typename E>
bool is_contained(R &&range, const E &element)
{
   return std::find(adl_begin(range), adl_end(range), element) != adl_end(range);
}

/// Wrapper function around std::count to count the number of times an element
/// \p Element occurs in the given range \p range.
template <typename R, typename E>
auto count(R &&range, const E &element) ->
typename std::iterator_traits<decltype(adl_begin(range))>::difference_type
{
   return std::count(adl_begin(range), adl_end(range), element);
}

/// Wrapper function around std::count_if to count the number of times an
/// element satisfying a given predicate occurs in a range.
template <typename R, typename UnaryPredicate>
auto count_if(R &&range, UnaryPredicate pred) ->
typename std::iterator_traits<decltype(adl_begin(range))>::difference_type
{
   return std::count_if(adl_begin(range), adl_end(range), pred);
}

/// Wrapper function around std::transform to apply a function to a range and
/// store the result elsewhere.
template <typename R, typename OutputIt, typename UnaryPredicate>
OutputIt transform(R &&range, OutputIt dfirst, UnaryPredicate pred)
{
   return std::transform(adl_begin(range), adl_end(range), dfirst, pred);
}

/// Provide wrappers to std::partition which take ranges instead of having to
/// pass begin/end explicitly.
template <typename R, typename UnaryPredicate>
auto partition(R &&range, UnaryPredicate pred) -> decltype(adl_begin(range))
{
   return std::partition(adl_begin(range), adl_end(range), pred);
}

/// Provide wrappers to std::lower_bound which take ranges instead of having to
/// pass begin/end explicitly.
template <typename R, typename ForwardIt>
auto lower_bound(R &&range, ForwardIt iter) -> decltype(adl_begin(range))
{
   return std::lower_bound(adl_begin(range), adl_end(range), iter);
}

template <typename R, typename ForwardIt, typename Compare>
auto lower_bound(R &&range, ForwardIt iter, Compare compare)
-> decltype(adl_begin(range))
{
   return std::lower_bound(adl_begin(range), adl_end(range), iter, compare);
}

/// Provide wrappers to std::upper_bound which take ranges instead of having to
/// pass begin/end explicitly.
template <typename R, typename ForwardIt>
auto upper_bound(R &&range, ForwardIt iter) -> decltype(adl_begin(range))
{
   return std::upper_bound(adl_begin(range), adl_end(range), iter);
}

template <typename R, typename ForwardIt, typename Compare>
auto upper_bound(R &&range, ForwardIt iter, Compare compare)
-> decltype(adl_begin(range))
{
   return std::upper_bound(adl_begin(range), adl_end(range), iter, compare);
}

/// Wrapper function around std::equal to detect if all elements
/// in a container are same.
template <typename R>
bool is_splat(R &&range)
{
   size_t range_size = size(range);
   return range_size != 0 && (range_size == 1 ||
                              std::equal(adl_begin(range) + 1, adl_end(range), adl_begin(range)));
}

///  Given a range of type R, iterate the entire range and return a
/// SmallVector with elements of the vector.  This is useful, for example,
/// when you want to iterate a range and then sort the results.
template <unsigned Size, typename R>
SmallVector<typename std::remove_const<internal::ValueOfRange<R>>::type, Size>
to_vector(R &&range)
{
   return {adl_begin(range), adl_end(range)};
}

/// Provide a container algorithm similar to C++ Library Fundamentals v2's
/// `erase_if` which is equivalent to:
///
///   C.erase(remove_if(C, pred), C.end());
///
/// This version works for any container with an erase method call accepting
/// two iterators.
template <typename Container, typename UnaryPredicate>
void erase_if(Container &container, UnaryPredicate pred)
{
   container.erase(remove_if(container, pred), container.end());
}

//===----------------------------------------------------------------------===//
//     Extra additions to <memory>
//===----------------------------------------------------------------------===//

// Implement make_unique according to N3656.

///  Constructs a `new T()` with the given args and returns a
///        `unique_ptr<T>` which owns the object.
///
/// Example:
///
///     auto p = make_unique<int>();
///     auto p = make_unique<std::tuple<int, int>>(0, 1);
template <typename T, typename... Args>
typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type
make_unique(Args &&... args)
{
   return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

///  Constructs a `new T[n]` with the given args and returns a
///        `unique_ptr<T[]>` which owns the object.
///
/// \param n size of the new array.
///
/// Example:
///
///     auto p = make_unique<int[]>(2); // value-initializes the array with 0's.
template <typename T>
typename std::enable_if<std::is_array<T>::value && std::extent<T>::value == 0,
std::unique_ptr<T>>::type
make_unique(size_t n)
{
   return std::unique_ptr<T>(new typename std::remove_extent<T>::type[n]());
}

/// This function isn't used and is only here to provide better compile errors.
template <typename T, typename... Args>
typename std::enable_if<std::extent<T>::value != 0>::type
      make_unique(Args &&...) = delete;

struct FreeDeleter
{
   void operator()(void* v)
   {
      ::free(v);
   }
};

template<typename First, typename Second>
struct PairHash
{
   size_t operator()(const std::pair<First, Second> &pred) const
   {
      return std::hash<First>()(pred.first) * 31 + std::hash<Second>()(pred.second);
   }
};

/// A functor like C++14's std::less<void> in its absence.
struct Less
{
   template <typename A, typename B> bool operator()(A &&a, B &&b) const
   {
      return std::forward<A>(a) < std::forward<B>(b);
   }
};

/// A functor like C++14's std::equal<void> in its absence.
struct Equal
{
   template <typename A, typename B> bool operator()(A &&a, B &&b) const
   {
      return std::forward<A>(a) == std::forward<B>(b);
   }
};

/// Binary functor that adapts to any other binary functor after dereferencing
/// operands.
template <typename T> struct Deref
{
   T m_func;

   // Could be further improved to cope with non-derivable functors and
   // non-binary functors (should be a variadic template member function
   // operator()).
   template <typename A, typename B>
   auto operator()(A &lhs, B &rhs) const -> decltype(m_func(*lhs, *rhs))
   {
      assert(lhs);
      assert(rhs);
      return m_func(*lhs, *rhs);
   }
};

namespace internal {

template <typename R> class EnumeratorIter;

template <typename R>
struct ResultPair
{
   friend class EnumeratorIter<R>;

   ResultPair() = default;
   ResultPair(std::size_t index, IterOfRange<R> iter)
      : m_index(index), m_iter(iter)
   {}

   ResultPair<R> &operator=(const ResultPair<R> &other)
   {
      m_index = other.m_index;
      m_iter = other.iter;
      return *this;
   }

   std::size_t getIndex() const
   {
      return m_index;
   }

   const ValueOfRange<R> &getValue() const
   {
      return *m_iter;
   }

   ValueOfRange<R> &getValue()
   {
      return *m_iter;
   }

private:
   std::size_t m_index = std::numeric_limits<std::size_t>::max();
   IterOfRange<R> m_iter;
};

template <typename R>
class EnumeratorIter
      : public IteratorFacadeBase<
      EnumeratorIter<R>, std::forward_iterator_tag, ResultPair<R>,
      typename std::iterator_traits<IterOfRange<R>>::difference_type,
      typename std::iterator_traits<IterOfRange<R>>::pointer,
      typename std::iterator_traits<IterOfRange<R>>::reference>
{
   using result_type = ResultPair<R>;

public:
   explicit EnumeratorIter(IterOfRange<R> endIter)
      : m_result(std::numeric_limits<size_t>::max(), endIter)
   {}

   EnumeratorIter(std::size_t index, IterOfRange<R> iter)
      : m_result(index, iter)
   {}

   result_type &operator*()
   {
      return m_result;
   }

   const result_type &operator*() const
   {
      return m_result;
   }

   EnumeratorIter<R> &operator++()
   {
      assert(m_result.m_index != std::numeric_limits<size_t>::max());
      ++m_result.m_iter;
      ++m_result.m_index;
      return *this;
   }

   bool operator==(const EnumeratorIter<R> &rhs) const
   {
      // Don't compare indices here, only iterators.  It's possible for an end
      // iterator to have different indices depending on whether it was created
      // by calling std::end() versus incrementing a valid iterator.
      return m_result.m_iter == rhs.m_result.m_iter;
   }

   EnumeratorIter<R> &operator=(const EnumeratorIter<R> &Other)
   {
      m_result = Other.m_result;
      return *this;
   }

private:
   result_type m_result;
};

template <typename R>
class Enumerator {
public:
   explicit Enumerator(R &&m_range)
      : m_range(std::forward<R>(m_range))
   {}

   EnumeratorIter<R> begin()
   {
      return EnumeratorIter<R>(0, std::begin(m_range));
   }

   EnumeratorIter<R> end()
   {
      return EnumeratorIter<R>(std::end(m_range));
   }

private:
   R m_range;
};

} // end namespace internal

/// Given an input range, returns a new range whose values are are pair (A,B)
/// such that A is the 0-based index of the item in the sequence, and B is
/// the value from the original sequence.  Example:
///
/// std::vector<char> Items = {'A', 'B', 'C', 'D'};
/// for (auto X : enumerate(Items)) {
///   printf("Item %d - %c\n", X.index(), X.value());
/// }
///
/// Output:
///   Item 0 - A
///   Item 1 - B
///   Item 2 - C
///   Item 3 - D
///
template <typename R>
internal::Enumerator<R> enumerate(R &&range)
{
   return internal::Enumerator<R>(std::forward<R>(range));
}

namespace internal {

template <typename Func, typename Tuple, std::size_t... Index>
auto apply_tuple_impl(Func &&func, Tuple &&tuple, index_sequence<Index...>)
-> decltype(std::forward<Func>(func)(std::get<Index>(std::forward<Tuple>(tuple))...))
{
   return std::forward<Func>(func)(std::get<Index>(std::forward<Tuple>(tuple))...);
}

} // end namespace internal

/// Given an input tuple (a1, a2, ..., an), pass the arguments of the
/// tuple variadically to f as if by calling f(a1, a2, ..., an) and
/// return the result.
template <typename Func, typename Tuple>
auto apply_tuple(Func &&func, Tuple &&tuple) -> decltype(internal::apply_tuple_impl(
                                                            std::forward<Func>(func), std::forward<Tuple>(tuple),
                                                            build_index_impl<
                                                            std::tuple_size<typename std::decay<Tuple>::type>::value>{}))
{
   using Indices = build_index_impl<
   std::tuple_size<typename std::decay<Tuple>::type>::value>;

   return internal::apply_tuple_impl(std::forward<Func>(func), std::forward<Tuple>(tuple),
                                     Indices{});
}

/// Return true if the sequence [begin, end) has exactly N items. Runs in O(N)
/// time. Not meant for use with random-access iterators.
template <typename IterTy>
bool has_n_items(
      IterTy &&begin, IterTy &&end, unsigned N,
      typename std::enable_if<
      !std::is_same<
      typename std::iterator_traits<typename std::remove_reference<
      decltype(begin)>::type>::iterator_category,
      std::random_access_iterator_tag>::value,
      void>::type * = nullptr)
{
   for (; N; --N, ++begin) {
      if (begin == end) {
         return false; // Too few.
      }
   }
   return begin == end;
}

/// Return true if the sequence [begin, end) has N or more items. Runs in O(N)
/// time. Not meant for use with random-access iterators.
template <typename IterTy>
bool has_n_items_or_more(
      IterTy &&begin, IterTy &&end, unsigned N,
      typename std::enable_if<
      !std::is_same<
      typename std::iterator_traits<typename std::remove_reference<
      decltype(begin)>::type>::iterator_category,
      std::random_access_iterator_tag>::value,
      void>::type * = nullptr)
{
   for (; N; --N, ++begin) {
      if (begin == end) {
         return false; // Too few.
      }
   }
   return true;
}

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_STL_EXTRAS_H
