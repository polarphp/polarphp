// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/09.

/*
*  xxHash - Fast Hash algorithm
*  Copyright (C) 2012-2016, Yann Collet
*
*  BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are
*  met:
*
*  * Redistributions of source code must retain the above copyright
*  notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above
*  copyright notice, this list of conditions and the following disclaimer
*  in the documentation and/or other materials provided with the
*  distribution.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  limitED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  limitED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  data, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*  You can contact the author at :
*  - xxHash homepage: http://www.xxhash.com
*  - xxHash source repository : https://github.com/Cyan4973/xxHash
*/

/* based on revision d2df04efcbef7d7f6886d345861e5dfda4edacc1 Removed
 * everything but a simple interface for computing XXh64. */

#include "polarphp/utils/FastHash.h"
#include "polarphp/utils/Endian.h"

#include <stdlib.h>
#include <string.h>

namespace polar {
namespace utils {

static const uint64_t PRIME64_1 = 11400714785074694791ULL;
static const uint64_t PRIME64_2 = 14029467366897019727ULL;
static const uint64_t PRIME64_3 = 1609587929392839161ULL;
static const uint64_t PRIME64_4 = 9650029242287828579ULL;
static const uint64_t PRIME64_5 = 2870177450012600261ULL;

namespace {
uint64_t rotl64(uint64_t X, size_t R)
{
   return (X << R) | (X >> (64 - R));
}

uint64_t round(uint64_t acc, uint64_t input)
{
   acc += input * PRIME64_2;
   acc = rotl64(acc, 31);
   acc *= PRIME64_1;
   return acc;
}

uint64_t mergeRound(uint64_t acc, uint64_t value)
{
   value = round(0, value);
   acc ^= value;
   acc = acc * PRIME64_1 + PRIME64_4;
   return acc;
}

} // anonymous namespace

uint64_t fast_hash64(StringRef data)
{
   size_t length = data.getSize();
   uint64_t seed = 0;
   const char *ptr = data.getData();
   const char *const bend = ptr + length;
   uint64_t h64;

   if (length >= 32) {
      const char *const limit = bend - 32;
      uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
      uint64_t v2 = seed + PRIME64_2;
      uint64_t v3 = seed + 0;
      uint64_t v4 = seed - PRIME64_1;

      do {
         v1 = round(v1, endian::read64le(ptr));
         ptr += 8;
         v2 = round(v2, endian::read64le(ptr));
         ptr += 8;
         v3 = round(v3, endian::read64le(ptr));
         ptr += 8;
         v4 = round(v4, endian::read64le(ptr));
         ptr += 8;
      } while (ptr <= limit);

      h64 = rotl64(v1, 1) + rotl64(v2, 7) + rotl64(v3, 12) + rotl64(v4, 18);
      h64 = mergeRound(h64, v1);
      h64 = mergeRound(h64, v2);
      h64 = mergeRound(h64, v3);
      h64 = mergeRound(h64, v4);

   } else {
      h64 = seed + PRIME64_5;
   }

   h64 += (uint64_t)length;

   while (ptr + 8 <= bend) {
      uint64_t const K1 = round(0, endian::read64le(ptr));
      h64 ^= K1;
      h64 = rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
      ptr += 8;
   }

   if (ptr + 4 <= bend) {
      h64 ^= (uint64_t)(endian::read32le(ptr)) * PRIME64_1;
      h64 = rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
      ptr += 4;
   }

   while (ptr < bend) {
      h64 ^= (*ptr) * PRIME64_5;
      h64 = rotl64(h64, 11) * PRIME64_1;
      ptr++;
   }

   h64 ^= h64 >> 33;
   h64 *= PRIME64_2;
   h64 ^= h64 >> 29;
   h64 *= PRIME64_3;
   h64 ^= h64 >> 32;

   return h64;
}

uint64_t fast_hash64(ArrayRef<uint8_t> data)
{
   return fast_hash64({(const char *)data.getData(), data.getSize()});
}

} // utils
} // polar
