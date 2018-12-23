// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/04.

#ifndef POLARPHP_UTILS_ENDIAN_H
#define POLARPHP_UTILS_ENDIAN_H

#include "polarphp/utils/AlignOf.h"
#include "polarphp/utils/SwapByteOrder.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace polar {
namespace utils {

enum class Endianness
{
   Big,
   Little,
   Native
};

// These are named values for common alignments.
static const int ALIGNED = 0;
static const int UNALIGNED = 1;

namespace internal {

/// ::value is either alignment, or alignof(T) if alignment is 0.
template<typename T, int alignment>
struct PickAlignment
{
   enum { value = alignment == 0 ? alignof(T) : alignment };
};

} // end namespace internal

namespace endian {

constexpr Endianness system_endianness()
{
#if POLAR_BYTE_ORDER == POLAR_BIG_ENDIAN
   return Endianness::Big;
#else
   return Endianness::Little;
#endif
}

template <typename value_type>
inline value_type byte_swap(value_type value, Endianness endian)
{
   if ((endian != Endianness::Native) && (endian != system_endianness())) {
      polar::utils::swap_byte_order(value);
   }
   return value;
}

/// Swap the bytes of value to match the given endianness.
template<typename value_type, Endianness endian>
inline value_type byte_swap(value_type value)
{
   return byte_swap(value, endian);
}

/// Read a value of a particular endianness from memory.
template <typename value_type, std::size_t alignment>
inline value_type read(const void *memory, Endianness endian)
{
   value_type ret;
   memcpy(&ret,
          POLAR_ASSUME_ALIGNED(
             memory, (internal::PickAlignment<value_type, alignment>::value)),
          sizeof(value_type));
   return byte_swap<value_type>(ret, endian);
}

template<typename value_type,
         Endianness endian,
         std::size_t alignment>
inline value_type read(const void *memory)
{
   return read<value_type, alignment>(memory, endian);
}

/// Read a value of a particular endianness from a buffer, and increment the
/// buffer past that value.
template <typename value_type, std::size_t alignment, typename CharT>
inline value_type read_next(const CharT *&memory, Endianness endian)
{
   value_type ret = read<value_type, alignment>(memory, endian);
   memory += sizeof(value_type);
   return ret;
}

template<typename value_type, Endianness endian, std::size_t alignment,
         typename CharT>
inline value_type read_next(const CharT *&memory)
{
   return read_next<value_type, alignment, CharT>(memory, endian);
}

/// Write a value to memory with a particular endianness.
template <typename value_type, std::size_t alignment>
inline void write(void *memory, value_type value, Endianness endian)
{
   value = byte_swap<value_type>(value, endian);
   memcpy(POLAR_ASSUME_ALIGNED(
             memory, (internal::PickAlignment<value_type, alignment>::value)),
          &value, sizeof(value_type));
}

template<typename value_type,
         Endianness endian,
         std::size_t alignment>
inline void write(void *memory, value_type value)
{
   write<value_type, alignment>(memory, value, endian);
}

template <typename value_type>
using make_unsigned_t = typename std::make_unsigned<value_type>::type;

/// Read a value of a particular endianness from memory, for a location
/// that starts at the given bit offset within the first byte.
template <typename value_type, Endianness endian, std::size_t alignment>
inline value_type read_at_bit_alignment(const void *memory, uint64_t startBit)
{
   assert(startBit < 8);
   if (startBit == 0) {
      return read<value_type, endian, alignment>(memory);
   } else {
      // Read two values and compose the result from them.
      value_type val[2];
      memcpy(&val[0],
            POLAR_ASSUME_ALIGNED(
               memory, (internal::PickAlignment<value_type, alignment>::value)),
            sizeof(value_type) * 2);
      val[0] = byte_swap<value_type, endian>(val[0]);
      val[1] = byte_swap<value_type, endian>(val[1]);

      // Shift bits from the lower value into place.
      make_unsigned_t<value_type> lowerVal = val[0] >> startBit;
      // Mask off upper bits after right shift in case of signed type.
      make_unsigned_t<value_type> numBitsFirstVal =
            (sizeof(value_type) * 8) - startBit;
      lowerVal &= ((make_unsigned_t<value_type>)1 << numBitsFirstVal) - 1;

      // Get the bits from the upper value.
      make_unsigned_t<value_type> upperVal =
            val[1] & (((make_unsigned_t<value_type>)1 << startBit) - 1);
      // Shift them in to place.
      upperVal <<= numBitsFirstVal;

      return lowerVal | upperVal;
   }
}

/// Write a value to memory with a particular endianness, for a location
/// that starts at the given bit offset within the first byte.
template <typename value_type, Endianness endian, std::size_t alignment>
inline void write_at_bit_alignment(void *memory, value_type value,
                                   uint64_t startBit)
{
   assert(startBit < 8);
   if (startBit == 0) {
      write<value_type, endian, alignment>(memory, value);
   } else {
      // Read two values and shift the result into them.
      value_type val[2];
      memcpy(&val[0],
            POLAR_ASSUME_ALIGNED(
               memory, (internal::PickAlignment<value_type, alignment>::value)),
            sizeof(value_type) * 2);
      val[0] = byte_swap<value_type, endian>(val[0]);
      val[1] = byte_swap<value_type, endian>(val[1]);

      // Mask off any existing bits in the upper part of the lower value that
      // we want to replace.
      val[0] &= ((make_unsigned_t<value_type>)1 << startBit) - 1;
      make_unsigned_t<value_type> numBitsFirstVal =
            (sizeof(value_type) * 8) - startBit;
      make_unsigned_t<value_type> lowerVal = value;
      if (startBit > 0) {
         // Mask off the upper bits in the new value that are not going to go into
         // the lower value. This avoids a left shift of a negative value, which
         // is undefined behavior.
         lowerVal &= (((make_unsigned_t<value_type>)1 << numBitsFirstVal) - 1);
         // Now shift the new bits into place
         lowerVal <<= startBit;
      }
      val[0] |= lowerVal;

      // Mask off any existing bits in the lower part of the upper value that
      // we want to replace.
      val[1] &= ~(((make_unsigned_t<value_type>)1 << startBit) - 1);
      // Next shift the bits that go into the upper value into position.
      make_unsigned_t<value_type> upperVal = value >> numBitsFirstVal;
      // Mask off upper bits after right shift in case of signed type.
      upperVal &= ((make_unsigned_t<value_type>)1 << startBit) - 1;
      val[1] |= upperVal;

      // Finally, rewrite values.
      val[0] = byte_swap<value_type, endian>(val[0]);
      val[1] = byte_swap<value_type, endian>(val[1]);
      memcpy(POLAR_ASSUME_ALIGNED(
                memory, (internal::PickAlignment<value_type, alignment>::value)),
             &val[0], sizeof(value_type) * 2);
   }
}

} // end namespace endian

namespace internal {

template<typename value_type,
         Endianness endian,
         std::size_t alignment>
struct PackedEndianSpecificIntegral
{
   PackedEndianSpecificIntegral() = default;

   explicit PackedEndianSpecificIntegral(value_type value)
   {
      *this = value;
   }

   operator value_type() const {
      return endian::read<value_type, endian, alignment>(
               (const void*)m_value.m_buffer);
   }

   void operator=(value_type newValue)
   {
      endian::write<value_type, endian, alignment>(
               (void*)m_value.m_buffer, newValue);
   }

   PackedEndianSpecificIntegral &operator+=(value_type newValue)
   {
      *this = *this + newValue;
      return *this;
   }

   PackedEndianSpecificIntegral &operator-=(value_type newValue)
   {
      *this = *this - newValue;
      return *this;
   }

   PackedEndianSpecificIntegral &operator|=(value_type newValue)
   {
      *this = *this | newValue;
      return *this;
   }

   PackedEndianSpecificIntegral &operator&=(value_type newValue)
   {
      *this = *this & newValue;
      return *this;
   }

private:
   AlignedCharArray<PickAlignment<value_type, alignment>::value,
   sizeof(value_type)> m_value;

public:
   struct Ref
   {
      explicit Ref(void *ptr) : m_ptr(ptr)
      {}

      operator value_type() const
      {
         return endian::read<value_type, endian, alignment>(m_ptr);
      }

      void operator=(value_type newValue)
      {
         endian::write<value_type, endian, alignment>(m_ptr, newValue);
      }

   private:
      void *m_ptr;
   };
};

} // end namespace internal

using ulittle16_t =
internal::PackedEndianSpecificIntegral<uint16_t, Endianness::Little, UNALIGNED>;
using ulittle32_t =
internal::PackedEndianSpecificIntegral<uint32_t, Endianness::Little, UNALIGNED>;
using ulittle64_t =
internal::PackedEndianSpecificIntegral<uint64_t, Endianness::Little, UNALIGNED>;

using little16_t =
internal::PackedEndianSpecificIntegral<int16_t, Endianness::Little, UNALIGNED>;
using little32_t =
internal::PackedEndianSpecificIntegral<int32_t, Endianness::Little, UNALIGNED>;
using little64_t =
internal::PackedEndianSpecificIntegral<int64_t, Endianness::Little, UNALIGNED>;

using aligned_ulittle16_t =
internal::PackedEndianSpecificIntegral<uint16_t, Endianness::Little, ALIGNED>;
using aligned_ulittle32_t =
internal::PackedEndianSpecificIntegral<uint32_t, Endianness::Little, ALIGNED>;
using aligned_ulittle64_t =
internal::PackedEndianSpecificIntegral<uint64_t, Endianness::Little, ALIGNED>;

using aligned_little16_t =
internal::PackedEndianSpecificIntegral<int16_t, Endianness::Little, ALIGNED>;
using aligned_little32_t =
internal::PackedEndianSpecificIntegral<int32_t, Endianness::Little, ALIGNED>;
using aligned_little64_t =
internal::PackedEndianSpecificIntegral<int64_t, Endianness::Little, ALIGNED>;

using ubig16_t =
internal::PackedEndianSpecificIntegral<uint16_t, Endianness::Big, UNALIGNED>;
using ubig32_t =
internal::PackedEndianSpecificIntegral<uint32_t, Endianness::Big, UNALIGNED>;
using ubig64_t =
internal::PackedEndianSpecificIntegral<uint64_t, Endianness::Big, UNALIGNED>;

using big16_t =
internal::PackedEndianSpecificIntegral<int16_t, Endianness::Big, UNALIGNED>;
using big32_t =
internal::PackedEndianSpecificIntegral<int32_t, Endianness::Big, UNALIGNED>;
using big64_t =
internal::PackedEndianSpecificIntegral<int64_t, Endianness::Big, UNALIGNED>;

using aligned_ubig16_t =
internal::PackedEndianSpecificIntegral<uint16_t, Endianness::Big, ALIGNED>;
using aligned_ubig32_t =
internal::PackedEndianSpecificIntegral<uint32_t, Endianness::Big, ALIGNED>;
using aligned_ubig64_t =
internal::PackedEndianSpecificIntegral<uint64_t, Endianness::Big, ALIGNED>;

using aligned_big16_t =
internal::PackedEndianSpecificIntegral<int16_t, Endianness::Big, ALIGNED>;
using aligned_big32_t =
internal::PackedEndianSpecificIntegral<int32_t, Endianness::Big, ALIGNED>;
using aligned_big64_t =
internal::PackedEndianSpecificIntegral<int64_t, Endianness::Big, ALIGNED>;

using UNALIGNED_uint16_t =
internal::PackedEndianSpecificIntegral<uint16_t, Endianness::Native, UNALIGNED>;
using UNALIGNED_uint32_t =
internal::PackedEndianSpecificIntegral<uint32_t, Endianness::Native, UNALIGNED>;
using UNALIGNED_uint64_t =
internal::PackedEndianSpecificIntegral<uint64_t, Endianness::Native, UNALIGNED>;

using UNALIGNED_int16_t =
internal::PackedEndianSpecificIntegral<int16_t, Endianness::Native, UNALIGNED>;
using UNALIGNED_int32_t =
internal::PackedEndianSpecificIntegral<int32_t, Endianness::Native, UNALIGNED>;
using UNALIGNED_int64_t =
internal::PackedEndianSpecificIntegral<int64_t, Endianness::Native, UNALIGNED>;

namespace endian {

template <typename T> inline T read(const void *ptr, Endianness endian)
{
   return read<T, UNALIGNED>(ptr, endian);
}

template <typename T, Endianness Endian> inline T read(const void *ptr)
{
   return *(const internal::PackedEndianSpecificIntegral<T, Endian, UNALIGNED> *)ptr;
}

inline uint16_t read16(const void *ptr, Endianness endian)
{
   return read<uint16_t>(ptr, endian);
}

inline uint32_t read32(const void *ptr, Endianness endian)
{
   return read<uint32_t>(ptr, endian);
}

inline uint64_t read64(const void *ptr, Endianness endian)
{
   return read<uint64_t>(ptr, endian);
}

template <Endianness Endian> inline uint16_t read16(const void *ptr)
{
   return read<uint16_t, Endian>(ptr);
}

template <Endianness Endian> inline uint32_t read32(const void *ptr)
{
   return read<uint32_t, Endian>(ptr);
}

template <Endianness Endian> inline uint64_t read64(const void *ptr)
{
   return read<uint64_t, Endian>(ptr);
}

inline uint16_t read16le(const void *ptr)
{
   return read16<Endianness::Little>(ptr);
}

inline uint32_t read32le(const void *ptr)
{
   return read32<Endianness::Little>(ptr);
}

inline uint64_t read64le(const void *ptr)
{
   return read64<Endianness::Little>(ptr);
}

inline uint16_t read16be(const void *ptr)
{
   return read16<Endianness::Big>(ptr);
}

inline uint32_t read32be(const void *ptr)
{
   return read32<Endianness::Big>(ptr);
}

inline uint64_t read64be(const void *ptr)
{
   return read64<Endianness::Big>(ptr);
}

template <typename T>
inline void write(void *ptr, T value, Endianness endian)
{
   write<T, UNALIGNED>(ptr, value, endian);
}

template <typename T, Endianness Endian>
inline void write(void *ptr, T value)
{
   *(internal::PackedEndianSpecificIntegral<T, Endian, UNALIGNED> *)ptr = value;
}

inline void write16(void *ptr, uint16_t value, Endianness endian)
{
   write<uint16_t>(ptr, value, endian);
}

inline void write32(void *ptr, uint32_t value, Endianness endian)
{
   write<uint32_t>(ptr, value, endian);
}

inline void write64(void *ptr, uint64_t value, Endianness endian)
{
   write<uint64_t>(ptr, value, endian);
}

template <Endianness Endian>
inline void write16(void *ptr, uint16_t value)
{
   write<uint16_t, Endian>(ptr, value);
}

template <Endianness Endian>
inline void write32(void *ptr, uint32_t value)
{
   write<uint32_t, Endian>(ptr, value);
}

template <Endianness Endian>
inline void write64(void *ptr, uint64_t value)
{
   write<uint64_t, Endian>(ptr, value);
}

inline void write16le(void *ptr, uint16_t value)
{
   write16<Endianness::Little>(ptr, value);
}

inline void write32le(void *ptr, uint32_t value)
{
   write32<Endianness::Little>(ptr, value);
}

inline void write64le(void *ptr, uint64_t value)
{
   write64<Endianness::Little>(ptr, value);
}

inline void write16be(void *ptr, uint16_t value)
{
   write16<Endianness::Big>(ptr, value);
}

inline void write32be(void *ptr, uint32_t value)
{
   write32<Endianness::Big>(ptr, value);
}

inline void write64be(void *ptr, uint64_t value)
{
   write64<Endianness::Big>(ptr, value);
}

} // endian

} // utils
} // polar

#endif // POLARPHP_UTILS_ENDIAN_H
