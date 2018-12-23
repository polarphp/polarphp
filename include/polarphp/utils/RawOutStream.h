// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/30.

#ifndef POLARPHP_UTILS_RAW_OUT_STREAM_H
#define POLARPHP_UTILS_RAW_OUT_STREAM_H

#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <system_error>

namespace polar {

namespace fs {
enum OpenFlags : unsigned;
enum FileAccess : unsigned;
enum OpenFlags : unsigned;
enum CreationDisposition : unsigned;
} // fs

namespace utils {

class FormatvObjectBase;
class FormatObjectBase;
class FormattedString;
class FormattedNumber;
class FormattedBytes;

using polar::basic::SmallVectorImpl;
using polar::basic::SmallVector;
using polar::basic::StringRef;
using polar::fs::OpenFlags;

/// This class implements an extremely fast bulk output stream that can *only*
/// output to a stream.  It does not support seeking, reopening, rewinding, line
/// buffered disciplines etc. It is a simple buffer that outputs
/// a chunk at a time.
class RawOutStream
{
private:
   /// The buffer is handled in such a way that the buffer is
   /// uninitialized, unbuffered, or out of space when m_outBufCur >=
   /// m_outBufEnd. Thus a single comparison suffices to determine if we
   /// need to take the slow path to write a single character.
   ///
   /// The buffer is in one of three states:
   ///  1. Unbuffered (m_bufferMode == Unbuffered)
   ///  1. Uninitialized (m_bufferMode != Unbuffered && m_outBufStart == 0).
   ///  2. Buffered (m_bufferMode != Unbuffered && m_outBufStart != 0 &&
   ///               m_outBufEnd - m_outBufStart >= 1).
   ///
   /// If buffered, then the RawOutStream owns the buffer if (m_bufferMode ==
   /// InternalBuffer); otherwise the buffer has been set via SetBuffer and is
   /// managed by the subclass.
   ///
   /// If a subclass installs an external buffer using SetBuffer then it can wait
   /// for a \see write_impl() call to handle the data which has been put into
   /// this buffer.
   char *m_outBufStart;
   char *m_outBufEnd;
   char *m_outBufCur;

   enum class BufferKind
   {
      Unbuffered = 0,
      InternalBuffer,
      ExternalBuffer
   } m_bufferMode;

public:
   // color order matches ANSI escape sequence, don't change
   enum class Colors
   {
      BLACK = 0,
      RED,
      GREEN,
      YELLOW,
      BLUE,
      MAGENTA,
      CYAN,
      WHITE,
      SAVEDCOLOR
   };

   explicit RawOutStream(bool unbuffered = false)
      : m_bufferMode(unbuffered ? BufferKind::Unbuffered : BufferKind::InternalBuffer)
   {
      // Start out ready to flush.
      m_outBufStart = m_outBufEnd = m_outBufCur = nullptr;
   }

   RawOutStream(const RawOutStream &) = delete;
   void operator=(const RawOutStream &) = delete;

   virtual ~RawOutStream();

   /// tell - Return the current offset with the file.
   uint64_t tell() const
   {
      return getCurrentPos() + getNumBytesInBuffer();
   }

   //===--------------------------------------------------------------------===//
   // Configuration Interface
   //===--------------------------------------------------------------------===//

   /// Set the stream to be buffered, with an automatically determined buffer
   /// size.
   void setBuffered();

   /// Set the stream to be buffered, using the specified buffer size.
   void setBufferSize(size_t Size)
   {
      flush();
      setBufferAndMode(new char[Size], Size, BufferKind::InternalBuffer);
   }

   size_t getBufferSize() const
   {
      // If we're supposed to be buffered but haven't actually gotten around
      // to allocating the buffer yet, return the value that would be used.
      if (m_bufferMode != BufferKind::Unbuffered && m_outBufStart == nullptr) {
         return getPreferredBufferSize();
      }

      // Otherwise just return the size of the allocated buffer.
      return m_outBufEnd - m_outBufStart;
   }

   /// Set the stream to be unbuffered. When unbuffered, the stream will flush
   /// after every write. This routine will also flush the buffer immediately
   /// when the stream is being set to unbuffered.
   void setUnbuffered()
   {
      flush();
      setBufferAndMode(nullptr, 0, BufferKind::Unbuffered);
   }

   size_t getNumBytesInBuffer() const
   {
      return m_outBufCur - m_outBufStart;
   }

   //===--------------------------------------------------------------------===//
   // Data Output Interface
   //===--------------------------------------------------------------------===//

   void flush()
   {
      if (m_outBufCur != m_outBufStart) {
         flushNonEmpty();
      }
   }

   RawOutStream &operator<<(char character)
   {
      if (m_outBufCur >= m_outBufEnd) {
         return write(character);
      }
      *m_outBufCur++ = character;
      return *this;
   }

   RawOutStream &operator<<(unsigned char character)
   {
      if (m_outBufCur >= m_outBufEnd) {
         return write(character);
      }
      *m_outBufCur++ = character;
      return *this;
   }

   RawOutStream &operator<<(signed char character)
   {
      if (m_outBufCur >= m_outBufEnd) {
         return write(character);
      }
      *m_outBufCur++ = character;
      return *this;
   }

   RawOutStream &operator<<(StringRef str)
   {
      // Inline fast path, particularly for strings with a known length.
      size_t size = str.getSize();

      // Make sure we can use the fast path.
      if (size > (size_t)(m_outBufEnd - m_outBufCur)) {
         return write(str.getData(), size);
      }

      if (size) {
         memcpy(m_outBufCur, str.getData(), size);
         m_outBufCur += size;
      }
      return *this;
   }

   RawOutStream &operator<<(const char *str)
   {
      // Inline fast path, particularly for constant strings where a sufficiently
      // smart compiler will simplify strlen.
      return this->operator<<(StringRef(str));
   }

   RawOutStream &operator<<(const std::string &str)
   {
      // Avoid the fast path, it would only increase code size for a marginal win.
      return write(str.data(), str.length());
   }

   RawOutStream &operator<<(const SmallVectorImpl<char> &str)
   {
      return write(str.getData(), str.getSize());
   }

   RawOutStream &operator<<(unsigned long num);
   RawOutStream &operator<<(long num);
   RawOutStream &operator<<(unsigned long long num);
   RawOutStream &operator<<(long long num);
   RawOutStream &operator<<(const void *ptr);

   RawOutStream &operator<<(unsigned int num)
   {
      return this->operator<<(static_cast<unsigned long>(num));
   }

   RawOutStream &operator<<(int num)
   {
      return this->operator<<(static_cast<long>(num));
   }

   RawOutStream &operator<<(double num);

   /// Output \p N in hexadecimal, without any prefix or padding.
   RawOutStream &writeHex(unsigned long long num);

   /// Output a formatted UUID with dash separators.
   using uuid_t = uint8_t[16];
   RawOutStream &writeUuid(const uuid_t uuid);

   /// Output \p Str, turning '\\', '\t', '\n', '"', and anything that doesn't
   /// satisfy std::isprint into an escape sequence.
   RawOutStream &writeEscaped(StringRef str, bool useHexEscapes = false);

   RawOutStream &write(unsigned char character);
   RawOutStream &write(const char *ptr, size_t size);

   // Formatted output, see the format() function in Support/Format.h.
   RawOutStream &operator<<(const FormatObjectBase &Fmt);

   // Formatted output, see the leftJustify() function in Support/Format.h.
   RawOutStream &operator<<(const FormattedString &);

   // Formatted output, see the formatHex() function in Support/Format.h.
   RawOutStream &operator<<(const FormattedNumber &);

   // Formatted output, see the formatv() function in Support/FormatVariadic.h.
   RawOutStream &operator<<(const FormatvObjectBase &);

   // Formatted output, see the format_bytes() function in Support/Format.h.
   RawOutStream &operator<<(const FormattedBytes &);

   /// indent - Insert 'numSpaces' spaces.
   RawOutStream &indent(unsigned numSpaces);

   /// write_zeros - Insert 'NumZeros' nulls.
   RawOutStream &writeZeros(unsigned numZeros);

   /// Changes the foreground color of text that will be output from this point
   /// forward.
   /// @param Color ANSI color to use, the special SAVEDCOLOR can be used to
   /// change only the bold attribute, and keep colors untouched
   /// @param Bold bold/brighter text, default false
   /// @param BG if true change the background, default: change foreground
   /// @returns itself so it can be used within << invocations
   virtual RawOutStream &changeColor(enum Colors color,
                                     bool bold = false,
                                     bool bg = false)
   {
      (void)color;
      (void)bold;
      (void)bg;
      return *this;
   }

   /// Resets the colors to terminal defaults. Call this when you are done
   /// outputting colored text, or before program exit.
   virtual RawOutStream &resetColor()
   {
      return *this;
   }

   /// Reverses the foreground and background colors.
   virtual RawOutStream &reverseColor()
   {
      return *this;
   }

   /// This function determines if this stream is connected to a "tty" or
   /// "console" window. That is, the output would be displayed to the user
   /// rather than being put on a pipe or stored in a file.
   virtual bool isDisplayed() const
   {
      return false;
   }

   /// This function determines if this stream is displayed and supports colors.
   virtual bool hasColors() const
   {
      return isDisplayed();
   }

   //===--------------------------------------------------------------------===//
   // Subclass Interface
   //===--------------------------------------------------------------------===//

private:
   /// The is the piece of the class that is implemented by subclasses.  This
   /// writes the \p Size bytes starting at
   /// \p Ptr to the underlying stream.
   ///
   /// This function is guaranteed to only be called at a point at which it is
   /// safe for the subclass to install a new buffer via SetBuffer.
   ///
   /// \param Ptr The start of the data to be written. For buffered streams this
   /// is guaranteed to be the start of the buffer.
   ///
   /// \param Size The number of bytes to be written.
   ///
   /// \invariant { Size > 0 }
   virtual void writeImpl(const char *ptr, size_t size) = 0;

   /// Return the current position within the stream, not counting the bytes
   /// currently in the buffer.
   virtual uint64_t getCurrentPos() const = 0;

protected:
   /// Use the provided buffer as the RawOutStream buffer. This is intended for
   /// use only by subclasses which can arrange for the output to go directly
   /// into the desired output buffer, instead of being copied on each flush.
   void setBuffer(char *bufferStart, size_t size)
   {
      setBufferAndMode(bufferStart, size, BufferKind::ExternalBuffer);
   }

   /// Return an efficient buffer size for the underlying output mechanism.
   virtual size_t getPreferredBufferSize() const;

   /// Return the beginning of the current stream buffer, or 0 if the stream is
   /// unbuffered.
   const char *getBufferStart() const
   {
      return m_outBufStart;
   }

   //===--------------------------------------------------------------------===//
   // Private Interface
   //===--------------------------------------------------------------------===//
private:
   /// Install the given buffer and mode.
   void setBufferAndMode(char *bufferStart, size_t size, BufferKind mode);

   /// Flush the current buffer, which is known to be non-empty. This outputs the
   /// currently buffered data and resets the buffer to empty.
   void flushNonEmpty();

   /// Copy data into the buffer. Size must not be greater than the number of
   /// unused bytes in the buffer.
   void copyToBuffer(const char *ptr, size_t size);

   virtual void anchor();
};

/// An abstract base class for streams implementations that also support a
/// pwrite operation. This is useful for code that can mostly stream out data,
/// but needs to patch in a header that needs to know the output size.
class RawPwriteStream : public RawOutStream
{
   virtual void pwriteImpl(const char *ptr, size_t size, uint64_t offset) = 0;
   void anchor() override;
public:
   explicit RawPwriteStream(bool unbuffered = false)
      : RawOutStream(unbuffered)
   {}

   void pwrite(const char *ptr, size_t size, uint64_t offset)
   {
#ifndef NDBEBUG
      uint64_t pos = tell();
      // /dev/null always reports a pos of 0, so we cannot perform this check
      // in that case.
      if (pos) {
         assert(size + offset <= pos && "We don't support extending the stream");
      }
#endif
      pwriteImpl(ptr, size, offset);
   }
};

//===----------------------------------------------------------------------===//
// File Output Streams
//===----------------------------------------------------------------------===//

/// A RawOutStream that writes to a file descriptor.
///
class RawFdOutStream : public RawPwriteStream
{
   int m_fd;
   bool m_shouldClose;
   bool m_supportsSeeking;

#ifdef _WIN32
   /// True if this fd refers to a Windows console device. Mintty and other
   /// terminal emulators are TTYs, but they are not consoles.
   bool m_isWindowsConsole = false;
#endif

   std::error_code m_errorCode;
   uint64_t m_pos;
   /// See RawOutStream::write_impl.
   void writeImpl(const char *ptr, size_t size) override;

   void pwriteImpl(const char *Ptr, size_t Size, uint64_t offset) override;

   /// Return the current position within the stream, not counting the bytes
   /// currently in the buffer.
   uint64_t getCurrentPos() const override
   {
      return m_pos;
   }

   /// Determine an efficient buffer size.
   size_t getPreferredBufferSize() const override;

   /// Set the flag indicating that an output error has been encountered.
   void errorDetected(std::error_code errorCode)
   {
      m_errorCode = errorCode;
   }

   void anchor() override;
public:
   /// Open the specified file for writing. If an error occurs, information
   /// about the error is put into EC, and the stream should be immediately
   /// destroyed;
   /// \p Flags allows optional flags to control how the file will be opened.
   ///
   /// As a special case, if Filename is "-", then the stream will use
   /// STDOUT_FILENO instead of opening a file. This will not close the stdout
   /// descriptor.
   RawFdOutStream(StringRef filename, std::error_code &errorCode);
   RawFdOutStream(StringRef filename, std::error_code &errorCode,
                  fs::CreationDisposition disp);
   RawFdOutStream(StringRef filename, std::error_code &errorCode,
                  fs::FileAccess access);
   RawFdOutStream(StringRef filename, std::error_code &errorCode,
                  OpenFlags flags);
   RawFdOutStream(StringRef filename, std::error_code &errorCode,
                  fs::CreationDisposition disp,
                  fs::FileAccess access,
                  fs::OpenFlags flags);

   /// FD is the file descriptor that this writes to.  If ShouldClose is true,
   /// this closes the file when the stream is destroyed. If FD is for stdout or
   /// stderr, it will not be closed.
   RawFdOutStream(int fd, bool shouldClose, bool unbuffered = false);

   ~RawFdOutStream() override;

   /// Manually flush the stream and close the file. Note that this does not call
   /// fsync.
   void close();

   bool supportsSeeking()
   {
      return m_supportsSeeking;
   }

   /// Flushes the stream and repositions the underlying file descriptor position
   /// to the offset specified from the beginning of the file.
   uint64_t seek(uint64_t off);

   RawOutStream &changeColor(enum Colors colors, bool bold=false,
                             bool bg=false) override;
   RawOutStream &resetColor() override;

   RawOutStream &reverseColor() override;

   bool isDisplayed() const override;

   bool hasColors() const override;

   std::error_code getErrorCode() const
   {
      return m_errorCode;
   }

   /// Return the value of the flag in this RawFdOutStream indicating whether an
   /// output error has been encountered.
   /// This doesn't implicitly flush any pending output.  Also, it doesn't
   /// guarantee to detect all errors unless the stream has been closed.
   bool hasError() const
   {
      return bool(m_errorCode);
   }

   /// Set the flag read by has_error() to false. If the error flag is set at the
   /// time when this RawOutStream's destructor is called, report_fatal_error is
   /// called to report the error. Use clear_error() after handling the error to
   /// avoid this behavior.
   ///
   ///   "Errors should never pass silently.
   ///    Unless explicitly silenced."
   ///      - from The Zen of Python, by Tim Peters
   ///
   void clearError()
   {
      m_errorCode = std::error_code();
   }
};

/// This returns a reference to a RawOutStream for standard output. Use it like:
/// outs() << "foo" << "bar";
RawOutStream &out_stream();

/// This returns a reference to a RawOutStream for standard error. Use it like:
/// errs() << "foo" << "bar";
RawOutStream &error_stream();

/// This returns a reference to a RawOutStream which simply discards output.
RawOutStream &null_stream();

//===----------------------------------------------------------------------===//
// Output Stream Adaptors
//===----------------------------------------------------------------------===//

/// A RawOutStream that writes to an std::string.  This is a simple adaptor
/// class. This class does not encounter output errors.
class RawStringOutStream : public RawOutStream
{
   std::string &m_outStream;

   /// See RawOutStream::write_impl.
   void writeImpl(const char *ptr, size_t size) override;

   /// Return the current position within the stream, not counting the bytes
   /// currently in the buffer.
   uint64_t getCurrentPos() const override
   {
      return m_outStream.size();
   }

public:
   explicit RawStringOutStream(std::string &out) : m_outStream(out)
   {}

   ~RawStringOutStream() override;

   /// Flushes the stream contents to the target string and returns  the string's
   /// reference.
   std::string& getStr()
   {
      flush();
      return m_outStream;
   }
};

/// A RawOutStream that writes to an SmallVector or SmallString.  This is a
/// simple adaptor class. This class does not encounter output errors.
/// RawSvectorOutStream operates without a buffer, delegating all memory
/// management to the SmallString. Thus the SmallString is always up-to-date,
/// may be used directly and there is no need to call flush().
class RawSvectorOutStream : public RawPwriteStream
{
   SmallVectorImpl<char> &m_outStream;

   /// See RawOutStream::write_impl.
   void writeImpl(const char *ptr, size_t size) override;

   void pwriteImpl(const char *ptr, size_t size, uint64_t offset) override;

   /// Return the current position within the stream.
   uint64_t getCurrentPos() const override;

public:
   /// Construct a new RawSvectorOutStream.
   ///
   /// \param O The vector to write to; this should generally have at least 128
   /// bytes free to avoid any extraneous memory overhead.
   explicit RawSvectorOutStream(SmallVectorImpl<char> &outStream)
      : m_outStream(outStream)
   {
      setUnbuffered();
   }

   ~RawSvectorOutStream() override = default;

   void flush() = delete;

   /// Return a StringRef for the vector contents.
   StringRef getStr()
   {
      return StringRef(m_outStream.getData(), m_outStream.getSize());
   }
};

/// A RawOutStream that discards all output.
class RawNullOutStream : public RawPwriteStream
{
   /// See RawOutStream::write_impl.
   void writeImpl(const char *ptr, size_t size) override;
   void pwriteImpl(const char *ptr, size_t size, uint64_t offset) override;

   /// Return the current position within the stream, not counting the bytes
   /// currently in the buffer.
   uint64_t getCurrentPos() const override;

public:
   explicit RawNullOutStream() = default;
   ~RawNullOutStream() override;
};

class BufferOstream : public RawSvectorOutStream
{
   RawOutStream &m_outStream;
   SmallVector<char, 0> m_buffer;

public:
   BufferOstream(RawOutStream &outStream)
      : RawSvectorOutStream(m_buffer), m_outStream(outStream)
   {}

   ~BufferOstream() override
   {
      m_outStream << getStr();
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_RAW_OUT_STREAM_H
