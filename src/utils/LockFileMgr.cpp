// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 POLARPHP_UTILS_LOCK_FILE_MGR_H software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of POLARPHP_UTILS_LOCK_FILE_MGR_H project authors
//
// Created by polarboy on 2018/07/04.

#include "polarphp/utils/LockFileMgr.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/OptionalError.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/Signals.h"
#include "polarphp/utils/RawOutStream.h"
#include <cerrno>
#include <ctime>
#include <memory>
#include <optional>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <tuple>

#ifdef POLAR_OS_WIN
#include <windows.h>
#endif
#ifdef POLAR_OS_UNIX
#include <unistd.h>
#endif

namespace polar {
namespace utils {

#if defined(__APPLE__) && defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && (__MAC_OS_X_VERSION_MIN_REQUIRED > 1050)
#define USE_OSX_GETHOSTUUID 1
#else
#define USE_OSX_GETHOSTUUID 0
#endif

#if USE_OSX_GETHOSTUUID
#include <uuid/uuid.h>
#endif

/// \brief Attempt to read the lock file with the given name, if it exists.
///
/// \param m_lockFilename The name of the lock file to read.
///
/// \returns The process ID of the process that owns this lock file
std::optional<std::pair<std::string, int> >
LockFileManager::readLockFile(StringRef m_lockFilename)
{
   // Read the owning host and pid out of the lock file. If it appears that the
   // owning process is dead, the lock file is invalid.
   OptionalError<std::unique_ptr<MemoryBuffer>> memoryBufferOrErr =
         MemoryBuffer::getFile(m_lockFilename);
   if (!memoryBufferOrErr) {
      polar::fs::remove(m_lockFilename);
      return std::nullopt;
   }
   MemoryBuffer &memoryBuffer = *memoryBufferOrErr.get();

   StringRef hostname;
   StringRef pidStr;
   std::tie(hostname, pidStr) = polar::basic::get_token(memoryBuffer.getBuffer(), " ");
   pidStr = pidStr.substr(pidStr.findFirstNotOf(" "));
   int pid;
   if (!pidStr.getAsInteger(10, pid)) {
      auto m_owner = std::make_pair(std::string(hostname), pid);
      if (processStillExecuting(m_owner.first, m_owner.second)) {
         return m_owner;
      }
   }
   // Delete the lock file. It's invalid anyway.
   polar::fs::remove(m_lockFilename);
   return std::nullopt;
}

static std::error_code get_host_id(SmallVectorImpl<char> &hostID)
{
   hostID.clear();

#if USE_OSX_GETHOSTUUID
   // On OS X, use the more stable hardware UUID instead of hostname.
   struct timespec wait = {1, 0}; // 1 second.
   uuid_t uuid;
   if (gethostuuid(uuid, &wait) != 0) {
      return std::error_code(errno, std::system_category());
   }
   uuid_string_t uuidStr;
   uuid_unparse(uuid, uuidStr);
   StringRef uuidRef(uuidStr);
   hostID.append(uuidRef.begin(), uuidRef.end());

#elif defined(POLAR_OS_UNIX)
   char hostName[256];
   hostName[255] = 0;
   hostName[0] = 0;
   gethostname(hostName, 255);
   StringRef hostNameRef(hostName);
   hostID.append(hostNameRef.begin(), hostNameRef.end());

#else
   StringRef dummy("localhost");
   hostID.append(dummy.begin(), dummy.end());
#endif

   return std::error_code();
}

bool LockFileManager::processStillExecuting(StringRef hostID, int pid)
{
#if defined(POLAR_OS_UNIX) && !defined(__ANDROID__)
   SmallString<256> storedHostID;
   if (get_host_id(storedHostID)) {
      return true; // Conservatively assume it's executing on error
   }
   // Check whether the process is dead. If so, we're done.
   if (storedHostID == hostID && getsid(pid) == -1 && errno == ESRCH) {
      return false;
   }

#endif
   return true;
}

namespace {

/// An RAII helper object ensure that the unique lock file is removed.
///
/// Ensures that if there is an error or a signal before we finish acquiring the
/// lock, the unique file will be removed. And if we successfully take the lock,
/// the signal handler is left in place so that signals while the lock is held
/// will remove the unique lock file. The caller should ensure there is a
/// matching call to sys::DontRemoveFileOnSignal when the lock is released.
class RemoveUniqueLockFileOnSignal
{
   StringRef m_filename;
   bool m_removeImmediately;
public:
   RemoveUniqueLockFileOnSignal(StringRef Name)
      : m_filename(Name), m_removeImmediately(true)
   {
      remove_file_on_signal(m_filename, nullptr);
   }

   ~RemoveUniqueLockFileOnSignal()
   {
      if (!m_removeImmediately) {
         // Leave the signal handler enabled. It will be removed when the lock is
         // released.
         return;
      }
      fs::remove(m_filename);
      dont_remove_file_on_signal(m_filename);
   }

   void lockAcquired()
   {
      m_removeImmediately = false;
   }
};

} // end anonymous namespace

LockFileManager::LockFileManager(StringRef filename)
{
   this->m_filename = filename;
   if (std::error_code errorCode = fs::make_absolute(this->m_filename)) {
      std::string str("failed to obtain absolute path for ");
      str.append(this->m_filename.getStr());
      setError(errorCode, str);
      return;
   }
   m_lockFilename = this->m_filename;
   m_lockFilename += ".lock";

   // If the lock file already exists, don't bother to try to create our own
   // lock file; it won't work anyway. Just figure out who owns this lock file.
   if ((m_owner = readLockFile(m_lockFilename))) {
      return;
   }

   // Create a lock file that is unique to this instance.
   m_uniqueLockFilename = m_lockFilename;
   m_uniqueLockFilename += "-%%%%%%%%";
   int uniqueLockFileID;
   if (std::error_code errorCode = fs::create_unique_file(
          m_uniqueLockFilename, uniqueLockFileID, m_uniqueLockFilename)) {
      std::string str("failed to create unique file ");
      str.append(m_uniqueLockFilename.getStr());
      setError(errorCode, str);
      return;
   }

   // Write our process ID to our unique lock file.
   {
      SmallString<256> hostId;
      if (auto errorCode = get_host_id(hostId)) {
         setError(errorCode, "failed to get host id");
         return;
      }

      RawFdOutStream out(uniqueLockFileID, /*shouldClose=*/true);
      out << hostId << ' ';
#if POLAR_ON_UNIX
      out << getpid();
#else
      out << "1";
#endif
      out.close();
      if (out.hasError()) {
         // We failed to write out PID, so report the error, remove the
         // unique lock file, and fail.
         std::string str("failed to write to ");
         str.append(m_uniqueLockFilename.getStr());
         setError(out.getErrorCode(), str);
         fs::remove(m_uniqueLockFilename);
         return;
      }
   }

   // Clean up the unique file on signal, which also releases the lock if it is
   // held since the .lock symlink will point to a nonexistent file.
   RemoveUniqueLockFileOnSignal removeUniqueFile(m_uniqueLockFilename);

   while (true) {
      // Create a link from the lock file name. If this succeeds, we're done.
      std::error_code errorCode =
            fs::create_link(m_uniqueLockFilename, m_lockFilename);
      if (!errorCode) {
         removeUniqueFile.lockAcquired();
         return;
      }

      if (errorCode != ErrorCode::file_exists) {
         std::string str("failed to create link ");
         RawStringOutStream oss(str);
         oss << m_lockFilename.getStr() << " to " << m_uniqueLockFilename.getStr();
         setError(errorCode, oss.getStr());
         return;
      }

      // Someone else managed to create the lock file first. Read the process ID
      // from the lock file.
      if ((m_owner = readLockFile(m_lockFilename))) {
         // Wipe out our unique lock file (it's useless now)
         fs::remove(m_uniqueLockFilename);
         return;
      }

      if (!fs::exists(m_lockFilename)) {
         // The previous owner released the lock file before we could read it.
         // Try to get ownership again.
         continue;
      }

      // There is a lock file that nobody owns; try to clean it up and get
      // ownership.
      if ((errorCode = fs::remove(m_lockFilename))) {
         std::string str("failed to remove lockfile ");
         str.append(m_uniqueLockFilename.getStr());
         setError(errorCode, str);
         return;
      }
   }
}

LockFileManager::LockFileState LockFileManager::getState() const
{
   if (m_owner) {
      return LFS_Shared;
   }
   if (m_errorCode) {
      return LFS_Error;
   }
   return LFS_Owned;
}

std::string LockFileManager::getErrorMessage() const
{
   if (m_errorCode) {
      std::string str(m_errorDiagMsg);
      std::string errCodeMsg = m_errorCode.message();
      RawStringOutStream oss(str);
      if (!errCodeMsg.empty()) {
         oss << ": " << errCodeMsg;
      }
      return oss.getStr();
   }
   return "";
}

LockFileManager::~LockFileManager()
{
   if (getState() != LFS_Owned) {
      return;
   }
   // Since we own the lock, remove the lock file and our own unique lock file.
   fs::remove(m_lockFilename);
   fs::remove(m_uniqueLockFilename);
   // The unique file is now gone, so remove it from the signal handler. This
   // matches a sys::RemoveFileOnSignal() in LockFileManager().
   dont_remove_file_on_signal(m_uniqueLockFilename);
}

LockFileManager::WaitForUnlockResult LockFileManager::waitForUnlock() {
   if (getState() != LFS_Shared)
      return Res_Success;

#ifdef POLAR_OS_WIN32
   unsigned long interval = 1;
#else
   struct timespec interval;
   interval.tv_sec = 0;
   interval.tv_nsec = 1000000;
#endif
   // Don't wait more than 40s per iteration. Total timeout for the file
   // to appear is ~1.5 minutes.
   const unsigned maxSeconds = 40;
   do {
      // Sleep for the designated interval, to allow the owning process time to
      // finish up and remove the lock file.
      // FIXME: Should we hook in to system APIs to get a notification when the
      // lock file is deleted?
#ifdef POLAR_OS_WIN32
      Sleep(interval);
#else
      nanosleep(&interval, nullptr);
#endif

      if (polar::fs::access(m_lockFilename.getCStr(), polar::fs::AccessMode::Exist) ==
          ErrorCode::no_such_file_or_directory) {
         // If the original file wasn't created, somone thought the lock was dead.
         if (!polar::fs::exists(m_filename)) {
            return Res_OwnerDied;
         }
         return Res_Success;
      }

      // If the process owning the lock died without cleaning up, just bail out.
      if (!processStillExecuting((*m_owner).first, (*m_owner).second)) {
         return Res_OwnerDied;
      }
      // Exponentially increase the time we wait for the lock to be removed.
#ifdef POLAR_OS_WIN32
      interval *= 2;
#else
      interval.tv_sec *= 2;
      interval.tv_nsec *= 2;
      if (interval.tv_nsec >= 1000000000) {
         ++interval.tv_sec;
         interval.tv_nsec -= 1000000000;
      }
#endif
   } while (
         #ifdef POLAR_OS_WIN32
            interval < maxSeconds * 1000
         #else
            interval.tv_sec < (time_t)maxSeconds
         #endif
            );

   // Give up.
   return Res_Timeout;
}

std::error_code LockFileManager::unsafeRemoveLockFile()
{
   return polar::fs::remove(m_lockFilename);
}

} // utils
} // polar
