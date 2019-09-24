//===- LEB128.cpp - LEB128 utility functions implementation -----*- C++ -*-===//
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
// Created by polarboy on 2018/07/04.

#include "polarphp/utils/Leb128.h"

namespace polar::utils {

/// Utility function to get the size of the ULEB128-encoded value.
unsigned get_uleb128_size(uint64_t value)
{
   unsigned size = 0;
   do {
      value >>= 7;
      size += sizeof(int8_t);
   } while (value);
   return size;
}

/// Utility function to get the size of the SLEB128-encoded value.
unsigned get_sleb128_size(int64_t value)
{
   unsigned size = 0;
   int sign = value >> (8 * sizeof(value) - 1);
   bool isMore;

   do {
      unsigned byte = value & 0x7f;
      value >>= 7;
      isMore = value != sign || ((byte ^ sign) & 0x40) != 0;
      size += sizeof(int8_t);
   } while (isMore);
   return size;
}

} // polar::utils
