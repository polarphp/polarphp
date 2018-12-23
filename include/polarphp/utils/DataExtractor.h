// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/29.

#ifndef POLARPHP_UTILS_DATA_EXTRACTOR_H
#define POLARPHP_UTILS_DATA_EXTRACTOR_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/global/DataTypes.h"

namespace polar {
namespace utils {

using polar::basic::StringRef;

/// An auxiliary type to facilitate extraction of 3-byte entities.
struct Uint24
{
   uint8_t m_bytes[3];
   Uint24(uint8_t uint)
   {
      m_bytes[0] = m_bytes[1] = m_bytes[2] = uint;
   }

   Uint24(uint8_t uint0, uint8_t uint1, uint8_t uint2)
   {
      m_bytes[0] = uint0;
      m_bytes[1] = uint1;
      m_bytes[2] = uint2;
   }

   uint32_t getAsUint32(bool m_isLittleEndian) const
   {
      int loIx = m_isLittleEndian ? 0 : 2;
      return m_bytes[loIx] + (m_bytes[1] << 8) + (m_bytes[2-loIx] << 16);
   }
};

using uint24_t = Uint24;
static_assert(sizeof(uint24_t) == 3, "sizeof(uint24_t) != 3");

/// Needed by swapByteOrder().
inline uint24_t get_swapped_bytes(uint24_t c)
{
   return uint24_t(c.m_bytes[2], c.m_bytes[1], c.m_bytes[0]);
}

class DataExtractor
{
   StringRef m_data;
   uint8_t m_isLittleEndian;
   uint8_t m_addressSize;
public:
   /// Construct with a buffer that is owned by the caller.
   ///
   /// This constructor allows us to use data that is owned by the
   /// caller. The data must stay around as long as this object is
   /// valid.
   DataExtractor(StringRef data, bool isLittleEndian, uint8_t addressSize)
      : m_data(data), m_isLittleEndian(isLittleEndian), m_addressSize(addressSize)
   {}

   /// \brief Get the data pointed to by this extractor.
   StringRef getData() const
   {
      return m_data;
   }

   /// \brief Get the endianness for this extractor.
   bool isLittleEndian() const
   {
      return m_isLittleEndian;
   }

   /// \brief Get the address size for this extractor.
   uint8_t getAddressSize() const
   {
      return m_addressSize;
   }

   /// \brief Set the address size for this extractor.
   void setAddressSize(uint8_t size)
   {
      m_addressSize = size;
   }

   /// Extract a C string from \a *offsetPtr.
   ///
   /// Returns a pointer to a C String from the data at the offset
   /// pointed to by \a offsetPtr. A variable length NULL terminated C
   /// string will be extracted and the \a offsetPtr will be
   /// updated with the offset of the byte that follows the NULL
   /// terminator byte.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @return
   ///     A pointer to the C string value in the data. If the offset
   ///     pointed to by \a offsetPtr is out of bounds, or if the
   ///     offset plus the length of the C string is out of bounds,
   ///     NULL will be returned.
   const char *getCStr(uint32_t *offsetPtr) const;

   /// Extract a C string from \a *OffsetPtr.
   ///
   /// Returns a StringRef for the C String from the data at the offset
   /// pointed to by \a OffsetPtr. A variable length NULL terminated C
   /// string will be extracted and the \a OffsetPtr will be
   /// updated with the offset of the byte that follows the NULL
   /// terminator byte.
   ///
   /// \param[in,out] OffsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// \return
   ///     A StringRef for the C string value in the data. If the offset
   ///     pointed to by \a OffsetPtr is out of bounds, or if the
   ///     offset plus the length of the C string is out of bounds,
   ///     a default-initialized StringRef will be returned.
   StringRef getCStrRef(uint32_t *OffsetPtr) const;

   /// Extract an unsigned integer of size \a byte_size from \a
   /// *offsetPtr.
   ///
   /// Extract a single unsigned integer value and update the offset
   /// pointed to by \a offsetPtr. The size of the extracted integer
   /// is specified by the \a byte_size argument. \a byte_size should
   /// have a value greater than or equal to one and less than or equal
   /// to eight since the return value is 64 bits wide. Any
   /// \a byte_size values less than 1 or greater than 8 will result in
   /// nothing being extracted, and zero being returned.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @param[in] byte_size
   ///     The size in byte of the integer to extract.
   ///
   /// @return
   ///     The unsigned integer value that was extracted, or zero on
   ///     failure.
   uint64_t getUnsigned(uint32_t *offsetPtr, uint32_t byte_size) const;

   /// Extract an signed integer of size \a byte_size from \a *offsetPtr.
   ///
   /// Extract a single signed integer value (sign extending if required)
   /// and update the offset pointed to by \a offsetPtr. The size of
   /// the extracted integer is specified by the \a byte_size argument.
   /// \a byte_size should have a value greater than or equal to one
   /// and less than or equal to eight since the return value is 64
   /// bits wide. Any \a byte_size values less than 1 or greater than
   /// 8 will result in nothing being extracted, and zero being returned.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @param[in] size
   ///     The size in bytes of the integer to extract.
   ///
   /// @return
   ///     The sign extended signed integer value that was extracted,
   ///     or zero on failure.
   int64_t getSigned(uint32_t *offsetPtr, uint32_t size) const;

   //------------------------------------------------------------------
   /// Extract an pointer from \a *offsetPtr.
   ///
   /// Extract a single pointer from the data and update the offset
   /// pointed to by \a offsetPtr. The size of the extracted pointer
   /// is \a getAddressSize(), so the address size has to be
   /// set correctly prior to extracting any pointer values.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @return
   ///     The extracted pointer value as a 64 integer.
   uint64_t getAddress(uint32_t *offsetPtr) const
   {
      return getUnsigned(offsetPtr, m_addressSize);
   }

   /// Extract a uint8_t value from \a *offsetPtr.
   ///
   /// Extract a single uint8_t from the binary data at the offset
   /// pointed to by \a offsetPtr, and advance the offset on success.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @return
   ///     The extracted uint8_t value.
   uint8_t getU8(uint32_t *offsetPtr) const;

   /// Extract \a count uint8_t values from \a *offsetPtr.
   ///
   /// Extract \a count uint8_t values from the binary data at the
   /// offset pointed to by \a offsetPtr, and advance the offset on
   /// success. The extracted values are copied into \a dst.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @param[out] dst
   ///     A buffer to copy \a count uint8_t values into. \a dst must
   ///     be large enough to hold all requested data.
   ///
   /// @param[in] count
   ///     The number of uint8_t values to extract.
   ///
   /// @return
   ///     \a dst if all values were properly extracted and copied,
   ///     NULL otherise.
   uint8_t *getU8(uint32_t *offsetPtr, uint8_t *dst, uint32_t count) const;

   //------------------------------------------------------------------
   /// Extract a uint16_t value from \a *offsetPtr.
   ///
   /// Extract a single uint16_t from the binary data at the offset
   /// pointed to by \a offsetPtr, and update the offset on success.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @return
   ///     The extracted uint16_t value.
   //------------------------------------------------------------------
   uint16_t getU16(uint32_t *offsetPtr) const;

   /// Extract \a count uint16_t values from \a *offsetPtr.
   ///
   /// Extract \a count uint16_t values from the binary data at the
   /// offset pointed to by \a offsetPtr, and advance the offset on
   /// success. The extracted values are copied into \a dst.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @param[out] dst
   ///     A buffer to copy \a count uint16_t values into. \a dst must
   ///     be large enough to hold all requested data.
   ///
   /// @param[in] count
   ///     The number of uint16_t values to extract.
   ///
   /// @return
   ///     \a dst if all values were properly extracted and copied,
   ///     NULL otherise.
   uint16_t *getU16(uint32_t *offsetPtr, uint16_t *dst, uint32_t count) const;

   /// Extract a 24-bit unsigned value from \a *offsetPtr and return it
   /// in a uint32_t.
   ///
   /// Extract 3 bytes from the binary data at the offset pointed to by
   /// \a offsetPtr, construct a uint32_t from them and update the offset
   /// on success.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the 3 bytes if the value is extracted correctly. If the offset
   ///     is out of bounds or there are not enough bytes to extract this value,
   ///     the offset will be left unmodified.
   ///
   /// @return
   ///     The extracted 24-bit value represented in a uint32_t.
   uint32_t getU24(uint32_t *offsetPtr) const;

   /// Extract a uint32_t value from \a *offsetPtr.
   ///
   /// Extract a single uint32_t from the binary data at the offset
   /// pointed to by \a offsetPtr, and update the offset on success.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @return
   ///     The extracted uint32_t value.
   uint32_t getU32(uint32_t *offsetPtr) const;

   /// Extract \a count uint32_t values from \a *offsetPtr.
   ///
   /// Extract \a count uint32_t values from the binary data at the
   /// offset pointed to by \a offsetPtr, and advance the offset on
   /// success. The extracted values are copied into \a dst.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @param[out] dst
   ///     A buffer to copy \a count uint32_t values into. \a dst must
   ///     be large enough to hold all requested data.
   ///
   /// @param[in] count
   ///     The number of uint32_t values to extract.
   ///
   /// @return
   ///     \a dst if all values were properly extracted and copied,
   ///     NULL otherise.
   uint32_t *getU32(uint32_t *offsetPtr, uint32_t *dst, uint32_t count) const;

   /// Extract a uint64_t value from \a *offsetPtr.
   ///
   /// Extract a single uint64_t from the binary data at the offset
   /// pointed to by \a offsetPtr, and update the offset on success.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @return
   ///     The extracted uint64_t value.
   uint64_t getU64(uint32_t *offsetPtr) const;

   /// Extract \a count uint64_t values from \a *offsetPtr.
   ///
   /// Extract \a count uint64_t values from the binary data at the
   /// offset pointed to by \a offsetPtr, and advance the offset on
   /// success. The extracted values are copied into \a dst.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @param[out] dst
   ///     A buffer to copy \a count uint64_t values into. \a dst must
   ///     be large enough to hold all requested data.
   ///
   /// @param[in] count
   ///     The number of uint64_t values to extract.
   ///
   /// @return
   ///     \a dst if all values were properly extracted and copied,
   ///     NULL otherise.
   uint64_t *getU64(uint32_t *offsetPtr, uint64_t *dst, uint32_t count) const;

   /// Extract a signed LEB128 value from \a *offsetPtr.
   ///
   /// Extracts an signed LEB128 number from this object's data
   /// starting at the offset pointed to by \a offsetPtr. The offset
   /// pointed to by \a offsetPtr will be updated with the offset of
   /// the byte following the last extracted byte.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @return
   ///     The extracted signed integer value.
   int64_t getSLEB128(uint32_t *offsetPtr) const;

   /// Extract a unsigned LEB128 value from \a *offsetPtr.
   ///
   /// Extracts an unsigned LEB128 number from this object's data
   /// starting at the offset pointed to by \a offsetPtr. The offset
   /// pointed to by \a offsetPtr will be updated with the offset of
   /// the byte following the last extracted byte.
   ///
   /// @param[in,out] offsetPtr
   ///     A pointer to an offset within the data that will be advanced
   ///     by the appropriate number of bytes if the value is extracted
   ///     correctly. If the offset is out of bounds or there are not
   ///     enough bytes to extract this value, the offset will be left
   ///     unmodified.
   ///
   /// @return
   ///     The extracted unsigned integer value.
   uint64_t getULEB128(uint32_t *offsetPtr) const;

   /// Test the validity of \a offset.
   ///
   /// @return
   ///     \b true if \a offset is a valid offset into the data in this
   ///     object, \b false otherwise.
   bool isValidOffset(uint32_t offset) const
   {
      return m_data.getSize() > offset;
   }

   /// Test the availability of \a length bytes of data from \a offset.
   ///
   /// @return
   ///     \b true if \a offset is a valid offset and there are \a
   ///     length bytes available at that offset, \b false otherwise.
   bool isValidOffsetForDataOfSize(uint32_t offset, uint32_t length) const
   {
      return offset + length >= offset && isValidOffset(offset + length - 1);
   }

   /// Test the availability of enough bytes of data for a pointer from
   /// \a offset. The size of a pointer is \a getAddressSize().
   ///
   /// @return
   ///     \b true if \a offset is a valid offset and there are enough
   ///     bytes for a pointer available at that offset, \b false
   ///     otherwise.
   bool isValidOffsetForAddress(uint32_t offset) const
   {
      return isValidOffsetForDataOfSize(offset, m_addressSize);
   }
};

} // utils
} // polar


#endif // POLARPHP_UTILS_DATA_EXTRACTOR_H
