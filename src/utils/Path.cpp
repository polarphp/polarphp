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

#include "polarphp/utils/Path.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/global/Config.h"
#include "polarphp/utils/Endian.h"
#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Process.h"
#include "polarphp/utils/Signals.h"
#include <cctype>
#include <cstring>

#if !defined(_MSC_VER) && !defined(__MINGW32__)
#include <unistd.h>
#else
#include <io.h>
#endif

using namespace polar::utils::endian;

enum FSEntity
{
   FS_Dir,
   FS_File,
   FS_Name
};

namespace polar {
namespace fs {

using polar::basic::StringRef;
using polar::utils::error_code_to_error;
using polar::fs::path::Style;
using polar::sys::Process;
using polar::basic::SmallVector;
using polar::basic::Twine;
using polar::basic::SmallVectorImpl;
using polar::basic::SmallString;
using polar::utils::ErrorCode;

namespace {

inline Style real_style(Style style)
{
#ifdef _WIN32
   return (style == Style::posix) ? Style::posix : Style::windows;
#else
   return (style == Style::windows) ? Style::windows : Style::posix;
#endif
}

inline const char *separators(Style style)
{
   if (real_style(style) == Style::windows) {
      return "\\/";
   }
   return "/";
}

inline char preferred_separator(Style style) {
   if (real_style(style) == Style::windows) {
      return '\\';
   }
   return '/';
}

StringRef find_first_component(StringRef path, Style style)
{
   // Look for this first component in the following order.
   // * empty (in this case we return an empty string)
   // * either C: or {//,\\}net.
   // * {/,\}
   // * {file,directory}name

   if (path.empty()) {
      return path;
   }
   if (real_style(style) == Style::windows) {
      // C:
      if (path.getSize() >= 2 &&
          std::isalpha(static_cast<unsigned char>(path[0])) && path[1] == ':') {
         return path.substr(0, 2);
      }
   }

   // //net
   if ((path.getSize() > 2) && is_separator(path[0], style) &&
       path[0] == path[1] && !is_separator(path[2], style)) {
      // Find the next directory separator.
      size_t end = path.findFirstOf(separators(style), 2);
      return path.substr(0, end);
   }

   // {/,\}
   if (is_separator(path[0], style)) {
      return path.substr(0, 1);
   }

   // * {file,directory}name
   size_t end = path.findFirstOf(separators(style));
   return path.substr(0, end);
}

// Returns the first character of the filename in str. For paths ending in
// '/', it returns the position of the '/'.
size_t filename_pos(StringRef str, Style style)
{
   if (str.getSize() > 0 && is_separator(str[str.getSize() - 1], style)) {
      return str.getSize() - 1;
   }
   size_t pos = str.findLastOf(separators(style), str.getSize() - 1);

   if (real_style(style) == Style::windows) {
      if (pos == StringRef::npos) {
         pos = str.findLastOf(':', str.getSize() - 2);
      }
   }
   if (pos == StringRef::npos || (pos == 1 && is_separator(str[0], style))) {
      return 0;
   }
   return pos + 1;
}

// Returns the position of the root directory in str. If there is no root
// directory in str, it returns StringRef::npos.
size_t root_dir_start(StringRef str, Style style)
{
   // case "c:/"
   if (real_style(style) == Style::windows) {
      if (str.getSize() > 2 && str[1] == ':' && is_separator(str[2], style)) {
         return 2;
      }
   }

   // case "//net"
   if (str.getSize() > 3 && is_separator(str[0], style) && str[0] == str[1] &&
       !is_separator(str[2], style)) {
      return str.findFirstOf(separators(style), 2);
   }

   // case "/"
   if (str.getSize() > 0 && is_separator(str[0], style)) {
      return 0;
   }
   return StringRef::npos;
}

// Returns the position past the end of the "parent path" of path. The parent
// path will not end in '/', unless the parent is the root directory. If the
// path has no parent, 0 is returned.
size_t parent_path_end(StringRef path, Style style)
{
   size_t endPos = filename_pos(path, style);
   bool filenameWasSep =
         path.getSize() > 0 && is_separator(path[endPos], style);

   // Skip separators until we reach root dir (or the start of the string).
   size_t rootDirPos = root_dir_start(path, style);
   while (endPos > 0 &&
          (rootDirPos == StringRef::npos || endPos > rootDirPos) &&
          is_separator(path[endPos - 1], style)) {
      --endPos;
   }


   if (endPos == rootDirPos && !filenameWasSep) {
      // We've reached the root dir and the input path was *not* ending in a
      // sequence of slashes. Include the root dir in the parent path.
      return rootDirPos + 1;
   }

   // Otherwise, just include before the last slash.
   return endPos;
}

std::error_code create_unique_entity(const Twine &model, int &resultFD,
                                     SmallVectorImpl<char> &resultPath, bool makeAbsolute,
                                     unsigned mode, FSEntity type,
                                     polar::fs::OpenFlags flags = polar::fs::F_None) {
   SmallString<128> modelStorage;
   model.toVector(modelStorage);

   if (makeAbsolute) {
      // Make model absolute by prepending a temp directory if it's not already.
      if (!path::is_absolute(Twine(modelStorage))) {
         SmallString<128> tdir;
         path::system_temp_directory(true, tdir);
         path::append(tdir, Twine(modelStorage));
         modelStorage.swap(tdir);
      }
   }

   // From here on, DO NOT modify model. It may be needed if the randomly chosen
   // path already exists.
   resultPath = modelStorage;
   // Null terminate.
   resultPath.push_back(0);
   resultPath.pop_back();

   // Limit the number of attempts we make, so that we don't infinite loop. E.g.
   // "permission denied" could be for a specific file (so we retry with a
   // different name) or for the whole directory (retry would always fail).
   // Checking which is racy, so we try a number of times, then give up.
   std::error_code errorCode;
   for (int retries = 128; retries > 0; --retries) {
      // Replace '%' with random chars.
      for (unsigned i = 0, e = modelStorage.size(); i != e; ++i) {
         if (modelStorage[i] == '%') {
            resultPath[i] =
                  "0123456789abcdef"[sys::Process::getRandomNumber() & 15];
         }
      }

      // Try to open + create the file.
      switch (type) {
      case FS_File: {
         errorCode = polar::fs::open_file_for_read_write(Twine(resultPath.begin()), resultFD,
                                                         polar::fs::CD_CreateNew, flags, mode);
         if (errorCode) {
            // errc::permission_denied happens on Windows when we try to open a file
            // that has been marked for deletion.
            if (errorCode == ErrorCode::file_exists || errorCode == ErrorCode::permission_denied) {
               continue;
            }

            return errorCode;
         }

         return std::error_code();
      }

      case FS_Name: {
         errorCode = polar::fs::access(resultPath.begin(), polar::fs::AccessMode::Exist);
         if (errorCode == ErrorCode::no_such_file_or_directory) {
            return std::error_code();
         }
         if (errorCode) {
            return errorCode;
         }
         continue;
      }

      case FS_Dir: {
         errorCode = polar::fs::create_directory(resultPath.begin(), false);
         if (errorCode) {
            if (errorCode == ErrorCode::file_exists) {
               continue;
            }
            return errorCode;
         }
         return std::error_code();
      }
      }
      polar_unreachable("Invalid Type");
   }
   return errorCode;
}

} // end unnamed namespace

namespace path {

ConstIterator begin(StringRef path, Style style)
{
   ConstIterator iter;
   iter.m_path      = path;
   iter.m_component = find_first_component(path, style);
   iter.m_position  = 0;
   iter.m_style = style;
   return iter;
}

ConstIterator end(StringRef path)
{
   ConstIterator iter;
   iter.m_path      = path;
   iter.m_position  = path.getSize();
   return iter;
}

ConstIterator &ConstIterator::operator++()
{
   assert(m_position < m_path.getSize() && "Tried to increment past end!");
   // Increment Position to past the current component
   m_position += m_component.getSize();
   // Check for end.
   if (m_position == m_path.getSize()) {
      m_component = StringRef();
      return *this;
   }
   // Both POSIX and Windows treat paths that begin with exactly two separators
   // specially.
   bool wasNet = m_component.getSize() > 2 && is_separator(m_component[0], m_style) &&
         m_component[1] == m_component[0] && !is_separator(m_component[2], m_style);
   // Handle separators.
   if (is_separator(m_path[m_position], m_style)) {
      // Root dir.
      if (wasNet ||
          // c:/
          (real_style(m_style) == Style::windows && m_component.endsWith(":"))) {
         m_component = m_path.substr(m_position, 1);
         return *this;
      }

      // Skip extra separators.
      while (m_position != m_path.getSize() && is_separator(m_path[m_position], m_style)) {
         ++m_position;
      }

      // Treat trailing '/' as a '.'.
      if (m_position == m_path.getSize() && m_component != "/") {
         --m_position;
         m_component = ".";
         return *this;
      }
   }

   // Find next component.
   size_t endPos = m_path.findFirstOf(separators(m_style), m_position);
   m_component = m_path.slice(m_position, endPos);
   return *this;
}

bool ConstIterator::operator==(const ConstIterator &rhs) const
{
   return m_path.begin() == rhs.m_path.begin() && m_position == rhs.m_position;
}

ptrdiff_t ConstIterator::operator-(const ConstIterator &rhs) const
{
   return m_position - rhs.m_position;
}

ReverseIterator rbegin(StringRef path, Style style)
{
   ReverseIterator iter;
   iter.m_path = path;
   iter.m_position = path.getSize();
   iter.m_style = style;
   return ++iter;
}

ReverseIterator rend(StringRef path)
{
   ReverseIterator iter;
   iter.m_path = path;
   iter.m_component = path.substr(0, 0);
   iter.m_position = 0;
   return iter;
}

ReverseIterator &ReverseIterator::operator++()
{
   size_t rootDirPos = root_dir_start(m_path, m_style);

   // Skip separators unless it's the root directory.
   size_t endPos = m_position;
   while (endPos > 0 && (endPos - 1) != rootDirPos &&
          is_separator(m_path[endPos - 1], m_style))
      --endPos;

   // Treat trailing '/' as a '.', unless it is the root dir.
   if (m_position == m_path.size() && !m_path.empty() &&
       is_separator(m_path.back(), m_style) &&
       (rootDirPos == StringRef::npos || endPos - 1 > rootDirPos)) {
      --m_position;
      m_component = ".";
      return *this;
   }

   // Find next separator.
   size_t start_pos = filename_pos(m_path.substr(0, endPos), m_style);
   m_component = m_path.slice(start_pos, endPos);
   m_position = start_pos;
   return *this;
}

bool ReverseIterator::operator==(const ReverseIterator &rhs) const
{
   return m_path.begin() == rhs.m_path.begin() && m_component == rhs.m_component &&
         m_position == rhs.m_position;
}

ptrdiff_t ReverseIterator::operator-(const ReverseIterator &rhs) const
{
   return m_position - rhs.m_position;
}

StringRef root_path(StringRef path, Style style)
{
   ConstIterator b = begin(path, style), pos = b, e = end(path);
   if (b != e) {
      bool hasNet =
            b->getSize() > 2 && is_separator((*b)[0], style) && (*b)[1] == (*b)[0];
      bool hasDrive = (real_style(style) == Style::windows) && b->endsWith(":");

      if (hasNet || hasDrive) {
         if ((++pos != e) && is_separator((*pos)[0], style)) {
            // {C:/,//net/}, so get the first two components.
            return path.substr(0, b->getSize() + pos->getSize());
         } else {
            // just {C:,//net}, return the first component.
            return *b;
         }
      }

      // POSIX style root directory.
      if (is_separator((*b)[0], style)) {
         return *b;
      }
   }

   return StringRef();
}

StringRef root_name(StringRef path, Style style)
{
   ConstIterator b = begin(path, style), e = end(path);
   if (b != e) {
      bool hasNet =
            b->getSize() > 2 && is_separator((*b)[0], style) && (*b)[1] == (*b)[0];
      bool hasDrive = (real_style(style) == Style::windows) && b->endsWith(":");

      if (hasNet || hasDrive) {
         // just {C:,//net}, return the first component.
         return *b;
      }
   }

   // No path or no name.
   return StringRef();
}

StringRef root_directory(StringRef path, Style style)
{
   ConstIterator b = begin(path, style), pos = b, e = end(path);
   if (b != e) {
      bool hasNet =
            b->getSize() > 2 && is_separator((*b)[0], style) && (*b)[1] == (*b)[0];
      bool hasDrive = (real_style(style) == Style::windows) && b->endsWith(":");

      if ((hasNet || hasDrive) &&
          // {C:,//net}, skip to the next component.
          (++pos != e) && is_separator((*pos)[0], style)) {
         return *pos;
      }

      // POSIX style root directory.
      if (!hasNet && is_separator((*b)[0], style)) {
         return *b;
      }
   }

   // No path or no root.
   return StringRef();
}

StringRef relative_path(StringRef path, Style style)
{
   StringRef root = root_path(path, style);
   return path.substr(root.getSize());
}

void append(SmallVectorImpl<char> &path, Style style, const Twine &a,
            const Twine &b, const Twine &c, const Twine &d)
{
   SmallString<32> a_storage;
   SmallString<32> b_storage;
   SmallString<32> c_storage;
   SmallString<32> d_storage;

   SmallVector<StringRef, 4> components;
   if (!a.isTriviallyEmpty()) components.push_back(a.toStringRef(a_storage));
   if (!b.isTriviallyEmpty()) components.push_back(b.toStringRef(b_storage));
   if (!c.isTriviallyEmpty()) components.push_back(c.toStringRef(c_storage));
   if (!d.isTriviallyEmpty()) components.push_back(d.toStringRef(d_storage));

   for (auto &component : components) {
      bool path_has_sep =
            !path.empty() && is_separator(path[path.getSize() - 1], style);
      if (path_has_sep) {
         // Strip separators from beginning of component.
         size_t loc = component.findFirstNotOf(separators(style));
         StringRef c = component.substr(loc);

         // Append it.
         path.append(c.begin(), c.end());
         continue;
      }

      bool componentHasSep =
            !component.empty() && is_separator(component[0], style);
      if (!componentHasSep &&
          !(path.empty() || has_root_name(component, style))) {
         // Add a separator.
         path.push_back(preferred_separator(style));
      }

      path.append(component.begin(), component.end());
   }
}

void append(SmallVectorImpl<char> &path, const Twine &a, const Twine &b,
            const Twine &c, const Twine &d)
{
   append(path, Style::native, a, b, c, d);
}

void append(SmallVectorImpl<char> &path, ConstIterator begin,
            ConstIterator end, Style style)
{
   for (; begin != end; ++begin) {
      path::append(path, style, *begin);
   }
}

StringRef parent_path(StringRef path, Style style)
{
   size_t endPos = parent_path_end(path, style);
   if (endPos == StringRef::npos) {
      return StringRef();
   } else {
      return path.substr(0, endPos);
   }
}

void remove_filename(SmallVectorImpl<char> &path, Style style)
{
   size_t endPos = parent_path_end(StringRef(path.begin(), path.getSize()), style);
   if (endPos != StringRef::npos) {
      path.setSize(endPos);
   }
}

void replace_extension(SmallVectorImpl<char> &path, const Twine &extension,
                       Style style)
{
   StringRef p(path.begin(), path.getSize());
   SmallString<32> ext_storage;
   StringRef ext = extension.toStringRef(ext_storage);

   // Erase existing extension.
   size_t pos = p.findLastOf('.');
   if (pos != StringRef::npos && pos >= filename_pos(p, style)) {
      path.setSize(pos);
   }
   // Append '.' if needed.
   if (ext.getSize() > 0 && ext[0] != '.') {
      path.push_back('.');
   }
   // Append extension.
   path.append(ext.begin(), ext.end());
}

void replace_path_prefix(SmallVectorImpl<char> &pathVector,
                         const StringRef &oldPrefix, const StringRef &newPrefix,
                         Style style)
{
   if (oldPrefix.empty() && newPrefix.empty()) {
      return;
   }
   StringRef origPath(pathVector.begin(), pathVector.getSize());
   if (!origPath.startsWith(oldPrefix)) {
      return;
   }
   // If prefixes have the same size we can simply copy the new one over.
   if (oldPrefix.getSize() == newPrefix.getSize()) {
      polar::basic::copy(newPrefix, pathVector.begin());
      return;
   }

   StringRef relPath = origPath.substr(oldPrefix.getSize());
   SmallString<256> newPath;
   path::append(newPath, style, newPrefix);
   path::append(newPath, style, relPath);
   pathVector.swap(newPath);
}

void native(const Twine &path, SmallVectorImpl<char> &result, Style style)
{
   assert((!path.isSingleStringRef() ||
           path.getSingleStringRef().getData() != result.getData()) &&
          "path and result are not allowed to overlap!");
   // Clear result.
   result.clear();
   path.toVector(result);
   native(result, style);
}

void native(SmallVectorImpl<char> &pathVector, Style style)
{
   if (pathVector.empty()) {
      return;
   }
   if (real_style(style) == Style::windows) {
      std::replace(pathVector.begin(), pathVector.end(), '/', '\\');
      if (pathVector[0] == '~' && (pathVector.getSize() == 1 || is_separator(pathVector[1], style))) {
         SmallString<128> pathHome;
         home_directory(pathHome);
         pathHome.append(pathVector.begin() + 1, pathVector.end());
         pathVector = pathHome;
      }
   } else {
      for (auto piter = pathVector.begin(), pend = pathVector.end(); piter < pend; ++piter) {
         if (*piter == '\\') {
            auto pnext = piter + 1;
            if (pnext < pend && *pnext == '\\') {
               ++piter; // increment once, the for loop will move over the escaped slash
            } else {
               *piter = '/';
            }
         }
      }
   }
}

std::string convert_to_slash(StringRef path, Style style)
{
   if (real_style(style) != Style::windows) {
      return path;
   }
   std::string str = path.getStr();
   std::replace(str.begin(), str.end(), '\\', '/');
   return str;
}

StringRef filename(StringRef path, Style style)
{
   return *rbegin(path, style);
}

StringRef stem(StringRef path, Style style)
{
   StringRef fname = filename(path, style);
   size_t pos = fname.findLastOf('.');
   if (pos == StringRef::npos) {
      return fname;
   } else {
      if ((fname.getSize() == 1 && fname == ".") ||
          (fname.getSize() == 2 && fname == "..")) {
         return fname;
      } else {
         return fname.substr(0, pos);
      }
   }
}

StringRef extension(StringRef path, Style style)
{
   StringRef fname = filename(path, style);
   size_t pos = fname.findLastOf('.');
   if (pos == StringRef::npos) {
      return StringRef();
   } else {
      if ((fname.getSize() == 1 && fname == ".") ||
          (fname.getSize() == 2 && fname == "..")) {
         return StringRef();
      } else {
         return fname.substr(pos);
      }
   }
}

bool is_separator(char value, Style style)
{
   if (value == '/') {
      return true;
   }
   if (real_style(style) == Style::windows) {
      return value == '\\';
   }
   return false;
}

StringRef get_separator(Style style)
{
   if (real_style(style) == Style::windows) {
      return "\\";
   }
   return "/";
}

bool has_root_name(const Twine &path, Style style)
{
   SmallString<128> pathStorage;
   StringRef p = path.toStringRef(pathStorage);
   return !root_name(p, style).empty();
}

bool has_root_directory(const Twine &path, Style style)
{
   SmallString<128> pathStorage;
   StringRef p = path.toStringRef(pathStorage);
   return !root_directory(p, style).empty();
}

bool has_root_path(const Twine &path, Style style)
{
   SmallString<128> pathStorage;
   StringRef p = path.toStringRef(pathStorage);
   return !root_path(p, style).empty();
}

bool has_relative_path(const Twine &path, Style style)
{
   SmallString<128> pathStorage;
   StringRef p = path.toStringRef(pathStorage);
   return !relative_path(p, style).empty();
}

bool has_filename(const Twine &path, Style style)
{
   SmallString<128> pathStorage;
   StringRef p = path.toStringRef(pathStorage);
   return !filename(p, style).empty();
}

bool has_parent_path(const Twine &path, Style style)
{
   SmallString<128> pathStorage;
   StringRef p = path.toStringRef(pathStorage);
   return !parent_path(p, style).empty();
}

bool has_stem(const Twine &path, Style style)
{
   SmallString<128> pathStorage;
   StringRef p = path.toStringRef(pathStorage);
   return !stem(p, style).empty();
}

bool has_extension(const Twine &path, Style style)
{
   SmallString<128> pathStorage;
   StringRef p = path.toStringRef(pathStorage);
   return !extension(p, style).empty();
}

bool is_absolute(const Twine &path, Style style)
{
   SmallString<128> pathStorage;
   StringRef p = path.toStringRef(pathStorage);
   bool rootDir = has_root_directory(p, style);
   bool rootName =
         (real_style(style) != Style::windows) || has_root_name(p, style);
   return rootDir && rootName;
}

bool is_relative(const Twine &path, Style style)
{
   return !is_absolute(path, style);
}

StringRef remove_leading_dotslash(StringRef pathStr, Style style)
{
   // Remove leading "./" (or ".//" or "././" etc.)
   while (pathStr.getSize() > 2 && pathStr[0] == '.' && is_separator(pathStr[1], style)) {
      pathStr = pathStr.substr(2);
      while (pathStr.getSize() > 0 && is_separator(pathStr[0], style)) {
         pathStr = pathStr.substr(1);
      }
   }
   return pathStr;
}

static SmallString<256> remove_dots(StringRef path, bool remove_dot_dot,
                                    Style style)
{
   SmallVector<StringRef, 16> components;
   // Skip the root path, then look for traversal in the components.
   StringRef rel = path::relative_path(path, style);
   for (StringRef C :
        polar::basic::make_range(path::begin(rel, style), path::end(rel))) {
      if (C == ".")
         continue;
      // Leading ".." will remain in the path unless it's at the root.
      if (remove_dot_dot && C == "..") {
         if (!components.empty() && components.getBack() != "..") {
            components.pop_back();
            continue;
         }
         if (path::is_absolute(path, style)) {
            continue;
         }
      }
      components.push_back(C);
   }

   SmallString<256> buffer = path::root_path(path, style);
   for (StringRef C : components)
      path::append(buffer, style, C);
   return buffer;
}

bool remove_dots(SmallVectorImpl<char> &path, bool remove_dot_dot,
                 Style style)
{
   StringRef p(path.getData(), path.getSize());

   SmallString<256> result = remove_dots(p, remove_dot_dot, style);
   if (result == path) {
      return false;
   }
   path.swap(result);
   return true;
}

} // path

std::error_code get_unique_id(const Twine path, UniqueId &result)
{
   FileStatus fstatus;
   std::error_code errorCode = status(path, fstatus);
   if (errorCode) {
      return errorCode;
   }
   result = fstatus.getUniqueId();
   return std::error_code();
}

std::error_code create_unique_file(const Twine &model, int &resultFd,
                                   SmallVectorImpl<char> &resultPath,
                                   unsigned mode)
{
   return create_unique_entity(model, resultFd, resultPath, false, mode, FS_File);
}

static std::error_code create_unique_file(const Twine &model, int &resultFd,
                                          SmallVectorImpl<char> &resultPath,
                                          unsigned mode, OpenFlags flags)
{
   return create_unique_entity(model, resultFd, resultPath, false, mode, FS_File,
                               flags);
}

std::error_code create_unique_file(const Twine &model,
                                   SmallVectorImpl<char> &resultPath,
                                   unsigned mode) {
   int fd;
   auto errorCode = create_unique_file(model, fd, resultPath, mode);
   if (errorCode) {
      return errorCode;
   }
   // FD is only needed to avoid race conditions. Close it right away.
   close(fd);
   return errorCode;
}

namespace {

std::error_code create_temporary_file(const Twine &model, int &resultFD,
                                      SmallVectorImpl<char> &resultPath, FSEntity type) {
   SmallString<128> storage;
   StringRef p = model.toNullTerminatedStringRef(storage);
   assert(p.findFirstOf(separators(Style::native)) == StringRef::npos &&
          "model must be a simple filename.");
   // Use P.begin() so that createUniqueEntity doesn't need to recreate storage.
   return create_unique_entity(p.begin(), resultFD, resultPath, true,
                               owner_read | owner_write, type);
}

std::error_code create_temporary_file(const Twine &prefix, StringRef suffix, int &resultFD,
                                      SmallVectorImpl<char> &resultPath, FSEntity type) {
   const char *middle = suffix.empty() ? "-%%%%%%" : "-%%%%%%.";
   return create_temporary_file(prefix + middle + suffix, resultFD, resultPath,
                                type);
}

} // anonymous namespace

std::error_code create_temporary_file(const Twine &prefix, StringRef suffix,
                                      int &resultFD,
                                      SmallVectorImpl<char> &resultPath)
{
   return create_temporary_file(prefix, suffix, resultFD, resultPath, FS_File);
}

std::error_code create_temporary_file(const Twine &prefix, StringRef suffix,
                                      SmallVectorImpl<char> &resultPath)
{
   int fd;
   auto errorCode = create_temporary_file(prefix, suffix, fd, resultPath);
   if (errorCode) {
      return errorCode;
   }
   // FD is only needed to avoid race conditions. Close it right away.
   close(fd);
   return errorCode;
}

// This is a mkdtemp with a different pattern. We use create_unique_entity mostly
// for consistency. We should try using mkdtemp.
std::error_code create_unique_directory(const Twine &prefix,
                                        SmallVectorImpl<char> &resultPath)
{
   int dummy;
   return create_unique_entity(prefix + "-%%%%%%", dummy, resultPath,
                               true, 0, FS_Dir);
}

static std::error_code make_absolute(const Twine &currentDirectory,
                                     SmallVectorImpl<char> &path,
                                     bool useCurrentDirectory)
{
   StringRef p(path.getData(), path.getSize());

   bool rootDirectory = path::has_root_directory(p);
   bool rootName =
         (real_style(Style::native) != Style::windows) || path::has_root_name(p);

   // Already absolute.
   if (rootName && rootDirectory) {
      return std::error_code();
   }

   // All of the following conditions will need the current directory.
   SmallString<128> currentDir;
   if (useCurrentDirectory) {
      currentDirectory.toVector(currentDir);
   } else if (std::error_code ec = current_path(currentDir)) {
      return ec;
   }
   // Relative path. Prepend the current directory.
   if (!rootName && !rootDirectory) {
      // Append path to the current directory.
      path::append(currentDir, p);
      // Set path to the result.
      path.swap(currentDir);
      return std::error_code();
   }

   if (!rootName && rootDirectory) {
      StringRef cdrn = path::root_name(currentDir);
      SmallString<128> curDirRootName(cdrn.begin(), cdrn.end());
      path::append(curDirRootName, p);
      // Set path to the result.
      path.swap(curDirRootName);
      return std::error_code();
   }

   if (rootName && !rootDirectory) {
      StringRef pRootName      = path::root_name(p);
      StringRef bRootDirectory = path::root_directory(currentDir);
      StringRef bRelativePath  = path::relative_path(currentDir);
      StringRef pRelativePath  = path::relative_path(p);

      SmallString<128> res;
      path::append(res, pRootName, bRootDirectory, bRelativePath, pRelativePath);
      path.swap(res);
      return std::error_code();
   }

   polar_unreachable("All rootName and rootDirectory combinations should have "
                     "occurred above!");
}

std::error_code make_absolute(const Twine &currentDirectory,
                              SmallVectorImpl<char> &path)
{
   return make_absolute(currentDirectory, path, true);
}

std::error_code make_absolute(SmallVectorImpl<char> &path)
{
   return make_absolute(Twine(), path, false);
}

std::error_code create_directories(const Twine &path, bool ignoreExisting,
                                   Permission perms) {
   SmallString<128> pathStorage;
   StringRef p = path.toStringRef(pathStorage);

   // Be optimistic and try to create the directory
   std::error_code errorCode = create_directory(p, ignoreExisting, perms);
   // If we succeeded, or had any error other than the parent not existing, just
   // return it.
   if (errorCode != ErrorCode::no_such_file_or_directory) {
      return errorCode;
   }

   // We failed because of a no_such_file_or_directory, try to create the
   // parent.
   StringRef parent = path::parent_path(p);
   if (parent.empty()) {
      return errorCode;
   }
   if ((errorCode = create_directories(parent, ignoreExisting, perms))) {
      return errorCode;
   }
   return create_directory(p, ignoreExisting, perms);
}

namespace {
static std::error_code copy_file_internal(int readFD, int writeFD) {
   const size_t BufSize = 4096;
   char *buf = new char[BufSize];
   int bytesRead = 0, bytesWritten = 0;
   for (;;) {
      bytesRead = read(readFD, buf, BufSize);
      if (bytesRead <= 0)
         break;
      while (bytesRead) {
         bytesWritten = write(writeFD, buf, bytesRead);
         if (bytesWritten < 0)
            break;
         bytesRead -= bytesWritten;
      }
      if (bytesWritten < 0)
         break;
   }
   delete[] buf;

   if (bytesRead < 0 || bytesWritten < 0) {
      return std::error_code(errno, std::generic_category());
   }

   return std::error_code();
}
} // anonymous namespace

std::error_code copy_file(const Twine &from, const Twine &to) {
   int readFD, writeFD;
   if (std::error_code errorCode = open_file_for_read(from, readFD, OF_None)) {
      return errorCode;
   }
   if (std::error_code errorCode = open_file_for_write(to, writeFD, CD_CreateAlways, F_None)) {
      close(readFD);
      return errorCode;
   }

   std::error_code errorCode = copy_file_internal(readFD, writeFD);
   close(readFD);
   close(writeFD);
   return errorCode;
}

std::error_code copy_file(const Twine &from, int toFD)
{
   int readFD;
   if (std::error_code errorCode = open_file_for_read(from, readFD, OF_None)) {
      return errorCode;
   }
   std::error_code errorCode = copy_file_internal(readFD, toFD);
   close(readFD);
   return errorCode;
}

OptionalError<Md5::Md5Result> md5_contents(int fd)
{
   Md5 hash;
   constexpr size_t bufSize = 4096;
   std::vector<uint8_t> buf(bufSize);
   int bytesRead = 0;
   for (;;) {
      bytesRead = read(fd, buf.data(), bufSize);
      if (bytesRead <= 0) {
         break;
      }
      hash.update(polar::basic::make_array_ref(buf.data(), bytesRead));
   }

   if (bytesRead < 0) {
      return std::error_code(errno, std::generic_category());
   }

   Md5::Md5Result result;
   hash.final(result);
   return result;
}

OptionalError<Md5::Md5Result> md5_contents(const Twine &path)
{
   int fd;
   if (auto errorCode = open_file_for_read(path, fd)) {
      return errorCode;
   }
   auto result = md5_contents(fd);
   close(fd);
   return result;
}

bool exists(const BasicFileStatus &status)
{
   return status_known(status) && status.getType() != FileType::file_not_found;
}

bool status_known(const BasicFileStatus &status)
{
   return status.getType() != FileType::status_error;
}

FileType get_file_type(const Twine &path, bool follow)
{
   FileStatus fstatus;
   if (status(path, fstatus, follow)) {
      return FileType::status_error;
   }
   return fstatus.getType();
}

bool is_directory(const BasicFileStatus &status)
{
   return status.getType() == FileType::directory_file;
}

std::error_code is_directory(const Twine &path, bool &result)
{
   FileStatus fstatus;
   if (std::error_code errorCode = status(path, fstatus)) {
      return errorCode;
   }
   result = is_directory(fstatus);
   return std::error_code();
}

bool is_regular_file(const BasicFileStatus &status)
{
   return status.getType() == FileType::regular_file;
}

std::error_code is_regular_file(const Twine &path, bool &result)
{
   FileStatus fstatus;
   if (std::error_code errorCode = status(path, fstatus)) {
      return errorCode;
   }
   result = is_regular_file(fstatus);
   return std::error_code();
}

bool is_symlink_file(const BasicFileStatus &status)
{
   return status.getType() == FileType::symlink_file;
}

std::error_code is_symlink_file(const Twine &path, bool &result)
{
   FileStatus fstatus;
   if (std::error_code errorCode = status(path, fstatus, false)) {
      return errorCode;
   }
   result = is_symlink_file(fstatus);
   return std::error_code();
}

bool is_other(const BasicFileStatus &status)
{
   return exists(status) &&
         !is_regular_file(status) &&
         !is_directory(status);
}

std::error_code is_other(const Twine &path, bool &result)
{
   FileStatus fstatus;
   if (std::error_code errorCode = status(path, fstatus)) {
      return errorCode;
   }
   result = is_other(fstatus);
   return std::error_code();
}

OptionalError<Permission> get_permissions(const Twine &path) {
   FileStatus fstatus;
   if (std::error_code errorCode = status(path, fstatus)) {
      return errorCode;
   }
   return fstatus.getPermissions();
}

void DirectoryEntry::replaceFilename(const Twine &filename, FileType type,
                                     BasicFileStatus status)
{
   SmallString<128> pathStr = path::parent_path(m_path);
   path::append(pathStr, filename);
   m_path = pathStr.getStr();
   m_type = type;
   m_status = status;
}

TempFile::TempFile(StringRef name, int fd)
   : m_tmpName(name), m_fd(fd)
{}

TempFile::TempFile(TempFile &&other)
{
   *this = std::move(other);
}

TempFile &TempFile::operator=(TempFile &&other)
{
   m_tmpName = std::move(other.m_tmpName);
   m_fd = other.m_fd;
   other.m_done = true;
   return *this;
}

TempFile::~TempFile()
{
   assert(m_done);
}

Error TempFile::discard()
{
   m_done = true;
   std::error_code removeErrorCode;
   // On windows closing will remove the file.
#ifndef _WIN32
   // Always try to close and remove.
   if (!m_tmpName.empty()) {
      removeErrorCode = fs::remove(m_tmpName);
      polar::utils::dont_remove_file_on_signal(m_tmpName);
   }
#endif

   if (!removeErrorCode){
      m_tmpName = "";
   }
   if (m_fd != -1 && close(m_fd) == -1) {
      std::error_code errorCode = std::error_code(errno, std::generic_category());
      return error_code_to_error(errorCode);
   }
   m_fd = -1;
   return error_code_to_error(removeErrorCode);
}

Error TempFile::keep(const Twine &name)
{
   assert(!m_done);
   m_done = true;
   // Always try to close and rename.
#ifdef _WIN32
   // If we cant't cancel the delete don't rename.
   std::error_code renameErrorCode = cancel_delete_on_close(fd);
   if (!renameErrorCode) {
      renameErrorCode = rename_fd(m_fd, name);
   }
   // If we can't rename, discard the temporary file.
   if (renameErrorCode) {
      remove_fd(m_fd);
   }

#else
   std::error_code renameErrorCode = fs::rename(m_tmpName, name);
   // If we can't rename, discard the temporary file.
   if (renameErrorCode) {
      remove(m_tmpName);
   }
   utils::dont_remove_file_on_signal(m_tmpName);
#endif

   if (!renameErrorCode) {
      m_tmpName = "";
   }
   if (close(m_fd) == -1) {
      std::error_code errorCode(errno, std::generic_category());
      return error_code_to_error(errorCode);
   }
   m_fd = -1;
   return error_code_to_error(renameErrorCode);
}

Error TempFile::keep()
{
   assert(!m_done);
   m_done = true;

#ifdef _WIN32
   if (std::error_code errorCode = cancel_delete_on_close(m_fd)) {
      return error_code_to_error(errorCode);
   }
#else
   utils::dont_remove_file_on_signal(m_tmpName);
#endif

   m_tmpName = "";

   if (close(m_fd) == -1) {
      std::error_code errorCode(errno, std::generic_category());
      return error_code_to_error(errorCode);
   }
   m_fd = -1;

   return Error::getSuccess();
}

Expected<TempFile> TempFile::create(const Twine &model, unsigned mode)
{
   int fd;
   SmallString<128> resultPath;
   if (std::error_code errorCode = create_unique_file(model, fd, resultPath, mode,
                                                      fs::OF_Delete))
      return error_code_to_error(errorCode);

   TempFile ret(resultPath, fd);
#ifndef _WIN32
   if (utils::remove_file_on_signal(resultPath)) {
      // Make sure we delete the file when RemoveFileOnSignal fails.
      polar::utils::consume_error(ret.discard());
      std::error_code errorCode(ErrorCode::operation_not_permitted);
      return error_code_to_error(errorCode);
   }
#endif
   return std::move(ret);
}

} // fs
} // polar
