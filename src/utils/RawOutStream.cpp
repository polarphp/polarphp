// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/07.

//===----------------------------------------------------------------------===//
//
// This implements support for bulk buffered stream output.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/RawOutStream.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/global/Global.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Format.h"
#include "polarphp/utils/FormatVariadic.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/utils/NativeFormatting.h"
#include "polarphp/utils/Process.h"
#include "polarphp/utils/Program.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <iterator>
#include <sys/stat.h>
#include <system_error>

// <fcntl.h> may provide O_BINARY.
#if defined(POLAR_HAVE_FCNTL_H)
# include <fcntl.h>
#endif

#if defined(POLAR_HAVE_UNISTD_H)
# include <unistd.h>
#endif

#if defined(__CYGWIN__)
#include <io.h>
#endif

#if defined(_MSC_VER)
#include <io.h>
#ifndef STDIN_FILENO
# define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
# define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
# define STDERR_FILENO 2
#endif
#endif

#ifdef _WIN32
#include "polarphp/utils/ConvertUtf.h"
#include "Windows/WindowsSupport.h"
#endif

namespace polar {
namespace utils {

using polar::basic::is_print;

using polar::sys::change_stdout_to_binary;
using polar::sys::Process;

RawOutStream::~RawOutStream()
{
   // RawOutStream's subclasses should take care to flush the buffer
   // in their destructors.
   assert(m_outBufCur == m_outBufStart &&
          "RawOutStream destructor called with non-empty buffer!");

   if (m_bufferMode == BufferKind::InternalBuffer) {
      delete [] m_outBufStart;
   }
}

size_t RawOutStream::getPreferredBufferSize() const
{
   // BUFSIZ is intended to be a reasonable default.
   return BUFSIZ;
}

void RawOutStream::setBuffered()
{
   // Ask the subclass to determine an appropriate buffer size.
   if (size_t size = getPreferredBufferSize()) {
      setBufferSize(size);
   } else {
      // It may return 0, meaning this stream should be unbuffered.
      setUnbuffered();
   }
}

void RawOutStream::setBufferAndMode(char *bufferStart, size_t size,
                                    BufferKind mode)
{
   assert(((mode == BufferKind::Unbuffered && !bufferStart && size == 0) ||
           (mode != BufferKind::Unbuffered && bufferStart && size != 0)) &&
          "stream must be unbuffered or have at least one byte");
   // Make sure the current buffer is free of content (we can't flush here; the
   // child buffer management logic will be in write_impl).
   assert(getNumBytesInBuffer() == 0 && "Current buffer is non-empty!");

   if (m_bufferMode == BufferKind::InternalBuffer) {
      delete [] m_outBufStart;
   }
   m_outBufStart = bufferStart;
   m_outBufEnd = m_outBufStart + size;
   m_outBufCur = m_outBufStart;
   m_bufferMode = mode;
   assert(m_outBufStart <= m_outBufEnd && "Invalid size!");
}

RawOutStream &RawOutStream::operator<<(unsigned long N)
{
   write_integer(*this, static_cast<uint64_t>(N), 0, IntegerStyle::Integer);
   return *this;
}

RawOutStream &RawOutStream::operator<<(long N)
{
   write_integer(*this, static_cast<int64_t>(N), 0, IntegerStyle::Integer);
   return *this;
}

RawOutStream &RawOutStream::operator<<(unsigned long long N)
{
   write_integer(*this, static_cast<uint64_t>(N), 0, IntegerStyle::Integer);
   return *this;
}

RawOutStream &RawOutStream::operator<<(long long N)
{
   write_integer(*this, static_cast<int64_t>(N), 0, IntegerStyle::Integer);
   return *this;
}

RawOutStream &RawOutStream::writeHex(unsigned long long N)
{
   write_hex(*this, N, HexPrintStyle::Lower);
   return *this;
}

RawOutStream &RawOutStream::writeUuid(const uuid_t uuid)
{
   for (int index = 0; index < 16; ++index) {
      *this << format("%02" PRIX32, uuid[index]);
      if (index == 3 || index == 5 || index == 7 || index == 9){
         *this << "-";
      }
   }
   return *this;
}

RawOutStream &RawOutStream::writeEscaped(StringRef str,
                                         bool useHexEscapes)
{
   for (unsigned char c : str) {
      switch (c) {
      case '\\':
         *this << '\\' << '\\';
         break;
      case '\t':
         *this << '\\' << 't';
         break;
      case '\n':
         *this << '\\' << 'n';
         break;
      case '"':
         *this << '\\' << '"';
         break;
      default:
         if (is_print(c)) {
            *this << c;
            break;
         }

         // Write out the escaped representation.
         if (useHexEscapes) {
            *this << '\\' << 'x';
            *this << polar::basic::hexdigit((c >> 4 & 0xF));
            *this << polar::basic::hexdigit((c >> 0) & 0xF);
         } else {
            // Always use a full 3-character octal escape.
            *this << '\\';
            *this << char('0' + ((c >> 6) & 7));
            *this << char('0' + ((c >> 3) & 7));
            *this << char('0' + ((c >> 0) & 7));
         }
      }
   }

   return *this;
}

RawOutStream &RawOutStream::operator<<(const void *ptr)
{
   write_hex(*this, (uintptr_t)ptr, HexPrintStyle::PrefixLower);
   return *this;
}

RawOutStream &RawOutStream::operator<<(double value)
{
   write_double(*this,value, FloatStyle::Exponent);
   return *this;
}

void RawOutStream::flushNonEmpty()
{
   assert(m_outBufCur > m_outBufStart && "Invalid call to flush_nonempty.");
   size_t length = m_outBufCur - m_outBufStart;
   m_outBufCur = m_outBufStart;
   writeImpl(m_outBufStart, length);
}

RawOutStream &RawOutStream::write(unsigned char character)
{
   // Group exceptional cases into a single branch.
   if (POLAR_UNLIKELY(m_outBufCur >= m_outBufEnd)) {
      if (POLAR_UNLIKELY(!m_outBufStart)) {
         if (m_bufferMode == BufferKind::Unbuffered) {
            writeImpl(reinterpret_cast<char*>(&character), 1);
            return *this;
         }
         // Set up a buffer and start over.
         setBuffered();
         return write(character);
      }
      flushNonEmpty();
   }
   *m_outBufCur++ = character;
   return *this;
}

RawOutStream &RawOutStream::write(const char *ptr, size_t size)
{
   // Group exceptional cases into a single branch.
   if (POLAR_UNLIKELY(size_t(m_outBufEnd - m_outBufCur) < size)) {
      if (POLAR_UNLIKELY(!m_outBufStart)) {
         if (m_bufferMode == BufferKind::Unbuffered) {
            writeImpl(ptr, size);
            return *this;
         }
         // Set up a buffer and start over.
         setBuffered();
         return write(ptr, size);
      }

      size_t numBytes = m_outBufEnd - m_outBufCur;

      // If the buffer is empty at this point we have a string that is larger
      // than the buffer. Directly write the chunk that is a multiple of the
      // preferred buffer size and put the remainder in the buffer.
      if (POLAR_UNLIKELY(m_outBufCur == m_outBufStart)) {
         assert(numBytes != 0 && "undefined behavior");
         size_t bytesToWrite = size - (size % numBytes);
         writeImpl(ptr, bytesToWrite);
         size_t bytesRemaining = size - bytesToWrite;
         if (bytesRemaining > size_t(m_outBufEnd - m_outBufCur)) {
            // Too much left over to copy into our buffer.
            return write(ptr + bytesToWrite, bytesRemaining);
         }
         copyToBuffer(ptr + bytesToWrite, bytesRemaining);
         return *this;
      }
      // We don't have enough space in the buffer to fit the string in. Insert as
      // much as possible, flush and start over with the remainder.
      copyToBuffer(ptr, numBytes);
      flushNonEmpty();
      return write(ptr + numBytes, size - numBytes);
   }
   copyToBuffer(ptr, size);
   return *this;
}

void RawOutStream::copyToBuffer(const char *ptr, size_t size)
{
   assert(size <= size_t(m_outBufEnd - m_outBufCur) && "Buffer overrun!");

   // Handle short strings specially, memcpy isn't very good at very short
   // strings.
   switch (size) {
   case 4: m_outBufCur[3] = ptr[3]; POLAR_FALLTHROUGH;
   case 3: m_outBufCur[2] = ptr[2]; POLAR_FALLTHROUGH;
   case 2: m_outBufCur[1] = ptr[1]; POLAR_FALLTHROUGH;
   case 1: m_outBufCur[0] = ptr[0]; POLAR_FALLTHROUGH;
   case 0: break;
   default:
      memcpy(m_outBufCur, ptr, size);
      break;
   }

   m_outBufCur += size;
}

// Formatted output.
RawOutStream &RawOutStream::operator<<(const FormatObjectBase &format)
{
   // If we have more than a few bytes left in our output buffer, try
   // formatting directly onto its end.
   size_t nextBufferSize = 127;
   size_t bufferBytesLeft = m_outBufEnd - m_outBufCur;
   if (bufferBytesLeft > 3) {
      size_t bytesUsed = format.print(m_outBufCur, bufferBytesLeft);

      // Common case is that we have plenty of space.
      if (bytesUsed <= bufferBytesLeft) {
         m_outBufCur += bytesUsed;
         return *this;
      }

      // Otherwise, we overflowed and the return value tells us the size to try
      // again with.
      nextBufferSize = bytesUsed;
   }

   // If we got here, we didn't have enough space in the output buffer for the
   // string.  Try printing into a SmallVector that is resized to have enough
   // space.  Iterate until we win.
   SmallVector<char, 128> vector;

   while (true) {
      vector.resize(nextBufferSize);
      // Try formatting into the SmallVector.
      size_t bytesUsed = format.print(vector.getData(), nextBufferSize);
      // If BytesUsed fit into the vector, we win.
      if (bytesUsed <= nextBufferSize) {
         return write(vector.getData(), bytesUsed);
      }
      // Otherwise, try again with a new size.
      assert(bytesUsed > nextBufferSize && "Didn't grow buffer!?");
      nextBufferSize = bytesUsed;
   }
}

RawOutStream &RawOutStream::operator<<(const FormatvObjectBase &object)
{
   SmallString<128> S;
   object.format(*this);
   return *this;
}

RawOutStream &RawOutStream::operator<<(const FormattedString &fstring)
{
   if (fstring.m_str.getSize() >= fstring.m_width || fstring.m_justify == FormattedString::JustifyNone) {
      this->operator<<(fstring.m_str);
      return *this;
   }
   const size_t difference = fstring.m_width - fstring.m_str.getSize();
   switch (fstring.m_justify) {
   case FormattedString::JustifyLeft:
      this->operator<<(fstring.m_str);
      this->indent(difference);
      break;
   case FormattedString::JustifyRight:
      this->indent(difference);
      this->operator<<(fstring.m_str);
      break;
   case FormattedString::JustifyCenter: {
      int padAmount = difference / 2;
      this->indent(padAmount);
      this->operator<<(fstring.m_str);
      this->indent(difference - padAmount);
      break;
   }
   default:
      polar_unreachable("Bad Justification");
   }
   return *this;
}

RawOutStream &RawOutStream::operator<<(const FormattedNumber &fnumber)
{
   if (fnumber.m_hex) {
      HexPrintStyle style;
      if (fnumber.m_upper && fnumber.m_hexPrefix) {
         style = HexPrintStyle::PrefixUpper;
      } else if (fnumber.m_upper && !fnumber.m_hexPrefix) {
         style = HexPrintStyle::Upper;
      } else if (!fnumber.m_upper && fnumber.m_hexPrefix) {
         style = HexPrintStyle::PrefixLower;
      } else {
         style = HexPrintStyle::Lower;
      }
      write_hex(*this, fnumber.m_hexValue, style, fnumber.m_width);
   } else {
      SmallString<16> buffer;
      RawSvectorOutStream stream(buffer);
      write_integer(stream, fnumber.m_decValue, 0, IntegerStyle::Integer);
      if (buffer.getSize() < fnumber.m_width) {
         indent(fnumber.m_width - buffer.getSize());
      }
      (*this) << buffer;
   }
   return *this;
}

RawOutStream &RawOutStream::operator<<(const FormattedBytes &fbytes)
{
   if (fbytes.m_bytes.empty()) {
      return *this;
   }
   size_t lineIndex = 0;
   auto bytes = fbytes.m_bytes;
   const size_t size = bytes.getSize();
   HexPrintStyle hps = fbytes.m_upper ? HexPrintStyle::Upper : HexPrintStyle::Lower;
   uint64_t offsetWidth = 0;
   if (fbytes.m_firstByteOffset.has_value()) {
      // Figure out how many nibbles are needed to print the largest offset
      // represented by this data set, so that we can align the offset field
      // to the right width.
      size_t lines = size / fbytes.m_numPerLine;
      uint64_t maxOffset = *fbytes.m_firstByteOffset + lines * fbytes.m_numPerLine;
      unsigned power = 0;
      if (maxOffset > 0) {
         power = log2_ceil_64(maxOffset);
      }
      offsetWidth = std::max<uint64_t>(4, align_to(power, 4) / 4);
   }

   // The width of a block of data including all spaces for group separators.
   unsigned numByteGroups =
         align_to(fbytes.m_numPerLine, fbytes.m_byteGroupSize) / fbytes.m_byteGroupSize;
   unsigned blockCharWidth = fbytes.m_numPerLine * 2 + numByteGroups - 1;

   while (!bytes.empty()) {
      indent(fbytes.m_indentLevel);
      if (fbytes.m_firstByteOffset.has_value()) {
         uint64_t offset = fbytes.m_firstByteOffset.value();
         write_hex(*this, offset + lineIndex, hps, offsetWidth);
         *this << ": ";
      }

      auto line = bytes.takeFront(fbytes.m_numPerLine);

      size_t charsPrinted = 0;
      // Print the hex bytes for this line in groups
      for (size_t index = 0; index < line.getSize(); ++index, charsPrinted += 2) {
         if (index && (index % fbytes.m_byteGroupSize) == 0) {
            ++charsPrinted;
            *this << " ";
         }
         write_hex(*this, line[index], hps, 2);
      }

      if (fbytes.m_ascii) {
         // Print any spaces needed for any bytes that we didn't print on this
         // line so that the ASCII bytes are correctly aligned.
         assert(blockCharWidth >= charsPrinted);
         indent(blockCharWidth - charsPrinted + 2);
         *this << "|";

         // Print the ASCII char values for each byte on this line
         for (uint8_t byte : line) {
            if (isprint(byte)) {
               *this << static_cast<char>(byte);
            } else {
               *this << '.';
            }
         }
         *this << '|';
      }

      bytes = bytes.dropFront(line.getSize());
      lineIndex += line.getSize();
      if (lineIndex < size) {
         *this << '\n';
      }
   }
   return *this;
}

namespace {

template <char C>
RawOutStream &write_padding(RawOutStream &outStream, unsigned numChars)
{
   static const char chars[] = {C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C,
                                C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C,
                                C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C,
                                C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C,
                                C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C};

   // Usually the indentation is small, handle it with a fastpath.
   if (numChars < polar::basic::array_lengthof(chars)) {
      return outStream.write(chars, numChars);
   }
   while (numChars) {
      unsigned numToWrite = std::min(numChars,
                                     (unsigned)polar::basic::array_lengthof(chars)-1);
      outStream.write(chars, numToWrite);
      numChars -= numToWrite;
   }
   return outStream;
}

} // anonymous namespace

/// indent - Insert 'NumSpaces' spaces.
RawOutStream &RawOutStream::indent(unsigned numSpaces)
{
   return write_padding<' '>(*this, numSpaces);
}

/// write_zeros - Insert 'NumZeros' nulls.
RawOutStream &RawOutStream::writeZeros(unsigned numZeros)
{
   return write_padding<'\0'>(*this, numZeros);
}

void RawOutStream::anchor()
{}

//===----------------------------------------------------------------------===//
//  Formatted Output
//===----------------------------------------------------------------------===//

// Out of line virtual method.
void FormatObjectBase::home()
{
}

//===----------------------------------------------------------------------===//
//  RawFdOutStream
//===----------------------------------------------------------------------===//

namespace {

int get_fd(StringRef filename, std::error_code &errorCode,
           fs::CreationDisposition disp, fs::FileAccess access,
           fs::OpenFlags flags)
{
   assert((access & fs::FA_Write) &&
          "Cannot make a raw_ostream from a read-only descriptor!");

   // Handle "-" as stdout. Note that when we do this, we consider ourself
   // the owner of stdout and may set the "binary" flag globally based on Flags.
   if (filename == "-") {
      errorCode = std::error_code();
      // If user requested binary then put stdout into binary mode if
      // possible.
      if (!(flags & fs::OF_Text)) {
         sys::change_stdout_to_binary();
      }
      return STDOUT_FILENO;
   }

   int fd;
   if (access & fs::FA_Read) {
      errorCode = fs::open_file_for_read_write(filename, fd, disp, flags);
   } else {
      errorCode = fs::open_file_for_write(filename, fd, disp, flags);
   }
   if (errorCode) {
      return -1;
   }
   return fd;
}

} // anonymous namespace

RawFdOutStream::RawFdOutStream(StringRef filename, std::error_code &errorCode)
   : RawFdOutStream(filename, errorCode, fs::CD_CreateAlways, fs::FA_Write,
                    fs::OF_None)
{}

RawFdOutStream::RawFdOutStream(StringRef filename, std::error_code &errorCode,
                               fs::CreationDisposition disp)
   : RawFdOutStream(filename, errorCode, disp, fs::FA_Write, fs::OF_None)
{}

RawFdOutStream::RawFdOutStream(StringRef filename, std::error_code &errorCode,
                               fs::FileAccess access)
   : RawFdOutStream(filename, errorCode, fs::CD_CreateAlways, access,
                    fs::OF_None)
{}

RawFdOutStream::RawFdOutStream(StringRef filename, std::error_code &errorCode,
                               fs::OpenFlags flags)
   : RawFdOutStream(filename, errorCode, fs::CD_CreateAlways, fs::FA_Write,
                    flags)
{}

RawFdOutStream::RawFdOutStream(StringRef filename, std::error_code &errorCode,
                               fs::CreationDisposition disp,
                               fs::FileAccess access,
                               fs::OpenFlags flags)
   : RawFdOutStream(get_fd(filename, errorCode, disp, access, flags), true)
{}

/// FD is the file descriptor that this writes to.  If ShouldClose is true, this
/// closes the file when the stream is destroyed.
RawFdOutStream::RawFdOutStream(int fd, bool shouldClose, bool unbuffered)
   : RawPwriteStream(unbuffered),
     m_fd(fd),
     m_shouldClose(shouldClose)
{
   if (m_fd < 0 ) {
      m_shouldClose = false;
      return;
   }

   // Do not attempt to close stdout or stderr. We used to try to maintain the
   // property that tools that support writing file to stdout should not also
   // write informational output to stdout, but in practice we were never able to
   // maintain this invariant. Many features have been added to POLAR and clang
   // (-fdump-record-layouts, optimization remarks, etc) that print to stdout, so
   // users must simply be aware that mixed output and remarks is a possibility.
   if (m_fd <= STDERR_FILENO) {
      m_shouldClose = false;
   }
#ifdef _WIN32
   // Check if this is a console device. This is not equivalent to isatty.
   m_isWindowsConsole =
         ::GetFileType((HANDLE)::_get_osfhandle(fd)) == FILE_TYPE_CHAR;
#endif
   // Get the starting position.
   off_t loc = ::lseek(m_fd, 0, SEEK_CUR);
#ifdef _WIN32
   // MSVCRT's _lseek(SEEK_CUR) doesn't return -1 for pipes.
   fs::FileStatus fileStatus;
   std::error_code errorCode = status(FD, fileStatus);
   m_supportsSeeking = !errorCode && fileStatus.getType() == fs::FileType::regular_file;
#else
   m_supportsSeeking = loc != (off_t)-1;
#endif
   if (!m_supportsSeeking) {
      m_pos = 0;
   } else {
      m_pos = static_cast<uint64_t>(loc);
   }
}

RawFdOutStream::~RawFdOutStream()
{
   if (m_fd >= 0) {
      flush();
      if (m_shouldClose) {
         if (auto errorCode = Process::safelyCloseFileDescriptor(m_fd)) {
            errorDetected(errorCode);
         }
      }
   }

#ifdef __MINGW32__
   // On mingw, global dtors should not call exit().
   // report_fatal_error() invokes exit(). We know report_fatal_error()
   // might not write messages to stderr when any errors were detected
   // on m_fd == 2.
   if (m_fd == 2) return;
#endif

   // If there are any pending errors, report them now. Clients wishing
   // to avoid report_fatal_error calls should check for errors with
   // has_error() and clear the error flag with clear_error() before
   // destructing RawOutStream objects which may have errors.
   if (hasError()) {
      report_fatal_error("IO failure on output stream: " + getErrorCode().message(),
                         /*GenCrashDiag=*/false);
   }
}

#if defined(_WIN32)
// The most reliable way to print unicode in a Windows console is with
// WriteConsoleW. To use that, first transcode from UTF-8 to UTF-16. This
// assumes that LLVM programs always print valid UTF-8 to the console. The data
// might not be UTF-8 for two major reasons:
// 1. The program is printing binary (-filetype=obj -o -), in which case it
// would have been gibberish anyway.
// 2. The program is printing text in a semi-ascii compatible codepage like
// shift-jis or cp1252.
//
// Most LLVM programs don't produce non-ascii text unless they are quoting
// user source input. A well-behaved LLVM program should either validate that
// the input is UTF-8 or transcode from the local codepage to UTF-8 before
// quoting it. If they don't, this may mess up the encoding, but this is still
// probably the best compromise we can make.
static bool write_console_impl(int FD, StringRef Data) {
   SmallVector<wchar_t, 256> wideText;

   // Fall back to ::write if it wasn't valid UTF-8.
   if (auto EC = sys::windows::UTF8ToUTF16(Data, wideText))
      return false;

   // On Windows 7 and earlier, WriteConsoleW has a low maximum amount of data
   // that can be written to the console at a time.
   size_t maxWriteSize = wideText.size();
   if (!RunningWindows8OrGreater()) {
      maxWriteSize = 32767;
   }
   size_t wcharsWritten = 0;
   do {
      size_t wcharsToWrite =
            std::min(maxWriteSize, wideText.size() - wcharsWritten);
      DWORD ActuallyWritten;
      bool Success =
            ::WriteConsoleW((HANDLE)::_get_osfhandle(FD), &wideText[wcharsWritten],
                            wcharsToWrite, &ActuallyWritten,
                            /*Reserved=*/nullptr);

      // The most likely reason for WriteConsoleW to fail is that FD no longer
      // points to a console. Fall back to ::write. If this isn't the first loop
      // iteration, something is truly wrong.
      if (!Success)
         return false;

      wcharsWritten += ActuallyWritten;
   } while (wcharsWritten != wideText.size());
   return true;
}
#endif

void RawFdOutStream::writeImpl(const char *ptr, size_t size)
{
   assert(m_fd >= 0 && "File already closed.");
   m_pos += size;

#if defined(_WIN32)
   // If this is a Windows console device, try re-encoding from UTF-8 to UTF-16
   // and using WriteConsoleW. If that fails, fall back to plain write().
   if (m_isWindowsConsole) {
      if (write_console_impl(fd, StringRef(ptr, size))) {
         return;
      }
   }
#endif

   // The maximum write size is limited to INT32_MAX. A write
   // greater than SSIZE_MAX is implementation-defined in POSIX,
   // and Windows _write requires 32 bit input.
   size_t maxWriteSize = INT32_MAX;

#if defined(__linux__)
   // It is observed that Linux returns EINVAL for a very large write (>2G).
   // Make it a reasonably small value.
   maxWriteSize = 1024 * 1024 * 1024;
#endif

   do {
      size_t chunkSize = std::min(size, maxWriteSize);
      ssize_t ret = ::write(m_fd, ptr, chunkSize);

      if (ret < 0) {
         // If it's a recoverable error, swallow it and retry the write.
         //
         // Ideally we wouldn't ever see EAGAIN or EWOULDBLOCK here, since
         // RawOutStream isn't designed to do non-blocking I/O. However, some
         // programs, such as old versions of bjam, have mistakenly used
         // O_NONBLOCK. For compatibility, emulate blocking semantics by
         // spinning until the write succeeds. If you don't want spinning,
         // don't use O_NONBLOCK file descriptors with RawOutStream.
         if (errno == EINTR || errno == EAGAIN
    #ifdef EWOULDBLOCK
             || errno == EWOULDBLOCK
    #endif
             )
            continue;

         // Otherwise it's a non-recoverable error. Note it and quit.
         errorDetected(std::error_code(errno, std::generic_category()));
         break;
      }

      // The write may have written some or all of the data. Update the
      // size and buffer pointer to reflect the remainder that needs
      // to be written. If there are no bytes left, we're done.
      ptr += ret;
      size -= ret;
   } while (size > 0);
}

void RawFdOutStream::close()
{
   assert(m_shouldClose);
   m_shouldClose = false;
   flush();
   if (auto errorCode = Process::safelyCloseFileDescriptor(m_fd)) {
      errorDetected(errorCode);
   }
   m_fd = -1;
}

uint64_t RawFdOutStream::seek(uint64_t off)
{
   assert(m_supportsSeeking && "Stream does not support seeking!");
   flush();
#ifdef _WIN32
   m_pos = ::_lseeki64(FD, off, SEEK_SET);
#elif defined(HAVE_LSEEK64)
   m_pos = ::lseek64(m_fd, off, SEEK_SET);
#else
   m_pos = ::lseek(m_fd, off, SEEK_SET);
#endif
   if (m_pos == (uint64_t)-1) {
      errorDetected(std::error_code(errno, std::generic_category()));
   }
   return m_pos;
}

void RawFdOutStream::pwriteImpl(const char *ptr, size_t size,
                                uint64_t offset)
{
   uint64_t pos = tell();
   seek(offset);
   write(ptr, size);
   seek(pos);
}

size_t RawFdOutStream::getPreferredBufferSize() const
{
#if defined(_WIN32)
   // Disable buffering for console devices. Console output is re-encoded from
   // UTF-8 to UTF-16 on Windows, and buffering it would require us to split the
   // buffer on a valid UTF-8 codepoint boundary. Terminal buffering is disabled
   // below on most other OSs, so do the same thing on Windows and avoid that
   // complexity.
   if (m_isWindowsConsole) {
      return 0;
   }

   return RawOutStream::getPreferredBufferSize();
#elif !defined(__minix)
   // Minix has no st_blksize.
   assert(m_fd >= 0 && "File not yet open!");
   struct stat statbuf;
   if (fstat(m_fd, &statbuf) != 0) {
      return 0;
   }
   // If this is a terminal, don't use buffering. Line buffering
   // would be a more traditional thing to do, but it's not worth
   // the complexity.
   if (S_ISCHR(statbuf.st_mode) && isatty(m_fd)) {
      return 0;
   }
   // Return the preferred block size.
   return statbuf.st_blksize;
#else
   return RawOutStream::getPreferredBufferSize();
#endif
}

RawOutStream &RawFdOutStream::changeColor(enum Colors colors, bool bold,
                                          bool bg)
{

   if (Process::colorNeedsFlush()) {
      flush();
   }
   const char *colorcode =
         (colors == Colors::SAVEDCOLOR) ? Process::outputBold(bg)
                                        : Process::outputColor(polar::as_integer(colors), bold, bg);
   if (colorcode) {
      size_t len = strlen(colorcode);
      write(colorcode, len);
      // don't account colors towards output characters
      m_pos -= len;
   }
   return *this;
}

RawOutStream &RawFdOutStream::resetColor()
{
   if (Process::colorNeedsFlush()) {
      flush();
   }
   const char *colorcode = Process::resetColor();
   if (colorcode) {
      size_t len = strlen(colorcode);
      write(colorcode, len);
      // don't account colors towards output characters
      m_pos -= len;
   }
   return *this;
}

RawOutStream &RawFdOutStream::reverseColor()
{
   if (Process::colorNeedsFlush()) {
      flush();
   }
   const char *colorcode = Process::outputReverse();
   if (colorcode) {
      size_t len = strlen(colorcode);
      write(colorcode, len);
      // don't account colors towards output characters
      m_pos -= len;
   }
   return *this;
}

bool RawFdOutStream::isDisplayed() const
{
   return false;
   return Process::fileDescriptorIsDisplayed(m_fd);
}

bool RawFdOutStream::hasColors() const
{
   return false;
   return Process::fileDescriptorHasColors(m_fd);
}

void RawFdOutStream::anchor()
{}

//===----------------------------------------------------------------------===//
//  outs(), errs(), nulls()
//===----------------------------------------------------------------------===//

/// outs() - This returns a reference to a RawOutStream for standard output.
/// Use it like: out_stream() << "foo" << "bar";
RawOutStream &out_stream()
{
   // Set buffer settings to model stdout behavior.
   std::error_code errorCode;
   static RawFdOutStream stream("-", errorCode, fs::F_None);
   assert(!errorCode);
   return stream;
}

/// errs() - This returns a reference to a RawOutStream for standard error.
/// Use it like: error_stream() << "foo" << "bar";
RawOutStream &error_stream()
{
   // Set standard error to be unbuffered by default.
   static RawFdOutStream stream(STDERR_FILENO, false, true);
   return stream;
}

/// nulls() - This returns a reference to a RawOutStream which discards output.
RawOutStream &null_stream()
{
   static RawNullOutStream stream;
   return stream;
}

//===----------------------------------------------------------------------===//
//  RawStringOutStream
//===----------------------------------------------------------------------===//

RawStringOutStream::~RawStringOutStream()
{
   flush();
}

void RawStringOutStream::writeImpl(const char *ptr, size_t size)
{
   m_outStream.append(ptr, size);
}

//===----------------------------------------------------------------------===//
//  RawSvectorOutStream
//===----------------------------------------------------------------------===//

uint64_t RawSvectorOutStream::getCurrentPos() const
{
   return m_outStream.getSize();
}

void RawSvectorOutStream::writeImpl(const char *ptr, size_t size)
{
   m_outStream.append(ptr, ptr + size);
}

void RawSvectorOutStream::pwriteImpl(const char *ptr, size_t size,
                                     uint64_t offset)
{
   memcpy(m_outStream.getData() + offset, ptr, size);
}

//===----------------------------------------------------------------------===//
//  RawNullOutStream
//===----------------------------------------------------------------------===//

RawNullOutStream::~RawNullOutStream()
{
#ifndef NDEBUG
   // ~RawOutStream asserts that the buffer is empty. This isn't necessary
   // with RawNullOutStream, but it's better to have RawNullOutStream follow
   // the rules than to change the rules just for RawNullOutStream.
   flush();
#endif
}

void RawNullOutStream::writeImpl(const char *ptr, size_t size)
{
}

uint64_t RawNullOutStream::getCurrentPos() const
{
   return 0;
}

void RawNullOutStream::pwriteImpl(const char *ptr, size_t size,
                                  uint64_t offset)
{}

void RawPwriteStream::anchor()
{}

} // utils
} // polar
