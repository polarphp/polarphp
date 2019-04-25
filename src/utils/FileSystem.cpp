//===--- FileSystem.cpp - Extra helpers for manipulating files ------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/25.

#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Path.h"
#include "polarphp/utils/Process.h"
#include "polarphp/utils/Signals.h"
#include "polarphp/utils/VirtualFileSystem.h"

using polar::basic::StringRef;
using polar::basic::SmallString;
using polar::utils::OptionalError;
using polar::utils::ErrorCode;
using polar::utils::RawFdOutStream;

namespace {

class OpenFileRAII
{
   static const int INVALID_FD = -1;
public:
   int fd = INVALID_FD;

   ~OpenFileRAII()
   {
      if (fd != INVALID_FD) {
         polar::sys::Process::safelyCloseFileDescriptor(fd);
      }
   }
};

/// Does some simple checking to see if a temporary file can be written next to
/// \p outputPath and then renamed into place.
///
/// Helper for swift::atomicallyWritingToFile.
///
/// If the result is an error, the write won't succeed at all, and the calling
/// operation should bail out early.
OptionalError<bool>
can_use_temporary_for_write(const StringRef outputPath)
{
   namespace fs = polar::fs;
   if (outputPath == "-") {
      // Special case: "-" represents stdout, and LLVM's output stream APIs are
      // aware of this. It doesn't make sense to use a temporary in this case.
      return false;
   }

   fs::FileStatus status;
   (void)fs::status(outputPath, status);
   if (!fs::exists(status)) {
      // Assume we'll be able to write to both a temporary file and to the final
      // destination if the final destination doesn't exist yet.
      return true;
   }

   // Fail early if we can't write to the final destination.
   if (!fs::can_write(outputPath)) {
      return polar::utils::make_error_code(ErrorCode::operation_not_permitted);
   }
   // Only use a temporary if the output is a regular file. This handles
   // things like '-o /dev/null'
   return fs::is_regular_file(status);
}

/// Attempts to open a temporary file next to \p outputPath, with the intent
/// that once the file has been written it will be renamed into place.
///
/// Helper for swift::atomicallyWritingToFile.
///
/// \param[out] openedStream On success, a stream opened for writing to the
/// temporary file that was just created.
/// \param outputPath The path to the final output file, which is used to decide
/// where to put the temporary.
///
/// \returns The path to the temporary file that was opened, or \c None if the
/// file couldn't be created.
std::optional<std::string>
try_to_open_temporary_file(std::optional<RawFdOutStream> &openedStream,
                           const StringRef outputPath)
{
   namespace fs = polar::fs;

   // Create a temporary file path.
   // Insert a placeholder for a random suffix before the extension (if any).
   // Then because some tools glob for build artifacts (such as clang's own
   // GlobalModuleIndex.cpp), also append .tmp.
   SmallString<128> tempPath;
   const StringRef outputExtension = polar::fs::path::extension(outputPath);
   tempPath = outputPath.dropBack(outputExtension.size());
   tempPath += "-%%%%%%%%";
   tempPath += outputExtension;
   tempPath += ".tmp";

   int fd;
   const unsigned perms = fs::all_read | fs::all_write;
   std::error_code errorCode = fs::create_unique_file(tempPath, fd, tempPath, perms);

   if (errorCode) {
      // Ignore the specific error; the caller has to fall back to not using a
      // temporary anyway.
      return std::nullopt;
   }

   openedStream.emplace(fd, /*shouldClose=*/true);
   // Make sure the temporary file gets removed if we crash.
   polar::utils::remove_file_on_signal(tempPath);
   return tempPath.getStr().getStr();
}

} // end anonymous namespace

namespace polar::fs {

std::error_code atomically_writing_to_file(
      const StringRef outputPath,
      const FunctionRef<void(RawPwriteStream &)> action)
{
   namespace fs = polar::fs;

   // FIXME: This is mostly a simplified version of
   // clang::CompilerInstance::createOutputFile. It would be great to share the
   // implementation.
   assert(!outputPath.empty());
   OptionalError<bool> canUseTemporary = can_use_temporary_for_write(outputPath);
   if (std::error_code error = canUseTemporary.getError()) {
      return error;
   }
   std::optional<std::string> temporaryPath;
   {
      std::optional<RawFdOutStream> outStream;
      if (canUseTemporary.get()) {
         temporaryPath = try_to_open_temporary_file(outStream, outputPath);

         if (!temporaryPath) {
            assert(!outStream.has_value());
            // If we failed to create the temporary, fall back to writing to the
            // file directly. This handles the corner case where we cannot write to
            // the directory, but can write to the file.
         }
      }

      if (!outStream.has_value()) {
         std::error_code error;
         outStream.emplace(outputPath, error, fs::F_None);
         if (error) {
            return error;
         }
      }
      action(outStream.value());
      // In addition to scoping the use of 'outStream', ending the scope here also
      // ensures that it's been flushed (by destroying it).
   }

   if (!temporaryPath.has_value()) {
      // If we didn't use a temporary, we're done!
      return std::error_code();
   }

   return move_file_if_different(temporaryPath.value(), outputPath);
}

std::error_code move_file_if_different(const Twine &source,
                                       const Twine &destination)
{
   namespace fs = polar::fs;

   // First check for a self-move.
   if (fs::equivalent(source, destination)) {
      return std::error_code();
   }

   OpenFileRAII sourceFile;
   fs::FileStatus sourceStatus;
   if (std::error_code error = fs::open_file_for_read(source, sourceFile.fd)) {
      // If we can't open the source file, fail.
      return error;
   }
   if (std::error_code error = fs::status(sourceFile.fd, sourceStatus)) {
      // If we can't stat the source file, fail.
      return error;
   }

   OpenFileRAII destFile;
   fs::FileStatus destStatus;
   bool couldReadDest = !fs::open_file_for_read(destination, destFile.fd);
   if (couldReadDest) {
      couldReadDest = !fs::status(destFile.fd, destStatus);
   }

   // If we could read the destination file, and it matches the source file in
   // size, they may be the same. Do an actual comparison of the contents.
   if (couldReadDest && sourceStatus.getSize() == destStatus.getSize()) {
      uint64_t size = sourceStatus.getSize();
      bool same = false;
      if (size == 0) {
         same = true;
      } else {
         std::error_code sourceRegionErr;
         fs::MappedFileRegion sourceRegion(sourceFile.fd,
                                           fs::MappedFileRegion::readonly,
                                           size, 0, sourceRegionErr);
         if (sourceRegionErr) {
            return sourceRegionErr;
         }
         std::error_code destRegionErr;
         fs::MappedFileRegion destRegion(destFile.fd,
                                         fs::MappedFileRegion::readonly,
                                         size, 0, destRegionErr);

         if (!destRegionErr) {
            same = (0 == memcmp(sourceRegion.getConstData(), destRegion.getConstData(),
                                size));
         }
      }

      // If the file contents are the same, we are done. Just delete the source.
      if (same) {
         return fs::remove(source);
      }
   }
   // If we get here, we weren't able to prove that the files are the same.
   return fs::rename(source, destination);
}

} // polar::fs

namespace polar::vfs {

OptionalError<std::unique_ptr<MemoryBuffer>>
get_file_or_stdin(FileSystem &fs,
                  const Twine &filename,
                  int64_t fileSize,
                  bool requiresNullTerminator,
                  bool isVolatile)
{
   SmallString<256> nameBuf;
   StringRef nameRef = filename.toStringRef(nameBuf);

   if (nameRef == "-") {
      return MemoryBuffer::getStdIn();
   }

   return fs.getBufferForFile(filename, fileSize,
                              requiresNullTerminator, isVolatile);
}

} // polar::vfs
