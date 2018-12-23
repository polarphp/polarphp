// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

#ifndef POLARPHP_UTILS_LOCK_FILE_MGR_H
#define POLARPHP_UTILS_LOCK_FILE_MGR_H

#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/FileSystem.h"
#include <system_error>
#include <utility> // for std::pair

namespace polar {

// forward declare class with namespace
namespace basic {
class StringRef;
} // basic

namespace utils {

using polar::basic::StringRef;

/// \brief Class that manages the creation of a lock file to aid
/// implicit coordination between different processes.
///
/// The implicit coordination works by creating a ".lock" file alongside
/// the file that we're coordinating for, using the atomicity of the file
/// system to ensure that only a single process can create that ".lock" file.
/// When the lock file is removed, the owning process has finished the
/// operation.
class LockFileManager
{
public:
   /// \brief Describes the state of a lock file.
   enum LockFileState
   {
      /// \brief The lock file has been created and is owned by this instance
      /// of the object.
      LFS_Owned,
      /// \brief The lock file already exists and is owned by some other
      /// instance.
      LFS_Shared,
      /// \brief An error occurred while trying to create or find the lock
      /// file.
      LFS_Error
   };

   /// \brief Describes the result of waiting for the owner to release the lock.
   enum WaitForUnlockResult
   {
      /// \brief The lock was released successfully.
      Res_Success,
      /// \brief m_owner died while holding the lock.
      Res_OwnerDied,
      /// \brief Reached timeout while waiting for the owner to release the lock.
      Res_Timeout
   };

private:
   SmallString<128> m_filename;
   SmallString<128> m_lockFilename;
   SmallString<128> m_uniqueLockFilename;

   std::optional<std::pair<std::string, int> > m_owner;
   std::error_code m_errorCode;
   std::string m_errorDiagMsg;

   LockFileManager(const LockFileManager &) = delete;
   LockFileManager &operator=(const LockFileManager &) = delete;

   static std::optional<std::pair<std::string, int> >
   readLockFile(StringRef lockFileName);

   static bool processStillExecuting(StringRef hostname, int PID);

public:

   LockFileManager(StringRef fileName);
   ~LockFileManager();

   /// \brief Determine the state of the lock file.
   LockFileState getState() const;

   operator LockFileState() const
   {
      return getState();
   }

   /// \brief For a shared lock, wait until the owner releases the lock.
   WaitForUnlockResult waitForUnlock();

   /// \brief Remove the lock file.  This may delete a different lock file than
   /// the one previously read if there is a race.
   std::error_code unsafeRemoveLockFile();

   /// \brief Get error message, or "" if there is no error.
   std::string getErrorMessage() const;

   /// \brief Set error and error message
   void setError(const std::error_code &errorCode, StringRef errorMsg = "")
   {
      m_errorCode = errorCode;
      m_errorDiagMsg = errorMsg.getStr();
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_LOCK_FILE_MGR_H
