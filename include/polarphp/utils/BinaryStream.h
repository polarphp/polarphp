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

#ifndef POLARPHP_UTILS_BINARY_STREAM_H
#define POLARPHP_UTILS_BINARY_STREAM_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/BitmaskEnum.h"
#include "polarphp/utils/BinaryStreamError.h"
#include "polarphp/utils/Endian.h"
#include "polarphp/utils/Error.h"
#include <cstdint>

namespace polar {
namespace utils {

using polar::basic::ArrayRef;

enum BinaryStreamFlags
{
   BSF_None = 0,
   BSF_Write = 1,  // Stream supports writing.
   BSF_Append = 2, // Writing can occur at offset == length.
   POLAR_MARK_AS_BITMASK_ENUM(/* LargestValue = */ BSF_Append)
};

/// \brief An interface for accessing data in a stream-like format, but which
/// discourages copying.  Instead of specifying a buffer in which to copy
/// data on a read, the API returns an ArrayRef to data owned by the stream's
/// implementation.  Since implementations may not necessarily store data in a
/// single contiguous buffer (or even in memory at all), in such cases a it may
/// be necessary for an implementation to cache such a buffer so that it can
/// return it.
class BinaryStream
{
public:
   virtual ~BinaryStream() = default;

   virtual Endianness getEndian() const = 0;

   /// \brief Given an offset into the stream and a number of bytes, attempt to
   /// read the bytes and set the output ArrayRef to point to data owned by the
   /// stream.
   virtual Error readBytes(uint32_t offset, uint32_t size,
                           ArrayRef<uint8_t> &buffer) = 0;

   /// \brief Given an offset into the stream, read as much as possible without
   /// copying any data.
   virtual Error readLongestContiguousChunk(uint32_t offset,
                                            ArrayRef<uint8_t> &buffer) = 0;

   /// \brief Return the number of bytes of data in this stream.
   virtual uint32_t getLength() = 0;

   /// \brief Return the properties of this stream.
   virtual BinaryStreamFlags getFlags() const
   {
      return BSF_None;
   }

protected:
   Error checkOffsetForRead(uint32_t offset, uint32_t dataSize)
   {
      if (offset > getLength()) {
         return make_error<BinaryStreamError>(StreamErrorCode::invalid_offset);
      }
      if (getLength() < dataSize + offset) {
         return make_error<BinaryStreamError>(StreamErrorCode::stream_too_short);
      }
      return Error::getSuccess();
   }
};

/// \brief A BinaryStream which can be read from as well as written to.  Note
/// that writing to a BinaryStream always necessitates copying from the input
/// buffer to the stream's backing store.  Streams are assumed to be buffered
/// so that to be portable it is necessary to call commit() on the stream when
/// all data has been written.
class WritableBinaryStream : public BinaryStream
{
public:
   ~WritableBinaryStream() override = default;

   /// \brief Attempt to write the given bytes into the stream at the desired
   /// offset. This will always necessitate a copy.  Cannot shrink or grow the
   /// stream, only writes into existing allocated space.
   virtual Error writeBytes(uint32_t offset, ArrayRef<uint8_t> data) = 0;

   /// \brief For buffered streams, commits changes to the backing store.
   virtual Error commit() = 0;

   /// \brief Return the properties of this stream.
   BinaryStreamFlags getFlags() const override
   {
      return BSF_Write;
   }

protected:
   Error checkOffsetForWrite(uint32_t offset, uint32_t dataSize)
   {
      if (!(getFlags() & BSF_Append)) {
         return checkOffsetForRead(offset, dataSize);
      }
      if (offset > getLength()) {
         return make_error<BinaryStreamError>(StreamErrorCode::invalid_offset);
      }
      return Error::getSuccess();
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_BINARY_STREAM_H
