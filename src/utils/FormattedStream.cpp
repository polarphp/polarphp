// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/04.

#include "polarphp/utils/FormattedStream.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/RawOutStream.h"
#include <algorithm>

namespace polar {
namespace utils {

/// update_position - Examine the given char sequence and figure out which
/// column we end up in after output, and how many line breaks are contained.
///
namespace {

void update_position(std::pair<unsigned, unsigned> &position, const char *ptr, size_t size)
{
   unsigned &column = position.first;
   unsigned &line = position.second;

   // Keep track of the current column and line by scanning the string for
   // special characters
   for (const char *end = ptr + size; ptr != end; ++ptr) {
      ++column;
      switch (*ptr) {
      case '\n':
         line += 1;
         POLAR_FALLTHROUGH;
      case '\r':
         column = 0;
         break;
      case '\t':
         // Assumes tab stop = 8 characters.
         column += (8 - (column & 0x7)) & 0x7;
         break;
      }
   }
}

} // anonymous namespace

/// ComputePosition - Examine the current output and update line and column
/// counts.
void FormattedRawOutStream::computePosition(const char *ptr, size_t size)
{
   // If our previous scan pointer is inside the buffer, assume we already
   // scanned those bytes. This depends on RawOutStream to not change our buffer
   // in unexpected ways.
   if (ptr <= m_scanned && m_scanned <= ptr + size) {
      // Scan all characters added since our last scan to determine the new
      // column.
      update_position(m_position, m_scanned, size - (m_scanned - ptr));
   } else {
      update_position(m_position, ptr, size);
   }
   // Update the scanning pointer.
   m_scanned = ptr + size;
}

/// PadToColumn - Align the output to some column number.
///
/// \param newCol - The column to move to.
///
FormattedRawOutStream &FormattedRawOutStream::padToColumn(unsigned newCol)
{
   // Figure out what's in the buffer and add it to the column count.
   computePosition(getBufferStart(), getNumBytesInBuffer());
   // Output spaces until we reach the desired column.
   indent(std::max(int(newCol - getColumn()), 1));
   return *this;
}

void FormattedRawOutStream::writeImpl(const char *ptr, size_t size)
{
   // Figure out what's in the buffer and add it to the column count.
   computePosition(ptr, size);
   // Write the data to the underlying stream (which is unbuffered, so
   // the data will be immediately written out).
   m_theStream->write(ptr, size);

   // Reset the scanning pointer.
   m_scanned = nullptr;
}

/// fouts() - This returns a reference to a FormattedRawOutStream for
/// standard output.  Use it like: fouts() << "foo" << "bar";
FormattedRawOutStream &formatted_out_stream()
{
   static FormattedRawOutStream stream(out_stream());
   return stream;
}

/// ferrs() - This returns a reference to a FormattedRawOutStream for
/// standard error.  Use it like: ferrs() << "foo" << "bar";
FormattedRawOutStream &formatted_error_stream()
{
   static FormattedRawOutStream stream(error_stream());
   return stream;
}

/// fdbgs() - This returns a reference to a FormattedRawOutStream for
/// the debug stream.  Use it like: fdbgs() << "foo" << "bar";
FormattedRawOutStream &formatted_debug_stream()
{
   static FormattedRawOutStream stream(debug_stream());
   return stream;
}

} // utils
} // polar
