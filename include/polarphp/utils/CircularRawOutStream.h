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

#ifndef POLARPHP_UTILS_CIRCULAR_RAW_OUT_STREAM_H
#define POLARPHP_UTILS_CIRCULAR_RAW_OUT_STREAM_H

#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace utils {

/// CircularRawOutStream - A RawOutStream which *can* save its data
/// to a circular buffer, or can pass it through directly to an
/// underlying stream if specified with a buffer of zero.
///
class CircularRawOutStream : public RawOutStream
{
public:
   /// TAKE_OWNERSHIP - Tell this stream that it owns the underlying
   /// stream and is responsible for cleanup, memory management
   /// issues, etc.
   ///
   static const bool TAKE_OWNERSHIP = true;

   /// REFERENCE_ONLY - Tell this stream it should not manage the
   /// held stream.
   ///
   static const bool REFERENCE_ONLY = false;

private:
   /// m_theStream - The real stream we output to. We set it to be
   /// unbuffered, since we're already doing our own buffering.
   ///
   RawOutStream *m_theStream;

   /// m_ownsStream - Are we responsible for managing the underlying
   /// stream?
   ///
   bool m_ownsStream;

   /// m_bufferSize - The size of the buffer in bytes.
   ///
   size_t m_bufferSize;

   /// m_bufferArray - The actual buffer storage.
   ///
   char *m_bufferArray;

   /// m_cur - Pointer to the current output point in m_bufferArray.
   ///
   char *m_cur;

   /// m_filled - Indicate whether the buffer has been completely
   /// filled.  This helps avoid garbage output.
   ///
   bool m_filled;

   /// Banner - A pointer to a banner to print before dumping the
   /// log.
   ///
   const char *m_banner;

   /// flushBuffer - Dump the contents of the buffer to Stream.
   ///
   void flushBuffer()
   {
      if (m_filled) {
         // Write the older portion of the buffer.
         m_theStream->write(m_cur, m_bufferArray + m_bufferSize - m_cur);
      }
      // Write the newer portion of the buffer.
      m_theStream->write(m_bufferArray, m_cur - m_bufferArray);
      m_cur = m_bufferArray;
      m_filled = false;
   }

   void writeImpl(const char *ptr, size_t size) override;

   /// current_pos - Return the current position within the stream,
   /// not counting the bytes currently in the buffer.
   ///
   uint64_t getCurrentPos() const override
   {
      // This has the same effect as calling m_theStream.current_pos(),
      // but that interface is private.
      return m_theStream->tell() - m_theStream->getNumBytesInBuffer();
   }

public:
   /// CircularRawOutStream - Construct an optionally
   /// circular-buffered stream, handing it an underlying stream to
   /// do the "real" output.
   ///
   /// As a side effect, if BuffSize is nonzero, the given Stream is
   /// set to be Unbuffered.  This is because CircularRawOutStream
   /// does its own buffering, so it doesn't want another layer of
   /// buffering to be happening underneath it.
   ///
   /// "Owns" tells the CircularRawOutStream whether it is
   /// responsible for managing the held stream, doing memory
   /// management of it, etc.
   ///
   CircularRawOutStream(RawOutStream &stream, const char *header,
                        size_t buffSize = 0, bool owns = REFERENCE_ONLY)
      : RawOutStream(/*unbuffered*/ true), m_theStream(nullptr),
        m_ownsStream(owns), m_bufferSize(buffSize), m_bufferArray(nullptr),
        m_filled(false), m_banner(header)
   {
      if (m_bufferSize != 0) {
         m_bufferArray = new char[m_bufferSize];
      }
      m_cur = m_bufferArray;
      setStream(stream, owns);
   }

   ~CircularRawOutStream() override
   {
      flush();
      flushBufferWithBanner();
      releaseStream();
      delete[] m_bufferArray;
   }

   /// setStream - Tell the CircularRawOutStream to output a
   /// different stream.  "Owns" tells CircularRawOutStream whether
   /// it should take responsibility for managing the underlying
   /// stream.
   ///
   void setStream(RawOutStream &stream, bool owns = REFERENCE_ONLY)
   {
      releaseStream();
      m_theStream = &stream;
      m_ownsStream = owns;
   }

   /// flushBufferWithBanner - Force output of the buffer along with
   /// a small header.
   ///
   void flushBufferWithBanner();

private:
   /// releaseStream - Delete the held stream if needed. Otherwise,
   /// transfer the buffer settings from this CircularRawOutStream
   /// back to the underlying stream.
   ///
   void releaseStream()
   {
      if (!m_theStream) {
         return;
      }
      if (m_ownsStream) {
         delete m_theStream;
      }
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_CIRCULAR_RAW_OUT_STREAM_H
