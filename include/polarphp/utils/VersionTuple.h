// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/17.

//===----------------------------------------------------------------------===//
///
/// \file
/// Defines the polar::utils::VersionTuple class, which represents a version in
/// the form major[.minor[.subminor]].
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_VERSION_TUPLE_H
#define POLARPHP_UTILS_VERSION_TUPLE_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/RawOutStream.h"

#include <optional>
#include <string>
#include <tuple>

namespace polar {
namespace utils {

/// Represents a version number in the form major[.minor[.subminor[.build]]].
class VersionTuple
{
public:
   VersionTuple()
      : m_major(0), m_minor(0),
        m_hasMinor(false), m_subminor(0),
        m_hasSubminor(false), m_build(0),
        m_hasBuild(false)
   {}

   explicit VersionTuple(unsigned major)
      : m_major(major), m_minor(0),
        m_hasMinor(false), m_subminor(0),
        m_hasSubminor(false), m_build(0),
        m_hasBuild(false)
   {}

   explicit VersionTuple(unsigned major, unsigned minor)
      : m_major(major), m_minor(minor),
        m_hasMinor(true), m_subminor(0),
        m_hasSubminor(false), m_build(0),
        m_hasBuild(false) {}

   explicit VersionTuple(unsigned major, unsigned minor, unsigned subminor)
      : m_major(major), m_minor(minor),
        m_hasMinor(true), m_subminor(subminor),
        m_hasSubminor(true), m_build(0),
        m_hasBuild(false)
   {}

   explicit VersionTuple(unsigned major, unsigned minor, unsigned subminor,
                         unsigned build)
      : m_major(major), m_minor(minor),
        m_hasMinor(true), m_subminor(subminor),
        m_hasSubminor(true), m_build(build),
        m_hasBuild(true)
   {}

   /// Determine whether this version information is empty
   /// (e.g., all version components are zero).
   bool empty() const
   {
      return m_major == 0 && m_minor == 0 && m_subminor == 0 && m_build == 0;
   }

   /// Retrieve the major version number.
   unsigned getMajor() const
   {
      return m_major;
   }

   /// Retrieve the minor version number, if provided.
   std::optional<unsigned> getMinor() const
   {
      if (!m_hasMinor) {
         return std::nullopt;
      }
      return m_minor;
   }

   /// Retrieve the subminor version number, if provided.
   std::optional<unsigned> getSubminor() const
   {
      if (!m_hasSubminor) {
         return std::nullopt;
      }
      return m_subminor;
   }

   /// Retrieve the build version number, if provided.
   std::optional<unsigned> getBuild() const
   {
      if (!m_hasBuild) {
         return std::nullopt;
      }
      return m_build;
   }

   /// Determine if two version numbers are equivalent. If not
   /// provided, minor and subminor version numbers are considered to be zero.
   friend bool operator==(const VersionTuple &lhs, const VersionTuple &rhs)
   {
      return lhs.m_major == rhs.m_major && lhs.m_minor == rhs.m_minor &&
            lhs.m_subminor == rhs.m_subminor && lhs.m_build == rhs.m_build;
   }

   /// Determine if two version numbers are not equivalent.
   ///
   /// If not provided, minor and subminor version numbers are considered to be
   /// zero.
   friend bool operator!=(const VersionTuple &lhs, const VersionTuple &rhs)
   {
      return !(lhs == rhs);
   }

   /// Determine whether one version number precedes another.
   ///
   /// If not provided, minor and subminor version numbers are considered to be
   /// zero.
   friend bool operator<(const VersionTuple &lhs, const VersionTuple &rhs)
   {
      return std::tie(lhs.m_major, lhs.m_minor, lhs.m_subminor, lhs.m_build) <
            std::tie(rhs.m_major, rhs.m_minor, rhs.m_subminor, rhs.m_build);
   }

   /// Determine whether one version number follows another.
   ///
   /// If not provided, minor and subminor version numbers are considered to be
   /// zero.
   friend bool operator>(const VersionTuple &lhs, const VersionTuple &rhs)
   {
      return rhs < lhs;
   }

   /// Determine whether one version number precedes or is
   /// equivalent to another.
   ///
   /// If not provided, minor and subminor version numbers are considered to be
   /// zero.
   friend bool operator<=(const VersionTuple &lhs, const VersionTuple &rhs)
   {
      return !(rhs < lhs);
   }

   /// Determine whether one version number follows or is
   /// equivalent to another.
   ///
   /// If not provided, minor and subminor version numbers are considered to be
   /// zero.
   friend bool operator>=(const VersionTuple &lhs, const VersionTuple &rhs)
   {
      return !(lhs < rhs);
   }

   /// Retrieve a string representation of the version number.
   std::string getAsString() const;

   /// Try to parse the given string as a version number.
   /// \returns \c true if the string does not match the regular expression
   ///   [0-9]+(\.[0-9]+){0,3}
   bool tryParse(StringRef string);
private:
   unsigned m_major : 32;
   unsigned m_minor : 31;
   unsigned m_hasMinor : 1;
   unsigned m_subminor : 31;
   unsigned m_hasSubminor : 1;
   unsigned m_build : 31;
   unsigned m_hasBuild : 1;
};

/// Print a version number.
RawOutStream &operator<<(RawOutStream &Out, const VersionTuple &version);

} // utils
} // polar

#endif // POLARPHP_UTILS_VERSION_TUPLE_H
