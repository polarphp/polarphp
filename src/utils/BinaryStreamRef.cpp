// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/02.

#include "polarphp/utils/BinaryStreamRef.h"
#include "polarphp/utils/BinaryByteStream.h"

#include <optional>

namespace polar {
namespace utils {

using polar::basic::make_array_ref;

namespace {

class ArrayRefImpl : public BinaryStream
{
public:
   ArrayRefImpl(ArrayRef<uint8_t> data, Endianness endian)
      : m_stream(data, endian)
   {}

   Endianness getEndian() const override
   {
      return m_stream.getEndian();
   }

   Error readBytes(uint32_t offset, uint32_t size,
                   ArrayRef<uint8_t> &buffer) override
   {
      return m_stream.readBytes(offset, size, buffer);
   }

   Error readLongestContiguousChunk(uint32_t offset,
                                    ArrayRef<uint8_t> &buffer) override
   {
      return m_stream.readLongestContiguousChunk(offset, buffer);
   }

   uint32_t getLength() override
   {
      return m_stream.getLength();
   }

private:
   BinaryByteStream m_stream;
};

class MutableArrayRefImpl : public WritableBinaryStream
{
public:
   MutableArrayRefImpl(MutableArrayRef<uint8_t> data, Endianness endian)
      : m_stream(data, endian)
   {}

   // Inherited via WritableBinaryStream
   Endianness getEndian() const override
   {
      return m_stream.getEndian();
   }

   Error readBytes(uint32_t offset, uint32_t size,
                   ArrayRef<uint8_t> &buffer) override
   {
      return m_stream.readBytes(offset, size, buffer);
   }

   Error readLongestContiguousChunk(uint32_t offset,
                                    ArrayRef<uint8_t> &buffer) override
   {
      return m_stream.readLongestContiguousChunk(offset, buffer);
   }

   uint32_t getLength() override
   {
      return m_stream.getLength();
   }

   Error writeBytes(uint32_t offset, ArrayRef<uint8_t> data) override
   {
      return m_stream.writeBytes(offset, data);
   }

   Error commit() override
   {
      return m_stream.commit();
   }

private:
   MutableBinaryByteStream m_stream;
};
} // anonymous namespace

BinaryStreamRef::BinaryStreamRef(BinaryStream &stream)
   : BinaryStreamRefBase(stream)
{}

BinaryStreamRef::BinaryStreamRef(BinaryStream &stream, uint32_t offset,
                                 std::optional<uint32_t> length)
   : BinaryStreamRefBase(stream, offset, length)
{}

BinaryStreamRef::BinaryStreamRef(ArrayRef<uint8_t> data, Endianness endian)
   : BinaryStreamRefBase(std::make_shared<ArrayRefImpl>(data, endian), 0,
                         data.getSize())
{}

BinaryStreamRef::BinaryStreamRef(StringRef data, Endianness endian)
   : BinaryStreamRef(make_array_ref(data.getBytesBegin(), data.getBytesEnd()),
                     endian)
{}

Error BinaryStreamRef::readBytes(uint32_t offset, uint32_t size,
                                 ArrayRef<uint8_t> &buffer) const
{
   if (auto errorCode = checkOffsetForRead(offset, size)) {
      return errorCode;
   }
   return m_borrowedImpl->readBytes(m_viewOffset + offset, size, buffer);
}

Error BinaryStreamRef::readLongestContiguousChunk(
      uint32_t offset, ArrayRef<uint8_t> &buffer) const
{
   if (auto errorCode = checkOffsetForRead(offset, 1)) {
      return errorCode;
   }
   if (auto errorCode =
       m_borrowedImpl->readLongestContiguousChunk(m_viewOffset + offset, buffer)) {
      return errorCode;
   }

   // This StreamRef might refer to a smaller window over a larger stream.  In
   // that case we will have read out more bytes than we should return, because
   // we should not read past the end of the current view.
   uint32_t maxLength = getLength() - offset;
   if (buffer.getSize() > maxLength) {
      buffer = buffer.slice(0, maxLength);
   }
   return Error::getSuccess();
}

WritableBinaryStreamRef::WritableBinaryStreamRef(WritableBinaryStream &stream)
   : BinaryStreamRefBase(stream)
{}

WritableBinaryStreamRef::WritableBinaryStreamRef(WritableBinaryStream &stream,
                                                 uint32_t offset,
                                                 std::optional<uint32_t> length)
   : BinaryStreamRefBase(stream, offset, length)
{}

WritableBinaryStreamRef::WritableBinaryStreamRef(MutableArrayRef<uint8_t> data,
                                                 Endianness endian)
   : BinaryStreamRefBase(std::make_shared<MutableArrayRefImpl>(data, endian),
                         0, data.getSize())
{}


Error WritableBinaryStreamRef::writeBytes(uint32_t offset,
                                          ArrayRef<uint8_t> data) const
{
   if (auto errorCode = checkOffsetForWrite(offset, data.getSize())) {
      return errorCode;
   }
   return m_borrowedImpl->writeBytes(m_viewOffset + offset, data);
}

WritableBinaryStreamRef::operator BinaryStreamRef() const
{
   return BinaryStreamRef(*m_borrowedImpl, m_viewOffset, m_length);
}

/// \brief For buffered streams, commits changes to the backing store.
Error WritableBinaryStreamRef::commit()
{
   return m_borrowedImpl->commit();
}

} // utils
} // polar
