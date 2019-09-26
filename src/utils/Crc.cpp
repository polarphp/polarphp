//===--- crc.cpp - Cyclic Redundancy Check implementation -----------------===//
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
// Created by polarboy on 2019/09/26.

#include "polarphp/utils/Crc.h"
#include <mutex>

namespace polar::utils {

//#if POLAR_ENABLE_ZLIB == 0 || !HAVE_ZLIB_H
#ifndef HAVE_ZLIB_H
using CRC32Table = std::array<uint32_t, 256>;

static void init_crc32_table(CRC32Table *table) {
   auto shuffle = [](uint32_t value) {
      return (value & 1) ? (value >> 1) ^ 0xEDB88320U : value >> 1;
   };

   for (size_t index = 0; index < table->size(); ++index) {
      uint32_t value = shuffle(index);
      value = shuffle(value);
      value = shuffle(value);
      value = shuffle(value);
      value = shuffle(value);
      value = shuffle(value);
      value = shuffle(value);
      (*table)[index] = shuffle(value);
   }
}

uint32_t crc32(uint32_t crc, StringRef str)
{
   static std::once_flag InitFlag;
   static CRC32Table table;
   std::call_once(InitFlag, init_crc32_table, &table);

   const uint8_t *ptr = reinterpret_cast<const uint8_t *>(str.data());
   size_t len = str.size();
   crc ^= 0xFFFFFFFFU;
   for (; len >= 8; len -= 8) {
      crc = table[(crc ^ *ptr++) & 0xFF] ^ (crc >> 8);
      crc = table[(crc ^ *ptr++) & 0xFF] ^ (crc >> 8);
      crc = table[(crc ^ *ptr++) & 0xFF] ^ (crc >> 8);
      crc = table[(crc ^ *ptr++) & 0xFF] ^ (crc >> 8);
      crc = table[(crc ^ *ptr++) & 0xFF] ^ (crc >> 8);
      crc = table[(crc ^ *ptr++) & 0xFF] ^ (crc >> 8);
      crc = table[(crc ^ *ptr++) & 0xFF] ^ (crc >> 8);
      crc = table[(crc ^ *ptr++) & 0xFF] ^ (crc >> 8);
   }
   while (len--) {
      crc = table[(crc ^ *ptr++) & 0xFF] ^ (crc >> 8);
   }
   return crc ^ 0xFFFFFFFFU;
}
#else
#include <zlib.h>
uint32_t crc32(uint32_t crc, StringRef str)
{
   return ::crc32(crc, (const Bytef *)str.data(), str.size());
}
#endif

} // polar::utils
