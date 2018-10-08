// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/08.

#ifndef POLAR_DEVLTOOLS_FILECHECKER_SOURCE_LOCATION_H
#define POLAR_DEVLTOOLS_FILECHECKER_SOURCE_LOCATION_H

#include <cassert>

namespace polar {
namespace filechecker {

/// Represents a location in source code.
class SourceLocation
{
public:
   SourceLocation() = default;
   bool isValid() const
   {
      return m_ptr != nullptr;
   }
   bool operator==(const SourceLocation &other) const
   {
      return m_ptr == other.m_ptr;
   }
   bool operator!=(const SourceLocation &other) const
   {
      return m_ptr != other.m_ptr;
   }

   const char *getPointer() const
   {
      return m_ptr;
   }

   static SourceLocation getFromPointer(const char *ptr)
   {
      SourceLocation location;
      location.m_ptr = ptr;
      return location;
   }
private:
   const char *m_ptr = nullptr;
};

/// Represents a range in source code.
///
/// SMRange is implemented using a half-open range, as is the convention in C++.
/// In the string "abc", the range [1,3) represents the substring "bc", and the
/// range [2,2) represents an empty range between the characters "b" and "c".
///
class SourceRange
{
public:
   SourceRange() = default;
   SourceRange(std::nullptr_t)
   {}
   SourceRange(SourceLocation start, SourceLocation end)
      : m_start(start),
        m_end(end)
   {
      assert(m_start.isValid() == m_end.isValid() &&
             "Start and End should either both be valid or both be invalid!");
   }
   bool isValid() const
   {
      return m_start.isValid();
   }

   const SourceLocation &getStart() const
   {
      return m_start;
   }
   const SourceLocation &getEnd() const
   {
      return m_end;
   }
   SourceRange &setStart(const SourceLocation &location)
   {
      m_start = location;
      return *this;
   }

   SourceRange &setEnd(const SourceLocation &location)
   {
      m_end = location;
      return *this;
   }
protected:
   SourceLocation m_start;
   SourceLocation m_end;
};

} // filechecker
} // polar

#endif // POLAR_DEVLTOOLS_FILECHECKER_SOURCE_LOCATION_H
