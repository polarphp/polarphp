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

#ifndef POLARPHP_UTILS_FORMATTED_STREAM_H
#define POLARPHP_UTILS_FORMATTED_STREAM_H

#include "polarphp/utils/RawOutStream.h"
#include <utility>

namespace polar {
namespace utils {

/// FormattedRawOutStream - A RawOutStream that wraps another one and keeps track
/// of line and column position, allowing padding out to specific column
/// boundaries and querying the number of lines written to the stream.
///
class FormattedRawOutStream : public RawOutStream
{
   /// m_theStream - The real stream we output to. We set it to be
   /// unbuffered, since we're already doing our own buffering.
   ///
   RawOutStream *m_theStream;

   /// Position - The current output column and line of the data that's
   /// been flushed and the portion of the buffer that's been
   /// scanned.  The line and column scheme is zero-based.
   ///
   std::pair<unsigned, unsigned> m_position;

   /// Scanned - This points to one past the last character in the
   /// buffer we've scanned.
   ///
   const char *m_scanned;

   void writeImpl(const char *ptr, size_t size) override;

   /// current_pos - Return the current position within the stream,
   /// not counting the bytes currently in the buffer.
   uint64_t getCurrentPos() const override
   {
      // Our current position in the stream is all the contents which have been
      // written to the underlying stream (*not* the current position of the
      // underlying stream).
      return m_theStream->tell();
   }

   /// ComputePosition - Examine the given output buffer and figure out the new
   /// position after output.
   ///
   void computePosition(const char *ptr, size_t size);

   void setStream(RawOutStream &stream)
   {
      releaseStream();
      m_theStream = &stream;

      // This FormattedRawOutStream inherits from RawOutStream, so it'll do its
      // own buffering, and it doesn't need or want m_theStream to do another
      // layer of buffering underneath. Resize the buffer to what m_theStream
      // had been using, and tell m_theStream not to do its own buffering.
      if (size_t bufferSize = m_theStream->getBufferSize()) {
         setBufferSize(bufferSize);
      } else {
         setUnbuffered();
      }
      m_theStream->setUnbuffered();
      m_scanned = nullptr;
   }

public:
   /// FormattedRawOutStream - Open the specified file for
   /// writing. If an error occurs, information about the error is
   /// put into ErrorInfo, and the stream should be immediately
   /// destroyed; the string will be empty if no error occurred.
   ///
   /// As a side effect, the given Stream is set to be Unbuffered.
   /// This is because FormattedRawOutStream does its own buffering,
   /// so it doesn't want another layer of buffering to be happening
   /// underneath it.
   ///
   FormattedRawOutStream(RawOutStream &stream)
      : m_theStream(nullptr),
        m_position(0, 0)
   {
      setStream(stream);
   }

   explicit FormattedRawOutStream()
      : m_theStream(nullptr), m_position(0, 0)
   {
      m_scanned = nullptr;
   }

   ~FormattedRawOutStream() override
   {
      flush();
      releaseStream();
   }

   /// PadToColumn - Align the output to some column number.  If the current
   /// column is already equal to or more than NewCol, PadToColumn inserts one
   /// space.
   ///
   /// \param NewCol - The column to move to.
   FormattedRawOutStream &padToColumn(unsigned newCol);

   /// getColumn - Return the column number
   unsigned getColumn()
   {
      return m_position.first;
   }

   /// getLine - Return the line number
   unsigned getLine()
   {
      return m_position.second;
   }

   RawOutStream &resetColor() override
   {
      m_theStream->resetColor();
      return *this;
   }

   RawOutStream &reverseColor() override
   {
      m_theStream->reverseColor();
      return *this;
   }

   RawOutStream &changeColor(enum Colors color, bool bold, bool bg) override
   {
      m_theStream->changeColor(color, bold, bg);
      return *this;
   }

   bool isDisplayed() const override
   {
      return m_theStream->isDisplayed();
   }

private:
   void releaseStream()
   {
      // Transfer the buffer settings from this RawOutStream back to the underlying
      // stream.
      if (!m_theStream) {
         return;
      }
      if (size_t bufferSize = getBufferSize()) {
         m_theStream->setBufferSize(bufferSize);
      } else {
         m_theStream->setUnbuffered();
      }
   }
};

/// formatted_out_stream() - This returns a reference to a FormattedRawOutStream for
/// standard output.  Use it like: formatted_out_stream() << "foo" << "bar";
FormattedRawOutStream &formatted_out_stream();

/// formatted_error_stream() - This returns a reference to a FormattedRawOutStream for
/// standard error.  Use it like: formatted_error_stream() << "foo" << "bar";
FormattedRawOutStream &formatted_error_stream();

/// formatted_debug_stream() - This returns a reference to a FormattedRawOutStream for
/// debug output.  Use it like: formatted_debug_stream() << "foo" << "bar";
FormattedRawOutStream &formatted_debug_stream();

} // utils
} // polar

#endif // POLARPHP_UTILS_FORMATTED_STREAM_H
