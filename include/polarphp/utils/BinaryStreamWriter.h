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

#ifndef POLARPHP_UTILS_BINARY_STREAM_WRITER_H
#define POLARPHP_UTILS_BINARY_STREAM_WRITER_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/BinaryStreamArray.h"
#include "polarphp/utils/BinaryStreamError.h"
#include "polarphp/utils/BinaryStreamRef.h"
#include "polarphp/utils/Endian.h"
#include "polarphp/utils/Error.h"
#include <cstdint>
#include <type_traits>
#include <utility>

namespace polar {
namespace utils {

/// \brief Provides write only access to a subclass of `WritableBinaryStream`.
/// Provides bounds checking and helpers for writing certain common data types
/// such as null-terminated strings, integers in various flavors of endianness,
/// etc.  Can be subclassed to provide reading and writing of custom datatypes,
/// although no methods are overridable.
class BinaryStreamWriter
{
public:
   BinaryStreamWriter() = default;
   explicit BinaryStreamWriter(WritableBinaryStreamRef streamRef);
   explicit BinaryStreamWriter(WritableBinaryStream &stream);
   explicit BinaryStreamWriter(MutableArrayRef<uint8_t> data,
                               Endianness endian);

   BinaryStreamWriter(const BinaryStreamWriter &other)
      : m_stream(other.m_stream), m_offset(other.m_offset)
   {}

   BinaryStreamWriter &operator=(const BinaryStreamWriter &other)
   {
      m_stream = other.m_stream;
      m_offset = other.m_offset;
      return *this;
   }

   virtual ~BinaryStreamWriter()
   {}

   /// Write the bytes specified in \p buffer to the underlying stream.
   /// On success, updates the offset so that subsequent writes will occur
   /// at the next unwritten position.
   ///
   /// \returns a success error code if the data was successfully written,
   /// otherwise returns an appropriate error code.
   Error writeBytes(ArrayRef<uint8_t> buffer);

   /// Write the the integer \p value to the underlying stream in the
   /// specified endianness.  On success, updates the offset so that
   /// subsequent writes occur at the next unwritten position.
   ///
   /// \returns a success error code if the data was successfully written,
   /// otherwise returns an appropriate error code.
   template <typename T> Error writeInteger(T value)
   {
      static_assert(std::is_integral<T>::value,
                    "Cannot call writeInteger with non-integral value!");
      uint8_t buffer[sizeof(T)];
      endian::write<T, UNALIGNED>(
               buffer, value, m_stream.getEndian());
      return writeBytes(buffer);
   }

   /// Similar to writeInteger
   template <typename T> Error writeEnum(T num)
   {
      static_assert(std::is_enum<T>::value,
                    "Cannot call writeEnum with non-Enum type");

      using U = typename std::underlying_type<T>::type;
      return writeInteger<U>(static_cast<U>(num));
   }

   /// Write the the string \p str to the underlying stream followed by a null
   /// terminator.  On success, updates the offset so that subsequent writes
   /// occur at the next unwritten position.  \p str need not be null terminated
   /// on input.
   ///
   /// \returns a success error code if the data was successfully written,
   /// otherwise returns an appropriate error code.
   Error writeCString(StringRef str);

   /// Write the the string \p str to the underlying stream without a null
   /// terminator.  On success, updates the offset so that subsequent writes
   /// occur at the next unwritten position.
   ///
   /// \returns a success error code if the data was successfully written,
   /// otherwise returns an appropriate error code.
   Error writeFixedString(StringRef str);

   /// Efficiently reads all data from \p streamRef, and writes it to this stream.
   /// This operation will not invoke any copies of the source data, regardless
   /// of the source stream's implementation.
   ///
   /// \returns a success error code if the data was successfully written,
   /// otherwise returns an appropriate error code.
   Error writeStreamRef(BinaryStreamRef streamRef);

   /// Efficiently reads \p size bytes from \p streamRef, and writes it to this stream.
   /// This operation will not invoke any copies of the source data, regardless
   /// of the source stream's implementation.
   ///
   /// \returns a success error code if the data was successfully written,
   /// otherwise returns an appropriate error code.
   Error writeStreamRef(BinaryStreamRef streamRef, uint32_t size);

   /// Writes the object \p obj to the underlying stream, as if by using memcpy.
   /// It is up to the caller to ensure that type of \p obj can be safely copied
   /// in this fashion, as no checks are made to ensure that this is safe.
   ///
   /// \returns a success error code if the data was successfully written,
   /// otherwise returns an appropriate error code.
   template <typename T> Error writeObject(const T &obj)
   {
      static_assert(!std::is_pointer<T>::value,
                    "writeObject should not be used with pointers, to write "
                    "the pointed-to value dereference the pointer before calling "
                    "writeObject");
      return writeBytes(
               ArrayRef<uint8_t>(reinterpret_cast<const uint8_t *>(&obj), sizeof(T)));
   }

   /// Writes an array of objects of type T to the underlying stream, as if by
   /// using memcpy.  It is up to the caller to ensure that type of \p obj can
   /// be safely copied in this fashion, as no checks are made to ensure that
   /// this is safe.
   ///
   /// \returns a success error code if the data was successfully written,
   /// otherwise returns an appropriate error code.
   template <typename T>
   Error writeArray(ArrayRef<T> array)
   {
      if (array.empty()) {
         return Error::getSuccess();
      }
      if (array.getSize() > UINT32_MAX / sizeof(T)) {
         return make_error<BinaryStreamError>(
                  StreamErrorCode::invalid_array_size);
      }
      return writeBytes(
               ArrayRef<uint8_t>(reinterpret_cast<const uint8_t *>(array.getData()),
                                 array.getSize() * sizeof(T)));
   }

   /// Writes all data from the array \p array to the underlying stream.
   ///
   /// \returns a success error code if the data was successfully written,
   /// otherwise returns an appropriate error code.
   template <typename T, typename U>
   Error writeArray(VarStreamArray<T, U> array)
   {
      return writeStreamRef(array.getUnderlyingStream());
   }

   /// Writes all elements from the array \p array to the underlying stream.
   ///
   /// \returns a success error code if the data was successfully written,
   /// otherwise returns an appropriate error code.
   template <typename T>
   Error writeArray(FixedStreamArray<T> array)
   {
      return writeStreamRef(array.getUnderlyingStream());
   }

   /// Splits the Writer into two Writers at a given offset.
   std::pair<BinaryStreamWriter, BinaryStreamWriter> split(uint32_t offset) const;

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

   Error padToAlignment(uint32_t align);

protected:
   WritableBinaryStreamRef m_stream;
   uint32_t m_offset = 0;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_BINARY_STREAM_WRITER_H
