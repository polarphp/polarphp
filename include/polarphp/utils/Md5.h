// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/08.

/* -*- C++ -*-
 * This code is derived from (original license follows):
 *
 * This is an OpenSSL-compatible implementation of the RSA Data Security, Inc.
 * Md5 Message-Digest Algorithm (RFC 1321).
 *
 * Homepage:
 * http://openwall.info/wiki/people/solar/software/public-domain-source-code/Md5
 *
 * Author:
 * Alexander Peslyak, better known as Solar Designer <solar at openwall.com>
 *
 * This software was written by Alexander Peslyak in 2001.  No copyright is
 * claimed, and the software is hereby placed in the public domain.
 * In case this attempt to disclaim copyright and place the software in the
 * public domain is deemed null and void, then the software is
 * Copyright (c) 2001 Alexander Peslyak and it is hereby released to the
 * general public under the following terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 *
 * See Md5.c for more information.
 */

#ifndef POLARPHP_UTILS_MD5_H
#define POLARPHP_UTILS_MD5_H

#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/Endian.h"
#include <array>
#include <cstdint>

namespace polar {

// forward declare class with namespace
namespace basic {
template <typename T>
class ArrayRef;
} // basic

namespace utils {

using polar::basic::SmallString;
using polar::basic::ArrayRef;
using polar::basic::StringRef;

class Md5
{
   // Any 32-bit or wider unsigned integer data type will do.
   typedef uint32_t Md5U32PlusType;

   Md5U32PlusType a = 0x67452301;
   Md5U32PlusType b = 0xefcdab89;
   Md5U32PlusType c = 0x98badcfe;
   Md5U32PlusType d = 0x10325476;
   Md5U32PlusType m_hight = 0;
   Md5U32PlusType m_low = 0;
   uint8_t m_buffer[64];
   Md5U32PlusType m_block[16];

public:
   struct Md5Result {
      std::array<uint8_t, 16> m_bytes;

      operator std::array<uint8_t, 16>() const
      {
         return m_bytes;
      }

      const uint8_t &operator[](size_t index) const
      {
         return m_bytes[index];
      }
      uint8_t &operator[](size_t index)
      {
         return m_bytes[index];
      }

      SmallString<32> getDigest() const;

      uint64_t getLow() const
      {
         // Our Md5 implementation returns the result in little endian, so the low
         // word is first.
         return endian::read<uint64_t, Endianness::Little, UNALIGNED>(m_bytes.data());
      }

      uint64_t getHigh() const
      {
         return endian::read<uint64_t, Endianness::Little, UNALIGNED>(m_bytes.data() + 8);
      }
      std::pair<uint64_t, uint64_t> getWords() const
      {
         return std::make_pair(getHigh(), getLow());
      }
   };

   Md5();

   /// Updates the hash for the byte stream provided.
   void update(ArrayRef<uint8_t> data);

   /// Updates the hash for the StringRef provided.
   void update(StringRef Str);

   /// Finishes off the hash and puts the result in result.
   void final(Md5Result &result);

   /// Translates the m_bytes in \p Res to a hex string that is
   /// deposited into \p Str. The result will be of length 32.
   static void stringifyResult(Md5Result &result, SmallString<32> &str);

   /// Computes the hash for a given m_bytes.
   static std::array<uint8_t, 16> hash(ArrayRef<uint8_t> data);

private:
   const uint8_t *body(ArrayRef<uint8_t> data);
};

inline bool operator==(const Md5::Md5Result &lhs, const Md5::Md5Result &rhs)
{
   return lhs.m_bytes == rhs.m_bytes;
}

/// Helper to compute and return lower 64 bits of the given string's Md5 hash.
inline uint64_t md5_hash(StringRef str)
{
   Md5 hash;
   hash.update(str);
   Md5::Md5Result result;
   hash.final(result);
   // Return the least significant word.
   return result.getLow();
}

} // utils
} // polar

#endif // POLARPHP_UTILS_MD5_H
