// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/03.

#include "polarphp/utils/DataExtractor.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/Host.h"
#include "polarphp/utils/SwapByteOrder.h"

namespace polar {
namespace utils {

template <typename T>
static T getU(uint32_t *offsetPtr, const DataExtractor *extractor,
              bool isLittleEndian, const char *data)
{
   T value = 0;
   uint32_t offset = *offsetPtr;
   if (extractor->isValidOffsetForDataOfSize(offset, sizeof(value))) {
      std::memcpy(&value, &data[offset], sizeof(value));
      if (sys::sg_isLittleEndianHost != isLittleEndian) {
         polar::utils::swap_byte_order(value);
      }
      // Advance the offset
      *offsetPtr += sizeof(value);
   }
   return value;
}

template <typename T>
static T *getUs(uint32_t *offsetPtr, T *dst, uint32_t count,
                const DataExtractor *extractor, bool isLittleEndian, const char *data)
{
   uint32_t offset = *offsetPtr;
   if (count > 0 && extractor->isValidOffsetForDataOfSize(offset, sizeof(*dst)*count)) {
      for (T *value_ptr = dst, *end = dst + count; value_ptr != end;
           ++value_ptr, offset += sizeof(*dst)) {
         *value_ptr = getU<T>(offsetPtr, extractor, isLittleEndian, data);
      }
      // Advance the offset
      *offsetPtr = offset;
      // Return a non-NULL pointer to the converted data as an indicator of
      // success
      return dst;
   }
   return nullptr;
}

uint8_t DataExtractor::getU8(uint32_t *offsetPtr) const
{
   return getU<uint8_t>(offsetPtr, this, m_isLittleEndian, m_data.getData());
}

uint8_t *
DataExtractor::getU8(uint32_t *offsetPtr, uint8_t *dst, uint32_t count) const
{
   return getUs<uint8_t>(offsetPtr, dst, count, this, m_isLittleEndian,
                         m_data.getData());
}


uint16_t DataExtractor::getU16(uint32_t *offsetPtr) const
{
   return getU<uint16_t>(offsetPtr, this, m_isLittleEndian, m_data.getData());
}

uint16_t *DataExtractor::getU16(uint32_t *offsetPtr, uint16_t *dst,
                                uint32_t count) const
{
   return getUs<uint16_t>(offsetPtr, dst, count, this, m_isLittleEndian,
                          m_data.getData());
}

uint32_t DataExtractor::getU24(uint32_t *offsetPtr) const
{
   uint24_t ExtractedVal =
         getU<uint24_t>(offsetPtr, this, m_isLittleEndian, m_data.getData());
   // The 3 bytes are in the correct byte order for the host.
   return ExtractedVal.getAsUint32(sys::sg_isLittleEndianHost);
}

uint32_t DataExtractor::getU32(uint32_t *offsetPtr) const
{
   return getU<uint32_t>(offsetPtr, this, m_isLittleEndian, m_data.getData());
}

uint32_t *DataExtractor::getU32(uint32_t *offsetPtr, uint32_t *dst,
                                uint32_t count) const
{
   return getUs<uint32_t>(offsetPtr, dst, count, this, m_isLittleEndian,
                          m_data.getData());
}

uint64_t DataExtractor::getU64(uint32_t *offsetPtr) const
{
   return getU<uint64_t>(offsetPtr, this, m_isLittleEndian, m_data.getData());
}

uint64_t *DataExtractor::getU64(uint32_t *offsetPtr, uint64_t *dst,
                                uint32_t count) const
{
   return getUs<uint64_t>(offsetPtr, dst, count, this, m_isLittleEndian,
                          m_data.getData());
}

uint64_t
DataExtractor::getUnsigned(uint32_t *offsetPtr, uint32_t byte_size) const
{
   switch (byte_size) {
   case 1:
      return getU8(offsetPtr);
   case 2:
      return getU16(offsetPtr);
   case 4:
      return getU32(offsetPtr);
   case 8:
      return getU64(offsetPtr);
   }
   polar_unreachable("getUnsigned unhandled case!");
}

int64_t
DataExtractor::getSigned(uint32_t *offsetPtr, uint32_t byte_size) const {
   switch (byte_size) {
   case 1:
      return (int8_t)getU8(offsetPtr);
   case 2:
      return (int16_t)getU16(offsetPtr);
   case 4:
      return (int32_t)getU32(offsetPtr);
   case 8:
      return (int64_t)getU64(offsetPtr);
   }
   polar_unreachable("getSigned unhandled case!");
}

const char *DataExtractor::getCStr(uint32_t *offsetPtr) const {
   uint32_t offset = *offsetPtr;
   StringRef::size_type pos = m_data.find('\0', offset);
   if (pos != StringRef::npos) {
      *offsetPtr = pos + 1;
      return m_data.getData() + offset;
   }
   return nullptr;
}

StringRef DataExtractor::getCStrRef(uint32_t *OffsetPtr) const {
   uint32_t Start = *OffsetPtr;
   StringRef::size_type Pos = m_data.find('\0', Start);
   if (Pos != StringRef::npos) {
      *OffsetPtr = Pos + 1;
      return StringRef(m_data.getData() + Start, Pos - Start);
   }
   return StringRef();
}

uint64_t DataExtractor::getULEB128(uint32_t *offsetPtr) const {
   uint64_t result = 0;
   if (m_data.empty())
      return 0;

   unsigned shift = 0;
   uint32_t offset = *offsetPtr;
   uint8_t byte = 0;

   while (isValidOffset(offset)) {
      byte = m_data[offset++];
      result |= uint64_t(byte & 0x7f) << shift;
      shift += 7;
      if ((byte & 0x80) == 0)
         break;
   }

   *offsetPtr = offset;
   return result;
}

int64_t DataExtractor::getSLEB128(uint32_t *offsetPtr) const
{
   int64_t result = 0;
   if (m_data.empty()) {
      return 0;
   }
   unsigned shift = 0;
   uint32_t offset = *offsetPtr;
   uint8_t byte = 0;

   while (isValidOffset(offset)) {
      byte = m_data[offset++];
      result |= uint64_t(byte & 0x7f) << shift;
      shift += 7;
      if ((byte & 0x80) == 0) {
         break;
      }
   }

   // Sign bit of byte is 2nd high order bit (0x40)
   if (shift < 64 && (byte & 0x40)) {
      result |= -(1ULL << shift);
   }
   *offsetPtr = offset;
   return result;
}

} // utils
} // polar
