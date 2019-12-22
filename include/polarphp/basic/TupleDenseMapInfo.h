// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/06.

#ifndef POLARPHP_BASIC_TUPLE_DENSE_MAP_INFO_H
#define POLARPHP_BASIC_TUPLE_DENSE_MAP_INFO_H

#include "llvm/ADT/DenseMapInfo.h"
#include "polarphp/basic/StlExtras.h"
#include <type_traits>

namespace llvm {

using polar::is_tuple;

namespace {
template <typename T, size_t...I>
T get_tuple_empty_key(std::index_sequence<I...>)
{
   return std::make_tuple(DenseMapInfo<std::tuple_element_t<I, T>>::getEmptyKey()...);
}

template <typename T, size_t...I>
T get_tuple_tombstone_key(std::index_sequence<I...>)
{
   return std::make_tuple(DenseMapInfo<std::tuple_element_t<I, T>>::getTombstoneKey()...);
}

template <typename T, size_t...I>
unsigned get_tuple_hash_value(const T &tuple, std::index_sequence<I...>)
{
   return hash_combine(std::get<I>(tuple)...);
}

template <typename T, size_t I>
bool do_tuple_element_compare(const T &lhs, const T &rhs)
{
   return std::get<I>(lhs) == std::get<I>(rhs);
}

template <typename T, size_t I>
bool tuple_equal(const T &lhs, const T &rhs)
{
   if constexpr(I > 0) {
      return tuple_equal<T, I - 1>(lhs, rhs) && (std::get<I>(lhs) == std::get<I>(rhs));
   } else {
      return std::get<0>(lhs) == std::get<0>(rhs);
   }
}

} // internal

template <typename... Args>
struct DenseMapInfo<std::tuple<Args...>>
{
   using TupleType = std::tuple<Args...>;
   static_assert (is_tuple<TupleType>::value);
   static TupleType getEmptyKey()
   {
      return get_tuple_empty_key<TupleType>(std::make_index_sequence<sizeof... (Args)>{});
   }

   static TupleType getTombstoneKey()
   {
      return get_tuple_tombstone_key<TupleType>(std::make_index_sequence<sizeof... (Args)>{});
   }

   static unsigned getHashValue(const TupleType &value)
   {
      return get_tuple_hash_value(value, std::make_index_sequence<sizeof... (Args)>{});
   }

   static bool isEqual(const TupleType &lhs, const TupleType &rhs)
   {
      return tuple_equal<TupleType, sizeof... (Args) - 1>(lhs, rhs);
   }
};

} // namespace llvm

#endif
