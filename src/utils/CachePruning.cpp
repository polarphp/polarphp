// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/03.

#include "polarphp/utils/CachePruning.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/Error.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Path.h"
#include "polarphp/utils/RawOutStream.h"
#include <set>
#include <system_error>

#define DEBUG_TYPE "cache-pruning"

namespace polar {
namespace utils {

namespace {
struct FileInfo {
   TimePoint<> Time;
   uint64_t Size;
   std::string Path;

   /// Used to determine which files to prune first. Also used to determine
   /// set membership, so must take into account all fields.
   bool operator<(const FileInfo &other) const
   {
      if (Time < other.Time) {
         return true;
      } else if (other.Time < Time) {
         return false;
      }

      if (other.Size < Size) {
         return true;
      } else if (Size < other.Size) {
         return false;
      }
      return Path < other.Path;
   }
};
/// Write a new timestamp file with the given path. This is used for the pruning
/// interval option.
static void write_timestamp_file(StringRef timestampFile)
{
   std::error_code errorCode;
   RawFdOutStream outstream(timestampFile.getStr(), errorCode, polar::fs::F_None);
}

static Expected<std::chrono::seconds> parse_duration(StringRef duration)
{
   if (duration.empty()) {
      return make_error<StringError>("duration must not be empty",
                                     inconvertible_error_code());
   }
   StringRef numStr = duration.slice(0, duration.getSize() - 1);
   uint64_t num;
   if (numStr.getAsInteger(0, num)) {
      return make_error<StringError>("'" + numStr + "' not an integer",
                                     inconvertible_error_code());
   }

   switch (duration.back()) {
   case 's':
      return std::chrono::seconds(num);
   case 'm':
      return std::chrono::minutes(num);
   case 'h':
      return std::chrono::hours(num);
   default:
      return make_error<StringError>("'" + duration +
                                     "' must end with one of 's', 'm' or 'h'",
                                     inconvertible_error_code());
   }
}

} // anonymous namespace

Expected<CachePruningPolicy>
parse_cache_pruning_policy(StringRef policyStr)
{
   CachePruningPolicy policy;
   std::pair<StringRef, StringRef> pair = {"", policyStr};
   while (!pair.second.empty()) {
      pair = pair.second.split(':');
      StringRef key, value;
      std::tie(key, value) = pair.first.split('=');
      if (key == "prune_interval") {
         auto durationOrErr = parse_duration(value);
         if (!durationOrErr) {
            return durationOrErr.takeError();
         }
         policy.m_interval = *durationOrErr;
      } else if (key == "prune_after") {
         auto durationOrErr = parse_duration(value);
         if (!durationOrErr)
            return durationOrErr.takeError();
         policy.m_expiration = *durationOrErr;
      } else if (key == "cache_size") {
         if (value.back() != '%') {
            return make_error<StringError>("'" + value + "' must be a percentage",
                                           inconvertible_error_code());
         }
         StringRef sizeStr = value.dropBack();
         uint64_t size;
         if (sizeStr.getAsInteger(0, size)) {
            return make_error<StringError>("'" + sizeStr + "' not an integer",
                                           inconvertible_error_code());
         }

         if (size > 100) {
            return make_error<StringError>("'" + sizeStr +
                                           "' must be between 0 and 100",
                                           inconvertible_error_code());
         }

         policy.m_maxSizePercentageOfAvailableSpace = size;
      } else if (key == "cache_size_bytes") {
         uint64_t mult = 1;
         switch (tolower(value.back())) {
         case 'k':
            mult = 1024;
            value = value.dropBack();
            break;
         case 'm':
            mult = 1024 * 1024;
            value = value.dropBack();
            break;
         case 'g':
            mult = 1024 * 1024 * 1024;
            value = value.dropBack();
            break;
         }
         uint64_t size;
         if (value.getAsInteger(0, size)) {
            return make_error<StringError>("'" + value + "' not an integer",
                                           inconvertible_error_code());
         }
         policy.m_maxSizeBytes = size * mult;
      } else if (key == "cache_size_files") {
         if (value.getAsInteger(0, policy.m_maxSizeFiles)) {
            return make_error<StringError>("'" + value + "' not an integer",
                                           inconvertible_error_code());
         }

      } else {
         return make_error<StringError>("Unknown key: '" + key + "'",
                                        inconvertible_error_code());
      }
   }

   return policy;
}

/// Prune the cache of files that haven't been accessed in a long time.
bool prune_cache(StringRef path, CachePruningPolicy policy)
{
   using namespace std::chrono;

   if (path.empty()) {
      return false;
   }

   bool isPathDir;
   if (polar::fs::is_directory(path, isPathDir)) {
      return false;
   }

   if (!isPathDir) {
      return false;
   }

   policy.m_maxSizePercentageOfAvailableSpace =
         std::min(policy.m_maxSizePercentageOfAvailableSpace, 100u);

   if (policy.m_expiration == seconds(0) &&
       policy.m_maxSizePercentageOfAvailableSpace == 0 &&
       policy.m_maxSizeBytes == 0 && policy.m_maxSizeFiles == 0) {
      POLAR_DEBUG(debug_stream() << "No pruning settings set, exit early\n");
      // Nothing will be pruned, early exit
      return false;
   }

   // Try to stat() the timestamp file.
   SmallString<128> timestampFile(path);
   polar::fs::path::append(timestampFile, "llvmcache.timestamp");
   polar::fs::FileStatus fileStatus;
   const auto currentTime = system_clock::now();
   if (auto errorCode = polar::fs::status(timestampFile, fileStatus)) {
      if (errorCode == ErrorCode::no_such_file_or_directory) {
         // If the timestamp file wasn't there, create one now.
         write_timestamp_file(timestampFile);
      } else {
         // Unknown error?
         return false;
      }
   } else {
      if (!policy.m_interval) {
         return false;
      }
      if (policy.m_interval != seconds(0)) {
         // Check whether the time stamp is older than our pruning interval.
         // If not, do nothing.
         const auto timeStampModTime = fileStatus.getLastModificationTime();
         auto timeStampAge = currentTime - timeStampModTime;
         if (timeStampAge <= *policy.m_interval) {
            POLAR_DEBUG(debug_stream() << "Timestamp file too recent ("
                        << duration_cast<seconds>(timeStampAge).count()
                        << "s old), do not prune.\n");
            return false;
         }
      }
      // Write a new timestamp file so that nobody else attempts to prune.
      // There is a benign race condition here, if two processes happen to
      // notice at the same time that the timestamp is out-of-date.
      write_timestamp_file(timestampFile);
   }

   // Keep track of files to delete to get below the size limit.
   // Order by time of last use so that recently used files are preserved.
   std::set<FileInfo> fileInfos;
   uint64_t totalSize = 0;

   // Walk the entire directory cache, looking for unused files.
   std::error_code errorCode;
   SmallString<128> cachePathNative;
   polar::fs::path::native(path, cachePathNative);
   // Walk all of the files within this directory.
   for (polar::fs::DirectoryIterator file(cachePathNative, errorCode), fileEnd;
        file != fileEnd && !errorCode; file.increment(errorCode)) {
      // Ignore any files not beginning with the string "llvmcache-". This
      // includes the timestamp file as well as any files created by the user.
      // This acts as a safeguard against data loss if the user specifies the
      // wrong directory as their cache directory.
      if (!polar::fs::path::filename(file->getPath()).startsWith("polarcache-")) {
         continue;
      }

      // Look at this file. If we can't stat it, there's nothing interesting
      // there.
      OptionalError<polar::fs::BasicFileStatus> statusOrErr = file->getStatus();
      if (!statusOrErr) {
         POLAR_DEBUG(debug_stream() << "Ignore " << file->getPath() << " (can't stat)\n");
         continue;
      }

      // If the file hasn't been used recently enough, delete it
      const auto fileAccessTime = statusOrErr->getLastAccessedTime();
      auto fileAge = currentTime - fileAccessTime;
      if (policy.m_expiration != seconds(0) && fileAge > policy.m_expiration) {
         POLAR_DEBUG(debug_stream() << "Remove " << file->getPath() << " ("
                     << duration_cast<seconds>(fileAge).count() << "s old)\n");
         polar::fs::remove(file->getPath());
         continue;
      }

      // Leave it here for now, but add it to the list of size-based pruning.
      totalSize += statusOrErr->getSize();
      fileInfos.insert({fileAccessTime, statusOrErr->getSize(), file->getPath()});
   }

   auto fileInfo = fileInfos.begin();
   size_t numFiles = fileInfos.size();

   auto removeCacheFile = [&]() {
      // Remove the file.
      polar::fs::remove(fileInfo->Path);
      // Update size
      totalSize -= fileInfo->Size;
      numFiles--;
      POLAR_DEBUG(debug_stream() << " - Remove " << fileInfo->Path << " (size "
                  << fileInfo->Size << "), new occupancy is " << totalSize
                  << "%\n");
      ++fileInfo;
   };

   // Prune for number of files.
   if (policy.m_maxSizeFiles) {
      while (numFiles > policy.m_maxSizeFiles) {
         removeCacheFile();
      }
   }

   // Prune for size now if needed
   if (policy.m_maxSizePercentageOfAvailableSpace > 0 || policy.m_maxSizeBytes > 0) {
      auto errOrSpaceInfo = polar::fs::disk_space(path);
      if (!errOrSpaceInfo) {
         report_fatal_error("Can't get available size");
      }
      polar::fs::SpaceInfo spaceInfo = errOrSpaceInfo.get();
      auto availableSpace = totalSize + spaceInfo.free;
      if (policy.m_maxSizePercentageOfAvailableSpace == 0) {
         policy.m_maxSizePercentageOfAvailableSpace = 100;
      }
      if (policy.m_maxSizeBytes == 0) {
         policy.m_maxSizeBytes = availableSpace;
      }
      auto totalSizeTarget = std::min<uint64_t>(
               availableSpace * policy.m_maxSizePercentageOfAvailableSpace / 100ull,
               policy.m_maxSizeBytes);

      POLAR_DEBUG(debug_stream() << "Occupancy: " << ((100 * totalSize) / availableSpace)
                  << "% target is: " << policy.m_maxSizePercentageOfAvailableSpace
                  << "%, " << policy.m_maxSizeBytes << " bytes\n");

      // Remove the oldest accessed files first, till we get below the threshold.
      while (totalSize > totalSizeTarget && fileInfo != fileInfos.end()) {
         removeCacheFile();
      }
   }
   return true;
}

} // utils
} // polar
