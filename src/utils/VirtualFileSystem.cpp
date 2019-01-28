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

#include "polarphp/utils/VirtualFileSystem.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/IntrusiveRefCountPtr.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/StringSet.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include "polarphp/global/Config.h"
#include "polarphp/global/CompilerFeature.h"
#include "polarphp/utils/Casting.h"
#include "polarphp/utils/Chrono.h"

#include "polarphp/utils/Debug.h"
#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/OptionalError.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/Path.h"
#include "polarphp/utils/Process.h"
#include "polarphp/utils/SourceLocation.h"
#include "polarphp/utils/SourceMgr.h"
#include "polarphp/utils/yaml/YamlParser.h"
#include "polarphp/utils/RawOutStream.h"

#include <optional>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace polar {
namespace vfs {

using polar::fs::FileStatus;
using polar::fs::UniqueId;
using polar::fs::Permission;
using polar::fs::FileType;
using polar::utils::ErrorCode;
using polar::basic::SmallString;
using polar::basic::StringSet;
using polar::basic::StringMap;
using polar::utils::dyn_cast;
using polar::utils::isa;
using polar::utils::cast;
using polar::basic::StringRef;
using polar::basic::DenseMap;
using polar::utils::SMLocation;
using polar::basic::ArrayRef;

Status::Status(const FileStatus &status)
   : m_uid(status.getUniqueId()),
     m_mtime(status.getLastModificationTime()),
     m_user(status.getUser()), m_group(status.getGroup()),
     m_size(status.getSize()), m_type(status.getType()),
     m_perms(status.getPermissions())
{}

Status::Status(StringRef name, UniqueId uid, sys::TimePoint<> mtime,
               uint32_t user, uint32_t group, uint64_t size, FileType type,
               Permission perms)
   : m_name(name), m_uid(uid),
     m_mtime(mtime), m_user(user),
     m_group(group), m_size(size),
     m_type(type), m_perms(perms)
{}

Status Status::copyWithNewName(const Status &in, StringRef newName)
{
   return Status(newName, in.getUniqueId(), in.getLastModificationTime(),
                 in.getUser(), in.getGroup(), in.getSize(), in.getType(),
                 in.getPermissions());
}

Status Status::copyWithNewName(const FileStatus &in, StringRef newName)
{
   return Status(newName, in.getUniqueId(), in.getLastModificationTime(),
                 in.getUser(), in.getGroup(), in.getSize(), in.getType(),
                 in.getPermissions());
}

bool Status::equivalent(const Status &other) const
{
   assert(isStatusKnown() && other.isStatusKnown());
   return getUniqueId() == other.getUniqueId();
}

bool Status::isDirectory() const
{
   return m_type == FileType::directory_file;
}

bool Status::isRegularFile() const
{
   return m_type == FileType::regular_file;
}

bool Status::isOther() const
{
   return exists() && !isRegularFile() && !isDirectory() && !isSymlink();
}

bool Status::isSymlink() const
{
   return m_type == FileType::symlink_file;
}

bool Status::isStatusKnown() const
{
   return m_type != FileType::status_error;
}

bool Status::exists() const
{
   return isStatusKnown() && m_type != FileType::file_not_found;
}


File::~File()
{}

FileSystem::~FileSystem()
{}

OptionalError<std::unique_ptr<MemoryBuffer>>
FileSystem::getBufferForFile(const Twine &name, int64_t fileSize,
                             bool requiresNullTerminator, bool isVolatile)
{
   auto file = openFileForRead(name);
   if (!file) {
      return file.getError();
   }
   return (*file)->getBuffer(name, fileSize, requiresNullTerminator, isVolatile);
}

std::error_code FileSystem::makeAbsolute(SmallVectorImpl<char> &path) const
{
   if (polar::fs::path::is_absolute(path)) {
      return {};
   }
   auto workingDir = getCurrentWorkingDirectory();
   if (!workingDir) {
      return workingDir.getError();
   }
   return polar::fs::make_absolute(workingDir.get(), path);
}

std::error_code FileSystem::getRealPath(const Twine &, SmallVectorImpl<char> &) const
{
   return ErrorCode::operation_not_permitted;
}

std::error_code FileSystem::isLocal(const Twine &path, bool &result)
{
   return ErrorCode::operation_not_permitted;
}

bool FileSystem::exists(const Twine &path)
{
   auto status = getStatus(path);
   return status && status->exists();
}

#ifndef NDEBUG
namespace {
bool is_traversal_component(StringRef component)
{
   return component.equals("..") || component.equals(".");
}

bool path_has_traversal(StringRef path)
{

   for (StringRef comp : polar::basic::make_range(polar::fs::path::begin(path), polar::fs::path::end(path))) {
      if (is_traversal_component(comp)) {
         return true;
      }
   }
   return false;
}
} // anonymous namespace
#endif

//===-----------------------------------------------------------------------===/
// RealFileSystem implementation
//===-----------------------------------------------------------------------===/

namespace {

/// Wrapper around a raw file descriptor.
class RealFile : public File
{
   friend class RealFileSystem;

   int m_fd;
   Status m_status;
   std::string m_realName;

   RealFile(int fd, StringRef newName, StringRef NewRealPathName)
      : m_fd(fd), m_status(newName, {}, {}, {}, {}, {},
                           polar::fs::FileType::status_error, {}),
        m_realName(NewRealPathName.getStr())
   {
      assert(m_fd >= 0 && "Invalid or inactive file descriptor");
   }

public:
   ~RealFile() override;

   OptionalError<Status> getStatus() override;
   OptionalError<std::string> getName() override;
   OptionalError<std::unique_ptr<MemoryBuffer>> getBuffer(const Twine &name,
                                                          int64_t fileSize,
                                                          bool requiresNullTerminator,
                                                          bool isVolatile) override;
   std::error_code close() override;
};

} // namespace


RealFile::~RealFile() { close(); }

OptionalError<Status> RealFile::getStatus()
{
   assert(m_fd != -1 && "cannot stat closed file");
   if (!m_status.isStatusKnown()) {
      FileStatus realStatus;
      if (std::error_code errorCode = polar::fs::status(m_fd, realStatus)) {
         return errorCode;
      }
      m_status = Status::copyWithNewName(realStatus, m_status.getName());
   }
   return m_status;
}

OptionalError<std::string> RealFile::getName()
{
   return m_realName.empty() ? m_status.getName().getStr() : m_realName;
}

OptionalError<std::unique_ptr<MemoryBuffer>>
RealFile::getBuffer(const Twine &name, int64_t fileSize,
                    bool requiresNullTerminator, bool isVolatile)
{
   assert(m_fd != -1 && "cannot get buffer for closed file");
   return MemoryBuffer::getOpenFile(m_fd, name, fileSize, requiresNullTerminator,
                                    isVolatile);
}

std::error_code RealFile::close()
{
   std::error_code errorCode = polar::sys::Process::safelyCloseFileDescriptor(m_fd);
   m_fd = -1;
   return errorCode;
}


namespace {

/// The file system according to your operating system.
class RealFileSystem : public FileSystem
{
public:
   OptionalError<Status> getStatus(const Twine &path) override;
   OptionalError<std::unique_ptr<File>> openFileForRead(const Twine &path) override;
   DirectoryIterator dirBegin(const Twine &dir, std::error_code &errorCode) override;

   OptionalError<std::string> getCurrentWorkingDirectory() const override;
   std::error_code setCurrentWorkingDirectory(const Twine &path) override;
   std::error_code isLocal(const Twine &path, bool &result) override;
   std::error_code getRealPath(const Twine &path,
                               SmallVectorImpl<char> &output) const override;

private:
   mutable std::mutex m_cwdMutex;
   mutable std::string m_cwdCache;
};

} // namespace

OptionalError<Status> RealFileSystem::getStatus(const Twine &path)
{
   FileStatus realStatus;
   if (std::error_code errorCode = polar::fs::status(path, realStatus)) {
      return errorCode;
   }
   return Status::copyWithNewName(realStatus, path.getStr());
}

OptionalError<std::unique_ptr<File>>
RealFileSystem::openFileForRead(const Twine &name)
{
   int fd;
   SmallString<256> realName;
   if (std::error_code errorCode =
       polar::fs::open_file_for_read(name, fd, polar::fs::OF_None, &realName)) {
      return errorCode;
   }

   return std::unique_ptr<File>(new RealFile(fd, name.getStr(), realName.getStr()));
}

OptionalError<std::string> RealFileSystem::getCurrentWorkingDirectory() const
{
   std::lock_guard<std::mutex> lock(m_cwdMutex);
   if (!m_cwdCache.empty()) {
      return m_cwdCache;
   }

   SmallString<256> dir;
   if (std::error_code errorCode = polar::fs::current_path(dir)) {
      return errorCode;
   }

   m_cwdCache = dir.getStr();
   return m_cwdCache;
}

std::error_code RealFileSystem::setCurrentWorkingDirectory(const Twine &path)
{
   // FIXME: chdir is thread hostile; on the other hand, creating the same
   // behavior as chdir is complex: chdir resolves the path once, thus
   // guaranteeing that all subsequent relative path operations work
   // on the same path the original chdir resulted in. This makes a
   // difference for example on network filesystems, where symlinks might be
   // switched during runtime of the tool. Fixing this depends on having a
   // file system abstraction that allows openat() style interactions.
   if (auto errorCode = polar::fs::set_current_path(path))
      return errorCode;

   // Invalidate cache.
   std::lock_guard<std::mutex> lock(m_cwdMutex);
   m_cwdCache.clear();
   return std::error_code();
}

std::error_code RealFileSystem::isLocal(const Twine &path, bool &result)
{
   return polar::fs::is_local(path, result);
}

std::error_code
RealFileSystem::getRealPath(const Twine &path,
                            SmallVectorImpl<char> &output) const
{
   return polar::fs::real_path(path, output);
}


IntrusiveRefCountPtr<FileSystem> get_real_file_system()
{
   static IntrusiveRefCountPtr<FileSystem> fs = new RealFileSystem();
   return fs;
}

namespace {

class RealFSDirIter : public polar::vfs::internal::DirIterImpl
{
   polar::fs::DirectoryIterator m_iter;

public:
   RealFSDirIter(const Twine &path, std::error_code &errorCode)
      : m_iter(path, errorCode)
   {
      if (m_iter != polar::fs::DirectoryIterator())
         m_currentEntry = DirectoryEntry(m_iter->getPath(), m_iter->getType());
   }

   std::error_code increment() override
   {
      std::error_code errorCode;
      m_iter.increment(errorCode);
      m_currentEntry = (m_iter == polar::fs::DirectoryIterator())
            ? DirectoryEntry()
            : DirectoryEntry(m_iter->getPath(), m_iter->getType());
      return errorCode;
   }
};

} // namespace

DirectoryIterator RealFileSystem::dirBegin(const Twine &dir, std::error_code &errorCode)
{
   return DirectoryIterator(std::make_shared<RealFSDirIter>(dir, errorCode));
}

//===-----------------------------------------------------------------------===/
// OverlayFileSystem implementation
//===-----------------------------------------------------------------------===/

OverlayFileSystem::OverlayFileSystem(IntrusiveRefCountPtr<FileSystem> baseFs)
{
   m_fsList.push_back(std::move(baseFs));
}

void OverlayFileSystem::pushOverlay(IntrusiveRefCountPtr<FileSystem> fs)
{
   m_fsList.push_back(fs);
   // Synchronize added file systems by duplicating the working directory from
   // the first one in the list.
   fs->setCurrentWorkingDirectory(getCurrentWorkingDirectory().get());
}

OptionalError<Status> OverlayFileSystem::getStatus(const Twine &path)
{
   // FIXME: handle symlinks that cross file systems
   for (iterator iter = overlaysBegin(), end = overlaysEnd(); iter != end; ++iter) {
      OptionalError<Status> status = (*iter)->getStatus(path);
      if (status || status.getError() != ErrorCode::no_such_file_or_directory) {
         return status;
      }
   }
   return polar::utils::make_error_code(ErrorCode::no_such_file_or_directory);
}

OptionalError<std::unique_ptr<File>>
OverlayFileSystem::openFileForRead(const Twine &path)
{
   // FIXME: handle symlinks that cross file systems
   for (iterator iter = overlaysBegin(), end = overlaysEnd(); iter != end; ++iter) {
      auto result = (*iter)->openFileForRead(path);
      if (result || result.getError() != ErrorCode::no_such_file_or_directory) {
         return result;
      }
   }
   return polar::utils::make_error_code(ErrorCode::no_such_file_or_directory);
}

OptionalError<std::string>
OverlayFileSystem::getCurrentWorkingDirectory() const
{
   // All file systems are synchronized, just take the first working directory.
   return m_fsList.front()->getCurrentWorkingDirectory();
}

std::error_code
OverlayFileSystem::setCurrentWorkingDirectory(const Twine &path)
{
   for (auto &fs : m_fsList) {
      if (std::error_code errorCode = fs->setCurrentWorkingDirectory(path)) {
         return errorCode;
      }
   }
   return {};
}

std::error_code OverlayFileSystem::isLocal(const Twine &path, bool &result)
{
   for (auto &fs : m_fsList) {
      if (fs->exists(path)) {
         return fs->isLocal(path, result);
      }
   }
   return ErrorCode::no_such_file_or_directory;
}

std::error_code
OverlayFileSystem::getRealPath(const Twine &path,
                               SmallVectorImpl<char> &output) const
{
   for (auto &fs : m_fsList) {
      if (fs->exists(path)) {
         return fs->getRealPath(path, output);
      }
   }
   return ErrorCode::no_such_file_or_directory;
}

polar::vfs::internal::DirIterImpl::~DirIterImpl()
{}

namespace {

class OverlayFSDirIterImpl : public polar::vfs::internal::DirIterImpl
{
   OverlayFileSystem &m_overlays;
   std::string m_path;
   OverlayFileSystem::iterator m_currentFs;
   DirectoryIterator m_currentDirIter;
   StringSet<> m_seenNames;

   std::error_code incrementFs()
   {
      assert(m_currentFs != m_overlays.overlaysEnd() && "incrementing past end");
      ++m_currentFs;
      for (auto end = m_overlays.overlaysEnd(); m_currentFs != end; ++m_currentFs) {
         std::error_code errorCode;
         m_currentDirIter = (*m_currentFs)->dirBegin(m_path, errorCode);
         if (errorCode && errorCode != ErrorCode::no_such_file_or_directory) {
            return errorCode;
         }
         if (m_currentDirIter != DirectoryIterator()) {
            break; // found
         }
      }
      return {};
   }

   std::error_code incrementDirIter(bool isFirstTime)
   {
      assert((isFirstTime || m_currentDirIter != DirectoryIterator()) &&
             "incrementing past end");
      std::error_code errorCode;
      if (!isFirstTime) {
         m_currentDirIter.increment(errorCode);
      }
      if (!errorCode && m_currentDirIter == DirectoryIterator()) {
         errorCode = incrementFs();
      }
      return errorCode;
   }

   std::error_code incrementImpl(bool isFirstTime)
   {
      while (true) {
         std::error_code errorCode = incrementDirIter(isFirstTime);
         if (errorCode || m_currentDirIter == DirectoryIterator()) {
            m_currentEntry = DirectoryEntry();
            return errorCode;
         }
         m_currentEntry = *m_currentDirIter;
         StringRef name = polar::fs::path::filename(m_currentEntry.path());
         if (m_seenNames.insert(name).second) {
            return errorCode; // name not seen before
         }
      }
      polar_unreachable("returned above");
   }

public:
   OverlayFSDirIterImpl(const Twine &path, OverlayFileSystem &fs,
                        std::error_code &errorCode)
      : m_overlays(fs), m_path(path.getStr()), m_currentFs(m_overlays.overlaysBegin())
   {
      m_currentDirIter = (*m_currentFs)->dirBegin(m_path, errorCode);
      errorCode = incrementImpl(true);
   }

   std::error_code increment() override
   {
      return incrementImpl(false);
   }
};

} // namespace

DirectoryIterator OverlayFileSystem::dirBegin(const Twine &dir, std::error_code &errorCode)
{
   return DirectoryIterator(
            std::make_shared<OverlayFSDirIterImpl>(dir, *this, errorCode));
}

namespace internal {

enum InMemoryNodeKind { IME_File, IME_Directory, IME_HardLink };

/// The in memory file system is a tree of Nodes. Every node can either be a
/// file , hardlink or a directory.
class InMemoryNode
{
   InMemoryNodeKind m_kind;
   std::string m_filename;

public:
   InMemoryNode(StringRef filename, InMemoryNodeKind kind)
      : m_kind(kind),
        m_filename(polar::fs::path::filename(filename))
   {}
   virtual ~InMemoryNode() = default;

   /// Get the filename of this node (the name without the directory part).
   StringRef getFileName() const
   {
      return m_filename;
   }

   InMemoryNodeKind getKind() const
   {
      return m_kind;
   }

   virtual std::string toString(unsigned indent) const = 0;
};

class InMemoryFile : public InMemoryNode
{
   Status m_stat;
   std::unique_ptr<MemoryBuffer> m_buffer;

public:
   InMemoryFile(Status stat, std::unique_ptr<MemoryBuffer> buffer)
      : InMemoryNode(stat.getName(), IME_File),
        m_stat(std::move(stat)),
        m_buffer(std::move(buffer))
   {}

   /// Return the \p Status for this node. \p requestedName should be the name
   /// through which the caller referred to this node. It will override
   /// \p Status::name in the return value, to mimic the behavior of \p RealFile.
   Status getStatus(StringRef requestedName) const
   {
      return Status::copyWithNewName(m_stat, requestedName);
   }

   MemoryBuffer *getBuffer() const
   {
      return m_buffer.get();
   }

   std::string toString(unsigned indent) const override
   {
      return (std::string(indent, ' ') + m_stat.getName() + "\n").getStr();
   }

   static bool classof(const InMemoryNode *N)
   {
      return N->getKind() == IME_File;
   }
};

namespace {

class InMemoryHardLink : public InMemoryNode
{
   const InMemoryFile &m_resolvedFile;

public:
   InMemoryHardLink(StringRef path, const InMemoryFile &resolvedFile)
      : InMemoryNode(path, IME_HardLink),
        m_resolvedFile(resolvedFile)
   {}

   const InMemoryFile &getResolvedFile() const
   {
      return m_resolvedFile;
   }

   std::string toString(unsigned indent) const override
   {
      return std::string(indent, ' ') + "HardLink to -> " +
            m_resolvedFile.toString(0);
   }

   static bool classof(const InMemoryNode *node)
   {
      return node->getKind() == IME_HardLink;
   }
};

/// Adapt a InMemoryFile for vfs' File interface.  The goal is to make
/// \p InMemoryFileAdaptor mimic as much as possible the behavior of
/// \p RealFile.
class InMemoryFileAdaptor : public File
{
   const InMemoryFile &m_node;
   /// The name to use when returning a Status for this file.
   std::string m_requestedName;

public:
   explicit InMemoryFileAdaptor(const InMemoryFile &node,
                                std::string requestedName)
      : m_node(node),
        m_requestedName(std::move(requestedName))
   {}

   OptionalError<Status> getStatus() override
   {
      return m_node.getStatus(m_requestedName);
   }

   OptionalError<std::unique_ptr<MemoryBuffer>>
   getBuffer(const Twine &, int64_t, bool requiresNullTerminator, bool) override
   {
      MemoryBuffer *buffer = m_node.getBuffer();
      return MemoryBuffer::getMemBuffer(
               buffer->getBuffer(), buffer->getBufferIdentifier(), requiresNullTerminator);
   }

   std::error_code close() override
   {
      return {};
   }
};
} // namespace

class InMemoryDirectory : public InMemoryNode
{
   Status m_stat;
   StringMap<std::unique_ptr<InMemoryNode>> m_entries;

public:
   InMemoryDirectory(Status stat)
      : InMemoryNode(stat.getName(), IME_Directory),
        m_stat(std::move(stat))
   {}

   /// Return the \p Status for this node. \p requestedName should be the name
   /// through which the caller referred to this node. It will override
   /// \p Status::name in the return value, to mimic the behavior of \p RealFile.
   Status getStatus(StringRef requestedName) const
   {
      return Status::copyWithNewName(m_stat, requestedName);
   }

   InMemoryNode *getChild(StringRef name)
   {
      auto iter = m_entries.find(name);
      if (iter != m_entries.end()) {
         return iter->m_second.get();
      }
      return nullptr;
   }

   InMemoryNode *addChild(StringRef name, std::unique_ptr<InMemoryNode> child)
   {
      return m_entries.insert(make_pair(name, std::move(child)))
            .first->m_second.get();
   }

   using const_iterator = decltype(m_entries)::const_iterator;

   const_iterator begin() const
   {
      return m_entries.begin();
   }

   const_iterator end() const
   {
      return m_entries.end();
   }

   std::string toString(unsigned indent) const override
   {
      std::string result =
            (std::string(indent, ' ') + m_stat.getName() + "\n").getStr();
      for (const auto &entry : m_entries) {
         result += entry.m_second->toString(indent + 2);
      }
      return result;
   }

   static bool classof(const InMemoryNode *N)
   {
      return N->getKind() == IME_Directory;
   }
};

namespace {
Status get_node_status(const InMemoryNode *node, StringRef requestedName)
{
   if (auto dir = dyn_cast<internal::InMemoryDirectory>(node)) {
      return dir->getStatus(requestedName);
   }
   if (auto file = dyn_cast<internal::InMemoryFile>(node)) {
      return file->getStatus(requestedName);
   }
   if (auto link = dyn_cast<internal::InMemoryHardLink>(node)) {
      return link->getResolvedFile().getStatus(requestedName);
   }
   polar_unreachable("Unknown node type");
}
} // namespace
} // namespace internal

InMemoryFileSystem::InMemoryFileSystem(bool useNormalizedPaths)
   : m_root(new internal::InMemoryDirectory(
               Status("", get_next_virtual_unique_id(), polar::utils::TimePoint<>(), 0, 0,
                      0, polar::fs::FileType::directory_file,
                      polar::fs::Permission::all_all))),
     m_useNormalizedPaths(useNormalizedPaths)
{}

InMemoryFileSystem::~InMemoryFileSystem()
{}

std::string InMemoryFileSystem::toString() const
{
   return m_root->toString(/*indent=*/0);
}

bool InMemoryFileSystem::addFile(const Twine &pathTwine, time_t modificationTime,
                                 std::unique_ptr<MemoryBuffer> buffer,
                                 std::optional<uint32_t> user,
                                 std::optional<uint32_t> group,
                                 std::optional<polar::fs::FileType> type,
                                 std::optional<polar::fs::Permission> perms,
                                 const internal::InMemoryFile *hardLinkTarget)
{
   SmallString<128> path;
   pathTwine.toVector(path);

   // Fix up relative paths. This just prepends the current working directory.
   std::error_code errorCode = makeAbsolute(path);
   assert(!errorCode);
   (void)errorCode;

   if (useNormalizedPaths()) {
      polar::fs::path::remove_dots(path, /*remove_dot_dot=*/true);
   }

   if (path.empty()) {
      return false;
   }
   internal::InMemoryDirectory *dir = m_root.get();
   auto iter = polar::fs::path::begin(path), end = polar::fs::path::end(path);
   const auto resolvedUser = user.value_or(0);
   const auto resolvedGroup = group.value_or(0);
   const auto resolvedType = type.value_or(polar::fs::FileType::regular_file);
   const auto resolvedPerms = perms.value_or(polar::fs::all_all);
   assert(!(hardLinkTarget && buffer) && "HardLink cannot have a buffer");
   // Any intermediate directories we create should be accessible by
   // the owner, even if perms says otherwise for the final path.
   const auto newDirectoryPerms = resolvedPerms | polar::fs::owner_all;
   while (true) {
      StringRef name = *iter;
      internal::InMemoryNode *node = dir->getChild(name);
      ++iter;
      if (!node) {
         if (iter == end) {
            // end of the path.
            std::unique_ptr<internal::InMemoryNode> child;
            if (hardLinkTarget)
               child.reset(new internal::InMemoryHardLink(pathTwine.getStr(), *hardLinkTarget));
            else {
               // Create a new file or directory.
               Status stat(pathTwine.getStr(), get_next_virtual_unique_id(),
                           polar::utils::to_time_point(modificationTime), resolvedUser,
                           resolvedGroup, buffer->getBufferSize(), resolvedType,
                           resolvedPerms);
               if (resolvedType == polar::fs::FileType::directory_file) {
                  child.reset(new internal::InMemoryDirectory(std::move(stat)));
               } else {
                  child.reset(
                           new internal::InMemoryFile(std::move(stat), std::move(buffer)));
               }
            }
            dir->addChild(name, std::move(child));
            return true;
         }

         // Create a new directory. Use the path up to here.
         Status stat(
                  StringRef(path.getStr().begin(), name.end() - path.getStr().begin()),
                  get_next_virtual_unique_id(), polar::utils::to_time_point(modificationTime),
                  resolvedUser, resolvedGroup, 0, polar::fs::FileType::directory_file,
                  newDirectoryPerms);
         dir = polar::utils::cast<internal::InMemoryDirectory>(dir->addChild(
                                                                  name, std::make_unique<internal::InMemoryDirectory>(std::move(stat))));
         continue;
      }

      if (auto *newDir = dyn_cast<internal::InMemoryDirectory>(node)) {
         dir = newDir;
      } else {
         assert((isa<internal::InMemoryFile>(node) ||
                 isa<internal::InMemoryHardLink>(node)) &&
                "Must be either file, hardlink or directory!");

         // Trying to insert a directory in place of a file.
         if (iter != end) {
            return false;
         }

         // Return false only if the new file is different from the existing one.
         if (auto link = dyn_cast<internal::InMemoryHardLink>(node)) {
            return link->getResolvedFile().getBuffer()->getBuffer() ==
                  buffer->getBuffer();
         }
         return polar::utils::cast<internal::InMemoryFile>(node)->getBuffer()->getBuffer() ==
               buffer->getBuffer();
      }
   }
}

bool InMemoryFileSystem::addFile(const Twine &pathTwine, time_t modificationTime,
                                 std::unique_ptr<MemoryBuffer> buffer,
                                 std::optional<uint32_t> user,
                                 std::optional<uint32_t> group,
                                 std::optional<polar::fs::FileType> type,
                                 std::optional<polar::fs::Permission> perms)
{
   return addFile(pathTwine, modificationTime, std::move(buffer), user, group, type,
                  perms, /*hardLinkTarget=*/nullptr);
}

bool InMemoryFileSystem::addFileNoOwn(const Twine &pathTwine, time_t modificationTime,
                                      MemoryBuffer *buffer,
                                      std::optional<uint32_t> user,
                                      std::optional<uint32_t> group,
                                      std::optional<polar::fs::FileType> type,
                                      std::optional<polar::fs::Permission> perms)
{
   return addFile(pathTwine, modificationTime,
                  MemoryBuffer::getMemBuffer(
                     buffer->getBuffer(), buffer->getBufferIdentifier()),
                  std::move(user), std::move(group), std::move(type),
                  std::move(perms));
}

static OptionalError<const internal::InMemoryNode *>
lookupInMemoryNode(const InMemoryFileSystem &fs, internal::InMemoryDirectory *dir,
                   const Twine &pathWine) {
   SmallString<128> path;
   pathWine.toVector(path);

   // Fix up relative paths. This just prepends the current working directory.
   std::error_code errorCode = fs.makeAbsolute(path);
   assert(!errorCode);
   (void)errorCode;

   if (fs.useNormalizedPaths()) {
      polar::fs::path::remove_dots(path, /*remove_dot_dot=*/true);
   }

   if (path.empty()) {
      return dir;
   }
   auto iter = polar::fs::path::begin(path), end = polar::fs::path::end(path);
   while (true) {
      internal::InMemoryNode *node = dir->getChild(*iter);
      ++iter;
      if (!node) {
         return ErrorCode::no_such_file_or_directory;
      }
      // Return the file if it's at the end of the path.
      if (auto file = dyn_cast<internal::InMemoryFile>(node)) {
         if (iter == end) {
            return file;
         }
         return ErrorCode::no_such_file_or_directory;
      }

      // If node is HardLink then return the resolved file.
      if (auto file = dyn_cast<internal::InMemoryHardLink>(node)) {
         if (iter == end) {
            return &file->getResolvedFile();
         }
         return ErrorCode::no_such_file_or_directory;
      }
      // Traverse directories.
      dir = cast<internal::InMemoryDirectory>(node);
      if (iter == end) {
         return dir;
      }
   }
}

bool InMemoryFileSystem::addHardLink(const Twine &fromPath,
                                     const Twine &toPath)
{
   auto fromNode = lookupInMemoryNode(*this, m_root.get(), fromPath);
   auto toNode = lookupInMemoryNode(*this, m_root.get(), toPath);
   // fromPath must not have been added before. toPath must have been added
   // before. Resolved toPath must be a File.
   if (!toNode || fromNode || !isa<internal::InMemoryFile>(*toNode)) {
      return false;
   }
   return this->addFile(fromPath, 0, nullptr, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                        cast<internal::InMemoryFile>(*toNode));
}

OptionalError<Status> InMemoryFileSystem::getStatus(const Twine &path)
{
   auto node = lookupInMemoryNode(*this, m_root.get(), path);
   if (node) {
      return internal::get_node_status(*node, path.getStr());
   }
   return node.getError();
}

OptionalError<std::unique_ptr<File>>
InMemoryFileSystem::openFileForRead(const Twine &path)
{
   auto node = lookupInMemoryNode(*this, m_root.get(), path);
   if (!node) {
      return node.getError();
   }
   // When we have a file provide a heap-allocated wrapper for the memory buffer
   // to match the ownership semantics for File.
   if (auto *file = dyn_cast<internal::InMemoryFile>(*node)) {
      return std::unique_ptr<File>(
               new internal::InMemoryFileAdaptor(*file, path.getStr()));
   }
   // FIXME: ErrorCode::not_a_file?
   return make_error_code(ErrorCode::invalid_argument);
}

namespace {

/// Adaptor from InMemoryDir::iterator to DirectoryIterator.
class InMemoryDirIterator : public polar::vfs::internal::DirIterImpl
{
   internal::InMemoryDirectory::const_iterator m_iter;
   internal::InMemoryDirectory::const_iterator m_end;
   std::string m_requestedDirName;

   void setCurrentEntry()
   {
      if (m_iter != m_end) {
         SmallString<256> path(m_requestedDirName);
         polar::fs::path::append(path, m_iter->m_second->getFileName());
         polar::fs::FileType type;
         switch (m_iter->m_second->getKind()) {
         case internal::IME_File:
         case internal::IME_HardLink:
            type = polar::fs::FileType::regular_file;
            break;
         case internal::IME_Directory:
            type = polar::fs::FileType::directory_file;
            break;
         }
         m_currentEntry = DirectoryEntry(path.getStr(), type);
      } else {
         // When we're at the end, make m_currentEntry invalid and DirIterImpl will
         // do the rest.
         m_currentEntry = DirectoryEntry();
      }
   }

public:
   InMemoryDirIterator() = default;

   explicit InMemoryDirIterator(const internal::InMemoryDirectory &dir,
                                std::string requestedDirName)
      : m_iter(dir.begin()), m_end(dir.end()),
        m_requestedDirName(std::move(requestedDirName))
   {
      setCurrentEntry();
   }

   std::error_code increment() override
   {
      ++m_iter;
      setCurrentEntry();
      return {};
   }
};

} // namespace

DirectoryIterator InMemoryFileSystem::dirBegin(const Twine &dir,
                                               std::error_code &errorCode)
{
   auto node = lookupInMemoryNode(*this, m_root.get(), dir);
   if (!node) {
      errorCode = node.getError();
      return DirectoryIterator(std::make_shared<InMemoryDirIterator>());
   }

   if (auto *DirNode = dyn_cast<internal::InMemoryDirectory>(*node)) {
      return DirectoryIterator(
               std::make_shared<InMemoryDirIterator>(*DirNode, dir.getStr()));
   }
   errorCode = make_error_code(ErrorCode::not_a_directory);
   return DirectoryIterator(std::make_shared<InMemoryDirIterator>());
}

std::error_code InMemoryFileSystem::setCurrentWorkingDirectory(const Twine &pathTwine)
{
   SmallString<128> path;
   pathTwine.toVector(path);

   // Fix up relative paths. This just prepends the current working directory.
   std::error_code errorCode = makeAbsolute(path);
   assert(!errorCode);
   (void)errorCode;

   if (useNormalizedPaths()) {
      polar::fs::path::remove_dots(path, /*remove_dot_dot=*/true);
   }
   if (!path.empty()) {
      m_workingDirectory = path.getStr();
   }
   return {};
}

std::error_code
InMemoryFileSystem::getRealPath(const Twine &path,
                                SmallVectorImpl<char> &output) const
{
   auto cwd = getCurrentWorkingDirectory();
   if (!cwd || cwd->empty()) {
      return ErrorCode::operation_not_permitted;
   }
   path.toVector(output);
   if (auto errorCode = makeAbsolute(output)) {
      return errorCode;
   }
   polar::fs::path::remove_dots(output, /*remove_dot_dot=*/true);
   return {};
}

std::error_code InMemoryFileSystem::isLocal(const Twine &path, bool &result)
{
   result = false;
   return {};
}

//===-----------------------------------------------------------------------===/
// RedirectingFileSystem implementation
//===-----------------------------------------------------------------------===/

namespace {

enum EntryKind { EK_Directory, EK_File };

/// A single file or directory in the vfs.
class Entry
{
   EntryKind m_kind;
   std::string m_name;

public:
   Entry(EntryKind kind, StringRef name)
      : m_kind(kind),
        m_name(name)
   {}

   virtual ~Entry() = default;

   StringRef getName() const
   {
      return m_name;
   }

   EntryKind getKind() const
   {
      return m_kind;
   }
};

class RedirectingDirectoryEntry : public Entry
{
   std::vector<std::unique_ptr<Entry>> m_contents;
   Status m_status;

public:
   RedirectingDirectoryEntry(StringRef name,
                             std::vector<std::unique_ptr<Entry>> m_contents,
                             Status status)
      : Entry(EK_Directory, name),
        m_contents(std::move(m_contents)),
        m_status(std::move(status))
   {}

   RedirectingDirectoryEntry(StringRef name, Status status)
      : Entry(EK_Directory, name),
        m_status(std::move(status))
   {}

   Status getStatus()
   {
      return m_status;
   }

   void addContent(std::unique_ptr<Entry> content)
   {
      m_contents.push_back(std::move(content));
   }

   Entry *getLastContent() const
   {
      return m_contents.back().get();
   }

   using iterator = decltype(m_contents)::iterator;

   iterator contentsBegin()
   {
      return m_contents.begin();
   }

   iterator contentsEnd()
   {
      return m_contents.end();
   }

   static bool classof(const Entry *entry)
   {
      return entry->getKind() == EK_Directory;
   }
};

class RedirectingFileEntry : public Entry
{
public:
   enum NameKind { NK_NotSet, NK_External, NK_Virtual };

private:
   std::string m_externalContentsPath;
   NameKind m_useName;

public:
   RedirectingFileEntry(StringRef name, StringRef externalContentsPath,
                        NameKind useName)
      : Entry(EK_File, name),
        m_externalContentsPath(externalContentsPath),
        m_useName(useName)
   {}

   StringRef getExternalContentsPath() const
   {
      return m_externalContentsPath;
   }

   /// whether to use the external path as the name for this file.
   bool useExternalName(bool globalUseExternalName) const
   {
      return m_useName == NK_NotSet ? globalUseExternalName
                                    : (m_useName == NK_External);
   }

   NameKind getUseName() const
   {
      return m_useName;
   }

   static bool classof(const Entry *entry)
   {
      return entry->getKind() == EK_File;
   }
};

// FIXME: reuse implementation common with OverlayFSDirIterImpl as these
// iterators are conceptually similar.
class VfsFromYamlDirIterImpl : public polar::vfs::internal::DirIterImpl
{
   std::string m_dir;
   RedirectingDirectoryEntry::iterator m_current;
   RedirectingDirectoryEntry::iterator m_end;

   // To handle 'fallthrough' mode we need to iterate at first through
   // RedirectingDirectoryEntry and then through m_externalFs. These operations are
   // done sequentially, we just need to keep a track of what kind of iteration
   // we are currently performing.

   /// Flag telling if we should iterate through m_externalFs or stop at the last
   /// RedirectingDirectoryEntry::iterator.
   bool m_iterateExternalFs;
   /// Flag telling if we have switched to iterating through m_externalFs.
   bool m_isExternalFsCurrent = false;
   FileSystem &m_externalFs;
   DirectoryIterator m_externalDirIter;
   StringSet<> m_seenNames;

   /// To combine multiple iterations, different methods are responsible for
   /// different iteration steps.
   /// @{

   /// Responsible for dispatching between RedirectingDirectoryEntry iteration
   /// and m_externalFs iteration.
   std::error_code incrementImpl(bool isFirstTime);
   /// Responsible for RedirectingDirectoryEntry iteration.
   std::error_code incrementContent(bool isFirstTime);
   /// Responsible for m_externalFs iteration.
   std::error_code incrementExternal();
   /// @}

public:
   VfsFromYamlDirIterImpl(const Twine &path,
                          RedirectingDirectoryEntry::iterator begin,
                          RedirectingDirectoryEntry::iterator end,
                          bool m_iterateExternalFs, FileSystem &m_externalFs,
                          std::error_code &errorCode);

   std::error_code increment() override;
};

/// A virtual file system parsed from a YAML file.
///
/// Currently, this class allows creating virtual directories and mapping
/// virtual file paths to existing external files, available in \c m_externalFs.
///
/// The basic structure of the parsed file is:
/// \verbatim
/// {
///   'version': <version number>,
///   <optional configuration>
///   'roots': [
///              <directory entries>
///            ]
/// }
/// \endverbatim
///
/// All configuration options are optional.
///   'case-sensitive': <boolean, default=true>
///   'use-external-names': <boolean, default=true>
///   'overlay-relative': <boolean, default=false>
///   'fallthrough': <boolean, default=true>
///
/// Virtual directories are represented as
/// \verbatim
/// {
///   'type': 'directory',
///   'name': <string>,
///   'contents': [ <file or directory entries> ]
/// }
/// \endverbatim
///
/// The default attributes for virtual directories are:
/// \verbatim
/// MTime = now() when created
/// perms = 0777
/// user = group = 0
/// Size = 0
/// UniqueId = unspecified unique value
/// \endverbatim
///
/// Re-mapped files are represented as
/// \verbatim
/// {
///   'type': 'file',
///   'name': <string>,
///   'use-external-name': <boolean> # std::optional
///   'external-contents': <path to external file>
/// }
/// \endverbatim
///
/// and inherit their attributes from the external contents.
///
/// In both cases, the 'name' field may contain multiple path components (e.g.
/// /path/to/file). However, any directory that contains more than one child
/// must be uniquely represented by a directory entry.
class RedirectingFileSystem : public FileSystem
{
   friend class RedirectingFileSystemParser;

   /// The root(s) of the virtual file system.
   std::vector<std::unique_ptr<Entry>> m_roots;

   /// The file system to use for external references.
   IntrusiveRefCountPtr<FileSystem> m_externalFs;

   /// If m_isRelativeOverlay is set, this represents the directory
   /// path that should be prefixed to each 'external-contents' entry
   /// when reading from YAML files.
   std::string m_externalContentsPrefixDir;

   /// @name Configuration
   /// @{

   /// Whether to perform case-sensitive comparisons.
   ///
   /// Currently, case-insensitive matching only works correctly with ASCII.
   bool m_caseSensitive = true;

   /// m_isRelativeOverlay marks whether a m_externalContentsPrefixDir path must
   /// be prefixed in every 'external-contents' when reading from YAML files.
   bool m_isRelativeOverlay = false;

   /// Whether to use to use the value of 'external-contents' for the
   /// names of files.  This global value is overridable on a per-file basis.
   bool m_useExternalNames = true;

   /// Whether to attempt a file lookup in external file system after it wasn't
   /// found in vfs.
   bool m_isFallthrough = true;
   /// @}

   /// Virtual file paths and external files could be canonicalized without "..",
   /// "." and "./" in their paths. FIXME: some unittests currently fail on
   /// win32 when using remove_dots and remove_leading_dotslash on paths.
   bool m_useCanonicalizedPaths =
      #ifdef _WIN32
         false;
#else
         true;
#endif

private:
   RedirectingFileSystem(IntrusiveRefCountPtr<FileSystem> externalFs)
      : m_externalFs(std::move(externalFs))
   {}

   /// Looks up the path <tt>[start, end)</tt> in \p from, possibly
   /// recursing into the contents of \p from if it is a directory.
   OptionalError<Entry *> lookupPath(polar::fs::path::ConstIterator start,
                                     polar::fs::path::ConstIterator end, Entry *from) const;

   /// Get the status of a given an \c entry.
   OptionalError<Status> getStatus(const Twine &path, Entry *entry);

public:
   /// Looks up \p path in \c m_roots.
   OptionalError<Entry *> lookupPath(const Twine &path) const;

   /// Parses \p buffer, which is expected to be in YAML format and
   /// returns a virtual file system representing its contents.
   static RedirectingFileSystem *
   create(std::unique_ptr<MemoryBuffer> buffer,
          SourceMgr::DiagHandlerTy diagHandler, StringRef yamlFilePath,
          void *diagContext, IntrusiveRefCountPtr<FileSystem> externalFs);

   OptionalError<Status> getStatus(const Twine &path) override;
   OptionalError<std::unique_ptr<File>> openFileForRead(const Twine &path) override;

   std::error_code getRealPath(const Twine &path,
                               SmallVectorImpl<char> &output) const override;

   OptionalError<std::string> getCurrentWorkingDirectory() const override
   {
      return m_externalFs->getCurrentWorkingDirectory();
   }

   std::error_code setCurrentWorkingDirectory(const Twine &path) override
   {
      return m_externalFs->setCurrentWorkingDirectory(path);
   }

   std::error_code isLocal(const Twine &path, bool &result) override
   {
      return m_externalFs->isLocal(path, result);
   }

   DirectoryIterator dirBegin(const Twine &dir, std::error_code &errorCode) override
   {
      OptionalError<Entry *> entry = lookupPath(dir);
      if (!entry) {
         errorCode = entry.getError();
         if (m_isFallthrough && errorCode == ErrorCode::no_such_file_or_directory)
            return m_externalFs->dirBegin(dir, errorCode);
         return {};
      }
      OptionalError<Status> status = getStatus(dir, *entry);
      if (!status) {
         errorCode = status.getError();
         return {};
      }
      if (!status->isDirectory()) {
         errorCode = std::error_code(static_cast<int>(ErrorCode::not_a_directory),
                                     std::system_category());
         return {};
      }

      auto *redirectDirEntry = cast<RedirectingDirectoryEntry>(*entry);
      return DirectoryIterator(std::make_shared<VfsFromYamlDirIterImpl>(
                                  dir, redirectDirEntry->contentsBegin(), redirectDirEntry->contentsEnd(),
                                  /*m_iterateExternalFs=*/m_isFallthrough, *m_externalFs, errorCode));
   }

   void setExternalContentsPrefixDir(StringRef prefixDir)
   {
      m_externalContentsPrefixDir = prefixDir.getStr();
   }

   StringRef getExternalContentsPrefixDir() const
   {
      return m_externalContentsPrefixDir;
   }

#if !defined(NDEBUG) || defined(POLAR_ENABLE_DUMP)
   POLAR_DUMP_METHOD void dump() const
   {
      for (const auto &root : m_roots) {
         dumpEntry(root.get());
      }
   }

   POLAR_DUMP_METHOD void dumpEntry(Entry *entry, int numSpaces = 0) const
   {
      StringRef name = entry->getName();
      for (int i = 0, e = numSpaces; i < e; ++i) {
         debug_stream() << " ";
      }
      debug_stream() << "'" << name.getStr().c_str() << "'"
                     << "\n";

      if (entry->getKind() == EK_Directory) {
         auto *dirEntry = dyn_cast<RedirectingDirectoryEntry>(entry);
         assert(dirEntry && "Should be a directory");

         for (std::unique_ptr<Entry> &subEntry :
              polar::basic::make_range(dirEntry->contentsBegin(), dirEntry->contentsEnd())) {
            dumpEntry(subEntry.get(), numSpaces + 2);
         }
      }
   }
#endif
};

/// A helper class to hold the common YAML parsing state.
class RedirectingFileSystemParser
{
   yaml::Stream &m_stream;

   void error(yaml::Node *node, const Twine &msg)
   {
      m_stream.printError(node, msg);
   }

   // false on error
   bool parseScalarString(yaml::Node *node, StringRef &result,
                          SmallVectorImpl<char> &storage)
   {
      const auto *scalerNode = dyn_cast<yaml::ScalarNode>(node);

      if (!scalerNode) {
         error(node, "expected string");
         return false;
      }
      result = scalerNode->getValue(storage);
      return true;
   }

   // false on error
   bool parseScalarBool(yaml::Node *node, bool &result)
   {
      SmallString<5> storage;
      StringRef value;
      if (!parseScalarString(node, value, storage)) {
         return false;
      }
      if (value.equalsLower("true") || value.equalsLower("on") ||
          value.equalsLower("yes") || value == "1") {
         result = true;
         return true;
      } else if (value.equalsLower("false") || value.equalsLower("off") ||
                 value.equalsLower("no") || value == "0") {
         result = false;
         return true;
      }
      error(node, "expected boolean value");
      return false;
   }

   struct KeyStatus
   {
      bool Required;
      bool Seen = false;

      KeyStatus(bool required = false) : Required(required)
      {}
   };

   using KeyStatusPair = std::pair<StringRef, KeyStatus>;

   // false on error
   bool checkDuplicateOrUnknownKey(yaml::Node *keyNode, StringRef key,
                                   DenseMap<StringRef, KeyStatus> &keys)
   {
      if (!keys.count(key)) {
         error(keyNode, "unknown key");
         return false;
      }
      KeyStatus &keyStatus = keys[key];
      if (keyStatus.Seen) {
         error(keyNode, Twine("duplicate key '") + key + "'");
         return false;
      }
      keyStatus.Seen = true;
      return true;
   }

   // false on error
   bool checkMissingKeys(yaml::Node *obj, DenseMap<StringRef, KeyStatus> &keys)
   {
      for (const auto &iter : keys) {
         if (iter.second.Required && !iter.second.Seen) {
            error(obj, Twine("missing key '") + iter.first + "'");
            return false;
         }
      }
      return true;
   }

   Entry *lookupOrCreateEntry(RedirectingFileSystem *fs, StringRef name,
                              Entry *parentEntry = nullptr)
   {
      if (!parentEntry) { // Look for a existent root
         for (const auto &m_root : fs->m_roots) {
            if (name.equals(m_root->getName())) {
               parentEntry = m_root.get();
               return parentEntry;
            }
         }
      } else { // Advance to the next component
         auto *dirEntry = dyn_cast<RedirectingDirectoryEntry>(parentEntry);
         for (std::unique_ptr<Entry> &content :
              polar::basic::make_range(dirEntry->contentsBegin(), dirEntry->contentsEnd())) {
            auto *dirContent = dyn_cast<RedirectingDirectoryEntry>(content.get());
            if (dirContent && name.equals(content->getName())) {
               return dirContent;
            }
         }
      }

      // ... or create a new one
      std::unique_ptr<Entry> entry = polar::basic::make_unique<RedirectingDirectoryEntry>(
               name,
               Status("", get_next_virtual_unique_id(), std::chrono::system_clock::now(),
                      0, 0, 0, FileType::directory_file, polar::fs::all_all));

      if (!parentEntry) { // Add a new root to the overlay
         fs->m_roots.push_back(std::move(entry));
         parentEntry = fs->m_roots.back().get();
         return parentEntry;
      }

      auto *dirEntry = dyn_cast<RedirectingDirectoryEntry>(parentEntry);
      dirEntry->addContent(std::move(entry));
      return dirEntry->getLastContent();
   }

   void uniqueOverlayTree(RedirectingFileSystem *fs, Entry *srcEntry,
                          Entry *newParentEntry = nullptr)
   {
      StringRef name = srcEntry->getName();
      switch (srcEntry->getKind()) {
      case EK_Directory: {
         auto *dirEntry = dyn_cast<RedirectingDirectoryEntry>(srcEntry);
         assert(dirEntry && "Must be a directory");
         // Empty directories could be present in the YAML as a way to
         // describe a file for a current directory after some of its subdir
         // is parsed. This only leads to redundant walks, ignore it.
         if (!name.empty()) {
            newParentEntry = lookupOrCreateEntry(fs, name, newParentEntry);
         }
         for (std::unique_ptr<Entry> &subEntry :
              polar::basic::make_range(dirEntry->contentsBegin(), dirEntry->contentsEnd())) {
            uniqueOverlayTree(fs, subEntry.get(), newParentEntry);
         }
         break;
      }
      case EK_File: {
         auto *fileEntry = dyn_cast<RedirectingFileEntry>(srcEntry);
         assert(fileEntry && "Must be a file");
         assert(newParentEntry && "parent entry must exist");
         auto *dirEntry = dyn_cast<RedirectingDirectoryEntry>(newParentEntry);
         dirEntry->addContent(polar::basic::make_unique<RedirectingFileEntry>(
                                 name, fileEntry->getExternalContentsPath(), fileEntry->getUseName()));
         break;
      }
      }
   }

   std::unique_ptr<Entry> parseEntry(yaml::Node *node, RedirectingFileSystem *fs,
                                     bool isRootEntry)
   {
      auto *mappingNode = dyn_cast<yaml::MappingNode>(node);
      if (!mappingNode) {
         error(node, "expected mapping node for file or directory entry");
         return nullptr;
      }

      KeyStatusPair fields[] = {
         KeyStatusPair("name", true),
         KeyStatusPair("type", true),
         KeyStatusPair("contents", false),
         KeyStatusPair("external-contents", false),
         KeyStatusPair("use-external-name", false),
      };

      DenseMap<StringRef, KeyStatus> keys(std::begin(fields), std::end(fields));

      bool hasContents = false; // external or otherwise
      std::vector<std::unique_ptr<Entry>> entryArrayContents;
      std::string externalContentsPath;
      std::string name;
      yaml::Node *nameValueNode;
      auto useExternalName = RedirectingFileEntry::NK_NotSet;
      EntryKind kind;

      for (auto &iter : *mappingNode) {
         StringRef key;
         // Reuse the buffer for key and value, since we don't look at key after
         // parsing value.
         SmallString<256> buffer;
         if (!parseScalarString(iter.getKey(), key, buffer)) {
            return nullptr;
         }

         if (!checkDuplicateOrUnknownKey(iter.getKey(), key, keys)) {
            return nullptr;
         }

         StringRef value;
         if (key == "name") {
            if (!parseScalarString(iter.getValue(), value, buffer)) {
               return nullptr;
            }
            nameValueNode = iter.getValue();
            if (fs->m_useCanonicalizedPaths) {
               SmallString<256> path(value);
               // Guarantee that old YAML files containing paths with ".." and "."
               // are properly canonicalized before read into the vfs.
               path = polar::fs::path::remove_leading_dotslash(path);
               polar::fs::path::remove_dots(path, /*remove_dot_dot=*/true);
               name = path.getStr();
            } else {
               name = value;
            }
         } else if (key == "type") {
            if (!parseScalarString(iter.getValue(), value, buffer)) {
               return nullptr;
            }
            if (value == "file") {
               kind = EK_File;
            } else if (value == "directory") {
               kind = EK_Directory;
            } else {
               error(iter.getValue(), "unknown value for 'type'");
               return nullptr;
            }
         } else if (key == "contents") {
            if (hasContents) {
               error(iter.getKey(),
                     "entry already has 'contents' or 'external-contents'");
               return nullptr;
            }
            hasContents = true;
            auto *m_contents = dyn_cast<yaml::SequenceNode>(iter.getValue());
            if (!m_contents) {
               // FIXME: this is only for directories, what about files?
               error(iter.getValue(), "expected array");
               return nullptr;
            }

            for (auto &iter : *m_contents) {
               if (std::unique_ptr<Entry> entry =
                   parseEntry(&iter, fs, /*isRootEntry*/ false)) {
                  entryArrayContents.push_back(std::move(entry));
               } else {
                  return nullptr;
               }
            }
         } else if (key == "external-contents") {
            if (hasContents) {
               error(iter.getKey(),
                     "entry already has 'contents' or 'external-contents'");
               return nullptr;
            }
            hasContents = true;
            if (!parseScalarString(iter.getValue(), value, buffer)) {
               return nullptr;
            }
            SmallString<256> fullPath;
            if (fs->m_isRelativeOverlay) {
               fullPath = fs->getExternalContentsPrefixDir();
               assert(!fullPath.empty() &&
                      "External contents prefix directory must exist");
               polar::fs::path::append(fullPath, value);
            } else {
               fullPath = value;
            }

            if (fs->m_useCanonicalizedPaths) {
               // Guarantee that old YAML files containing paths with ".." and "."
               // are properly canonicalized before read into the vfs.
               fullPath = polar::fs::path::remove_leading_dotslash(fullPath);
               polar::fs::path::remove_dots(fullPath, /*remove_dot_dot=*/true);
            }
            externalContentsPath = fullPath.getStr();
         } else if (key == "use-external-name") {
            bool value;
            if (!parseScalarBool(iter.getValue(), value)) {
               return nullptr;
            }
            useExternalName = value ? RedirectingFileEntry::NK_External
                                    : RedirectingFileEntry::NK_Virtual;
         } else {
            polar_unreachable("key missing from keys");
         }
      }
      if (m_stream.failed()) {
         return nullptr;
      }
      // check for missing keys
      if (!hasContents) {
         error(node, "missing key 'contents' or 'external-contents'");
         return nullptr;
      }
      if (!checkMissingKeys(node, keys)) {
         return nullptr;
      }

      // check invalid configuration
      if (kind == EK_Directory &&
          useExternalName != RedirectingFileEntry::NK_NotSet) {
         error(node, "'use-external-name' is not supported for directories");
         return nullptr;
      }

      if (isRootEntry && !polar::fs::path::is_absolute(name)) {
         assert(nameValueNode && "name presence should be checked earlier");
         error(nameValueNode,
               "entry with relative path at the root level is not discoverable");
         return nullptr;
      }

      // Remove trailing slash(es), being careful not to remove the root path
      StringRef trimmed(name);
      size_t RootPathLen = polar::fs::path::root_path(trimmed).size();
      while (trimmed.size() > RootPathLen &&
             polar::fs::path::is_separator(trimmed.back())) {
         trimmed = trimmed.slice(0, trimmed.size() - 1);
      }

      // Get the last component
      StringRef lastComponent = polar::fs::path::filename(trimmed);

      std::unique_ptr<Entry> result;
      switch (kind) {
      case EK_File:
         result = polar::basic::make_unique<RedirectingFileEntry>(
                  lastComponent, std::move(externalContentsPath), useExternalName);
         break;
      case EK_Directory:
         result = polar::basic::make_unique<RedirectingDirectoryEntry>(
                  lastComponent, std::move(entryArrayContents),
                  Status("", get_next_virtual_unique_id(), std::chrono::system_clock::now(),
                         0, 0, 0, FileType::directory_file, polar::fs::all_all));
         break;
      }

      StringRef parent = polar::fs::path::parent_path(trimmed);
      if (parent.empty()) {
         return result;
      }

      // if 'name' contains multiple components, create implicit directory entries
      for (polar::fs::path::ReverseIterator iter = polar::fs::path::rbegin(parent),
           end = polar::fs::path::rend(parent);
           iter != end; ++iter) {
         std::vector<std::unique_ptr<Entry>> m_entries;
         m_entries.push_back(std::move(result));
         result = polar::basic::make_unique<RedirectingDirectoryEntry>(
                  *iter, std::move(m_entries),
                  Status("", get_next_virtual_unique_id(), std::chrono::system_clock::now(),
                         0, 0, 0, FileType::directory_file, polar::fs::all_all));
      }
      return result;
   }

public:
   RedirectingFileSystemParser(yaml::Stream &stream)
      : m_stream(stream)
   {}

   // false on error
   bool parse(yaml::Node *root, RedirectingFileSystem *fs)
   {
      auto *top = dyn_cast<yaml::MappingNode>(root);
      if (!top) {
         error(root, "expected mapping node");
         return false;
      }

      KeyStatusPair fields[] = {
         KeyStatusPair("version", true),
         KeyStatusPair("case-sensitive", false),
         KeyStatusPair("use-external-names", false),
         KeyStatusPair("overlay-relative", false),
         KeyStatusPair("fallthrough", false),
         KeyStatusPair("roots", true),
      };

      DenseMap<StringRef, KeyStatus> keys(std::begin(fields), std::end(fields));
      std::vector<std::unique_ptr<Entry>> rootEntries;

      // Parse configuration and 'roots'
      for (auto &iter : *top) {
         SmallString<10> keyBuffer;
         StringRef key;
         if (!parseScalarString(iter.getKey(), key, keyBuffer)) {
            return false;
         }

         if (!checkDuplicateOrUnknownKey(iter.getKey(), key, keys)) {
            return false;
         }
         if (key == "roots") {
            auto *m_roots = dyn_cast<yaml::SequenceNode>(iter.getValue());
            if (!m_roots) {
               error(iter.getValue(), "expected array");
               return false;
            }

            for (auto &iter : *m_roots) {
               if (std::unique_ptr<Entry> entry =
                   parseEntry(&iter, fs, /*isRootEntry*/ true)) {
                  rootEntries.push_back(std::move(entry));
               } else {
                  return false;
               }
            }
         } else if (key == "version") {
            StringRef versionString;
            SmallString<4> storage;
            if (!parseScalarString(iter.getValue(), versionString, storage)) {
               return false;
            }

            int version;
            if (versionString.getAsInteger<int>(10, version)) {
               error(iter.getValue(), "expected integer");
               return false;
            }
            if (version < 0) {
               error(iter.getValue(), "invalid version number");
               return false;
            }
            if (version != 0) {
               error(iter.getValue(), "version mismatch, expected 0");
               return false;
            }
         } else if (key == "case-sensitive") {
            if (!parseScalarBool(iter.getValue(), fs->m_caseSensitive)) {
               return false;
            }
         } else if (key == "overlay-relative") {
            if (!parseScalarBool(iter.getValue(), fs->m_isRelativeOverlay)) {
               return false;
            }
         } else if (key == "use-external-names") {
            if (!parseScalarBool(iter.getValue(), fs->m_useExternalNames)) {
               return false;
            }
         } else if (key == "fallthrough") {
            if (!parseScalarBool(iter.getValue(), fs->m_isFallthrough)) {
               return false;
            }
         } else {
            polar_unreachable("key missing from keys");
         }
      }

      if (m_stream.failed()) {
         return false;
      }

      if (!checkMissingKeys(top, keys)) {
         return false;
      }
      // Now that we sucessefully parsed the YAML file, canonicalize the internal
      // representation to a proper directory tree so that we can search faster
      // inside the vfs.
      for (auto &entry : rootEntries) {
         uniqueOverlayTree(fs, entry.get());
      }
      return true;
   }
};

} // namespace

RedirectingFileSystem *
RedirectingFileSystem::create(std::unique_ptr<MemoryBuffer> buffer,
                              SourceMgr::DiagHandlerTy diagHandler,
                              StringRef yamlFilePath, void *diagContext,
                              IntrusiveRefCountPtr<FileSystem> externalFs)
{
   SourceMgr sm;
   yaml::Stream stream(buffer->getMemBufferRef(), sm);
   sm.setDiagHandler(diagHandler, diagContext);
   yaml::DocumentIterator dirIter = stream.begin();
   yaml::Node *root = dirIter->getRoot();
   if (dirIter == stream.end() || !root) {
      sm.printMessage(SMLocation(), SourceMgr::DK_Error, "expected root node");
      return nullptr;
   }

   RedirectingFileSystemParser parser(stream);

   std::unique_ptr<RedirectingFileSystem> fs(
            new RedirectingFileSystem(std::move(externalFs)));

   if (!yamlFilePath.empty()) {
      // Use the YAML path from -ivfsoverlay to compute the dir to be prefixed
      // to each 'external-contents' path.
      //
      // Example:
      //    -ivfsoverlay dummy.cache/vfs/vfs.yaml
      // yields:
      //  fs->m_externalContentsPrefixDir => /<absolute_path_to>/dummy.cache/vfs
      //
      SmallString<256> overlayAbsDir = polar::fs::path::parent_path(yamlFilePath);
      std::error_code errorCode = polar::fs::make_absolute(overlayAbsDir);
      assert(!errorCode && "Overlay dir final path must be absolute");
      (void)errorCode;
      fs->setExternalContentsPrefixDir(overlayAbsDir);
   }

   if (!parser.parse(root, fs.get())) {
      return nullptr;
   }
   return fs.release();
}

OptionalError<Entry *> RedirectingFileSystem::lookupPath(const Twine &pathTwine) const
{
   SmallString<256> path;
   pathTwine.toVector(path);

   // Handle relative paths
   if (std::error_code errorCode = makeAbsolute(path)) {
      return errorCode;
   }
   // Canonicalize path by removing ".", "..", "./", etc components. This is
   // a vfs request, do bot bother about symlinks in the path components
   // but canonicalize in order to perform the correct entry search.
   if (m_useCanonicalizedPaths) {
      path = polar::fs::path::remove_leading_dotslash(path);
      polar::fs::path::remove_dots(path, /*remove_dot_dot=*/true);
   }

   if (path.empty()) {
      return make_error_code(ErrorCode::invalid_argument);
   }

   polar::fs::path::ConstIterator start = polar::fs::path::begin(path);
   polar::fs::path::ConstIterator end = polar::fs::path::end(path);
   for (const auto &m_root : m_roots) {
      OptionalError<Entry *> result = lookupPath(start, end, m_root.get());
      if (result || result.getError() != ErrorCode::no_such_file_or_directory)
         return result;
   }
   return make_error_code(ErrorCode::no_such_file_or_directory);
}

OptionalError<Entry *>
RedirectingFileSystem::lookupPath(polar::fs::path::ConstIterator start,
                                  polar::fs::path::ConstIterator end,
                                  Entry *from) const
{
#ifndef _WIN32
   assert(!is_traversal_component(*start) &&
          !is_traversal_component(from->getName()) &&
          "Paths should not contain traversal components");
#else
   // FIXME: this is here to support windows, remove it once canonicalized
   // paths become globally default.
   if (start->equals("."))
      ++start;
#endif

   StringRef fromName = from->getName();

   // Forward the search to the next component in case this is an empty one.
   if (!fromName.empty()) {
      if (m_caseSensitive ? !start->equals(fromName)
          : !start->equalsLower(fromName)) {
         // failure to match
         return polar::utils::make_error_code(ErrorCode::no_such_file_or_directory);
      }
      ++start;
      if (start == end) {
         // Match!
         return from;
      }
   }

   auto *dirEntry = dyn_cast<RedirectingDirectoryEntry>(from);
   if (!dirEntry) {
      return polar::utils::make_error_code(ErrorCode::not_a_directory);
   }

   for (const std::unique_ptr<Entry> &dirEntry :
        polar::basic::make_range(dirEntry->contentsBegin(), dirEntry->contentsEnd()))
   {
      OptionalError<Entry *> result = lookupPath(start, end, dirEntry.get());
      if (result || result.getError() != ErrorCode::no_such_file_or_directory) {
         return result;
      }
   }
   return make_error_code(ErrorCode::no_such_file_or_directory);
}

static Status get_redirected_file_status(const Twine &path, bool useExternalNames,
                                         Status externalStatus)
{
   Status status = externalStatus;
   if (!useExternalNames) {
      status = Status::copyWithNewName(status, path.getStr());
   }
   status.IsVFSMapped = true;
   return status;
}

OptionalError<Status> RedirectingFileSystem::getStatus(const Twine &path, Entry *entry)
{
   assert(entry != nullptr);
   if (auto *fileEntry = dyn_cast<RedirectingFileEntry>(entry)) {
      OptionalError<Status> status = m_externalFs->getStatus(fileEntry->getExternalContentsPath());
      assert(!status || status->getName() == fileEntry->getExternalContentsPath());
      if (status) {
         return get_redirected_file_status(path, fileEntry->useExternalName(m_useExternalNames),
                                           *status);
      }
      return status;
   } else { // directory
      auto *dirEntry = cast<RedirectingDirectoryEntry>(entry);
      return Status::copyWithNewName(dirEntry->getStatus(), path.getStr());
   }
}

OptionalError<Status> RedirectingFileSystem::getStatus(const Twine &path)
{
   OptionalError<Entry *> result = lookupPath(path);
   if (!result) {
      if (m_isFallthrough &&
          result.getError() == ErrorCode::no_such_file_or_directory) {
         return m_externalFs->getStatus(path);
      }
      return result.getError();
   }
   return getStatus(path, *result);
}

namespace {

/// Provide a file wrapper with an overriden status.
class FileWithFixedStatus : public File
{
   std::unique_ptr<File> m_innerFile;
   Status m_status;

public:
   FileWithFixedStatus(std::unique_ptr<File> innerFile, Status status)
      : m_innerFile(std::move(innerFile)),
        m_status(std::move(status))
   {}

   OptionalError<Status> getStatus() override
   {
      return m_status;
   }

   OptionalError<std::unique_ptr<MemoryBuffer>>

   getBuffer(const Twine &name, int64_t fileSize, bool requiresNullTerminator,
             bool isVolatile) override {
      return m_innerFile->getBuffer(name, fileSize, requiresNullTerminator,
                                    isVolatile);
   }

   std::error_code close() override
   {
      return m_innerFile->close();
   }
};

} // namespace

OptionalError<std::unique_ptr<File>>
RedirectingFileSystem::openFileForRead(const Twine &path)
{
   OptionalError<Entry *> entry = lookupPath(path);
   if (!entry) {
      if (m_isFallthrough &&
          entry.getError() == ErrorCode::no_such_file_or_directory) {
         return m_externalFs->openFileForRead(path);
      }
      return entry.getError();
   }

   auto *fileEntry = dyn_cast<RedirectingFileEntry>(*entry);
   if (!fileEntry) {// FIXME: ErrorCode::not_a_file?
      return make_error_code(ErrorCode::invalid_argument);
   }

   auto result = m_externalFs->openFileForRead(fileEntry->getExternalContentsPath());
   if (!result) {
      return result;
   }
   auto externalStatus = (*result)->getStatus();
   if (!externalStatus) {
      return externalStatus.getError();
   }

   // FIXME: Update the status with the name and VFSMapped.
   Status status = get_redirected_file_status(path, fileEntry->useExternalName(m_useExternalNames),
                                              *externalStatus);
   return std::unique_ptr<File>(
            polar::basic::make_unique<FileWithFixedStatus>(std::move(*result), status));
}

std::error_code
RedirectingFileSystem::getRealPath(const Twine &path,
                                   SmallVectorImpl<char> &output) const
{
   OptionalError<Entry *> result = lookupPath(path);
   if (!result) {
      if (m_isFallthrough &&
          result.getError() == ErrorCode::no_such_file_or_directory) {
         return m_externalFs->getRealPath(path, output);
      }
      return result.getError();
   }

   if (auto *fileEntry = dyn_cast<RedirectingFileEntry>(*result)) {
      return m_externalFs->getRealPath(fileEntry->getExternalContentsPath(), output);
   }
   // Even if there is a directory entry, fall back to m_externalFs if allowed,
   // because directories don't have a single external contents path.
   return m_isFallthrough ? m_externalFs->getRealPath(path, output)
                          : ErrorCode::invalid_argument;
}

IntrusiveRefCountPtr<FileSystem>
get_vfs_from_yaml(std::unique_ptr<MemoryBuffer> buffer,
                  SourceMgr::DiagHandlerTy diagHandler,
                  StringRef yamlFilePath, void *diagContext,
                  IntrusiveRefCountPtr<FileSystem> externalFs)
{
   return RedirectingFileSystem::create(std::move(buffer), diagHandler,
                                        yamlFilePath, diagContext,
                                        std::move(externalFs));
}

static void get_vfs_entries(Entry *srcEntry, SmallVectorImpl<StringRef> &path,
                            SmallVectorImpl<YAMLVFSEntry> &entries)
{
   auto kind = srcEntry->getKind();
   if (kind == EK_Directory) {
      auto *dirEntry = dyn_cast<RedirectingDirectoryEntry>(srcEntry);
      assert(dirEntry && "Must be a directory");
      for (std::unique_ptr<Entry> &subEntry :
           polar::basic::make_range(dirEntry->contentsBegin(), dirEntry->contentsEnd())) {
         path.push_back(subEntry->getName());
         get_vfs_entries(subEntry.get(), path, entries);
         path.pop_back();
      }
      return;
   }

   assert(kind == EK_File && "Must be a EK_File");
   auto *fileEntry = dyn_cast<RedirectingFileEntry>(srcEntry);
   assert(fileEntry && "Must be a file");
   SmallString<128> vpath;
   for (auto &comp : path) {
      polar::fs::path::append(vpath, comp);
   }
   entries.push_back(YAMLVFSEntry(vpath.getCStr(), fileEntry->getExternalContentsPath()));
}

void collect_vfs_from_yaml(std::unique_ptr<MemoryBuffer> buffer,
                           SourceMgr::DiagHandlerTy diagHandler,
                           StringRef yamlFilePath,
                           SmallVectorImpl<YAMLVFSEntry> &collectedEntries,
                           void *diagContext,
                           IntrusiveRefCountPtr<FileSystem> externalFs)
{
   RedirectingFileSystem *vfs = RedirectingFileSystem::create(
            std::move(buffer), diagHandler, yamlFilePath, diagContext,
            std::move(externalFs));
   OptionalError<Entry *> rootEntry = vfs->lookupPath("/");
   if (!rootEntry) {
      return;
   }
   SmallVector<StringRef, 8> components;
   components.push_back("/");
   get_vfs_entries(*rootEntry, components, collectedEntries);
}

UniqueId get_next_virtual_unique_id()
{
   static std::atomic<unsigned> uid;
   unsigned ID = ++uid;
   // The following assumes that uint64_t max will never collide with a real
   // dev_t value from the m_outStream.
   return UniqueId(std::numeric_limits<uint64_t>::max(), ID);
}

void YAMLVFSWriter::addFileMapping(StringRef virtualPath, StringRef realPath)
{
   assert(polar::fs::path::is_absolute(virtualPath) && "virtual path not absolute");
   assert(polar::fs::path::is_absolute(realPath) && "real path not absolute");
   assert(!path_has_traversal(virtualPath) && "path traversal is not supported");
   m_mappings.emplace_back(virtualPath, realPath);
}

namespace {

class JSONWriter
{
   RawOutStream &m_outStream;
   SmallVector<StringRef, 16> m_dirStack;

   unsigned getDirIndent()
   {
      return 4 * m_dirStack.size();
   }

   unsigned getFileIndent()
   {
      return 4 * (m_dirStack.size() + 1);
   }

   bool containedIn(StringRef parent, StringRef path);
   StringRef containedPart(StringRef parent, StringRef path);
   void startDirectory(StringRef path);
   void endDirectory();
   void writeEntry(StringRef vpath, StringRef rpath);

public:
   JSONWriter(RawOutStream &outStream)
      : m_outStream(outStream)
   {}

   void write(ArrayRef<YAMLVFSEntry> entries, std::optional<bool> useExternalNames,
              std::optional<bool> isCaseSensitive, std::optional<bool> isOverlayRelative,
              StringRef overlayDir);
};

} // namespace

bool JSONWriter::containedIn(StringRef parent, StringRef path)
{
   using namespace polar::utils;

   // Compare each path component.
   auto iparent = polar::fs::path::begin(parent), eparent = polar::fs::path::end(parent);
   for (auto ichild = polar::fs::path::begin(path), echild = polar::fs::path::end(path);
        iparent != eparent && ichild != echild; ++iparent, ++ichild) {
      if (*iparent != *ichild) {
         return false;
      }
   }
   // Have we exhausted the parent path?
   return iparent == eparent;
}

StringRef JSONWriter::containedPart(StringRef parent, StringRef path)
{
   assert(!parent.empty());
   assert(containedIn(parent, path));
   return path.slice(parent.size() + 1, StringRef::npos);
}

void JSONWriter::startDirectory(StringRef path)
{
   StringRef name =
         m_dirStack.empty() ? path : containedPart(m_dirStack.back(), path);
   m_dirStack.push_back(path);
   unsigned indent = getDirIndent();
   m_outStream.indent(indent) << "{\n";
   m_outStream.indent(indent + 2) << "'type': 'directory',\n";
   m_outStream.indent(indent + 2) << "'name': \"" << polar::yaml::escape(name) << "\",\n";
   m_outStream.indent(indent + 2) << "'contents': [\n";
}

void JSONWriter::endDirectory()
{
   unsigned indent = getDirIndent();
   m_outStream.indent(indent + 2) << "]\n";
   m_outStream.indent(indent) << "}";
   m_dirStack.pop_back();
}

void JSONWriter::writeEntry(StringRef vpath, StringRef rpath)
{
   unsigned indent = getFileIndent();
   m_outStream.indent(indent) << "{\n";
   m_outStream.indent(indent + 2) << "'type': 'file',\n";
   m_outStream.indent(indent + 2) << "'name': \"" << polar::yaml::escape(vpath) << "\",\n";
   m_outStream.indent(indent + 2) << "'external-contents': \""
                                  << polar::yaml::escape(rpath) << "\"\n";
   m_outStream.indent(indent) << "}";
}

void JSONWriter::write(ArrayRef<YAMLVFSEntry> entries,
                       std::optional<bool> useExternalNames,
                       std::optional<bool> isCaseSensitive,
                       std::optional<bool> isOverlayRelative,
                       StringRef overlayDir)
{
   using namespace polar::utils;

   m_outStream << "{\n"
                  "  'version': 0,\n";
   if (isCaseSensitive.has_value()) {
      m_outStream << "  'case-sensitive': '"
                  << (isCaseSensitive.value() ? "true" : "false") << "',\n";
   }
   if (useExternalNames.has_value()) {
      m_outStream << "  'use-external-names': '"
                  << (useExternalNames.value() ? "true" : "false") << "',\n";
   }
   bool useOverlayRelative = false;
   if (isOverlayRelative.has_value()) {
      useOverlayRelative = isOverlayRelative.value();
      m_outStream << "  'overlay-relative': '" << (useOverlayRelative ? "true" : "false")
                  << "',\n";
   }
   m_outStream << "  'roots': [\n";

   if (!entries.empty()) {
      const YAMLVFSEntry &entry = entries.front();
      startDirectory(polar::fs::path::parent_path(entry.VPath));
      StringRef rpath = entry.RPath;
      if (useOverlayRelative) {
         unsigned overlayDirLen = overlayDir.size();
         assert(rpath.substr(0, overlayDirLen) == overlayDir &&
                "Overlay dir must be contained in rpath");
         rpath = rpath.slice(overlayDirLen, rpath.size());
      }

      writeEntry(polar::fs::path::filename(entry.VPath), rpath);
      for (const auto &entry : entries.slice(1)) {
         StringRef dir = polar::fs::path::parent_path(entry.VPath);
         if (dir == m_dirStack.back()) {
            m_outStream << ",\n";
         } else {
            while (!m_dirStack.empty() && !containedIn(m_dirStack.back(), dir)) {
               m_outStream << "\n";
               endDirectory();
            }
            m_outStream << ",\n";
            startDirectory(dir);
         }
         StringRef rpath = entry.RPath;
         if (useOverlayRelative) {
            unsigned overlayDirLen = overlayDir.size();
            assert(rpath.substr(0, overlayDirLen) == overlayDir &&
                   "Overlay dir must be contained in rpath");
            rpath = rpath.slice(overlayDirLen, rpath.size());
         }
         writeEntry(polar::fs::path::filename(entry.VPath), rpath);
      }
      while (!m_dirStack.empty()) {
         m_outStream << "\n";
         endDirectory();
      }
      m_outStream << "\n";
   }
   m_outStream << "  ]\n"
               << "}\n";
}

void YAMLVFSWriter::write(RawOutStream &outStream)
{
   polar::basic::sort(m_mappings, [](const YAMLVFSEntry &lhs, const YAMLVFSEntry &rhs) {
      return lhs.VPath < rhs.VPath;
   });

   JSONWriter(outStream).write(m_mappings, m_useExternalNames, m_isCaseSensitive,
                               m_isOverlayRelative, m_overlayDir);
}

VfsFromYamlDirIterImpl::VfsFromYamlDirIterImpl(
      const Twine &pathTwine, RedirectingDirectoryEntry::iterator begin,
      RedirectingDirectoryEntry::iterator end, bool iterateExternalFs,
      FileSystem &externalFs, std::error_code &errorCode)
   : m_dir(pathTwine.getStr()), m_current(begin), m_end(end),
     m_iterateExternalFs(iterateExternalFs), m_externalFs(externalFs)
{
   errorCode = incrementImpl(/*isFirstTime=*/true);
}

std::error_code VfsFromYamlDirIterImpl::increment()
{
   return incrementImpl(/*isFirstTime=*/false);
}

std::error_code VfsFromYamlDirIterImpl::incrementExternal()
{
   assert(!(m_isExternalFsCurrent && m_externalDirIter == DirectoryIterator()) &&
          "incrementing past end");
   std::error_code errorCode;
   if (m_isExternalFsCurrent) {
      m_externalDirIter.increment(errorCode);
   } else if (m_iterateExternalFs) {
      m_externalDirIter = m_externalFs.dirBegin(m_dir, errorCode);
      m_isExternalFsCurrent = true;
      if (errorCode && errorCode != ErrorCode::no_such_file_or_directory) {
         return errorCode;
      }
      errorCode = {};
   }
   if (errorCode || m_externalDirIter == DirectoryIterator()) {
      m_currentEntry = DirectoryEntry();
   } else {
      m_currentEntry = *m_externalDirIter;
   }
   return errorCode;
}

std::error_code VfsFromYamlDirIterImpl::incrementContent(bool isFirstTime)
{
   assert((isFirstTime || m_current != m_end) && "cannot iterate past end");
   if (!isFirstTime) {
      ++m_current;
   }

   while (m_current != m_end) {
      SmallString<128> pathStr(m_dir);
      polar::fs::path::append(pathStr, (*m_current)->getName());
      polar::fs::FileType type;
      switch ((*m_current)->getKind()) {
      case EK_Directory:
         type = polar::fs::FileType::directory_file;
         break;
      case EK_File:
         type = polar::fs::FileType::regular_file;
         break;
      }
      m_currentEntry = DirectoryEntry(pathStr.getStr(), type);
      return {};
   }
   return incrementExternal();
}

std::error_code VfsFromYamlDirIterImpl::incrementImpl(bool isFirstTime)
{
   while (true) {
      std::error_code errorCode = m_isExternalFsCurrent ? incrementExternal()
                                                        : incrementContent(isFirstTime);
      if (errorCode || m_currentEntry.path().empty())
         return errorCode;
      StringRef name = polar::fs::path::filename(m_currentEntry.path());
      if (m_seenNames.insert(name).second)
         return errorCode; // name not seen before
   }
   polar_unreachable("returned above");
}

RecursiveDirectoryIterator::RecursiveDirectoryIterator(
      FileSystem &fs, const Twine &path, std::error_code &errorCode)
   : m_fs(&fs)
{
   DirectoryIterator iter = m_fs->dirBegin(path, errorCode);
   if (iter != DirectoryIterator()) {
      m_state = std::make_shared<internal::RecDirIterState>();
      m_state->Stack.push(iter);
   }
}

RecursiveDirectoryIterator &
RecursiveDirectoryIterator::increment(std::error_code &errorCode)
{
   assert(m_fs && m_state && !m_state->Stack.empty() && "incrementing past end");
   assert(!m_state->Stack.top()->path().empty() && "non-canonical end iterator");
   vfs::DirectoryIterator end;

   if (m_state->HasNoPushRequest)
      m_state->HasNoPushRequest = false;
   else {
      if (m_state->Stack.top()->type() == polar::fs::FileType::directory_file) {
         DirectoryIterator iter = m_fs->dirBegin(m_state->Stack.top()->path(), errorCode);
         if (iter != end) {
            m_state->Stack.push(iter);
            return *this;
         }
      }
   }
   while (!m_state->Stack.empty() && m_state->Stack.top().increment(errorCode) == end) {
      m_state->Stack.pop();
   }
   if (m_state->Stack.empty()) {
       m_state.reset(); // end iterator
   }
   return *this;
}

} // vfs
} // polar
