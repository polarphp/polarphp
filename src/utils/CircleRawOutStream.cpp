// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/18.

//===----------------------------------------------------------------------===//
//
// This implements support for circular buffered streams.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/CircularRawOutStream.h"
#include <algorithm>

namespace polar {
namespace utils {

void CircularRawOutStream::writeImpl(const char *ptr, size_t size)
{
   if (m_bufferSize == 0) {
      m_theStream->write(ptr, size);
      return;
   }
   // Write into the buffer, wrapping if necessary.
   while (size != 0) {
      unsigned bytes =
            std::min(unsigned(size), unsigned(m_bufferSize - (m_cur - m_bufferArray)));
      memcpy(m_cur, ptr, bytes);
      size -= bytes;
      m_cur += bytes;
      if (m_cur == m_bufferArray + m_bufferSize) {
         // Reset the output pointer to the start of the buffer.
         m_cur = m_bufferArray;
         m_filled = true;
      }
   }
}

void CircularRawOutStream::flushBufferWithBanner()
{
   if (m_bufferSize != 0) {
      // Write out the buffer
      m_theStream->write(m_banner, std::strlen(m_banner));
      flushBuffer();
   }
}

} // utils
} // polar
