// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/11.

#ifndef POLARPHP_UTILS_MEMORY_BUFFER_H
#define POLARPHP_UTILS_MEMORY_BUFFER_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/OptionalError.h"
#include "polarphp/utils/FileSystem.h"
#include <cstddef>
#include <cstdint>
#include <memory>

namespace polar {
namespace utils {

class MemoryBufferRef;

using polar::basic::Twine;
using polar::basic::ArrayRef;
using polar::basic::StringRef;
using polar::basic::MutableArrayRef;
using polar::fs::MappedFileRegion;

/// This interface provides simple read-only access to a block of memory, and
/// provides simple methods for reading files and standard input into a memory
/// buffer.  In addition to basic access to the characters in the file, this
/// interface guarantees you can read one character past the end of the file,
/// and that this character will read as '\0'.
///
/// The '\0' guarantee is needed to support an optimization -- it's intended to
/// be more efficient for clients which are reading all the data to stop
/// reading when they encounter a '\0' than to continually check the file
/// position to see if it has reached the end of the file.
class MemoryBuffer
{
   const char *m_bufferStart; // Start of the buffer.
   const char *m_bufferEnd;   // End of the buffer.
protected:
   MemoryBuffer() = default;

   void init(const char *bufStart, const char *bufEnd,
             bool requiresNullTerminator);

   static constexpr MappedFileRegion::MapMode sm_mapMode =
         MappedFileRegion::readonly;

public:
   MemoryBuffer(const MemoryBuffer &) = delete;
   MemoryBuffer &operator=(const MemoryBuffer &) = delete;
   virtual ~MemoryBuffer();

   const char *getBufferStart() const
   {
      return m_bufferStart;
   }

   const char *getBufferEnd() const
   {
      return m_bufferEnd;
   }

   size_t getBufferSize() const
   {
      return m_bufferEnd - m_bufferStart;
   }

   StringRef getBuffer() const
   {
      return StringRef(m_bufferStart, getBufferSize());
   }

   /// Return an identifier for this buffer, typically the filename it was read
   /// from.
   virtual StringRef getBufferIdentifier() const
   {
      return "Unknown buffer";
   }

   /// Open the specified file as a MemoryBuffer, returning a new MemoryBuffer
   /// if successful, otherwise returning null. If FileSize is specified, this
   /// means that the client knows that the file exists and that it has the
   /// specified size.
   ///
   /// \param IsVolatile Set to true to indicate that the contents of the file
   /// can change outside the user's control, e.g. when libclang tries to parse
   /// while the user is editing/updating the file or if the file is on an NFS.
   static OptionalError<std::unique_ptr<MemoryBuffer>>
   getFile(const Twine &filename, int64_t fileSize = -1,
           bool requiresNullTerminator = true, bool isVolatile = false);

   /// Read all of the specified file into a MemoryBuffer as a stream
   /// (i.e. until EOF reached). This is useful for special files that
   /// look like a regular file but have 0 size (e.g. /proc/cpuinfo on Linux).
   static OptionalError<std::unique_ptr<MemoryBuffer>>
   getFileAsStream(const Twine &filename);

   /// Given an already-open file descriptor, map some slice of it into a
   /// MemoryBuffer. The slice is specified by an \p Offset and \p MapSize.
   /// Since this is in the middle of a file, the buffer is not null terminated.
   static OptionalError<std::unique_ptr<MemoryBuffer>>
   getOpenFileSlice(int fd, const Twine &filename, uint64_t mapSize,
                    int64_t offset, bool isVolatile = false);

   /// Given an already-open file descriptor, read the file and return a
   /// MemoryBuffer.
   ///
   /// \param IsVolatile Set to true to indicate that the contents of the file
   /// can change outside the user's control, e.g. when libclang tries to parse
   /// while the user is editing/updating the file or if the file is on an NFS.
   static OptionalError<std::unique_ptr<MemoryBuffer>>
   getOpenFile(int fd, const Twine &filename, uint64_t fileSize,
               bool requiresNullTerminator = true, bool isVolatile = false);

   /// Open the specified memory range as a MemoryBuffer. Note that inputData
   /// must be null terminated if RequiresNullTerminator is true.
   static std::unique_ptr<MemoryBuffer>
   getMemBuffer(StringRef inputData, StringRef bufferName = "",
                bool requiresNullTerminator = true);

   static std::unique_ptr<MemoryBuffer>
   getMemBuffer(MemoryBufferRef ref, bool requiresNullTerminator = true);

   /// Open the specified memory range as a MemoryBuffer, copying the contents
   /// and taking ownership of it. inputData does not have to be null terminated.
   static std::unique_ptr<MemoryBuffer>
   getMemBufferCopy(StringRef inputData, const Twine &bufferName = "");

   /// Read all of stdin into a file buffer, and return it.
   static OptionalError<std::unique_ptr<MemoryBuffer>> getStdIn();

   /// Open the specified file as a MemoryBuffer, or open stdin if the Filename
   /// is "-".
   static OptionalError<std::unique_ptr<MemoryBuffer>>
   getFileOrStdIn(const Twine &filename, int64_t fileSize = -1,
                  bool requiresNullTerminator = true);

   /// Map a subrange of the specified file as a MemoryBuffer.
   static OptionalError<std::unique_ptr<MemoryBuffer>>
   getFileSlice(const Twine &filename, uint64_t mapSize, uint64_t offset,
                bool isVolatile = false);

   //===--------------------------------------------------------------------===//
   // Provided for performance analysis.
   //===--------------------------------------------------------------------===//

   /// The kind of memory backing used to support the MemoryBuffer.
   enum class BufferKind
   {
      MemoryBuffer_Malloc,
      MemoryBuffer_MMap
   };

   /// Return information on the memory mechanism used to support the
   /// MemoryBuffer.
   virtual BufferKind getBufferKind() const = 0;
   MemoryBufferRef getMemBufferRef() const;
};

/// This class is an extension of MemoryBuffer, which allows copy-on-write
/// access to the underlying contents.  It only supports creation methods that
/// are guaranteed to produce a writable buffer.  For example, mapping a file
/// read-only is not supported.
class WritableMemoryBuffer : public MemoryBuffer
{
protected:
   WritableMemoryBuffer() = default;

   static constexpr MappedFileRegion::MapMode sm_mapMode =
         MappedFileRegion::priv;

public:
   using MemoryBuffer::getBuffer;
   using MemoryBuffer::getBufferEnd;
   using MemoryBuffer::getBufferStart;

   // const_cast is well-defined here, because the underlying buffer is
   // guaranteed to have been initialized with a mutable buffer.
   char *getBufferStart()
   {
      return const_cast<char *>(MemoryBuffer::getBufferStart());
   }

   char *getBufferEnd()
   {
      return const_cast<char *>(MemoryBuffer::getBufferEnd());
   }

   MutableArrayRef<char> getBuffer()
   {
      return {getBufferStart(), getBufferEnd()};
   }

   static OptionalError<std::unique_ptr<WritableMemoryBuffer>>
   getFile(const Twine &filename, int64_t fileSize = -1,
           bool isVolatile = false);

   /// Map a subrange of the specified file as a WritableMemoryBuffer.
   static OptionalError<std::unique_ptr<WritableMemoryBuffer>>
   getFileSlice(const Twine &filename, uint64_t mapSize, uint64_t offset,
                bool isVolatile = false);

   /// Allocate a new MemoryBuffer of the specified size that is not initialized.
   /// Note that the caller should initialize the memory allocated by this
   /// method. The memory is owned by the MemoryBuffer object.
   static std::unique_ptr<WritableMemoryBuffer>
   getNewUninitMemBuffer(size_t size, const Twine &bufferName = "");

   /// Allocate a new zero-initialized MemoryBuffer of the specified size. Note
   /// that the caller need not initialize the memory allocated by this method.
   /// The memory is owned by the MemoryBuffer object.
   static std::unique_ptr<WritableMemoryBuffer>
   getNewMemBuffer(size_t size, const Twine &bufferName = "");

private:
   // Hide these base class factory function so one can't write
   //   WritableMemoryBuffer::getXXX()
   // and be surprised that he got a read-only Buffer.
   using MemoryBuffer::getFileAsStream;
   using MemoryBuffer::getFileOrStdIn;
   using MemoryBuffer::getMemBuffer;
   using MemoryBuffer::getMemBufferCopy;
   using MemoryBuffer::getOpenFile;
   using MemoryBuffer::getOpenFileSlice;
   using MemoryBuffer::getStdIn;
};

/// This class is an extension of MemoryBuffer, which allows write access to
/// the underlying contents and committing those changes to the original source.
/// It only supports creation methods that are guaranteed to produce a writable
/// buffer.  For example, mapping a file read-only is not supported.
class WriteThroughMemoryBuffer : public MemoryBuffer
{
protected:
   WriteThroughMemoryBuffer() = default;

   static constexpr MappedFileRegion::MapMode sm_mapMode =
         MappedFileRegion::readwrite;

public:
   using MemoryBuffer::getBuffer;
   using MemoryBuffer::getBufferEnd;
   using MemoryBuffer::getBufferStart;

   // const_cast is well-defined here, because the underlying buffer is
   // guaranteed to have been initialized with a mutable buffer.
   char *getBufferStart()
   {
      return const_cast<char *>(MemoryBuffer::getBufferStart());
   }
   char *getBufferEnd()
   {
      return const_cast<char *>(MemoryBuffer::getBufferEnd());
   }

   MutableArrayRef<char> getBuffer()
   {
      return {getBufferStart(), getBufferEnd()};
   }

   static OptionalError<std::unique_ptr<WriteThroughMemoryBuffer>>
   getFile(const Twine &filename, int64_t fileSize = -1);

   /// Map a subrange of the specified file as a ReadWriteMemoryBuffer.
   static OptionalError<std::unique_ptr<WriteThroughMemoryBuffer>>
   getFileSlice(const Twine &filename, uint64_t mapSize, uint64_t offset);

private:
   // Hide these base class factory function so one can't write
   //   WritableMemoryBuffer::getXXX()
   // and be surprised that he got a read-only Buffer.
   using MemoryBuffer::getFileAsStream;
   using MemoryBuffer::getFileOrStdIn;
   using MemoryBuffer::getMemBuffer;
   using MemoryBuffer::getMemBufferCopy;
   using MemoryBuffer::getOpenFile;
   using MemoryBuffer::getOpenFileSlice;
   using MemoryBuffer::getStdIn;
};

class MemoryBufferRef
{
   StringRef m_buffer;
   StringRef m_identifier;

public:
   MemoryBufferRef() = default;
   MemoryBufferRef(MemoryBuffer& buffer)
      : m_buffer(buffer.getBuffer()),
        m_identifier(buffer.getBufferIdentifier())
   {}

   MemoryBufferRef(StringRef buffer, StringRef identifier)
      : m_buffer(buffer),
        m_identifier(identifier)
   {}

   StringRef getBuffer() const
   {
      return m_buffer;
   }

   StringRef getBufferIdentifier() const
   {
      return m_identifier;
   }

   const char *getBufferStart() const
   {
      return m_buffer.begin();
   }
   const char *getBufferEnd() const
   {
      return m_buffer.end();
   }

   size_t getBufferSize() const
   {
      return m_buffer.getSize();
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_MEMORY_BUFFER_H
