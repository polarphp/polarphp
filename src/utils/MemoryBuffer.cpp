//===--- MemoryBuffer.cpp - Memory Buffer implementation ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
// Created by polarboy on 2018/06/11.

//===----------------------------------------------------------------------===//
//
//  This file implements the MemoryBuffer interface.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/global/Config.h"
#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/ErrorNumber.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/utils/Path.h"
#include "polarphp/utils/Process.h"
#include "polarphp/utils/Program.h"
#include "polarphp/utils/SmallVectorMemoryBuffer.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <new>
#include <sys/types.h>
#include <system_error>
#if !defined(_MSC_VER) && !defined(__MINGW32__)
#include <unistd.h>
#else
#include <io.h>
#endif

//===----------------------------------------------------------------------===//
// MemoryBuffer implementation itself.
//===----------------------------------------------------------------------===//

namespace polar::utils {

using polar::sys::Process;

MemoryBuffer::~MemoryBuffer()
{}

/// init - Initialize this MemoryBuffer as a reference to externally allocated
/// memory, memory that we know is already null terminated.
void MemoryBuffer::init(const char *bufStart, const char *bufEnd,
                        bool requiresNullTerminator)
{
   assert((!requiresNullTerminator || bufEnd[0] == 0) &&
         "Buffer is not null terminated!");
   m_bufferStart = bufStart;
   m_bufferEnd = bufEnd;
}

//===----------------------------------------------------------------------===//
// MemoryBufferMem implementation.
//===----------------------------------------------------------------------===//

namespace {

/// CopyStringRef - Copies contents of a StringRef into a block of memory and
/// null-terminates it.
void copy_string_ref(char *memory, StringRef data)
{
   if (!data.empty()) {
      memcpy(memory, data.getData(), data.getSize());
   }
   memory[data.getSize()] = 0; // Null terminate string.
}

struct NamedBufferAlloc
{
   const Twine &m_name;
   NamedBufferAlloc(const Twine &name) : m_name(name)
   {}
};

/// MemoryBufferMem - Named MemoryBuffer pointing to a block of memory.
template<typename MemoryBufferType>
class MemoryBufferMem : public MemoryBufferType
{
public:
   MemoryBufferMem(StringRef inputData, bool requiresNullTerminator)
   {
      MemoryBuffer::init(inputData.begin(), inputData.end(),
                         requiresNullTerminator);
   }

   /// Disable sized deallocation for MemoryBufferMem, because it has
   /// tail-allocated data.
   void operator delete(void *ptr)
   {
      ::operator delete(ptr);
   }

   StringRef getBufferIdentifier() const override
   {
      // The name is stored after the class itself.
      return StringRef(reinterpret_cast<const char *>(this + 1));
   }

   MemoryBuffer::BufferKind getBufferKind() const override
   {
      return MemoryBuffer::BufferKind::MemoryBuffer_Malloc;
   }
};

} // anonymous namespace

} // polar::utils

void *operator new(size_t size, const polar::utils::NamedBufferAlloc &alloc)
{
   polar::basic::SmallString<256> nameBuf;
   polar::basic::StringRef nameRef = alloc.m_name.toStringRef(nameBuf);

   char *mem = static_cast<char *>(operator new(size + nameRef.getSize() + 1));
   polar::utils::copy_string_ref(mem + size, nameRef);
   return mem;
}

namespace polar::utils {
namespace {

template <typename MemoryBufferType>
OptionalError<std::unique_ptr<MemoryBufferType>>
get_file_aux(const Twine &filename, int64_t fileSize, uint64_t mapSize,
             uint64_t offset, bool requiresNullTerminator, bool isVolatile);

} // anonymous namespace

std::unique_ptr<MemoryBuffer>
MemoryBuffer::getMemBuffer(StringRef inputData, StringRef bufferName,
                           bool requiresNullTerminator)
{
   auto *ret = new (NamedBufferAlloc(bufferName))
         MemoryBufferMem<MemoryBuffer>(inputData, requiresNullTerminator);
   return std::unique_ptr<MemoryBuffer>(ret);
}

std::unique_ptr<MemoryBuffer>
MemoryBuffer::getMemBuffer(MemoryBufferRef ref, bool requiresNullTerminator)
{
   return std::unique_ptr<MemoryBuffer>(getMemBuffer(
                                           ref.getBuffer(), ref.getBufferIdentifier(), requiresNullTerminator));
}

OptionalError<std::unique_ptr<WritableMemoryBuffer>>
get_mem_buffer_copy_impl(StringRef inputData, const Twine &bufferName)
{
   auto buf = WritableMemoryBuffer::getNewUninitMemBuffer(inputData.getSize(), bufferName);
   if (!buf) {
      return make_error_code(ErrorCode::not_enough_memory);
   }
   memcpy(buf->getBufferStart(), inputData.getData(), inputData.getSize());
   return std::move(buf);
}

std::unique_ptr<MemoryBuffer>
MemoryBuffer::getMemBufferCopy(StringRef inputData, const Twine &bufferName)
{
   auto buf = get_mem_buffer_copy_impl(inputData, bufferName);
   if (buf) {
      return std::move(*buf);
   }
   return nullptr;
}

OptionalError<std::unique_ptr<MemoryBuffer>>
MemoryBuffer::getFileOrStdIn(const Twine &filename, int64_t fileSize,
                             bool requiresNullTerminator)
{
   SmallString<256> nameBuf;
   StringRef nameRef = filename.toStringRef(nameBuf);

   if (nameRef == "-") {
      return getStdIn();
   }
   return getFile(filename, fileSize, requiresNullTerminator);
}

OptionalError<std::unique_ptr<MemoryBuffer>>
MemoryBuffer::getFileSlice(const Twine &filePath, uint64_t mapSize,
                           uint64_t offset, bool isVolatile)
{
   return get_file_aux<MemoryBuffer>(filePath, -1, mapSize, offset, false,
                                     isVolatile);
}

//===----------------------------------------------------------------------===//
// MemoryBuffer::getFile implementation.
//===----------------------------------------------------------------------===//

namespace {
/// Memory maps a file descriptor using fs::MappedFileRegion.
///
/// This handles converting the offset into a legal offset on the platform.
template<typename MemoryBufferType>
class MemoryBufferMMapFile : public MemoryBufferType
{
   MappedFileRegion m_mfr;

   static uint64_t getLegalMapOffset(uint64_t offset)
   {
      return offset & ~(MappedFileRegion::getAlignment() - 1);
   }

   static uint64_t getLegalMapSize(uint64_t len, uint64_t offset)
   {
      return len + (offset - getLegalMapOffset(offset));
   }

   const char *getStart(uint64_t len, uint64_t offset)
   {
      return m_mfr.getConstData() + (offset - getLegalMapOffset(offset));
   }

public:
   MemoryBufferMMapFile(bool requiresNullTerminator, fs::file_t fd, uint64_t len,
                        uint64_t offset, std::error_code &errorCode)
      : m_mfr(fd, MemoryBufferType::sm_mapMode, getLegalMapSize(len, offset),
              getLegalMapOffset(offset), errorCode)
   {
      if (!errorCode) {
         const char *start = getStart(len, offset);
         MemoryBuffer::init(start, start + len, requiresNullTerminator);
      }
   }

   /// Disable sized deallocation for MemoryBufferMMapFile, because it has
   /// tail-allocated data.
   void operator delete(void *ptr)
   {
      ::operator delete(ptr);
   }

   StringRef getBufferIdentifier() const override
   {
      // The name is stored after the class itself.
      return StringRef(reinterpret_cast<const char *>(this + 1));
   }

   MemoryBuffer::BufferKind getBufferKind() const override
   {
      return MemoryBuffer::BufferKind::MemoryBuffer_MMap;
   }
};
}

namespace {

OptionalError<std::unique_ptr<WritableMemoryBuffer>>
get_memory_buffer_for_stream(fs::file_t fd, const Twine &bufferName)
{
   const ssize_t chunkSize = 4096*4;
   SmallString<chunkSize> buffer;
   size_t readBytes;
   // Read into Buffer until we hit EOF.
   do {
      buffer.reserve(buffer.getSize() + chunkSize);
      if (auto errorCode = fs::read_native_file(
             fd, polar::basic::make_mutable_array_ref(buffer.end(), chunkSize), &readBytes)) {
         return errorCode;
      }
      buffer.setSize(buffer.size() + readBytes);
   } while (readBytes != 0);
   return get_mem_buffer_copy_impl(buffer, bufferName);
}

template <typename MemoryBufferType>
static OptionalError<std::unique_ptr<MemoryBufferType>>
get_open_file_impl(fs::file_t fd, const Twine &filename, uint64_t fileSize,
                   uint64_t mapSize, int64_t offset, bool requiresNullTerminator,
                   bool isVolatile);

template <typename MemoryBufferType>
static OptionalError<std::unique_ptr<MemoryBufferType>>
get_file_aux(const Twine &filename, int64_t fileSize, uint64_t mapSize,
             uint64_t offset, bool requiresNullTerminator, bool isVolatile)
{
   Expected<fs::file_t> fdOrErr =
         fs::open_native_file_for_read(filename, fs::OF_None);
   if (!fdOrErr) {
      return error_to_error_code(fdOrErr.takeError());
   }
   fs::file_t fd = *fdOrErr;
   auto ret = get_open_file_impl<MemoryBufferType>(fd, filename, fileSize, mapSize, offset,
                                                   requiresNullTerminator, isVolatile);
   fs::close_file(fd);
   return ret;
}

} // anonymous namespace


OptionalError<std::unique_ptr<MemoryBuffer>>
MemoryBuffer::getFile(const Twine &filename, int64_t fileSize,
                      bool requiresNullTerminator, bool isVolatile)
{
   return get_file_aux<MemoryBuffer>(filename, fileSize, fileSize, 0,
                                     requiresNullTerminator, isVolatile);
}

OptionalError<std::unique_ptr<WritableMemoryBuffer>>
WritableMemoryBuffer::getFile(const Twine &filename, int64_t fileSize,
                              bool isVolatile)
{
   return get_file_aux<WritableMemoryBuffer>(filename, fileSize, fileSize, 0,
                                             /*RequiresNullTerminator*/ false,
                                             isVolatile);
}

OptionalError<std::unique_ptr<WritableMemoryBuffer>>
WritableMemoryBuffer::getFileSlice(const Twine &filename, uint64_t mapSize,
                                   uint64_t offset, bool isVolatile)
{
   return get_file_aux<WritableMemoryBuffer>(filename, -1, mapSize, offset, false,
                                             isVolatile);
}

std::unique_ptr<WritableMemoryBuffer>
WritableMemoryBuffer::getNewUninitMemBuffer(size_t size, const Twine &bufferName)
{
   using MemBuffer = MemoryBufferMem<WritableMemoryBuffer>;
   // Allocate space for the MemoryBuffer, the data and the name. It is important
   // that MemoryBuffer and data are aligned so PointerIntPair works with them.
   // TODO: Is 16-byte alignment enough?  We copy small object files with large
   // alignment expectations into this buffer.
   SmallString<256> nameBuf;
   StringRef nameRef = bufferName.toStringRef(nameBuf);
   size_t alignedStringLen = align_to(sizeof(MemBuffer) + nameRef.getSize() + 1, 16);
   size_t realLen = alignedStringLen + size + 1;
   char *mem = static_cast<char*>(operator new(realLen, std::nothrow));
   if (!mem) {
      return nullptr;
   }

   // The name is stored after the class itself.
   copy_string_ref(mem + sizeof(MemBuffer), nameRef);

   // The buffer begins after the name and must be aligned.
   char *buf = mem + alignedStringLen;
   buf[size] = 0; // Null terminate buffer.

   auto *ret = new (mem) MemBuffer(StringRef(buf, size), true);
   return std::unique_ptr<WritableMemoryBuffer>(ret);
}

std::unique_ptr<WritableMemoryBuffer>
WritableMemoryBuffer::getNewMemBuffer(size_t size, const Twine &bufferName)
{
   auto strBuf = WritableMemoryBuffer::getNewUninitMemBuffer(size, bufferName);
   if (!strBuf) {
      return nullptr;
   }
   memset(strBuf->getBufferStart(), 0, size);
   return strBuf;
}

namespace {

bool should_use_mmap(fs::file_t fd,
                     size_t fileSize,
                     size_t mapSize,
                     off_t offset,
                     bool requiresNullTerminator,
                     int pageSize,
                     bool isVolatile)
{
   // mmap may leave the buffer without null terminator if the file size changed
   // by the time the last page is mapped in, so avoid it if the file size is
   // likely to change.
   if (isVolatile) {
      return false;
   }
   // We don't use mmap for small files because this can severely fragment our
   // address space.
   if (mapSize < 4 * 4096 || mapSize < (unsigned)pageSize) {
      return false;
   }

   if (!requiresNullTerminator) {
      return true;
   }

   // If we don't know the file size, use fstat to find out.  fstat on an open
   // file descriptor is cheaper than stat on a random path.
   // FIXME: this chunk of code is duplicated, but it avoids a fstat when
   // RequiresNullTerminator = false and mapSize != -1.
   if (fileSize == size_t(-1)) {
      fs::FileStatus status;
      if (fs::status(fd, status)) {
         return false;
      }
      fileSize = status.getSize();
   }

   // If we need a null terminator and the end of the map is inside the file,
   // we cannot use mmap.
   size_t end = offset + mapSize;
   assert(end <= fileSize);
   if (end != fileSize) {
      return false;
   }
   // Don't try to map files that are exactly a multiple of the system page size
   // if we need a null terminator.
   if ((fileSize & (pageSize -1)) == 0) {
      return false;
   }
#if defined(__CYGWIN__)
   // Don't try to map files that are exactly a multiple of the physical page size
   // if we need a null terminator.
   // FIXME: We should reorganize again getPageSize() on Win32.
   if ((fileSize & (4096 - 1)) == 0) {
      return false;
   }
#endif
   return true;
}

static OptionalError<std::unique_ptr<WriteThroughMemoryBuffer>>
get_read_write_file(const Twine &filename, uint64_t fileSize, uint64_t mapSize,
                    uint64_t offset)
{
   Expected<fs::file_t> fdOrErr = fs::open_native_file_for_read_write(
            filename, fs::CD_OpenExisting, fs::OF_None);
   if (!fdOrErr) {
      return error_to_error_code(fdOrErr.takeError());
   }
   fs::file_t fd = *fdOrErr;

   // Default is to map the full file.
   if (mapSize == uint64_t(-1)) {
      // If we don't know the file size, use fstat to find out.  fstat on an open
      // file descriptor is cheaper than stat on a random path.
      if (fileSize == uint64_t(-1)) {
         fs::FileStatus status;
         std::error_code errorCode = fs::status(fd, status);
         if (errorCode) {
            return errorCode;
         }
         // If this not a file or a block device (e.g. it's a named pipe
         // or character device), we can't mmap it, so error out.
         fs::FileType type = status.getType();
         if (type != fs::FileType::regular_file &&
             type != fs::FileType::block_file) {
            return make_error_code(ErrorCode::invalid_argument);
         }
         fileSize = status.getSize();
      }
      mapSize = fileSize;
   }

   std::error_code errorCode;
   std::unique_ptr<WriteThroughMemoryBuffer> result(
            new (NamedBufferAlloc(filename))
            MemoryBufferMMapFile<WriteThroughMemoryBuffer>(false, fd, mapSize,
                                                           offset, errorCode));
   if (errorCode) {
      return errorCode;
   }
   return std::move(result);
}

} // anonymous namespace

OptionalError<std::unique_ptr<WriteThroughMemoryBuffer>>
WriteThroughMemoryBuffer::getFile(const Twine &filename, int64_t fileSize)
{
   return get_read_write_file(filename, fileSize, fileSize, 0);
}

/// Map a subrange of the specified file as a WritableMemoryBuffer.
OptionalError<std::unique_ptr<WriteThroughMemoryBuffer>>
WriteThroughMemoryBuffer::getFileSlice(const Twine &filename, uint64_t mapSize,
                                       uint64_t offset)
{
   return get_read_write_file(filename, -1, mapSize, offset);
}

namespace {

template <typename MemoryBufferType>
OptionalError<std::unique_ptr<MemoryBufferType>>
get_open_file_impl(int fd, const Twine &filename, uint64_t fileSize,
                   uint64_t mapSize, int64_t offset, bool requiresNullTerminator,
                   bool isVolatile)
{
   static int pageSize = Process::getPageSizeEstimate();

   // Default is to map the full file.
   if (mapSize == uint64_t(-1)) {
      // If we don't know the file size, use fstat to find out.  fstat on an open
      // file descriptor is cheaper than stat on a random path.
      if (fileSize == uint64_t(-1)) {
         fs::FileStatus status;
         std::error_code errorCode = fs::status(fd, status);
         if (errorCode) {
            return errorCode;
         }

         // If this not a file or a block device (e.g. it's a named pipe
         // or character device), we can't trust the size. Create the memory
         // buffer by copying off the stream.
         fs::FileType type = status.getType();
         if (type != fs::FileType::regular_file &&
             type != fs::FileType::block_file) {
            return get_memory_buffer_for_stream(fd, filename);
         }
         fileSize = status.getSize();
      }
      mapSize = fileSize;
   }

   if (should_use_mmap(fd, fileSize, mapSize, offset, requiresNullTerminator,
                       pageSize, isVolatile)) {
      std::error_code errorCode;
      std::unique_ptr<MemoryBufferType> result(
               new (NamedBufferAlloc(filename)) MemoryBufferMMapFile<MemoryBufferType>(
                  requiresNullTerminator, fd, mapSize, offset, errorCode));
      if (!errorCode) {
         return std::move(result);
      }
   }

   auto buf = WritableMemoryBuffer::getNewUninitMemBuffer(mapSize, filename);
   if (!buf) {
      // Failed to create a buffer. The only way it can fail is if
      // new(std::nothrow) returns 0.
      return make_error_code(ErrorCode::not_enough_memory);
   }
   fs::read_native_file_slice(fd, buf->getBuffer(), offset);
   return std::move(buf);
}

} // anonymous namespace

OptionalError<std::unique_ptr<MemoryBuffer>>
MemoryBuffer::getOpenFile(fs::file_t fd, const Twine &filename, uint64_t fileSize,
                          bool requiresNullTerminator, bool isVolatile)
{
   return get_open_file_impl<MemoryBuffer>(fd, filename, fileSize, fileSize, 0,
                                           requiresNullTerminator, isVolatile);
}

OptionalError<std::unique_ptr<MemoryBuffer>>
MemoryBuffer::getOpenFileSlice(fs::file_t fd, const Twine &filename, uint64_t mapSize,
                               int64_t offset, bool isVolatile)
{
   assert(mapSize != uint64_t(-1));
   return get_open_file_impl<MemoryBuffer>(fd, filename, -1, mapSize, offset, false,
                                           isVolatile);
}

OptionalError<std::unique_ptr<MemoryBuffer>> MemoryBuffer::getStdIn()
{
   // Read in all of the data from stdin, we cannot mmap stdin.
   //
   // FIXME: That isn't necessarily true, we should try to mmap stdin and
   // fallback if it fails.
   polar::sys::change_stdin_to_binary();
   return get_memory_buffer_for_stream(fs::get_stdin_handle(), "<stdin>");
}

OptionalError<std::unique_ptr<MemoryBuffer>>
MemoryBuffer::getFileAsStream(const Twine &filename)
{
   Expected<fs::file_t> fdOrErr =
         fs::open_native_file_for_read(filename, fs::OF_None);
   if (!fdOrErr) {
      return error_to_error_code(fdOrErr.takeError());
   }
   fs::file_t fd = *fdOrErr;
   OptionalError<std::unique_ptr<MemoryBuffer>> ret =
         get_memory_buffer_for_stream(fd, filename);
   fs::close_file(fd);
   return ret;
}

MemoryBufferRef MemoryBuffer::getMemBufferRef() const
{
   StringRef data = getBuffer();
   StringRef identifier = getBufferIdentifier();
   return MemoryBufferRef(data, identifier);
}

SmallVectorMemoryBuffer::~SmallVectorMemoryBuffer()
{}

} // polar::utils
