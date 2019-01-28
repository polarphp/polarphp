// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/04.

#include "polarphp/utils/LineIterator.h"
#include "polarphp/utils/MemoryBuffer.h"

namespace polar {
namespace utils {

namespace {

bool is_at_line_end(const char *ptr)
{
   if (*ptr == '\n') {
      return true;
   }
   if (*ptr == '\r' && *(ptr + 1) == '\n') {
      return true;
   }
   return false;
}

bool skip_if_at_line_end(const char *&ptr)
{
   if (*ptr == '\n') {
      ++ptr;
      return true;
   }
   if (*ptr == '\r' && *(ptr + 1) == '\n') {
      ptr += 2;
      return true;
   }
   return false;
}

} // anonymous namespace

LineIterator::LineIterator(const MemoryBuffer &buffer, bool skipBlanks,
                           char commentMarker)
   : m_buffer(buffer.getBufferSize() ? &buffer : nullptr),
     m_commentMarker(commentMarker), m_skipBlanks(skipBlanks), m_lineNumber(1),
     m_currentLine(buffer.getBufferSize() ? buffer.getBufferStart() : nullptr,
                   0)
{
   // Ensure that if we are constructed on a non-empty memory buffer that it is
   // a null terminated buffer.
   if (buffer.getBufferSize()) {
      assert(buffer.getBufferEnd()[0] == '\0');
      // Make sure we don't skip a leading newline if we're keeping blanks
      if (skipBlanks || !is_at_line_end(buffer.getBufferStart())) {
         advance();
      }
   }
}

void LineIterator::advance()
{
   assert(m_buffer && "Cannot advance past the end!");

   const char *pos = m_currentLine.end();
   assert(pos == m_buffer->getBufferStart() || is_at_line_end(pos) || *pos == '\0');

   if (skip_if_at_line_end(pos)) {
      ++m_lineNumber;
   }
   if (!m_skipBlanks && is_at_line_end(pos)) {
      // Nothing to do for a blank line.
   } else if (m_commentMarker == '\0') {
      // If we're not stripping comments, this is simpler.
      while (skip_if_at_line_end(pos)) {
         ++m_lineNumber;
      }
   } else {
      // Skip comments and count line numbers, which is a bit more complex.
      for (;;) {
         if (is_at_line_end(pos) && !m_skipBlanks) {
            break;
         }
         if (*pos == m_commentMarker)
            do {
            ++pos;
         } while (*pos != '\0' && !is_at_line_end(pos));
         if (!skip_if_at_line_end(pos)) {
            break;
         }
         ++m_lineNumber;
      }
   }

   if (*pos == '\0') {
      // We've hit the end of the buffer, reset ourselves to the end state.
      m_buffer = nullptr;
      m_currentLine = StringRef();
      return;
   }

   // Measure the line.
   size_t length = 0;
   while (pos[length] != '\0' && !is_at_line_end(&pos[length])) {
      ++length;
   }
   m_currentLine = StringRef(pos, length);
}

} // utils
} // polar
