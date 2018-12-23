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

#include "polarphp/utils/BinaryStreamReader.h"
#include "polarphp/utils/BinaryStreamError.h"
#include "polarphp/utils/BinaryStreamRef.h"

namespace polar {
namespace utils {

BinaryStreamReader::BinaryStreamReader(BinaryStreamRef ref)
   : m_stream(ref)
{}

BinaryStreamReader::BinaryStreamReader(BinaryStream &stream)
   : m_stream(stream)
{}

BinaryStreamReader::BinaryStreamReader(ArrayRef<uint8_t> data,
                                       Endianness endian)
   : m_stream(data, endian)
{}

BinaryStreamReader::BinaryStreamReader(StringRef data, Endianness endian)
   : m_stream(data, endian)
{}

Error BinaryStreamReader::readLongestContiguousChunk(
      ArrayRef<uint8_t> &buffer)
{
   if (auto errorCode = m_stream.readLongestContiguousChunk(m_offset, buffer)) {
      return errorCode;
   }
   m_offset += buffer.getSize();
   return Error::getSuccess();
}

Error BinaryStreamReader::readBytes(ArrayRef<uint8_t> &buffer, uint32_t size)
{
   if (auto errorCode = m_stream.readBytes(m_offset, size, buffer)) {
      return errorCode;
   }
   m_offset += size;
   return Error::getSuccess();
}

Error BinaryStreamReader::readCString(StringRef &dest)
{
   uint32_t originalOffset = getOffset();
   uint32_t foundOffset = 0;
   while (true) {
      uint32_t thisOffset = getOffset();
      ArrayRef<uint8_t> buffer;
      if (auto errorCode = readLongestContiguousChunk(buffer)) {
         return errorCode;
      }
      StringRef str(reinterpret_cast<const char *>(buffer.begin()), buffer.getSize());
      size_t pos = str.findFirstOf('\0');
      if (POLAR_LIKELY(pos != StringRef::npos)) {
         foundOffset = pos + thisOffset;
         break;
      }
   }
   assert(foundOffset >= originalOffset);
   setOffset(originalOffset);
   size_t length = foundOffset - originalOffset;
   if (auto errorCode = readFixedString(dest, length)) {
      return errorCode;
   }
   // Now set the offset back to after the null terminator.
   setOffset(foundOffset + 1);
   return Error::getSuccess();
}

Error BinaryStreamReader::readWideString(ArrayRef<Utf16> &dest)
{
   uint32_t length = 0;
   uint32_t originalOffset = getOffset();
   const Utf16 *c;
   while (true) {
      if (auto errorCode = readObject(c)) {
         return errorCode;
      }
      if (*c == 0x0000) {
         break;
      }
      ++length;
   }
   uint32_t newOffset = getOffset();
   setOffset(originalOffset);
   if (auto errorCode = readArray(dest, length)) {
      return errorCode;
   }
   setOffset(newOffset);
   return Error::getSuccess();
}

Error BinaryStreamReader::readFixedString(StringRef &dest, uint32_t length)
{
   ArrayRef<uint8_t> bytes;
   if (auto errorCode = readBytes(bytes, length)) {
      return errorCode;
   }
   dest = StringRef(reinterpret_cast<const char *>(bytes.begin()), bytes.getSize());
   return Error::getSuccess();
}

Error BinaryStreamReader::readStreamRef(BinaryStreamRef &ref)
{
   return readStreamRef(ref, getBytesRemaining());
}

Error BinaryStreamReader::readStreamRef(BinaryStreamRef &ref, uint32_t length)
{
   if (getBytesRemaining() < length) {
      return make_error<BinaryStreamError>(StreamErrorCode::stream_too_short);
   }
   ref = m_stream.slice(m_offset, length);
   m_offset += length;
   return Error::getSuccess();
}

Error BinaryStreamReader::readSubstream(BinarySubstreamRef &m_stream,
                                        uint32_t size)
{
   m_stream.m_offset = getOffset();
   return readStreamRef(m_stream.m_streamData, size);
}

Error BinaryStreamReader::skip(uint32_t amount)
{
   if (amount > getBytesRemaining()) {
      return make_error<BinaryStreamError>(StreamErrorCode::stream_too_short);
   }
   m_offset += amount;
   return Error::getSuccess();
}

Error BinaryStreamReader::padToAlignment(uint32_t align)
{
   uint32_t newOffset = align_to(m_offset, align);
   return skip(newOffset - m_offset);
}

uint8_t BinaryStreamReader::peek() const
{
   ArrayRef<uint8_t> buffer;
   auto errorCode = m_stream.readBytes(m_offset, 1, buffer);
   assert(!errorCode && "Cannot peek an empty buffer!");
   polar::utils::consume_error(std::move(errorCode));
   return buffer[0];
}

std::pair<BinaryStreamReader, BinaryStreamReader>
BinaryStreamReader::split(uint32_t offset) const
{
   assert(getLength() >= offset);
   BinaryStreamRef first = m_stream.dropFront(offset);
   BinaryStreamRef second = first.dropFront(offset);
   first = first.keepFront(offset);
   BinaryStreamReader w1{first};
   BinaryStreamReader w2{second};
   return std::make_pair(w1, w2);
}

} // utils
} // polar
