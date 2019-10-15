//===- StringSet.h - The LLVM Compiler Driver -------------------*- C++ -*-===//
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
// Created by polarboy on 2018/06/28.

#ifndef POLARPHP_BASIC_ADT_STRING_SET_H
#define POLARPHP_BASIC_ADT_STRING_SET_H

#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/Allocator.h"
#include <cassert>
#include <initializer_list>
#include <utility>

namespace polar::basic {

/// StringSet - A wrapper for StringMap that provides set-like functionality.
template <class AllocatorTy = MallocAllocator>
class StringSet : public StringMap<char, AllocatorTy>
{
   using base = StringMap<char, AllocatorTy>;

public:
   StringSet() = default;
   StringSet(std::initializer_list<StringRef> strs)
   {
      for (StringRef str : strs) {
         insert(str);
      }
   }

   explicit StringSet(AllocatorTy allocator)
      : base(allocator)
   {}

   std::pair<typename base::iterator, bool> insert(StringRef key)
   {
      assert(!key.empty());
      return base::insert(std::make_pair(key, '\0'));
   }

   template <typename InputIter>
   void insert(const InputIter &begin, const InputIter &end)
   {
      for (auto iter = begin; iter != end; ++iter) {
         base::insert(std::make_pair(*iter, '\0'));
      }
   }

   template <typename ValueTy>
   std::pair<typename base::iterator, bool>
   insert(const StringMapEntry<ValueTy> &entry)
   {
      return insert(entry.getKey());
   }
};

} // polar::basic

#endif // POLARPHP_BASIC_ADT_STRING_SET_H
