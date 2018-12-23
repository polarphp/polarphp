// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/07.
//===- VirtualFileSystem.h - Virtual File System Layer ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// Defines the virtual file system interface vfs::FileSystem.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_VIRTUAL_FILESYSTEM_H
#define POLARPHP_UTILS_VIRTUAL_FILESYSTEM_H

#include "polarphp/basic/adt/IntrusiveRefCountPtr.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/Chrono.h"
#include "polarphp/utils/OptionalError.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/SourceMgr.h"

#include <cassert>
#include <cstdint>
#include <ctime>
#include <memory>
#include <stack>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

// forward declared class with namespace
namespace polar {
namespace utils {
class MemoryBuffer;
} // utils
} // polar

namespace polar {
namespace vfs {

using polar::basic::StringRef;
using polar::basic::Twine;
using polar::basic::SmallVectorImpl;
using polar::basic::ThreadSafeRefCountedBase;
using polar::basic::IntrusiveRefCountPtr;
using polar::basic::SmallVector;
using polar::utils::MemoryBuffer;
using polar::utils::OptionalError;
using polar::utils::SourceMgr;
using polar::utils::RawOutStream;

/// The result of a \p status operation.
class Status
{
   std::string m_name;
   polar::fs::UniqueId m_uid;
   polar::utils::TimePoint<> m_mtime;
   uint32_t m_user;
   uint32_t m_group;
   uint64_t m_size;
   polar::fs::FileType m_type = polar::fs::FileType::status_error;
   polar::fs::Permission m_perms;

public:
   // FIXME: remove when files support multiple names
   bool IsVFSMapped = false;

   Status() = default;
   Status(const polar::fs::FileStatus &status);
   Status(StringRef name, polar::fs::UniqueId uid,
          polar::utils::TimePoint<> mtime, uint32_t user, uint32_t group,
          uint64_t size, polar::fs::FileType type,
          polar::fs::Permission perms);

   /// Get a copy of a Status with a different name.
   static Status copyWithNewName(const Status &in, StringRef newName);
   static Status copyWithNewName(const polar::fs::FileStatus &in,
                                 StringRef newName);

   /// Returns the name that should be used for this file or directory.
   StringRef getName() const
   {
      return m_name;
   }

   /// @name Status interface from llvm::sys::fs
   /// @{
   polar::fs::FileType getType() const
   {
      return m_type;
   }

   polar::fs::Permission getPermissions() const
   {
      return m_perms;
   }

   polar::utils::TimePoint<> getLastModificationTime() const
   {
      return m_mtime;
   }

   polar::fs::UniqueId getUniqueId() const
   {
      return m_uid;
   }

   uint32_t getUser() const
   {
      return m_user;
   }

   uint32_t getGroup() const
   {
      return m_group;
   }

   uint64_t getSize() const
   {
      return m_size;
   }

   /// @}
   /// @name Status queries
   /// These are static queries in llvm::sys::fs.
   /// @{
   bool equivalent(const Status &other) const;
   bool isDirectory() const;
   bool isRegularFile() const;
   bool isOther() const;
   bool isSymlink() const;
   bool isStatusKnown() const;
   bool exists() const;
   /// @}
};

/// Represents an open file.
class File
{
public:
   /// Destroy the file after closing it (if open).
   /// Sub-classes should generally call close() inside their destructors.  We
   /// cannot do that from the base class, since close is virtual.
   virtual ~File();

   /// Get the status of the file.
   virtual OptionalError<Status> getStatus() = 0;

   /// Get the name of the file
   virtual OptionalError<std::string> getName()
   {
      if (auto Status = getStatus()) {
         return Status->getName().getStr();
      } else {
          return Status.getError();
      }
   }

   /// Get the contents of the file as a \p MemoryBuffer.
   virtual OptionalError<std::unique_ptr<MemoryBuffer>>
   getBuffer(const Twine &name, int64_t fileSize = -1,
             bool requiresNullTerminator = true, bool isVolatile = false) = 0;

   /// Closes the file.
   virtual std::error_code close() = 0;
};

/// A member of a directory, yielded by a DirectoryIterator.
/// Only information available on most platforms is included.
class DirectoryEntry
{
   std::string m_path;
   polar::fs::FileType m_type;

public:
   DirectoryEntry() = default;
   DirectoryEntry(std::string path, polar::fs::FileType type)
      : m_path(std::move(path)),
        m_type(type)
   {}

   StringRef path() const
   {
      return m_path;
   }

   polar::fs::FileType type() const
   {
      return m_type;
   }
};

namespace internal {

/// An interface for virtual file systems to provide an iterator over the
/// (non-recursive) contents of a directory.
struct DirIterImpl
{
   virtual ~DirIterImpl();

   /// Sets \c m_currentEntry to the next entry in the directory on success,
   /// to DirectoryEntry() at end,  or returns a system-defined \c error_code.
   virtual std::error_code increment() = 0;

   DirectoryEntry m_currentEntry;
};

} // namespace internal

/// An input iterator over the entries in a virtual path, similar to
/// llvm::sys::fs::DirectoryIterator.
class DirectoryIterator
{
   std::shared_ptr<internal::DirIterImpl> m_impl; // Input iterator semantics on copy

public:
   DirectoryIterator(std::shared_ptr<internal::DirIterImpl> impl)
      : m_impl(std::move(impl))
   {
      assert(m_impl.get() != nullptr && "requires non-null implementation");
      if (m_impl->m_currentEntry.path().empty()) {
         m_impl.reset(); // Normalize the end iterator to m_impl == nullptr.
      }
   }

   /// Construct an 'end' iterator.
   DirectoryIterator() = default;

   /// Equivalent to operator++, with an error code.
   DirectoryIterator &increment(std::error_code &errorCode)
   {
      assert(m_impl && "attempting to increment past end");
      errorCode = m_impl->increment();
      if (m_impl->m_currentEntry.path().empty()) {
          m_impl.reset(); // Normalize the end iterator to m_impl == nullptr.
      }
      return *this;
   }

   const DirectoryEntry &operator*() const
   {
      return m_impl->m_currentEntry;
   }

   const DirectoryEntry *operator->() const
   {
      return &m_impl->m_currentEntry;
   }

   bool operator==(const DirectoryIterator &other) const
   {
      if (m_impl && other.m_impl) {
         return m_impl->m_currentEntry.path() == other.m_impl->m_currentEntry.path();
      }
      return !m_impl && !other.m_impl;
   }

   bool operator!=(const DirectoryIterator &other) const
   {
      return !(*this == other);
   }
};

class FileSystem;

namespace internal {

/// Keeps state for the RecursiveDirectoryIterator.
struct RecDirIterState
{
   std::stack<DirectoryIterator, std::vector<DirectoryIterator>> Stack;
   bool HasNoPushRequest = false;
};

} // end namespace internal

/// An input iterator over the recursive contents of a virtual path,
/// similar to llvm::sys::fs::RecursiveDirectoryIterator.
class RecursiveDirectoryIterator
{
   FileSystem *m_fs;
   std::shared_ptr<internal::RecDirIterState>
   m_state; // Input iterator semantics on copy.

public:
   RecursiveDirectoryIterator(FileSystem &fs, const Twine &path,
                                std::error_code &errorCode);

   /// Construct an 'end' iterator.
   RecursiveDirectoryIterator() = default;

   /// Equivalent to operator++, with an error code.
   RecursiveDirectoryIterator &increment(std::error_code &errorCode);

   const DirectoryEntry &operator*() const
   {
      return *m_state->Stack.top();
   }

   const DirectoryEntry *operator->() const
   {
      return &*m_state->Stack.top();
   }

   bool operator==(const RecursiveDirectoryIterator &other) const
   {
      return m_state == other.m_state; // identity
   }

   bool operator!=(const RecursiveDirectoryIterator &other) const
   {
      return !(*this == other);
   }

   /// Gets the current level. Starting path is at level 0.
   int level() const
   {
      assert(!m_state->Stack.empty() &&
             "Cannot get level without any iteration state");
      return m_state->Stack.size() - 1;
   }

   void noPush()
   {
      m_state->HasNoPushRequest = true;
   }
};

/// The virtual file system interface.
class FileSystem : public ThreadSafeRefCountedBase<FileSystem>
{
public:
   virtual ~FileSystem();

   /// Get the status of the entry at \p path, if one exists.
   virtual OptionalError<Status> getStatus(const Twine &path) = 0;

   /// Get a \p File object for the file at \p path, if one exists.
   virtual OptionalError<std::unique_ptr<File>>
   openFileForRead(const Twine &path) = 0;

   /// This is a convenience method that opens a file, gets its content and then
   /// closes the file.
   OptionalError<std::unique_ptr<MemoryBuffer>>
   getBufferForFile(const Twine &name, int64_t fileSize = -1,
                    bool requiresNullTerminator = true, bool isVolatile = false);

   /// Get a DirectoryIterator for \p dir.
   /// \note The 'end' iterator is DirectoryIterator().
   virtual DirectoryIterator dirBegin(const Twine &dir,
                                        std::error_code &errorCode) = 0;

   /// Set the working directory. This will affect all following operations on
   /// this file system and may propagate down for nested file systems.
   virtual std::error_code setCurrentWorkingDirectory(const Twine &path) = 0;

   /// Get the working directory of this file system.
   virtual OptionalError<std::string> getCurrentWorkingDirectory() const = 0;

   /// Gets real path of \p path e.g. collapse all . and .. patterns, resolve
   /// symlinks. For real file system, this uses `llvm::sys::fs::real_path`.
   /// This returns errc::operation_not_permitted if not implemented by subclass.
   virtual std::error_code getRealPath(const Twine &path,
                                       SmallVectorImpl<char> &output) const;

   /// Check whether a file exists. Provided for convenience.
   bool exists(const Twine &path);

   /// Is the file mounted on a local filesystem?
   virtual std::error_code isLocal(const Twine &path, bool &result);

   /// Make \a path an absolute path.
   ///
   /// Makes \a path absolute using the current directory if it is not already.
   /// An empty \a path will result in the current directory.
   ///
   /// /absolute/path   => /absolute/path
   /// relative/../path => <current-directory>/relative/../path
   ///
   /// \param path A path that is modified to be an absolute path.
   /// \returns success if \a path has been made absolute, otherwise a
   ///          platform-specific error_code.
   std::error_code makeAbsolute(SmallVectorImpl<char> &path) const;
};

/// Gets an \p vfs::FileSystem for the 'real' file system, as seen by
/// the operating system.
IntrusiveRefCountPtr<FileSystem> get_real_file_system();

/// A file system that allows overlaying one \p AbstractFileSystem on top
/// of another.
///
/// Consists of a stack of >=1 \p FileSystem objects, which are treated as being
/// one merged file system. When there is a directory that exists in more than
/// one file system, the \p OverlayFileSystem contains a directory containing
/// the union of their contents.  The attributes (permissions, etc.) of the
/// top-most (most recently added) directory are used.  When there is a file
/// that exists in more than one file system, the file in the top-most file
/// system overrides the other(s).
class OverlayFileSystem : public FileSystem
{
   using FileSystemList = SmallVector<IntrusiveRefCountPtr<FileSystem>, 1>;

   /// The stack of file systems, implemented as a list in order of
   /// their addition.
   FileSystemList m_fsList;

public:
   OverlayFileSystem(IntrusiveRefCountPtr<FileSystem> base);

   /// Pushes a file system on top of the stack.
   void pushOverlay(IntrusiveRefCountPtr<FileSystem> fs);

   OptionalError<Status> getStatus(const Twine &path) override;
   OptionalError<std::unique_ptr<File>>
   openFileForRead(const Twine &path) override;
   DirectoryIterator dirBegin(const Twine &dir, std::error_code &errorCode) override;
   OptionalError<std::string> getCurrentWorkingDirectory() const override;
   std::error_code setCurrentWorkingDirectory(const Twine &path) override;
   std::error_code isLocal(const Twine &path, bool &result) override;
   std::error_code getRealPath(const Twine &path,
                               SmallVectorImpl<char> &output) const override;

   using iterator = FileSystemList::reverse_iterator;
   using const_iterator = FileSystemList::const_reverse_iterator;

   /// Get an iterator pointing to the most recently added file system.
   iterator overlaysBegin()
   {
      return m_fsList.rbegin();
   }

   const_iterator overlaysBegin() const
   {
      return m_fsList.rbegin();
   }

   /// Get an iterator pointing one-past the least recently added file
   /// system.
   iterator overlaysEnd()
   {
      return m_fsList.rend();
   }

   const_iterator overlaysEnd() const
   {
      return m_fsList.rend();
   }
};

/// By default, this delegates all calls to the underlying file system. This
/// is useful when derived file systems want to override some calls and still
/// proxy other calls.
class ProxyFileSystem : public FileSystem
{
public:
   explicit ProxyFileSystem(IntrusiveRefCountPtr<FileSystem> fs)
      : m_fs(std::move(fs))
   {}

   OptionalError<Status> getStatus(const Twine &path) override
   {
      return m_fs->getStatus(path);
   }

   OptionalError<std::unique_ptr<File>>
   openFileForRead(const Twine &path) override
   {
      return m_fs->openFileForRead(path);
   }

   DirectoryIterator dirBegin(const Twine &dir, std::error_code &errorCode) override
   {
      return m_fs->dirBegin(dir, errorCode);
   }

   OptionalError<std::string> getCurrentWorkingDirectory() const override
   {
      return m_fs->getCurrentWorkingDirectory();
   }

   std::error_code setCurrentWorkingDirectory(const Twine &path) override
   {
      return m_fs->setCurrentWorkingDirectory(path);
   }

   std::error_code getRealPath(const Twine &path,
                               SmallVectorImpl<char> &output) const override
   {
      return m_fs->getRealPath(path, output);
   }

protected:
   FileSystem &getUnderlyingFs()
   {
      return *m_fs;
   }

private:
   IntrusiveRefCountPtr<FileSystem> m_fs;
};

namespace internal {

class InMemoryDirectory;
class InMemoryFile;

} // namespace internal

/// An in-memory file system.
class InMemoryFileSystem : public FileSystem
{
   std::unique_ptr<internal::InMemoryDirectory> m_root;
   std::string m_workingDirectory;
   bool m_useNormalizedPaths = true;

   /// If hardLinkTarget is non-null, a hardlink is created to the to path which
   /// must be a file. If it is null then it adds the file as the public addFile.
   bool addFile(const Twine &path, time_t modificationTime,
                std::unique_ptr<MemoryBuffer> buffer,
                std::optional<uint32_t> user, std::optional<uint32_t> group,
                std::optional<polar::fs::FileType> type,
                std::optional<polar::fs::Permission> perms,
                const internal::InMemoryFile *hardLinkTarget);

public:
   explicit InMemoryFileSystem(bool useNormalizedPaths = true);
   ~InMemoryFileSystem() override;

   /// Add a file containing a buffer or a directory to the VFS with a
   /// path. The VFS owns the buffer.  If present, user, group, type
   /// and perms apply to the newly-created file or directory.
   /// \return true if the file or directory was successfully added,
   /// false if the file or directory already exists in the file system with
   /// different contents.
   bool addFile(const Twine &path, time_t modificationTime,
                std::unique_ptr<MemoryBuffer> buffer,
                std::optional<uint32_t> user = std::nullopt, std::optional<uint32_t> group = std::nullopt,
                std::optional<polar::fs::FileType> type = std::nullopt,
                std::optional<polar::fs::Permission> perms = std::nullopt);

   /// Add a hard link to a file.
   /// Here hard links are not intended to be fully equivalent to the classical
   /// filesystem. Both the hard link and the file share the same buffer and
   /// status (and thus have the same UniqueID). Because of this there is no way
   /// to distinguish between the link and the file after the link has been
   /// added.
   ///
   /// The to path must be an existing file or a hardlink. The from file must not
   /// have been added before. The to path must not be a directory. The from Node
   /// is added as a hard link which points to the resolved file of to Node.
   /// \return true if the above condition is satisfied and hardlink was
   /// successfully created, false otherwise.
   bool addHardLink(const Twine &from, const Twine &to);

   /// Add a buffer to the VFS with a path. The VFS does not own the buffer.
   /// If present, user, group, type and perms apply to the newly-created file
   /// or directory.
   /// \return true if the file or directory was successfully added,
   /// false if the file or directory already exists in the file system with
   /// different contents.
   bool addFileNoOwn(const Twine &path, time_t modificationTime,
                     MemoryBuffer *buffer, std::optional<uint32_t> user = std::nullopt,
                     std::optional<uint32_t> group = std::nullopt,
                     std::optional<polar::fs::FileType> type = std::nullopt,
                     std::optional<polar::fs::Permission> perms = std::nullopt);

   std::string toString() const;

   /// Return true if this file system normalizes . and .. in paths.
   bool useNormalizedPaths() const
   {
      return m_useNormalizedPaths;
   }

   OptionalError<Status> getStatus(const Twine &path) override;
   OptionalError<std::unique_ptr<File>>
   openFileForRead(const Twine &path) override;
   DirectoryIterator dirBegin(const Twine &dir, std::error_code &errorCode) override;

   OptionalError<std::string> getCurrentWorkingDirectory() const override
   {
      return m_workingDirectory;
   }
   /// Canonicalizes \p path by combining with the current working
   /// directory and normalizing the path (e.g. remove dots). If the current
   /// working directory is not set, this returns errc::operation_not_permitted.
   ///
   /// This doesn't resolve symlinks as they are not supported in in-memory file
   /// system.
   std::error_code getRealPath(const Twine &path,
                               SmallVectorImpl<char> &output) const override;
   std::error_code isLocal(const Twine &path, bool &result) override;
   std::error_code setCurrentWorkingDirectory(const Twine &path) override;
};

/// Get a globally unique ID for a virtual file or directory.
polar::fs::UniqueId get_next_virtual_unique_id();

/// Gets a \p FileSystem for a virtual file system described in YAML
/// format.
IntrusiveRefCountPtr<FileSystem>
get_vfs_from_yaml(std::unique_ptr<MemoryBuffer> buffer,
               SourceMgr::DiagHandlerTy diagHandler,
               StringRef yamlFilePath, void *diagContext = nullptr,
               IntrusiveRefCountPtr<FileSystem> externalFs = get_real_file_system());

struct YAMLVFSEntry
{
   template <typename T1, typename T2>
   YAMLVFSEntry(T1 &&vpath, T2 &&rpath)
      : VPath(std::forward<T1>(vpath)),
        RPath(std::forward<T2>(rpath))
   {}
   std::string VPath;
   std::string RPath;
};

/// Collect all pairs of <virtual path, real path> entries from the
/// \p yamlFilePath. This is used by the module dependency collector to forward
/// the entries into the reproducer output VFS YAML file.
void collect_vfs_from_yaml(
      std::unique_ptr<MemoryBuffer> buffer,
      SourceMgr::DiagHandlerTy diagHandler, StringRef yamlFilePath,
      SmallVectorImpl<YAMLVFSEntry> &collectedEntries,
      void *diagContext = nullptr,
      IntrusiveRefCountPtr<FileSystem> externalFs = get_real_file_system());

class YAMLVFSWriter
{
   std::vector<YAMLVFSEntry> m_mappings;
   std::optional<bool> m_isCaseSensitive;
   std::optional<bool> m_isOverlayRelative;
   std::optional<bool> m_useExternalNames;
   std::string m_overlayDir;

public:
   YAMLVFSWriter() = default;

   void addFileMapping(StringRef virtualPath, StringRef realPath);

   void setCaseSensitivity(bool caseSensitive)
   {
      m_isCaseSensitive = caseSensitive;
   }

   void setUseExternalNames(bool useExtNames)
   {
      m_useExternalNames = useExtNames;
   }

   void setOverlayDir(StringRef overlayDirectory)
   {
      m_isOverlayRelative = true;
      m_overlayDir.assign(overlayDirectory.getStr());
   }
   void write(RawOutStream &outStream);
};

} // vsf
} // polar

#endif // POLARPHP_UTILS_VIRTUAL_FILESYSTEM_H
