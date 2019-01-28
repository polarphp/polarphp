// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/23.

#include "polarphp/vm/utils/StreamBuffer.h"
#include "polarphp/vm/internal/DepsZendVmHeaders.h"

namespace polar {
namespace vmapi {

StreamBuffer::StreamBuffer(int error)
   : m_error(error)
{
   setp(m_buffer.begin(), m_buffer.end());
}

int StreamBuffer::overflow(int c)
{
   std::char_traits<char>::int_type eof = std::char_traits<char>::eof();
   if (m_error) {
      return c;
   }
   if (c == eof) {
      return sync(), eof;
   }
   *pptr() = c;
   pbump(1);
   return sync() == -1 ? eof : c;
}

int StreamBuffer::sync()
{
   size_t size = pptr() - pbase();
   if (m_error) {
      zend_error(m_error, "%.*s", static_cast<int>(size), pbase());
   } else {
      zend_write(pbase(), size);
   }
   // reset the buffer
   pbump(-size);
   return 0;
}

StreamBuffer::~StreamBuffer()
{}

} // vmapi
} // polar
