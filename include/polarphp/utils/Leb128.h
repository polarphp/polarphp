// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.
//===----------------------------------------------------------------------===//
//
// This file declares some utility functions for encoding SLEB128 and
// ULEB128 values.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_LEB128_H
#define POLARPHP_UTILS_LEB128_H

#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace utils {

/// Utility function to encode a SLEB128 value to an output stream.
inline unsigned encode_sleb128(int64_t value, RawOutStream &outstream,
                               unsigned padTo = 0)
{
   bool more;
   unsigned count = 0;
   do {
      uint8_t byte = value & 0x7f;
      // NOTE: this assumes that this signed shift is an arithmetic right shift.
      value >>= 7;
      more = !((((value == 0 ) && ((byte & 0x40) == 0)) ||
                ((value == -1) && ((byte & 0x40) != 0))));
      ++count;
      if (more || count < padTo) {
         byte |= 0x80; // Mark this byte to show that more bytes will follow.
      }
      outstream << char(byte);
   } while (more);

   // Pad with 0x80 and emit a terminating byte at the end.
   if (count < padTo) {
      uint8_t padValue = value < 0 ? 0x7f : 0x00;
      for (; count < padTo - 1; ++count) {
         outstream << char(padValue | 0x80);
      }
      outstream << char(padValue);
      ++count;
   }
   return count;
}

/// Utility function to encode a SLEB128 value to a buffer. Returns
/// the length in bytes of the encoded value.
inline unsigned encode_sleb128(int64_t value, uint8_t *p, unsigned padTo = 0)
{
   uint8_t *origPtr = p;
   unsigned count = 0;
   bool more;
   do {
      uint8_t byte = value & 0x7f;
      // NOTE: this assumes that this signed shift is an arithmetic right shift.
      value >>= 7;
      more = !((((value == 0 ) && ((byte & 0x40) == 0)) ||
                ((value == -1) && ((byte & 0x40) != 0))));
      ++count;
      if (more || count < padTo) {
         byte |= 0x80; // Mark this byte to show that more bytes will follow.
      }
      *p++ = byte;
   } while (more);

   // Pad with 0x80 and emit a terminating byte at the end.
   if (count < padTo) {
      uint8_t padValue = value < 0 ? 0x7f : 0x00;
      for (; count < padTo - 1; ++count) {
         *p++ = (padValue | 0x80);
      }
      *p++ = padValue;
   }
   return (unsigned)(p - origPtr);
}

/// Utility function to encode a ULEB128 value to an output stream.
inline unsigned encode_uleb128(uint64_t value, RawOutStream &outstream,
                               unsigned padTo = 0)
{
   unsigned count = 0;
   do {
      uint8_t byte = value & 0x7f;
      value >>= 7;
      ++count;
      if (value != 0 || count < padTo) {
         byte |= 0x80; // Mark this byte to show that more bytes will follow.
      }
      outstream << char(byte);
   } while (value != 0);

   // Pad with 0x80 and emit a null byte at the end.
   if (count < padTo) {
      for (; count < padTo - 1; ++count) {
         outstream << '\x80';
      }
      outstream << '\x00';
      ++count;
   }
   return count;
}

/// Utility function to encode a ULEB128 value to a buffer. Returns
/// the length in bytes of the encoded value.
inline unsigned encode_uleb128(uint64_t value, uint8_t *p,
                               unsigned padTo = 0)
{
   uint8_t *origPtr = p;
   unsigned count = 0;
   do {
      uint8_t byte = value & 0x7f;
      value >>= 7;
      ++count;
      if (value != 0 || count < padTo)
         byte |= 0x80; // Mark this byte to show that more bytes will follow.
      *p++ = byte;
   } while (value != 0);

   // Pad with 0x80 and emit a null byte at the end.
   if (count < padTo) {
      for (; count < padTo - 1; ++count)
         *p++ = '\x80';
      *p++ = '\x00';
   }
   return (unsigned)(p - origPtr);
}

/// Utility function to decode a ULEB128 value.
inline uint64_t decode_uleb128(const uint8_t *p, unsigned *n = nullptr,
                               const uint8_t *end = nullptr,
                               const char **error = nullptr)
{
   const uint8_t *origPtr = p;
   uint64_t value = 0;
   unsigned shift = 0;
   if (error) {
      *error = nullptr;
   }
   do {
      if (end && p == end) {
         if (error) {
            *error = "malformed uleb128, extends past end";
         }
         if (n) {
            *n = (unsigned)(p - origPtr);
         }
         return 0;
      }
      uint64_t slice = *p & 0x7f;
      if (shift >= 64 || slice << shift >> shift != slice) {
         if (error) {
            *error = "uleb128 too big for uint64";
         }
         if (n) {
            *n = (unsigned)(p - origPtr);
         }
         return 0;
      }
      value += uint64_t(*p & 0x7f) << shift;
      shift += 7;
   } while (*p++ >= 128);
   if (n) {
      *n = (unsigned)(p - origPtr);
   }
   return value;
}

/// Utility function to decode a SLEB128 value.
inline int64_t decode_sleb128(const uint8_t *p, unsigned *n = nullptr,
                              const uint8_t *end = nullptr,
                              const char **error = nullptr)
{
   const uint8_t *origPtr = p;
   int64_t value = 0;
   unsigned shift = 0;
   uint8_t byte;
   do {
      if (end && p == end) {
         if (error) {
            *error = "malformed sleb128, extends past end";
         }
         if (n) {
            *n = (unsigned)(p - origPtr);
         }
         return 0;
      }
      byte = *p++;
      value |= (int64_t(byte & 0x7f) << shift);
      shift += 7;
   } while (byte >= 128);
   // Sign extend negative numbers.
   if (byte & 0x40) {
      value |= (-1ULL) << shift;
   }
   if (n) {
      *n = (unsigned)(p - origPtr);
   }
   return value;
}

/// Utility function to get the size of the ULEB128-encoded value.
extern unsigned get_uleb128_size(uint64_t value);

/// Utility function to get the size of the SLEB128-encoded value.
extern unsigned get_sleb128_size(int64_t value);

} // utils
} // polar

#endif // POLARPHP_UTILS_LEB128_H
