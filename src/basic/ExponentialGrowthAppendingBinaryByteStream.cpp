//===-------- ExponentialGrowthAppendingBinaryByteStream.cpp --------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/08.

#include "polarphp/basic/ExponentialGrowthAppendingBinaryByteStream.h"

namespace polar {

using llvm::Error;

Error ExponentialGrowthAppendingBinaryByteStream::readBytes(
      uint32_t offset, uint32_t size, ArrayRef<uint8_t> &buffer)
{
   if (auto error = checkOffsetForRead(offset, size)) {
      return error;
   }

   buffer = ArrayRef<uint8_t>(m_data.data() + offset, size);
   return Error::success();
}

Error ExponentialGrowthAppendingBinaryByteStream::readLongestContiguousChunk(
      uint32_t offset, ArrayRef<uint8_t> &buffer)
{
   if (auto error = checkOffsetForRead(offset, 0)) {
      return error;
   }

   buffer = ArrayRef<uint8_t>(m_data.data() + offset, m_data.size() - offset);
   return Error::success();
}

void ExponentialGrowthAppendingBinaryByteStream::reserve(size_t size)
{
   m_data.reserve(size);
}

Error ExponentialGrowthAppendingBinaryByteStream::writeBytes(
      uint32_t offset, ArrayRef<uint8_t> buffer)
{
   if (buffer.empty()) {
       return Error::success();
   }
   if (auto error = checkOffsetForWrite(offset, buffer.size())) {
      return error;
   }
   // Resize the internal buffer if needed.
   uint32_t requiredSize = offset + buffer.size();
   if (requiredSize > m_data.size()) {
      m_data.resize(requiredSize);
   }
   ::memcpy(m_data.data() + offset, buffer.data(), buffer.size());
   return Error::success();
}

} // polar
