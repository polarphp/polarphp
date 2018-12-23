// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/28.

#ifndef POLARPHP_UTILS_BINARY_STREAM_READER_H
#define POLARPHP_UTILS_BINARY_STREAM_READER_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/utils/BinaryStreamArray.h"
#include "polarphp/utils/BinaryStreamRef.h"
#include "polarphp/utils/ConvertUtf.h"
#include "polarphp/utils/Endian.h"
#include "polarphp/utils/Error.h"
#include "polarphp/utils/TypeTraits.h"

#include <string>
#include <type_traits>

namespace polar {
namespace utils {

/// \brief Provides read only access to a subclass of `BinaryStream`.  Provides
/// bounds checking and helpers for writing certain common data types such as
/// null-terminated strings, integers in various flavors of endianness, etc.
/// Can be subclassed to provide reading of custom datatypes, although no
/// are overridable.
class BinaryStreamReader
{
public:
   BinaryStreamReader() = default;
   explicit BinaryStreamReader(BinaryStreamRef streamRef);
   explicit BinaryStreamReader(BinaryStream &stream);
   explicit BinaryStreamReader(ArrayRef<uint8_t> data,
                               Endianness endian);
   explicit BinaryStreamReader(StringRef data, Endianness endian);

   BinaryStreamReader(const BinaryStreamReader &other)
      : m_stream(other.m_stream),
        m_offset(other.m_offset)
   {}

   BinaryStreamReader &operator=(const BinaryStreamReader &other)
   {
      m_stream = other.m_stream;
      m_offset = other.m_offset;
      return *this;
   }

   virtual ~BinaryStreamReader()
   {}

   /// Read as much as possible from the underlying string at the current offset
   /// without invoking a copy, and set \p Buffer to the resulting data slice.
   /// Updates the stream's offset to point after the newly read data.
   ///
   /// \returns a success error code if the data was successfully read, otherwise
   /// returns an appropriate error code.
   Error readLongestContiguousChunk(ArrayRef<uint8_t> &buffer);

   /// Read \p Size bytes from the underlying stream at the current offset and
   /// and set \p Buffer to the resulting data slice.  Whether a copy occurs
   /// depends on the implementation of the underlying stream.  Updates the
   /// stream's offset to point after the newly read data.
   ///
   /// \returns a success error code if the data was successfully read, otherwise
   /// returns an appropriate error code.
   Error readBytes(ArrayRef<uint8_t> &buffer, uint32_t size);

   /// Read an integer of the specified endianness into \p dest and update the
   /// stream's offset.  The data is always copied from the stream's underlying
   /// buffer into \p dest. Updates the stream's offset to point after the newly
   /// read data.
   ///
   /// \returns a success error code if the data was successfully read, otherwise
   /// returns an appropriate error code.
   template <typename T> Error readInteger(T &dest)
   {
      static_assert(std::is_integral<T>::value,
                    "Cannot call readInteger with non-integral value!");

      ArrayRef<uint8_t> bytes;
      if (auto errorCode = readBytes(bytes, sizeof(T))) {
         return errorCode;
      }
      dest = endian::read<T, UNALIGNED>(
               bytes.getData(), m_stream.getEndian());
      return Error::getSuccess();
   }

   /// Similar to readInteger.
   template <typename T> Error readEnum(T &dest)
   {
      static_assert(std::is_enum<T>::value,
                    "Cannot call readEnum with non-enum value!");
      typename std::underlying_type<T>::type N;
      if (auto errorCode = readInteger(N)) {
         return errorCode;
      }
      dest = static_cast<T>(N);
      return Error::getSuccess();
   }

   /// Read a null terminated string from \p dest.  Whether a copy occurs depends
   /// on the implementation of the underlying stream.  Updates the stream's
   /// offset to point after the newly read data.
   ///
   /// \returns a success error code if the data was successfully read, otherwise
   /// returns an appropriate error code.
   Error readCString(StringRef &dest);

   /// Similar to readCString, however read a null-terminated UTF16 string
   /// instead.
   ///
   /// \returns a success error code if the data was successfully read, otherwise
   /// returns an appropriate error code.
   Error readWideString(ArrayRef<Utf16> &dest);

   /// Read a \p Length byte string into \p dest.  Whether a copy occurs depends
   /// on the implementation of the underlying stream.  Updates the stream's
   /// offset to point after the newly read data.
   ///
   /// \returns a success error code if the data was successfully read, otherwise
   /// returns an appropriate error code.
   Error readFixedString(StringRef &dest, uint32_t length);

   /// Read the entire remainder of the underlying stream into \p Ref.  This is
   /// equivalent to calling getUnderlyingStream().slice(Offset).  Updates the
   /// stream's offset to point to the end of the stream.  Never causes a copy.
   ///
   /// \returns a success error code if the data was successfully read, otherwise
   /// returns an appropriate error code.
   Error readStreamRef(BinaryStreamRef &ref);

   /// Read \p Length bytes from the underlying stream into \p Ref.  This is
   /// equivalent to calling getUnderlyingStream().slice(Offset, Length).
   /// Updates the stream's offset to point after the newly read object.  Never
   /// causes a copy.
   ///
   /// \returns a success error code if the data was successfully read, otherwise
   /// returns an appropriate error code.
   Error readStreamRef(BinaryStreamRef &ref, uint32_t length);

   /// Read \p Length bytes from the underlying stream into \p m_stream.  This is
   /// equivalent to calling getUnderlyingStream().slice(Offset, Length).
   /// Updates the stream's offset to point after the newly read object.  Never
   /// causes a copy.
   ///
   /// \returns a success error code if the data was successfully read, otherwise
   /// returns an appropriate error code.
   Error readSubstream(BinarySubstreamRef &stream, uint32_t size);

   /// Get a pointer to an object of type T from the underlying stream, as if by
   /// memcpy, and store the result into \p dest.  It is up to the caller to
   /// ensure that objects of type T can be safely treated in this manner.
   /// Updates the stream's offset to point after the newly read object.  Whether
   /// a copy occurs depends upon the implementation of the underlying
   /// stream.
   ///
   /// \returns a success error code if the data was successfully read, otherwise
   /// returns an appropriate error code.
   template <typename T>
   Error readObject(const T *&dest)
   {
      ArrayRef<uint8_t> buffer;
      if (auto errorCode = readBytes(buffer, sizeof(T))) {
         return errorCode;
      }
      dest = reinterpret_cast<const T *>(buffer.getData());
      return Error::getSuccess();
   }

   /// Get a reference to a \p numElements element array of objects of type T
   /// from the underlying stream as if by memcpy, and store the resulting array
   /// slice into \p array.  It is up to the caller to ensure that objects of
   /// type T can be safely treated in this manner.  Updates the stream's offset
   /// to point after the newly read object.  Whether a copy occurs depends upon
   /// the implementation of the underlying stream.
   ///
   /// \returns a success error code if the data was successfully read, otherwise
   /// returns an appropriate error code.
   template <typename T>
   Error readArray(ArrayRef<T> &array, uint32_t numElements)
   {
      ArrayRef<uint8_t> bytes;
      if (numElements == 0) {
         array = ArrayRef<T>();
         return Error::getSuccess();
      }
      if (numElements > UINT32_MAX / sizeof(T)) {
         return make_error<BinaryStreamError>(
                  StreamErrorCode::invalid_array_size);
      }
      if (auto errorCode = readBytes(bytes, numElements * sizeof(T))) {
         return errorCode;
      }
      assert(alignment_adjustment(bytes.getData(), alignof(T)) == 0 &&
             "Reading at invalid alignment!");

      array = ArrayRef<T>(reinterpret_cast<const T *>(bytes.getData()), numElements);
      return Error::getSuccess();
   }

   /// Read a VarStreamArray of size \p Size bytes and store the result into
   /// \p Array.  Updates the stream's offset to point after the newly read
   /// array.  Never causes a copy (although iterating the elements of the
   /// VarStreamArray may, depending upon the implementation of the underlying
   /// stream).
   ///
   /// \returns a success error code if the data was successfully read, otherwise
   /// returns an appropriate error code.
   template <typename T, typename U>
   Error readArray(VarStreamArray<T, U> &array, uint32_t size)
   {
      BinaryStreamRef stream;
      if (auto errorCode = readStreamRef(stream, size)) {
         return errorCode;
      }
      array.setUnderlyingStream(stream);
      return Error::getSuccess();
   }

   /// Read a FixedStreamArray of \p NumItems elements and store the result into
   /// \p Array.  Updates the stream's offset to point after the newly read
   /// array.  Never causes a copy (although iterating the elements of the
   /// FixedStreamArray may, depending upon the implementation of the underlying
   /// stream).
   ///
   /// \returns a success error code if the data was successfully read, otherwise
   /// returns an appropriate error code.
   template <typename T>
   Error readArray(FixedStreamArray<T> &array, uint32_t numItems)
   {
      if (numItems == 0) {
         array = FixedStreamArray<T>();
         return Error::getSuccess();
      }
      if (numItems > UINT32_MAX / sizeof(T)) {
         return make_error<BinaryStreamError>(
                  StreamErrorCode::invalid_array_size);
      }
      BinaryStreamRef view;
      if (auto errorCode = readStreamRef(view, numItems * sizeof(T))) {
         return errorCode;
      }
      array = FixedStreamArray<T>(view);
      return Error::getSuccess();
   }

   bool empty() const
   {
      return getBytesRemaining() == 0;
   }

   void setOffset(uint32_t offset)
   {
      m_offset = offset;
   }

   uint32_t getOffset() const
   {
      return m_offset;
   }

   uint32_t getLength() const
   {
      return m_stream.getLength();
   }

   uint32_t getBytesRemaining() const
   {
      return getLength() - getOffset();
   }

   /// Advance the stream's offset by \p Amount bytes.
   ///
   /// \returns a success error code if at least \p Amount bytes remain in the
   /// stream, otherwise returns an appropriate error code.
   Error skip(uint32_t amount);

   /// Examine the next byte of the underlying stream without advancing the
   /// stream's offset.  If the stream is empty the behavior is undefined.
   ///
   /// \returns the next byte in the stream.
   uint8_t peek() const;

   Error padToAlignment(uint32_t align);

   std::pair<BinaryStreamReader, BinaryStreamReader>
   split(uint32_t offset) const;

private:
   BinaryStreamRef m_stream;
   uint32_t m_offset = 0;
};

} // basic
} // polar

#endif // POLARPHP_UTILS_BINARY_STREAM_READER_H
