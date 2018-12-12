// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/11.

#ifndef POLARPHP_UTILS_SWAP_BYTE_ORDER_H
#define POLARPHP_UTILS_SWAP_BYTE_ORDER_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/global/ProcessorDetection.h"
#include "polarphp/global/DataTypes.h"
#include <cstddef>
#if defined(_MSC_VER) && !defined(_DEBUG)
#include <stdlib.h>
#endif

namespace polar {
namespace utils {

/// swap_byte_order16 - This function returns a byte-swapped representation of
/// the 16-bit argument.
inline uint16_t swap_byte_order16(uint16_t value)
{
#if defined(_MSC_VER) && !defined(_DEBUG)
   // The DLL version of the runtime lacks these functions (bug!?), but in a
   // release build they're replaced with BSWAP instructions anyway.
   return _byteswap_ushort(value);
#else
   uint16_t hi = value << 8;
   uint16_t lo = value >> 8;
   return hi | lo;
#endif
}

/// swap_byte_order32 - This function returns a byte-swapped representation of
/// the 32-bit argument.
inline uint32_t swap_byte_order32(uint32_t value)
{
#if defined(__llvm__) || (POLAR_GNUC_PREREQ(4, 3, 0) && !defined(__ICC))
   return __builtin_bswap32(value);
#elif defined(_MSC_VER) && !defined(_DEBUG)
   return _byteswap_ulong(value);
#else
   uint32_t byte0 = value & 0x000000FF;
   uint32_t byte1 = value & 0x0000FF00;
   uint32_t byte2 = value & 0x00FF0000;
   uint32_t byte3 = value & 0xFF000000;
   return (byte0 << 24) | (byte1 << 8) | (byte2 >> 8) | (byte3 >> 24);
#endif
}

/// swap_byte_order64 - This function returns a byte-swapped representation of
/// the 64-bit argument.
inline uint64_t swap_byte_order64(uint64_t value)
{
#if defined(__llvm__) || (POLAR_GNUC_PREREQ(4, 3, 0) && !defined(__ICC))
   return __builtin_bswap64(value);
#elif defined(_MSC_VER) && !defined(_DEBUG)
   return _byteswap_uint64(value);
#else
   uint64_t hi = swap_byte_order32(uint32_t(value));
   uint32_t lo = swap_byte_order32(uint32_t(value >> 32));
   return (hi << 32) | lo;
#endif
}

inline unsigned char get_swapped_bytes(unsigned char value)
{
   return value;
}

inline signed char get_swapped_bytes(signed char value)
{
   return value;
}

inline char get_swapped_bytes(char value)
{
   return value;
}

inline unsigned short get_swapped_bytes(unsigned short value)
{
   return swap_byte_order16(value);
}

inline signed short get_swapped_bytes(signed short value)
{
   return swap_byte_order16(value);
}

inline unsigned int get_swapped_bytes(unsigned int value)
{
   return swap_byte_order32(value);
}

inline signed int get_swapped_bytes(signed int value)
{
   return swap_byte_order32(value);
}

#if __LONG_MAX__ == __INT_MAX__
inline unsigned long get_swapped_bytes(unsigned long value)
{
   return swap_byte_order32(value);
}

inline signed long get_swapped_bytes(signed long value)
{
   return swap_byte_order32(value);
}

#elif __LONG_MAX__ == __LONG_LONG_MAX__
inline unsigned long get_swapped_bytes(unsigned long value)
{
   return swap_byte_order64(value);
}

inline signed long get_swapped_bytes(signed long value)
{
   return swap_byte_order64(value);
}
#else
#error "Unknown long size!"
#endif

inline unsigned long long get_swapped_bytes(unsigned long long value)
{
   return swap_byte_order64(value);
}

inline signed long long get_swapped_bytes(signed long long value)
{
   return swap_byte_order64(value);
}

inline float get_swapped_bytes(float value)
{
   union {
      uint32_t i;
      float f;
   } in, out;
   in.f = value;
   out.i = swap_byte_order32(in.i);
   return out.f;
}

inline double get_swapped_bytes(double value)
{
   union {
      uint64_t i;
      double d;
   } in, out;
   in.d = value;
   out.i = swap_byte_order64(in.i);
   return out.d;
}

template<typename T>
inline void swap_byte_order(T &value)
{
   value = get_swapped_bytes(value);
}

} // utils
} // polar

#endif // POLARPHP_UTILS_SWAP_BYTE_ORDER_H
