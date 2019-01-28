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

#include "polarphp/utils/BinaryStreamWriter.h"
#include "polarphp/utils/BinaryStreamError.h"
#include "polarphp/utils/BinaryStreamReader.h"
#include "polarphp/utils/BinaryStreamRef.h"

namespace polar {
namespace utils {

using polar::basic::arrayref_from_stringref;

BinaryStreamWriter::BinaryStreamWriter(WritableBinaryStreamRef ref)
   : m_stream(ref)
{}

BinaryStreamWriter::BinaryStreamWriter(WritableBinaryStream &stream)
   : m_stream(stream)
{}

BinaryStreamWriter::BinaryStreamWriter(MutableArrayRef<uint8_t> data,
                                       Endianness endian)
   : m_stream(data, endian)
{}

Error BinaryStreamWriter::writeBytes(ArrayRef<uint8_t> buffer)
{
   if (auto errorCode = m_stream.writeBytes(m_offset, buffer)) {
      return errorCode;
   }
   m_offset += buffer.getSize();
   return Error::getSuccess();
}

Error BinaryStreamWriter::writeCString(StringRef str)
{
   if (auto errorCode = writeFixedString(str)) {
      return errorCode;
   }
   if (auto errorCode = writeObject('\0')) {
      return errorCode;
   }
   return Error::getSuccess();
}

Error BinaryStreamWriter::writeFixedString(StringRef str)
{
   return writeBytes(arrayref_from_stringref(str));
}

Error BinaryStreamWriter::writeStreamRef(BinaryStreamRef ref)
{
   return writeStreamRef(ref, ref.getLength());
}

Error BinaryStreamWriter::writeStreamRef(BinaryStreamRef ref, uint32_t length)
{
   BinaryStreamReader srcReader(ref.slice(0, length));
   // This is a bit tricky.  If we just call readBytes, we are requiring that it
   // return us the entire stream as a contiguous buffer.  There is no guarantee
   // this can be satisfied by returning a reference straight from the buffer, as
   // an implementation may not store all data in a single contiguous buffer.  So
   // we iterate over each contiguous chunk, writing each one in succession.
   while (srcReader.getBytesRemaining() > 0) {
      ArrayRef<uint8_t> chunk;
      if (auto errorCode = srcReader.readLongestContiguousChunk(chunk)) {
         return errorCode;
      }
      if (auto errorCode = writeBytes(chunk)) {
         return errorCode;
      }
   }
   return Error::getSuccess();
}

std::pair<BinaryStreamWriter, BinaryStreamWriter>
BinaryStreamWriter::split(uint32_t offset) const
{
   assert(getLength() >= offset);
   WritableBinaryStreamRef first = m_stream.dropFront(m_offset);
   WritableBinaryStreamRef second = first.dropFront(offset);
   first = first.keepFront(offset);
   BinaryStreamWriter w1{first};
   BinaryStreamWriter w2{second};
   return std::make_pair(w1, w2);
}

Error BinaryStreamWriter::padToAlignment(uint32_t align)
{
   uint32_t newOffset = align_to(m_offset, align);
   if (newOffset > getLength()) {
      return make_error<BinaryStreamError>(StreamErrorCode::stream_too_short);
   }
   while (m_offset < newOffset) {
      if (auto errorCode = writeInteger('\0')) {
         return errorCode;
      }
   }
   return Error::getSuccess();
}

} // utils
} // polar
