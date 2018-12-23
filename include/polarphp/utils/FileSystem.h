// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/08.

//===----------------------------------------------------------------------===//
//
// This file declares the polar::fs namespace. It is designed after
// TR2/boost filesystem (v3), but modified to remove exception handling and the
// path class.
//
// All functions return an error_code and their actual work via the last out
// argument. The out argument is defined if and only if errc::success is
// returned. A function may return any error code in the generic or system
// category. However, they shall be equivalent to any error conditions listed
// in each functions respective documentation if the condition applies. [ note:
// this does not guarantee that error_code will be in the set of explicitly
// listed codes, but it does guarantee that if any of the explicitly listed
// errors occur, the correct error_code will be used ]. All functions may
// return errc::not_enough_memory if there is not enough memory to complete the
// operation.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_ERROR_FILESYSTEM_H
#define POLARPHP_UTILS_ERROR_FILESYSTEM_H

#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/global/Global.h"
#include "polarphp/utils/Chrono.h"
#include "polarphp/utils/Error.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/OptionalError.h"
#include "polarphp/utils/Md5.h"
#include <cassert>
#include <cstdint>
#include <ctime>
#include <memory>
#include <stack>
#include <string>
#include <system_error>
#include <tuple>
#include <vector>
#include <functional>

#ifdef POLAR_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

namespace polar {
namespace fs {

using polar::basic::Twine;
using polar::basic::SmallVectorImpl;
using polar::utils::OptionalError;
using polar::basic::SmallString;
using polar::utils::TimePoint;
using polar::basic::StringRef;
using polar::utils::Expected;
using polar::utils::Error;
using polar::utils::Md5;
using polar::basic::FunctionRef;

// forward declare class
class DirectoryEntry;

#if defined(_WIN32)
// A Win32 HANDLE is a typedef of void*
using file_t = void *;
#else
using file_t = int;
#endif

extern const file_t sg_kInvalidFile;

/// An enumeration for the file system's view of the type.
enum class FileType
{
   status_error,
   file_not_found,
   regular_file,
   directory_file,
   symlink_file,
   block_file,
   character_file,
   fifo_file,
   socket_file,
   type_unknown
};

/// SpaceInfo - Self explanatory.
struct SpaceInfo
{
   uint64_t capacity;
   uint64_t free;
   uint64_t available;
};

enum Permission
{
   no_perms = 0,
   owner_read = 0400,
   owner_write = 0200,
   owner_exe = 0100,
   owner_all = owner_read | owner_write | owner_exe,
   group_read = 040,
   group_write = 020,
   group_exe = 010,
   group_all = group_read | group_write | group_exe,
   others_read = 04,
   others_write = 02,
   others_exe = 01,
   others_all = others_read | others_write | others_exe,
   all_read = owner_read | group_read | others_read,
   all_write = owner_write | group_write | others_write,
   all_exe = owner_exe | group_exe | others_exe,
   all_all = owner_all | group_all | others_all,
   set_uid_on_exe = 04000,
   set_gid_on_exe = 02000,
   sticky_bit = 01000,
   all_perms = all_all | set_uid_on_exe | set_gid_on_exe | sticky_bit,
   perms_not_known = 0xFFFF
};

// Helper functions so that you can use & and | to manipulate perms bits:
inline Permission operator|(Permission lhs, Permission rhs)
{
   return static_cast<Permission>(static_cast<unsigned short>(lhs) |
                                  static_cast<unsigned short>(rhs));
}

inline Permission operator&(Permission lhs, Permission rhs)
{
   return static_cast<Permission>(static_cast<unsigned short>(lhs) &
                                  static_cast<unsigned short>(rhs));
}

inline Permission &operator|=(Permission &lhs, Permission rhs)
{
   lhs = lhs | rhs;
   return lhs;
}
inline Permission &operator&=(Permission &lhs, Permission rhs)
{
   lhs = lhs & rhs;
   return lhs;
}

inline Permission operator~(Permission value)
{
   // Avoid UB by explicitly truncating the (unsigned) ~ result.
   return static_cast<Permission>(
            static_cast<unsigned short>(~static_cast<unsigned short>(value)));
}

class UniqueId
{
   uint64_t m_device;
   uint64_t m_file;

public:
   UniqueId() = default;
   UniqueId(uint64_t device, uint64_t file)
      : m_device(device), m_file(file)
   {}

   bool operator==(const UniqueId &other) const
   {
      return m_device == other.m_device && m_file == other.m_file;
   }

   bool operator!=(const UniqueId &other) const
   {
      return !(*this == other);
   }

   bool operator<(const UniqueId &other) const
   {
      return std::tie(m_device, m_file) < std::tie(other.m_device, other.m_file);
   }

   uint64_t getDevice() const
   {
      return m_device;
   }

   uint64_t getFile() const
   {
      return m_file;
   }
};

/// Represents the result of a call to DirectoryIterator::status(). This is a
/// subset of the information returned by a regular sys::fs::status() call, and
/// represents the information provided by Windows FileFirstFile/FindNextFile.
class BasicFileStatus
{
protected:
#if defined(POLAR_ON_UNIX)
   time_t m_fsStatusAtime = 0;
   time_t m_fsStatusMtime = 0;
   uint32_t m_fsStatusAtimeNsec = 0;
   uint32_t m_fsStatusMtimeNsec = 0;
   uid_t m_fsStatusUid = 0;
   gid_t m_fsStatusGid = 0;
   off_t m_fsStatusSize = 0;
#elif defined (_WIN32)
   uint32_t m_lastAccessedTimeHigh = 0;
   uint32_t m_lastAccessedTimeLow = 0;
   uint32_t m_lastWriteTimeHigh = 0;
   uint32_t m_lastWriteTimeLow = 0;
   uint32_t m_fileSizeHigh = 0;
   uint32_t m_fileSizeLow = 0;
#endif
   FileType m_type = FileType::status_error;
   Permission m_permissions = perms_not_known;

public:
   BasicFileStatus() = default;

   explicit BasicFileStatus(FileType type) : m_type(type)
   {}

#if defined(POLAR_ON_UNIX)
   BasicFileStatus(FileType type, Permission perms, time_t aTime,
                   uint32_t aTimeNSec, time_t mTime, uint32_t mTimeNSec,
                   uid_t uid, gid_t gid, off_t size)
      : m_fsStatusAtime(aTime), m_fsStatusMtime(mTime),
        m_fsStatusAtimeNsec(aTimeNSec), m_fsStatusMtimeNsec(mTimeNSec),
        m_fsStatusUid(uid), m_fsStatusGid(gid),
        m_fsStatusSize(size), m_type(type),
        m_permissions(perms)
   {}
#elif defined(_WIN32)
   BasicFileStatus(FileType type, Permisson perms, uint32_t lastAccessTimeHigh,
                   uint32_t lastAccessTimeLow, uint32_t lastWriteTimeHigh,
                   uint32_t lastWriteTimeLow, uint32_t fileSizeHigh,
                   uint32_t fileSizeLow)
      : m_lastAccessedTimeHigh(lastAccessTimeHigh),
        m_lastAccessedTimeLow(lastAccessTimeLow),
        m_lastWriteTimeHigh(lastWriteTimeHigh),
        m_lastWriteTimeLow(lastWriteTimeLow), m_fileSizeHigh(fileSizeHigh),
        m_fileSizeLow(fileSizeLow), m_type(type), m_permissions(perms)
   {}
#endif

   // getters
   FileType getType() const
   {
      return m_type;
   }

   Permission getPermissions() const
   {
      return m_permissions;
   }
   /// The file access time as reported from the underlying file system.
   ///
   /// Also see comments on \c getLastModificationTime() related to the precision
   /// of the returned value.
   TimePoint<> getLastAccessedTime() const;

   /// The file modification time as reported from the underlying file system.
   ///
   /// The returned value allows for nanosecond precision but the actual
   /// resolution is an implementation detail of the underlying file system.
   /// There is no guarantee for what kind of resolution you can expect, the
   /// resolution can differ across platforms and even across mountpoints on the
   /// same machine.
   TimePoint<> getLastModificationTime() const;

#if defined(POLAR_ON_UNIX)
   uint32_t getUser() const
   {
      return m_fsStatusUid;
   }

   uint32_t getGroup() const
   {
      return m_fsStatusGid;
   }

   uint64_t getSize() const
   {
      return m_fsStatusSize;
   }
#elif defined (_WIN32)
   uint32_t getUser() const
   {
      return 9999; // Not applicable to Windows, so...
   }

   uint32_t getGroup() const
   {
      return 9999; // Not applicable to Windows, so...
   }

   uint64_t getSize() const
   {
      return (uint64_t(m_fileSizeHigh) << 32) + m_fileSizeLow;
   }
#endif

   // setters
   void setType(FileType type)
   {
      m_type = type;
   }

   void setPermissions(Permission permission)
   {
      m_permissions = permission;
   }
};

/// Represents the result of a call to sys::fs::status().
class FileStatus : public BasicFileStatus
{
   friend bool equivalent(FileStatus lhs, FileStatus rhs);

#if defined(POLAR_ON_UNIX)
   dev_t m_fsStatusDev = 0;
   nlink_t m_fsStatusNlinks = 0;
   ino_t m_fsStatusInode = 0;
#elif defined (_WIN32)
   uint32_t m_numLinks = 0;
   uint32_t m_volumeSerialNumber = 0;
   uint32_t m_fileIndexHigh = 0;
   uint32_t m_fileIndexLow = 0;
#endif

public:
   FileStatus() = default;

   explicit FileStatus(FileType type) : BasicFileStatus(type)
   {}

#if defined(POLAR_ON_UNIX)
   FileStatus(FileType type, Permission perms, dev_t dev,
              nlink_t links, ino_t inode,
              time_t atime, uint32_t aTimeNSec,
              time_t mtime, uint32_t mTimeNSec,
              uid_t uid, gid_t gid, off_t size)
      : BasicFileStatus(type, perms, atime, aTimeNSec, mtime, mTimeNSec, uid, gid, size),
        m_fsStatusDev(dev), m_fsStatusNlinks(links), m_fsStatusInode(inode)
   {}
#elif defined(_WIN32)
   FileStatus(FileType type, Permission perms, uint32_t linkCount,
              uint32_t lastAccessTimeHigh, uint32_t lastAccessTimeLow,
              uint32_t lastWriteTimeHigh, uint32_t lastWriteTimeLow,
              uint32_t volumeSerialNumber, uint32_t fileSizeHigh,
              uint32_t fileSizeLow, uint32_t fileIndexHigh,
              uint32_t fileIndexLow)
      : BasicFileStatus(type, perms, lastAccessTimeHigh, lastAccessTimeLow,
                        lastWriteTimeHigh, lastWriteTimeLow, fileSizeHigh,
                        fileSizeLow),
        m_numLinks(linkCount), m_volumeSerialNumber(volumeSerialNumber),
        m_fileIndexHigh(fileIndexHigh), m_fileIndexLow(fileIndexLow)
   {}
#endif

   UniqueId getUniqueId() const;
   uint32_t getLinkCount() const;
};

/// @}
/// @name Physical Operators
/// @{

/// Make \a path an absolute path.
///
/// Makes \a path absolute using the \a current_directory if it is not already.
/// An empty \a path will result in the \a current_directory.
///
/// /absolute/path   => /absolute/path
/// relative/../path => <current-directory>/relative/../path
///
/// @param path A path that is modified to be an absolute path.
/// @returns errc::success if \a path has been made absolute, otherwise a
///          platform-specific error_code.
std::error_code make_absolute(const Twine &currentDirectory,
                              SmallVectorImpl<char> &path);

/// Make \a path an absolute path.
///
/// Makes \a path absolute using the current directory if it is not already. An
/// empty \a path will result in the current directory.
///
/// /absolute/path   => /absolute/path
/// relative/../path => <current-directory>/relative/../path
///
/// @param path A path that is modified to be an absolute path.
/// @returns errc::success if \a path has been made absolute, otherwise a
///          platform-specific error_code.
std::error_code make_absolute(SmallVectorImpl<char> &path);

/// Create all the non-existent directories in path.
///
/// @param path Directories to create.
/// @returns errc::success if is_directory(path), otherwise a platform
///          specific error_code. If IgnoreExisting is false, also returns
///          error if the directory already existed.
std::error_code create_directories(const Twine &path,
                                   bool ignoreExisting = true,
                                   Permission perms = owner_all | group_all);

/// Create the directory in path.
///
/// @param path Directory to create.
/// @returns errc::success if is_directory(path), otherwise a platform
///          specific error_code. If IgnoreExisting is false, also returns
///          error if the directory already existed.
std::error_code create_directory(const Twine &path, bool ignoreExisting = true,
                                 Permission perms = owner_all | group_all);

/// Create a link from \a from to \a to.
///
/// The link may be a soft or a hard link, depending on the platform. The caller
/// may not assume which one. Currently on windows it creates a hard link since
/// soft links require extra privileges. On unix, it creates a soft link since
/// hard links don't work on SMB file systems.
///
/// @param to The path to hard link to.
/// @param from The path to hard link from. This is created.
/// @returns errc::success if the link was created, otherwise a platform
/// specific error_code.
std::error_code create_link(const Twine &to, const Twine &from);

/// Create a hard link from \a from to \a to, or return an error.
///
/// @param to The path to hard link to.
/// @param from The path to hard link from. This is created.
/// @returns errc::success if the link was created, otherwise a platform
/// specific error_code.
std::error_code create_hard_link(const Twine &to, const Twine &from);

/// Collapse all . and .. patterns, resolve all symlinks, and optionally
///        expand ~ expressions to the user's home directory.
///
/// @param path The path to resolve.
/// @param output The location to store the resolved path.
/// @param expand_tilde If true, resolves ~ expressions to the user's home
///                     directory.
std::error_code real_path(const Twine &path, SmallVectorImpl<char> &output,
                          bool expandTilde = false);

/// Expands ~ expressions to the user's home directory. On Unix ~user
/// directories are resolved as well.
///
/// @param path The path to resolve.
void expand_tilde(const Twine &path, SmallVectorImpl<char> &output);

/// Get the current path.
///
/// @param result Holds the current path on return.
/// @returns errc::success if the current path has been stored in result,
///          otherwise a platform-specific error_code.
std::error_code current_path(SmallVectorImpl<char> &result);

/// Set the current path.
///
/// @param path The path to set.
/// @returns errc::success if the current path was successfully set,
///          otherwise a platform-specific error_code.
std::error_code set_current_path(const Twine &path);

/// Remove path. Equivalent to POSIX remove().
///
/// @param path Input path.
/// @returns errc::success if path has been removed or didn't exist, otherwise a
///          platform-specific error code. If IgnoreNonExisting is false, also
///          returns error if the file didn't exist.
std::error_code remove(const Twine &path, bool ignoreNonExisting = true);

/// Recursively delete a directory.
///
/// @param path Input path.
/// @returns errc::success if path has been removed or didn't exist, otherwise a
///          platform-specific error code.
std::error_code remove_directories(const Twine &path, bool ignoreErrors = true);
std::error_code remove_directories_with_callback(const Twine &path, FunctionRef<bool(const DirectoryEntry &entry)> errorHandler);
/// Rename \a from to \a to.
///
/// Files are renamed as if by POSIX rename(), except that on Windows there may
/// be a short interval of time during which the destination file does not
/// exist.
///
/// @param from The path to rename from.
/// @param to The path to rename to. This is created.
std::error_code rename(const Twine &from, const Twine &to);

/// Copy the contents of \a From to \a To.
///
/// @param From The path to copy from.
/// @param To The path to copy to. This is created.
std::error_code copy_file(const Twine &from, const Twine &to);


/// Copy the contents of \a From to \a To.
///
/// @param From The path to copy from.
/// @param ToFD The open file descriptor of the destination file.
std::error_code copy_file(const Twine &from, int toFD);


/// Resize path to size. File is resized as if by POSIX truncate().
///
/// @param FD Input file descriptor.
/// @param Size Size to resize to.
/// @returns errc::success if \a path has been resized to \a size, otherwise a
///          platform-specific error_code.
std::error_code resize_file(int fd, uint64_t size);

/// Compute an MD5 hash of a file's contents.
///
/// @param FD Input file descriptor.
/// @returns An MD5Result with the hash computed, if successful, otherwise a
///          std::error_code.
OptionalError<Md5::Md5Result> md5_contents(int fd);

/// Version of compute_md5 that doesn't require an open file descriptor.
OptionalError<Md5::Md5Result> md5_contents(const Twine &path);

/// @}
/// @name Physical Observers
/// @{

/// Does file exist?
///
/// @param status A BasicFileStatus previously returned from stat.
/// @returns True if the file represented by status exists, false if it does
///          not.
bool exists(const BasicFileStatus &status);

enum class AccessMode { Exist, Write, Execute };

/// Can the file be accessed?
///
/// @param Path Input path.
/// @returns errc::success if the path can be accessed, otherwise a
///          platform-specific error_code.
std::error_code access(const Twine &path, AccessMode Mode);

/// Does file exist?
///
/// @param Path Input path.
/// @returns True if it exists, false otherwise.
inline bool exists(const Twine &path)
{
   return !access(path, AccessMode::Exist);
}

/// Can we execute this file?
///
/// @param Path Input path.
/// @returns True if we can execute it, false otherwise.
bool can_execute(const Twine &path);

/// Can we write this file?
///
/// @param Path Input path.
/// @returns True if we can write to it, false otherwise.
inline bool can_write(const Twine &path)
{
   return !access(path, AccessMode::Write);
}

/// Do FileStatus's represent the same thing?
///
/// @param A Input FileStatus.
/// @param B Input FileStatus.
///
/// assert(status_known(A) || status_known(B));
///
/// @returns True if A and B both represent the same file system entity, false
///          otherwise.
bool equivalent(FileStatus lhs, FileStatus rhs);

/// Do paths represent the same thing?
///
/// assert(status_known(A) || status_known(B));
///
/// @param A Input path A.
/// @param B Input path B.
/// @param result Set to true if stat(A) and stat(B) have the same device and
///               inode (or equivalent).
/// @returns errc::success if result has been successfully set, otherwise a
///          platform-specific error_code.
std::error_code equivalent(const Twine &lhs, const Twine &rhs, bool &result);

/// Simpler version of equivalent for clients that don't need to
///        differentiate between an error and false.
inline bool equivalent(const Twine &lhs, const Twine &rhs)
{
   bool result;
   return !equivalent(lhs, rhs, result) && result;
}

/// Is the file mounted on a local filesystem?
///
/// @param path Input path.
/// @param result Set to true if \a path is on fixed media such as a hard disk,
///               false if it is not.
/// @returns errc::success if result has been successfully set, otherwise a
///          platform specific error_code.
std::error_code is_local(const Twine &path, bool &result);

/// Version of is_local accepting an open file descriptor.
std::error_code is_local(int fd, bool &result);

/// Simpler version of is_local for clients that don't need to
///        differentiate between an error and false.
inline bool is_local(const Twine &path)
{
   bool result;
   return !is_local(path, result) && result;
}

/// Simpler version of is_local accepting an open file descriptor for
///        clients that don't need to differentiate between an error and false.
inline bool is_local(int fd)
{
   bool result;
   return !is_local(fd, result) && result;
}

/// Does status represent a directory?
///
/// @param Path The path to get the type of.
/// @param Follow For symbolic links, indicates whether to return the file type
///               of the link itself, or of the target.
/// @returns A value from the FileType enumeration indicating the type of file.
FileType get_file_type(const Twine &path, bool follow = true);

/// Does status represent a directory?
///
/// @param status A BasicFileStatus previously returned from status.
/// @returns status.type() == FileType::directory_file.
bool is_directory(const BasicFileStatus &status);

/// Is path a directory?
///
/// @param path Input path.
/// @param result Set to true if \a path is a directory (after following
///               symlinks, false if it is not. Undefined otherwise.
/// @returns errc::success if result has been successfully set, otherwise a
///          platform-specific error_code.
std::error_code is_directory(const Twine &path, bool &result);

/// Simpler version of is_directory for clients that don't need to
///        differentiate between an error and false.
inline bool is_directory(const Twine &path)
{
   bool result;
   return !is_directory(path, result) && result;
}

/// Does status represent a regular file?
///
/// @param status A BasicFileStatus previously returned from status.
/// @returns status_known(status) && status.type() == FileType::regular_file.
bool is_regular_file(const BasicFileStatus &status);

/// Is path a regular file?
///
/// @param path Input path.
/// @param result Set to true if \a path is a regular file (after following
///               symlinks), false if it is not. Undefined otherwise.
/// @returns errc::success if result has been successfully set, otherwise a
///          platform-specific error_code.
std::error_code is_regular_file(const Twine &path, bool &result);

/// Simpler version of is_regular_file for clients that don't need to
///        differentiate between an error and false.
inline bool is_regular_file(const Twine &path)
{
   bool result;
   if (is_regular_file(path, result)) {
      return false;
   }

   return result;
}

/// Does status represent a symlink file?
///
/// @param status A BasicFileStatus previously returned from status.
/// @returns status_known(status) && status.type() == FileType::symlink_file.
bool is_symlink_file(const BasicFileStatus &status);

/// Is path a symlink file?
///
/// @param path Input path.
/// @param result Set to true if \a path is a symlink file, false if it is not.
///               Undefined otherwise.
/// @returns errc::success if result has been successfully set, otherwise a
///          platform-specific error_code.
std::error_code is_symlink_file(const Twine &path, bool &result);

/// Simpler version of is_symlink_file for clients that don't need to
///        differentiate between an error and false.
inline bool is_symlink_file(const Twine &path)
{
   bool result;
   if (is_symlink_file(path, result)) {
      return false;
   }
   return result;
}

/// Does this status represent something that exists but is not a
///        directory or regular file?
///
/// @param status A BasicFileStatus previously returned from status.
/// @returns exists(s) && !is_regular_file(s) && !is_directory(s)
bool is_other(const BasicFileStatus &status);

/// Is path something that exists but is not a directory,
///        regular file, or symlink?
///
/// @param path Input path.
/// @param result Set to true if \a path exists, but is not a directory, regular
///               file, or a symlink, false if it does not. Undefined otherwise.
/// @returns errc::success if result has been successfully set, otherwise a
///          platform-specific error_code.
std::error_code is_other(const Twine &path, bool &result);

/// Get file status as if by POSIX stat().
///
/// @param path Input path.
/// @param result Set to the file status.
/// @param follow When true, follows symlinks.  Otherwise, the symlink itself is
///               statted.
/// @returns errc::success if result has been successfully set, otherwise a
///          platform-specific error_code.
std::error_code status(const Twine &path, FileStatus &result,
                       bool follow = true);

/// A version for when a file descriptor is already available.
std::error_code status(int fd, FileStatus &result);

/// Set file permissions.
///
/// @param Path File to set permissions on.
/// @param Permissions New file permissions.
/// @returns errc::success if the permissions were successfully set, otherwise
///          a platform-specific error_code.
/// @note On Windows, all permissions except *_write are ignored. Using any of
///       owner_write, group_write, or all_write will make the file writable.
///       Otherwise, the file will be marked as read-only.
std::error_code set_permissions(const Twine &path, Permission permissions);

/// Get file permissions.
///
/// @param Path File to get permissions from.
/// @returns the permissions if they were successfully retrieved, otherwise a
///          platform-specific error_code.
/// @note On Windows, if the file does not have the FILE_ATTRIBUTE_READONLY
///       attribute, all_all will be returned. Otherwise, all_read | all_exe
///       will be returned.
OptionalError<Permission> get_permissions(const Twine &path);

/// Get file size.
///
/// @param Path Input path.
/// @param Result Set to the size of the file in \a Path.
/// @returns errc::success if result has been successfully set, otherwise a
///          platform-specific error_code.
inline std::error_code file_size(const Twine &path, uint64_t &result)
{
   FileStatus fileStatus;
   std::error_code errorCode = status(path, fileStatus);
   if (errorCode) {
      return errorCode;
   }
   result = fileStatus.getSize();
   return std::error_code();
}

/// Set the file modification and access time.
///
/// @returns errc::success if the file times were successfully set, otherwise a
///          platform-specific error_code or errc::function_not_supported on
///          platforms where the functionality isn't available.
std::error_code set_last_access_and_modification_time(int fd, TimePoint<> time,
                                                      TimePoint<> modificationTime);

/// Simpler version that sets both file modification and access time to the same
/// time.
inline std::error_code set_last_access_and_modification_time(int fd,
                                                             TimePoint<> time)
{
   return set_last_access_and_modification_time(fd, time, time);
}

/// Is status available?
///
/// @param s Input file status.
/// @returns True if status() != status_error.
bool status_known(const BasicFileStatus &s);

/// Is status available?
///
/// @param path Input path.
/// @param result Set to true if status() != status_error.
/// @returns errc::success if result has been successfully set, otherwise a
///          platform-specific error_code.
std::error_code status_known(const Twine &path, bool &result);

enum CreationDisposition : unsigned
{
   /// CD_CreateAlways - When opening a file:
   ///   * If it already exists, truncate it.
   ///   * If it does not already exist, create a new file.
   CD_CreateAlways = 0,

   /// CD_CreateNew - When opening a file:
   ///   * If it already exists, fail.
   ///   * If it does not already exist, create a new file.
   CD_CreateNew = 1,

   /// CD_OpenExisting - When opening a file:
   ///   * If it already exists, open the file with the offset set to 0.
   ///   * If it does not already exist, fail.
   CD_OpenExisting = 2,

   /// CD_OpenAlways - When opening a file:
   ///   * If it already exists, open the file with the offset set to 0.
   ///   * If it does not already exist, create a new file.
   CD_OpenAlways = 3,
};

enum FileAccess : unsigned
{
   FA_Read = 1,
   FA_Write = 2,
};


enum OpenFlags : unsigned
{
   OF_None = 0,
   F_None = 0, // For compatibility

   /// The file should be opened in text mode on platforms that make this
   /// distinction.
   OF_Text = 1,
   F_Text = 1, // For compatibility

   /// The file should be opened in append mode.
   OF_Append = 2,
   F_Append = 2, // For compatibility

   /// Delete the file on close. Only makes a difference on windows.
   OF_Delete = 4,

   /// When a child process is launched, this file should remain open in the
   /// child process.
   OF_ChildInherit = 8,

   /// Force files Atime to be updated on access. Only makes a difference on windows.
   OF_UpdateAtime = 16,
};

/// Create a uniquely named file.
///
/// Generates a unique path suitable for a temporary file and then opens it as a
/// file. The name is based on \a model with '%' replaced by a random char in
/// [0-9a-f]. If \a model is not an absolute path, the temporary file will be
/// created in the current directory.
///
/// Example: clang-%%-%%-%%-%%-%%.s => clang-a0-b1-c2-d3-e4.s
///
/// This is an atomic operation. Either the file is created and opened, or the
/// file system is left untouched.
///
/// The intended use is for files that are to be kept, possibly after
/// renaming them. For example, when running 'clang -c foo.o', the file can
/// be first created as foo-abc123.o and then renamed.
///
/// @param Model Name to base unique path off of.
/// @param ResultFD Set to the opened file's file descriptor.
/// @param ResultPath Set to the opened file's absolute path.
/// @returns errc::success if Result{FD,Path} have been successfully set,
///          otherwise a platform-specific error_code.
std::error_code create_unique_file(const Twine &model, int &resultFD,
                                   SmallVectorImpl<char> &resultPath,
                                   unsigned mode = all_read | all_write);

/// Simpler version for clients that don't want an open file. An empty
/// file will still be created.
std::error_code create_unique_file(const Twine &model,
                                   SmallVectorImpl<char> &resultPath,
                                   unsigned mode = all_read | all_write);

/// Represents a temporary file.
///
/// The temporary file must be eventually discarded or given a final name and
/// kept.
///
/// The destructor doesn't implicitly discard because there is no way to
/// properly handle errors in a destructor.
class TempFile
{
   bool m_done = false;
   TempFile(StringRef name, int fd);

public:
   /// This creates a temporary file with createUniqueFile and schedules it for
   /// deletion with sys::RemoveFileOnSignal.
   static Expected<TempFile> create(const Twine &model,
                                    unsigned mode = all_read | all_write);
   TempFile(TempFile &&other);
   TempFile &operator=(TempFile &&other);

   // Name of the temporary file.
   std::string m_tmpName;

   // The open file descriptor.
   int m_fd = -1;

   // Keep this with the given name.
   Error keep(const Twine &name);

   // Keep this with the temporary name.
   Error keep();

   // Delete the file.
   Error discard();

   // This checks that keep or delete was called.
   ~TempFile();
};

/// Create a file in the system temporary directory.
///
/// The filename is of the form prefix-random_chars.suffix. Since the directory
/// is not know to the caller, Prefix and Suffix cannot have path separators.
/// The files are created with mode 0600.
///
/// This should be used for things like a temporary .s that is removed after
/// running the assembler.
std::error_code create_temporary_file(const Twine &prefix, StringRef suffix,
                                      int &resultFD,
                                      SmallVectorImpl<char> &resultPath);

/// Simpler version for clients that don't want an open file. An empty
/// file will still be created.
std::error_code create_temporary_file(const Twine &prefix, StringRef suffix,
                                      SmallVectorImpl<char> &resultPath);

std::error_code create_unique_directory(const Twine &prefix,
                                        SmallVectorImpl<char> &resultPath);

/// Get a unique name, not currently exisiting in the filesystem. Subject
/// to race conditions, prefer to use createUniqueFile instead.
///
/// Similar to createUniqueFile, but instead of creating a file only
/// checks if it exists. This function is subject to race conditions, if you
/// want to use the returned name to actually create a file, use
/// createUniqueFile instead.
std::error_code get_potentially_unique_filename(const Twine &model,
                                                SmallVectorImpl<char> &resultPath);

/// Get a unique temporary file name, not currently exisiting in the
/// filesystem. Subject to race conditions, prefer to use createTemporaryFile
/// instead.
///
/// Similar to createTemporaryFile, but instead of creating a file only
/// checks if it exists. This function is subject to race conditions, if you
/// want to use the returned name to actually create a file, use
/// createTemporaryFile instead.
std::error_code
get_potentially_unique_temp_filename(const Twine &prefix, StringRef suffix,
                                     SmallVectorImpl<char> &resultPath);

inline OpenFlags operator|(OpenFlags lhs, OpenFlags rhs)
{
   return OpenFlags(unsigned(lhs) | unsigned(rhs));
}

inline OpenFlags &operator|=(OpenFlags &lhs, OpenFlags rhs)
{
   lhs = lhs | rhs;
   return lhs;
}

inline FileAccess operator|(FileAccess lhs, FileAccess rhs)
{
   return FileAccess(unsigned(lhs) | unsigned(rhs));
}

inline FileAccess &operator|=(FileAccess &lhs, FileAccess rhs)
{
   lhs = lhs | rhs;
   return lhs;
}

/// @brief Opens a file with the specified creation disposition, access mode,
/// and flags and returns a file descriptor.
///
/// The caller is responsible for closing the file descriptor once they are
/// finished with it.
///
/// @param Name The path of the file to open, relative or absolute.
/// @param ResultFD If the file could be opened successfully, its descriptor
///                 is stored in this location. Otherwise, this is set to -1.
/// @param Disp Value specifying the existing-file behavior.
/// @param Access Value specifying whether to open the file in read, write, or
///               read-write mode.
/// @param Flags Additional flags.
/// @param Mode The access permissions of the file, represented in octal.
/// @returns errc::success if \a Name has been opened, otherwise a
///          platform-specific error_code.
std::error_code open_file(const Twine &name, int &resultFD,
                          CreationDisposition disp, FileAccess access,
                          OpenFlags flags, unsigned mode = 0666);

/// @brief Opens a file with the specified creation disposition, access mode,
/// and flags and returns a platform-specific file object.
///
/// The caller is responsible for closing the file object once they are
/// finished with it.
///
/// @param Name The path of the file to open, relative or absolute.
/// @param Disp Value specifying the existing-file behavior.
/// @param Access Value specifying whether to open the file in read, write, or
///               read-write mode.
/// @param Flags Additional flags.
/// @param Mode The access permissions of the file, represented in octal.
/// @returns errc::success if \a Name has been opened, otherwise a
///          platform-specific error_code.
Expected<file_t> open_native_file(const Twine &name, CreationDisposition disp,
                                  FileAccess access, OpenFlags flags,
                                  unsigned mode = 0666);


/// @brief Opens the file with the given name in a write-only or read-write
/// mode, returning its open file descriptor. If the file does not exist, it
/// is created.
///
/// The caller is responsible for closing the file descriptor once they are
/// finished with it.
///
/// @param Name The path of the file to open, relative or absolute.
/// @param ResultFD If the file could be opened successfully, its descriptor
///                 is stored in this location. Otherwise, this is set to -1.
/// @param Flags Additional flags used to determine whether the file should be
///              opened in, for example, read-write or in write-only mode.
/// @param Mode The access permissions of the file, represented in octal.
/// @returns errc::success if \a Name has been opened, otherwise a
///          platform-specific error_code.
inline std::error_code open_file_for_write(const Twine &name, int &resultFD,
                                           CreationDisposition disp = CD_CreateAlways,
                                           OpenFlags flags = OF_None, unsigned mode = 0666)
{
   return open_file(name, resultFD, disp, FA_Write, flags, mode);
}

/// @brief Opens the file with the given name in a write-only or read-write
/// mode, returning its open file descriptor. If the file does not exist, it
/// is created.
///
/// The caller is responsible for closing the freeing the file once they are
/// finished with it.
///
/// @param Name The path of the file to open, relative or absolute.
/// @param Flags Additional flags used to determine whether the file should be
///              opened in, for example, read-write or in write-only mode.
/// @param Mode The access permissions of the file, represented in octal.
/// @returns a platform-specific file descriptor if \a Name has been opened,
///          otherwise an error object.
inline Expected<file_t> open_native_file_for_write(const Twine &name,
                                                   CreationDisposition disp,
                                                   OpenFlags flags,
                                                   unsigned mode = 0666)
{
   return open_native_file(name, disp, FA_Write, flags, mode);
}

/// @brief Opens the file with the given name in a write-only or read-write
/// mode, returning its open file descriptor. If the file does not exist, it
/// is created.
///
/// The caller is responsible for closing the file descriptor once they are
/// finished with it.
///
/// @param Name The path of the file to open, relative or absolute.
/// @param ResultFD If the file could be opened successfully, its descriptor
///                 is stored in this location. Otherwise, this is set to -1.
/// @param Flags Additional flags used to determine whether the file should be
///              opened in, for example, read-write or in write-only mode.
/// @param Mode The access permissions of the file, represented in octal.
/// @returns errc::success if \a Name has been opened, otherwise a
///          platform-specific error_code.
inline std::error_code open_file_for_read_write(const Twine &name, int &resultFD,
                                                CreationDisposition disp,
                                                OpenFlags flags,
                                                unsigned mode = 0666) {
   return open_file(name, resultFD, disp, FA_Write | FA_Read, flags, mode);
}

/// @brief Opens the file with the given name in a write-only or read-write
/// mode, returning its open file descriptor. If the file does not exist, it
/// is created.
///
/// The caller is responsible for closing the freeing the file once they are
/// finished with it.
///
/// @param Name The path of the file to open, relative or absolute.
/// @param Flags Additional flags used to determine whether the file should be
///              opened in, for example, read-write or in write-only mode.
/// @param Mode The access permissions of the file, represented in octal.
/// @returns a platform-specific file descriptor if \a Name has been opened,
///          otherwise an error object.
inline Expected<file_t> open_native_file_for_read_write(const Twine &name,
                                                        CreationDisposition disp,
                                                        OpenFlags flags,
                                                        unsigned mode = 0666)
{
   return open_native_file(name, disp, FA_Write | FA_Read, flags, mode);
}



/// @brief Opens the file with the given name in a read-only mode, returning
/// its open file descriptor.
///
/// The caller is responsible for closing the file descriptor once they are
/// finished with it.
///
/// @param Name The path of the file to open, relative or absolute.
/// @param ResultFD If the file could be opened successfully, its descriptor
///                 is stored in this location. Otherwise, this is set to -1.
/// @param RealPath If nonnull, extra work is done to determine the real path
///                 of the opened file, and that path is stored in this
///                 location.
/// @returns errc::success if \a Name has been opened, otherwise a
///          platform-specific error_code.
std::error_code open_file_for_read(const Twine &name, int &resultFD,
                                   OpenFlags flags = OF_None,
                                   SmallVectorImpl<char> *realPath = nullptr);

/// @brief Opens the file with the given name in a read-only mode, returning
/// its open file descriptor.
///
/// The caller is responsible for closing the freeing the file once they are
/// finished with it.
///
/// @param Name The path of the file to open, relative or absolute.
/// @param RealPath If nonnull, extra work is done to determine the real path
///                 of the opened file, and that path is stored in this
///                 location.
/// @returns a platform-specific file descriptor if \a Name has been opened,
///          otherwise an error object.
Expected<file_t>
open_native_file_for_read(const Twine &name, OpenFlags flags = OF_None,
                          SmallVectorImpl<char> *realPath = nullptr);

/// @brief Close the file object.  This should be used instead of ::close for
/// portability.
///
/// @param F On input, this is the file to close.  On output, the file is
/// set to kInvalidFile.
void close_file(file_t &file);

std::error_code get_unique_id(const Twine path, UniqueId &result);

/// Get disk space usage information.
///
/// Note: Users must be careful about "Time Of Check, Time Of Use" kind of bug.
/// Note: Windows reports results according to the quota allocated to the user.
///
/// @param Path Input path.
/// @returns a SpaceInfo structure filled with the capacity, free, and
/// available space on the device \a Path is on. A platform specific error_code
/// is returned on error.
OptionalError<SpaceInfo> disk_space(const Twine &path);

/// This class represents a memory mapped file. It is based on
/// boost::iostreams::mapped_file.
class MappedFileRegion
{
public:
   enum MapMode {
      readonly, ///< May only access map via const_data as read only.
      readwrite, ///< May access map via data and modify it. Written to path.
      priv ///< May modify via data, but changes are lost on destruction.
   };

private:
   /// Platform-specific mapping state.
   size_t m_size;
   void *m_mapping;
   int m_fd;
   MapMode m_mode;

   std::error_code init(int fd, uint64_t offset, MapMode mode);

public:
   MappedFileRegion() = delete;
   MappedFileRegion(MappedFileRegion&) = delete;
   MappedFileRegion &operator =(MappedFileRegion&) = delete;

   /// \param fd An open file descriptor to map. MappedFileRegion takes
   ///   ownership if closefd is true. It must have been opended in the correct
   ///   mode.
   MappedFileRegion(int fd, MapMode mode, size_t length, uint64_t offset,
                    std::error_code &errorCode);

   ~MappedFileRegion();

   size_t getSize() const;
   char *getData() const;

   /// Get a const view of the data. Modifying this memory has undefined
   /// behavior.
   const char *getConstData() const;

   /// \returns The minimum alignment offset must be.
   static int getAlignment();
};

/// Return the path to the main executable, given the value of argv[0] from
/// program startup and the address of main itself. In extremis, this function
/// may fail and return an empty path.
std::string get_main_executable(const char *argv0, void *mainExecAddr);

/// @}
/// @name Iterators
/// @{

/// directory_entry - A single entry in a directory.
class DirectoryEntry
{
   // FIXME: different platforms make different information available "for free"
   // when traversing a directory. The design of this class wraps most of the
   // information in basic_file_status, so on platforms where we can't populate
   // that whole structure, callers end up paying for a stat().
   // std::filesystem::directory_entry may be a better model.
   std::string m_path;
   FileType m_type;           // Most platforms can provide this.
   bool m_followSymlinks;     // Affects the behavior of status().
   BasicFileStatus m_status;  // If available.

public:
   explicit DirectoryEntry(const Twine &path, bool followSymlinks = true,
                           FileType type = FileType::type_unknown,
                           BasicFileStatus fileStatus = BasicFileStatus())
      : m_path(path.getStr()),
        m_type(type),
        m_followSymlinks(followSymlinks),
        m_status(fileStatus)
   {}

   DirectoryEntry() = default;

   void replaceFilename(const Twine &filename, FileType type,
                        BasicFileStatus status = BasicFileStatus());

   const std::string &getPath() const
   {
      return m_path;
   }

   // Get basic information about entry file (a subset of fs::status()).
   // On most platforms this is a stat() call.
   // On windows the information was already retrieved from the directory.
   OptionalError<BasicFileStatus> getStatus() const;

   // Get the type of this file.
   // On most platforms (Linux/Mac/Windows/BSD), this was already retrieved.
   // On some platforms (e.g. Solaris) this is a stat() call.
   FileType getType() const
   {
      if (m_type != FileType::type_unknown) {
         return m_type;
      }
      auto status = getStatus();
      return status ? status->getType() : FileType::type_unknown;
   }

   bool operator==(const DirectoryEntry &other) const
   {
      return m_path == other.m_path;
   }

   bool operator!=(const DirectoryEntry &other) const
   {
      return !(*this == other);
   }

   bool operator< (const DirectoryEntry &other) const;
   bool operator<=(const DirectoryEntry &other) const;
   bool operator> (const DirectoryEntry &other) const;
   bool operator>=(const DirectoryEntry &other) const;
};

namespace internal {

struct DirIterState;

std::error_code directory_iterator_construct(DirIterState &, StringRef, bool);
std::error_code directory_iterator_increment(DirIterState &);
std::error_code directory_iterator_destruct(DirIterState &);

/// Keeps state for the DirectoryIterator.
struct DirIterState {
   ~DirIterState() {
      directory_iterator_destruct(*this);
   }

   intptr_t m_iterationHandle = 0;
   DirectoryEntry m_currentEntry;
};

} // end namespace internal

/// DirectoryIterator - Iterates through the entries in path. There is no
/// operator++ because we need an error_code. If it's really needed we can make
/// it call report_fatal_error on error.
class DirectoryIterator
{
   std::shared_ptr<internal::DirIterState> m_state;
   bool m_followSymlinks = true;

public:
   explicit DirectoryIterator(const Twine &path, std::error_code &errorCode,
                              bool followSymlinks = true)
      : m_followSymlinks(followSymlinks)
   {
      m_state = std::make_shared<internal::DirIterState>();
      SmallString<128> pathStorage;
      errorCode = internal::directory_iterator_construct(
               *m_state, path.toStringRef(pathStorage), m_followSymlinks);
   }

   explicit DirectoryIterator(const DirectoryEntry &direEntry, std::error_code &errorCode,
                              bool followSymlinks = true)
      : m_followSymlinks(followSymlinks)
   {
      m_state = std::make_shared<internal::DirIterState>();
      errorCode = internal::directory_iterator_construct(
               *m_state, direEntry.getPath(), m_followSymlinks);
   }

   /// Construct end iterator.
   DirectoryIterator() = default;

   // No operator++ because we need error_code.
   DirectoryIterator &increment(std::error_code &errorCode)
   {
      errorCode = internal::directory_iterator_increment(*m_state);
      return *this;
   }

   const DirectoryEntry &operator*() const
   {
      return m_state->m_currentEntry;
   }

   const DirectoryEntry *operator->() const
   {
      return &m_state->m_currentEntry;
   }

   bool operator==(const DirectoryIterator &other) const
   {
      if (m_state == other.m_state) {
         return true;
      }
      if (!other.m_state)
         return m_state->m_currentEntry == DirectoryEntry();
      if (!m_state)
         return other.m_state->m_currentEntry == DirectoryEntry();
      return m_state->m_currentEntry == other.m_state->m_currentEntry;
   }

   bool operator!=(const DirectoryIterator &other) const
   {
      return !(*this == other);
   }
};

namespace internal {

/// Keeps state for the RecursiveDirectoryIterator.
struct RecDirIterState
{
   std::stack<DirectoryIterator, std::vector<DirectoryIterator>> m_stack;
   uint16_t m_level = 0;
   bool m_hasNoPushRequest = false;
};

} // end namespace internal

/// RecursiveDirectoryIterator - Same as DirectoryIterator except for it
/// recurses down into child directories.
class RecursiveDirectoryIterator
{
   std::shared_ptr<internal::RecDirIterState> m_state;
   bool m_follow;

public:
   RecursiveDirectoryIterator() = default;
   explicit RecursiveDirectoryIterator(const Twine &path, std::error_code &errorCode,
                                       bool followSymlinks = true)
      : m_state(std::make_shared<internal::RecDirIterState>()),
        m_follow(followSymlinks)
   {
      m_state->m_stack.push(DirectoryIterator(path, errorCode, m_follow));
      if (m_state->m_stack.top() == DirectoryIterator()) {
         m_state.reset();
      }
   }

   // No operator++ because we need error_code.
   RecursiveDirectoryIterator &increment(std::error_code &errorCode)
   {
      const DirectoryIterator endIter = {};

      if (m_state->m_hasNoPushRequest) {
         m_state->m_hasNoPushRequest = false;
      } else {
         FileType type = m_state->m_stack.top()->getType();
         if (type == FileType::symlink_file && m_follow) {
            // Resolve the symlink: is it a directory to recurse into?
            OptionalError<BasicFileStatus> status = m_state->m_stack.top()->getStatus();
            if (status) {
                type = status->getType();
            }
            // Otherwise broken symlink, and we'll continue.
         }
         if (type == FileType::directory_file) {
            m_state->m_stack.push(DirectoryIterator(*m_state->m_stack.top(), errorCode, m_follow));
            if (m_state->m_stack.top() != endIter) {
               ++m_state->m_level;
               return *this;
            }
            m_state->m_stack.pop();
         }
      }

      while (!m_state->m_stack.empty()
             && m_state->m_stack.top().increment(errorCode) == endIter) {
         m_state->m_stack.pop();
         --m_state->m_level;
      }

      // Check if we are done. If so, create an end iterator.
      if (m_state->m_stack.empty()) {
         m_state.reset();
      }
      return *this;
   }

   const DirectoryEntry &operator*() const
   {
      return *m_state->m_stack.top();
   }

   const DirectoryEntry *operator->() const
   {
      return &*m_state->m_stack.top();
   }

   // observers
   /// Gets the current level. Starting path is at level 0.
   int getLevel() const
   {
      return m_state->m_level;
   }

   /// Returns true if no_push has been called for this DirectoryEntry.
   bool noPushRequest() const
   {
      return m_state->m_hasNoPushRequest;
   }

   // modifiers
   /// Goes up one level if Level > 0.
   void pop()
   {
      assert(m_state && "Cannot pop an end iterator!");
      assert(m_state->m_level > 0 && "Cannot pop an iterator with level < 1");

      const DirectoryIterator endIter = {};
      std::error_code errorCode;
      do {
         if (errorCode) {
            polar::utils::report_fatal_error("Error incrementing directory iterator.");
         }

         m_state->m_stack.pop();
         --m_state->m_level;
      } while (!m_state->m_stack.empty()
               && m_state->m_stack.top().increment(errorCode) == endIter);

      // Check if we are done. If so, create an end iterator.
      if (m_state->m_stack.empty()) {
         m_state.reset();
      }
   }

   /// Does not go down into the current DirectoryEntry.
   void noPush()
   {
      m_state->m_hasNoPushRequest = true;
   }

   bool operator==(const RecursiveDirectoryIterator &other) const
   {
      return m_state == other.m_state;
   }

   bool operator!=(const RecursiveDirectoryIterator &other) const
   {
      return !(*this == other);
   }
};

/// @}

} // fs
} // polar

#endif // POLARPHP_UTILS_ERROR_FILESYSTEM_H
