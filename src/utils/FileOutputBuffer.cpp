// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/21.

//===----------------------------------------------------------------------===//
//
// Utility for creating a in-memory buffer that will be written to a file.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/FileOutputBuffer.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/Memory.h"
#include "polarphp/utils/Path.h"
#include <system_error>

#if !defined(_MSC_VER) && !defined(__MINGW32__)
#include <unistd.h>
#else
#include <io.h>
#endif

namespace polar {
namespace utils {

using polar::sys::MemoryBlock;
using polar::sys::Memory;
using polar::sys::OwningMemoryBlock;

namespace {
// A FileOutputBuffer which creates a temporary file in the same directory
// as the final output file. The final output file is atomically replaced
// with the temporary file on commit().
class OnDiskBuffer : public FileOutputBuffer
{
public:
   OnDiskBuffer(StringRef path, fs::TempFile temp,
                std::unique_ptr<fs::MappedFileRegion> Buf)
      : FileOutputBuffer(path), m_buffer(std::move(Buf)), m_temp(std::move(temp))
   {}

   uint8_t *getBufferStart() const override
   {
      return (uint8_t *)m_buffer->getData();
   }

   uint8_t *getBufferEnd() const override
   {
      return (uint8_t *)m_buffer->getData() + m_buffer->getSize();
   }

   size_t getBufferSize() const override
   {
      return m_buffer->getSize();
   }

   Error commit() override
   {
      // Unmap buffer, letting OS flush dirty pages to file on disk.
      m_buffer.reset();
      // Atomically replace the existing file with the new one.
      return m_temp.keep(m_finalPath);
   }

   ~OnDiskBuffer() override {
      // Close the mapping before deleting the temp file, so that the removal
      // succeeds.
      m_buffer.reset();
      consume_error(m_temp.discard());
   }

   void discard() override
   {
      // Delete the temp file if it still was open, but keeping the mapping
      // active.
      consume_error(m_temp.discard());
   }

private:
   std::unique_ptr<fs::MappedFileRegion> m_buffer;
   fs::TempFile m_temp;
};

// A FileOutputBuffer which keeps data in memory and writes to the final
// output file on commit(). This is used only when we cannot use OnDiskBuffer.
class InMemoryBuffer : public FileOutputBuffer
{
public:
   InMemoryBuffer(StringRef path, MemoryBlock buffer, unsigned mode)
      : FileOutputBuffer(path), m_buffer(buffer), m_mode(mode)
   {}

   uint8_t *getBufferStart() const override
   {
      return (uint8_t *)m_buffer.getBase();
   }

   uint8_t *getBufferEnd() const override
   {
      return (uint8_t *)m_buffer.getBase() + m_buffer.getSize();
   }

   size_t getBufferSize() const override
   {
      return m_buffer.getSize();
   }

   Error commit() override
   {
      int fd;
      if (auto errorCode = polar::fs::open_file_for_write(m_finalPath, fd, fs::CD_CreateAlways, fs::F_None, m_mode)) {
         return error_code_to_error(errorCode);
      }
      RawFdOutStream outstream(fd, /*shouldClose=*/true, /*unbuffered=*/true);
      outstream << StringRef((const char *)m_buffer.getBase(), m_buffer.getSize());
      return Error::getSuccess();
   }

private:
   OwningMemoryBlock m_buffer;
   unsigned m_mode;
};

Expected<std::unique_ptr<InMemoryBuffer>>
createInMemoryBuffer(StringRef path, size_t size, unsigned mode)
{
   std::error_code errorCode;
   MemoryBlock block = Memory::allocateMappedMemory(
            size, nullptr, sys::Memory::MF_READ | sys::Memory::MF_WRITE, errorCode);
   if (errorCode) {
      return error_code_to_error(errorCode);
   }
   return std::make_unique<InMemoryBuffer>(path, block, mode);
}

Expected<std::unique_ptr<OnDiskBuffer>>
createOnDiskBuffer(StringRef path, size_t size, bool initExisting,
                   unsigned mode)
{
   Expected<fs::TempFile> fileOrErr =
         fs::TempFile::create(path + ".tmp%%%%%%%", mode);
   if (!fileOrErr) {
      return fileOrErr.takeError();
   }

   fs::TempFile file = std::move(*fileOrErr);
   if (initExisting) {
      if (auto errorCode = fs::copy_file(path, file.m_fd)) {
         return error_code_to_error(errorCode);
      }
   } else {
#ifndef POLAR_OS_WIN
      // On Windows, CreateFileMapping (the mmap function on Windows)
      // automatically extends the underlying file. We don't need to
      // extend the file beforehand. _chsize (ftruncate on Windows) is
      // pretty slow just like it writes specified amount of bytes,
      // so we should avoid calling that function.
      if (auto errorCode = fs::resize_file(file.m_fd, size)) {
         consume_error(file.discard());
         return error_code_to_error(errorCode);
      }
#endif
   }

   // Mmap it.
   std::error_code errorCode;
   auto mappedFile = std::make_unique<fs::MappedFileRegion>(
            file.m_fd, fs::MappedFileRegion::readwrite, size, 0, errorCode);
   if (errorCode) {
      consume_error(file.discard());
      return error_code_to_error(errorCode);
   }
   return std::make_unique<OnDiskBuffer>(path, std::move(file),
                                         std::move(mappedFile));
}

} // namespace


// Create an instance of FileOutputBuffer.
Expected<std::unique_ptr<FileOutputBuffer>>
FileOutputBuffer::create(StringRef path, size_t size, unsigned flags)
{
   unsigned mode = fs::all_read | fs::all_write;
   if (flags & F_executable) {
      mode |= fs::all_exe;
   }
   fs::FileStatus stat;
   fs::status(path, stat);

   if ((flags & F_modify) && size == size_t(-1)) {
      if (stat.getType() == fs::FileType::regular_file) {
         size = stat.getSize();
      } else if (stat.getType() == fs::FileType::file_not_found) {
         return error_code_to_error(ErrorCode::no_such_file_or_directory);
      } else {
         return error_code_to_error(ErrorCode::invalid_argument);
      }
   }

   // Usually, we want to create OnDiskBuffer to create a temporary file in
   // the same directory as the destination file and atomically replaces it
   // by rename(2).
   //
   // However, if the destination file is a special file, we don't want to
   // use rename (e.g. we don't want to replace /dev/null with a regular
   // file.) If that's the case, we create an in-memory buffer, open the
   // destination file and write to it on commit().
   switch (stat.getType()) {
   case fs::FileType::directory_file:
      return error_code_to_error(ErrorCode::is_a_directory);
   case fs::FileType::regular_file:
   case fs::FileType::file_not_found:
   case fs::FileType::status_error:
      return createOnDiskBuffer(path, size, !!(flags & F_modify), mode);
   default:
      return createInMemoryBuffer(path, size, mode);
   }
}

} // utils
} // polar
