
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

#ifndef POLARPHP_UTILS_BINARY_BYTE_STREAM_H
#define POLARPHP_UTILS_BINARY_BYTE_STREAM_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/BinaryStream.h"
#include "polarphp/utils/BinaryStreamError.h"
#include "polarphp/utils/Error.h"
#include "polarphp/utils/Endian.h"
#include "polarphp/utils/FileOutputBuffer.h"
#include "polarphp/utils/MemoryBuffer.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>

namespace polar {
namespace utils {

using polar::basic::make_array_ref;
using polar::basic::MutableArrayRef;
using polar::basic::ArrayRef;
using polar::basic::StringRef;

/// \brief An implementation of BinaryStream which holds its entire data set
/// in a single contiguous buffer.  BinaryByteStream guarantees that no read
/// operation will ever incur a copy.  Note that BinaryByteStream does not
/// own the underlying buffer.
class BinaryByteStream : public BinaryStream
{
public:
   BinaryByteStream() = default;
   BinaryByteStream(ArrayRef<uint8_t> data, Endianness endian)
      : m_endian(endian), m_data(data)
   {}

   BinaryByteStream(StringRef data, Endianness endian)
      : m_endian(endian), m_data(data.getBytesBegin(), data.getBytesEnd())
   {}

   Endianness getEndian() const override
   {
      return m_endian;
   }

   Error readBytes(uint32_t offset, uint32_t size,
                   ArrayRef<uint8_t> &buffer) override
   {
      if (auto errorCode = checkOffsetForRead(offset, size)) {
         return errorCode;
      }
      buffer = m_data.slice(offset, size);
      return Error::getSuccess();
   }

   Error readLongestContiguousChunk(uint32_t offset,
                                    ArrayRef<uint8_t> &buffer) override
   {
      if (auto errorCode = checkOffsetForRead(offset, 1)) {
         return errorCode;
      }
      buffer = m_data.slice(offset);
      return Error::getSuccess();
   }

   uint32_t getLength() override
   {
      return m_data.getSize();
   }

   ArrayRef<uint8_t> getData() const
   {
      return m_data;
   }

   StringRef getStr() const
   {
      const char *charData = reinterpret_cast<const char *>(m_data.getData());
      return StringRef(charData, m_data.getSize());
   }

protected:
   Endianness m_endian;
   ArrayRef<uint8_t> m_data;
};

/// \brief An implementation of BinaryStream whose data is backed by an llvm
/// MemoryBuffer object.  MemoryBufferByteStream owns the MemoryBuffer in
/// question.  As with BinaryByteStream, reading from a MemoryBufferByteStream
/// will never cause a copy.
class MemoryBufferByteStream : public BinaryByteStream
{
public:
   MemoryBufferByteStream(std::unique_ptr<MemoryBuffer> buffer,
                          Endianness endian)
      : BinaryByteStream(buffer->getBuffer(), endian),
        MemBuffer(std::move(buffer)) {}

   std::unique_ptr<MemoryBuffer> MemBuffer;
};

/// \brief An implementation of BinaryStream which holds its entire data set
/// in a single contiguous buffer.  As with BinaryByteStream, the mutable
/// version also guarantees that no read operation will ever incur a copy,
/// and similarly it does not own the underlying buffer.
class MutableBinaryByteStream : public WritableBinaryStream
{
public:
   MutableBinaryByteStream() = default;
   MutableBinaryByteStream(MutableArrayRef<uint8_t> data,
                           Endianness endian)
      : m_data(data), m_immutableStream(data, endian)
   {}

   Endianness getEndian() const override
   {
      return m_immutableStream.getEndian();
   }

   Error readBytes(uint32_t offset, uint32_t size,
                   ArrayRef<uint8_t> &buffer) override
   {
      return m_immutableStream.readBytes(offset, size, buffer);
   }

   Error readLongestContiguousChunk(uint32_t offset,
                                    ArrayRef<uint8_t> &buffer) override
   {
      return m_immutableStream.readLongestContiguousChunk(offset, buffer);
   }

   uint32_t getLength() override
   {
      return m_immutableStream.getLength();
   }

   Error writeBytes(uint32_t offset, ArrayRef<uint8_t> buffer) override
   {
      if (buffer.empty()) {
         return Error::getSuccess();
      }
      if (auto errorCode = checkOffsetForWrite(offset, buffer.getSize())) {
         return errorCode;
      }
      uint8_t *dataPtr = const_cast<uint8_t *>(m_data.getData());
      ::memcpy(dataPtr + offset, buffer.getData(), buffer.getSize());
      return Error::getSuccess();
   }

   Error commit() override
   {
      return Error::getSuccess();
   }

   MutableArrayRef<uint8_t> getData() const
   {
      return m_data;
   }

private:
   MutableArrayRef<uint8_t> m_data;
   BinaryByteStream m_immutableStream;
};

/// \brief An implementation of WritableBinaryStream which can write at its end
/// causing the underlying data to grow.  This class owns the underlying m_data.
class AppendingBinaryByteStream : public WritableBinaryStream
{
   std::vector<uint8_t> m_data;
   Endianness m_endian = Endianness::Little;

public:
   AppendingBinaryByteStream() = default;
   AppendingBinaryByteStream(Endianness endian)
      : m_endian(endian)
   {}

   void clear()
   {
      m_data.clear();
   }

   Endianness getEndian() const override
   {
      return m_endian;
   }

   Error readBytes(uint32_t offset, uint32_t size,
                   ArrayRef<uint8_t> &buffer) override
   {
      if (auto errorCode = checkOffsetForWrite(offset, buffer.getSize())) {
         return errorCode;
      }
      buffer = make_array_ref(m_data).slice(offset, size);
      return Error::getSuccess();
   }

   void insert(uint32_t offset, ArrayRef<uint8_t> bytes)
   {
      m_data.insert(m_data.begin() + offset, bytes.begin(), bytes.end());
   }

   Error readLongestContiguousChunk(uint32_t offset,
                                    ArrayRef<uint8_t> &buffer) override
   {
      if (auto errorCode = checkOffsetForWrite(offset, 1)) {
         return errorCode;
      }
      buffer = make_array_ref(m_data).slice(offset);
      return Error::getSuccess();
   }

   uint32_t getLength() override
   {
      return m_data.size();
   }

   Error writeBytes(uint32_t offset, ArrayRef<uint8_t> buffer) override
   {
      if (buffer.empty()) {
         return Error::getSuccess();
      }
      // This is well-defined for any case except where offset is strictly
      // greater than the current length.  If offset is equal to the current
      // length, we can still grow.  If offset is beyond the current length, we
      // would have to decide how to deal with the intermediate uninitialized
      // bytes.  So we punt on that case for simplicity and just say it's an
      // error.
      if (offset > getLength()) {
         return make_error<BinaryStreamError>(StreamErrorCode::invalid_offset);
      }
      uint32_t RequiredSize = offset + buffer.getSize();
      if (RequiredSize > m_data.size()) {
         m_data.resize(RequiredSize);
      }

      ::memcpy(m_data.data() + offset, buffer.getData(), buffer.getSize());
      return Error::getSuccess();
   }

   Error commit() override
   {
      return Error::getSuccess();
   }

   /// \brief Return the properties of this stream.
   virtual BinaryStreamFlags getFlags() const override
   {
      return BinaryStreamFlags(polar::as_integer(BSF_Write) | polar::as_integer(BSF_Append));
   }

   MutableArrayRef<uint8_t> getData()
   {
      return m_data;
   }
};

/// \brief An implementation of WritableBinaryStream backed by an llvm
/// FileOutputBuffer.
class m_fileBufferByteStream : public WritableBinaryStream
{
private:
   class Streamm_impl : public MutableBinaryByteStream
   {
   public:
      Streamm_impl(std::unique_ptr<FileOutputBuffer> buffer,
                   Endianness endian)
         : MutableBinaryByteStream(
              MutableArrayRef<uint8_t>(buffer->getBufferStart(),
                                       buffer->getBufferEnd()),
              endian),
           m_fileBuffer(std::move(buffer))
      {}

      Error commit() override {
         if (m_fileBuffer->commit())
            return make_error<BinaryStreamError>(
                     StreamErrorCode::filesystem_error);
         return Error::getSuccess();
      }

   private:
      std::unique_ptr<FileOutputBuffer> m_fileBuffer;
   };

public:
   m_fileBufferByteStream(std::unique_ptr<FileOutputBuffer> buffer,
                          Endianness endian)
      : m_impl(std::move(buffer), endian)
   {}

   Endianness getEndian() const override
   {
      return m_impl.getEndian();
   }

   Error readBytes(uint32_t offset, uint32_t size,
                   ArrayRef<uint8_t> &buffer) override
   {
      return m_impl.readBytes(offset, size, buffer);
   }

   Error readLongestContiguousChunk(uint32_t offset,
                                    ArrayRef<uint8_t> &buffer) override
   {
      return m_impl.readLongestContiguousChunk(offset, buffer);
   }

   uint32_t getLength() override
   {
      return m_impl.getLength();
   }

   Error writeBytes(uint32_t offset, ArrayRef<uint8_t> data) override
   {
      return m_impl.writeBytes(offset, data);
   }

   Error commit() override
   {
      return m_impl.commit();
   }

private:
   Streamm_impl m_impl;
};

} // basic
} // polar

#endif // POLARPHP_UTILS_BINARY_BYTE_STREAM_H
