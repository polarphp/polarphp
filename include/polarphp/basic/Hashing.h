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

#ifndef POLARPHP_BASIC_HASING_H
#define POLARPHP_BASIC_HASING_H

#include "llvm/ADT/Hashing.h"
#include <utility>

namespace polar {

using llvm::hash_combine;
using llvm::hash_code;

namespace {

template <typename T, std::size_t... I>
hash_code hash_value_impl(const T &tuple, std::index_sequence<I...>)
{
  return hash_combine(std::get<I>(tuple)...);
}

}

template <typename... ArgTypes, typename Indices = std::make_index_sequence<sizeof...(ArgTypes)>>
hash_code hash_value(const std::tuple<ArgTypes...> &tupleValue)
{
   return hash_value_impl(tupleValue, Indices{});
}

} // polar

#endif // POLARPHP_BASIC_HASING_H
