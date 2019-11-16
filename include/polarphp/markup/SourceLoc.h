//===--- SourceLoc.h - ReST source location and source manager classes ----===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/27.

#ifndef POLARPHP_MARKUP_SOURCELOC_H
#define POLARPHP_MARKUP_SOURCELOC_H

#include "llvm/ADTStringRef.h"
#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>

namespace polar::markup {

using polar::basic::StringRef;

class SourceLoc
{
public:
   SourceLoc()
      : m_value(scm_invalidValue)
   {}

   SourceLoc(const SourceLoc &) = default;

   bool isValid() const
   {
      return !isInvalid();
   }

   bool isInvalid() const
   {
      return m_value == m_invalidValue;
   }

   bool operator==(SourceLoc other) const
   {
      return m_value == other.m_value;
   }

   bool operator!=(SourceLoc other) const
   {
      return !(*this == other);
   }

   /// Return a source location advanced a specified number of bytes.
   SourceLoc getAdvancedLoc(int byteOffset) const
   {
      assert(isValid() && "Can't advance an invalid location");
      SourceLoc result = *this;
      result.m_value += byteOffset;
      return result;
   }
private:
   friend class SourceManagerBase;
   template<typename ExternalSourceLocTy>
   friend class SourceManager;
   unsigned m_value;
   static const unsigned scm_invalidValue = 0;
};

class SourceRange
{
public:
   /// The source range is a half-open byte range [m_start; m_end).
   SourceLoc m_start;
   SourceLoc m_end;

   SourceRange() {}
   SourceRange(SourceLoc loc)
      : m_start(loc),
        m_end(loc)
   {}

   SourceRange(SourceLoc start, SourceLoc end)
      : m_start(start),
        m_end(end)
   {
      assert(m_start.isValid() == m_end.isValid() &&
             "m_start and end should either both be valid or both be invalid!");
   }

   bool isValid() const
   {
      return m_start.isValid();
   }

   bool isInvalid() const
   {
      return m_start.isInvalid();
   }
};

class SourceManagerBase
{
protected:
   SourceLoc m_nextUnassignedLoc;

   /// All source pieces, in order of increasing source location.
   std::vector<SourceRange> m_registeredRanges;

public:
   SourceManagerBase()
      : m_nextUnassignedLoc()
   {
      m_nextUnassignedLoc.m_value = 1;
   }

   SourceManagerBase(const SourceManagerBase &) = delete;
   void operator=(const SourceManagerBase &) = delete;
   bool isBeforeInBuffer(SourceLoc lhs, SourceLoc rhs) const
   {
      // When we support multiple buffers, assert that locations come from the
      // same buffer.
      return lhs.m_value < rhs.m_value;
   }

   /// Returns true if range \c R contains the location \c loc.
   bool containsLoc(SourceRange range, SourceLoc loc) const
   {
      return loc == range.m_start ||
            (isBeforeInBuffer(range.m_start, loc) && isBeforeInBuffer(loc, range.m_end));
   }
};

template <typename ExternalSourceLocTy>
class SourceManager : public SourceManagerBase
{
public:
   SourceManager() = default;
   SourceManager(const SourceManager &) = delete;

   void operator=(const SourceManager &) = delete;

   SourceRange registerLine(StringRef line, ExternalSourceLocTy externalLoc);

   /// Returns the external source range and a byte offset inside it.
   std::pair<ExternalSourceLocTy, unsigned>
   toExternalSourceLoc(SourceLoc loc) const;
private:
   std::vector<ExternalSourceLocTy> m_externalLocs;
};

template <typename ExternalSourceLocTy>
SourceRange SourceManager<ExternalSourceLocTy>::registerLine(
      StringRef line, ExternalSourceLocTy externalLoc)
{
   if (line.size() > 4095) {
      return SourceRange();
   }
   SourceLoc m_start = m_nextUnassignedLoc;
   SourceLoc m_end = m_start.getAdvancedLoc(line.size());
   m_registeredRanges.push_back(SourceRange(m_start, m_end));
   m_externalLocs.push_back(externalLoc);
   m_nextUnassignedLoc = m_end.getAdvancedLoc(2);
#ifndef NDEBUG
   // To make debugging easier, make each line start at offset that is equal to
   // 1 mod 1000.
   m_nextUnassignedLoc.m_value = ((m_nextUnassignedLoc.m_value + 999) / 1000) * 1000 + 1;
#endif
   return SourceRange(m_start, m_end);
}

template <typename ExternalSourceLocTy>
std::pair<ExternalSourceLocTy, unsigned>
SourceManager<ExternalSourceLocTy>::toExternalSourceLoc(SourceLoc loc) const
{
   auto iter = std::lower_bound(m_registeredRanges.begin(), m_registeredRanges.end(),
                             loc, [this](const SourceRange &lhs, SourceLoc loc) {
      return this->isBeforeInBuffer(lhs.m_start, loc);
   });
   assert(iter != m_registeredRanges.end() && "unknown source location");
   const auto &InternalRange = *iter;
   assert(containsLoc(InternalRange, loc) && "unknown source location");
   const auto &externalLoc = m_externalLocs[iter - m_registeredRanges.begin()];
   return { externalLoc, loc.m_value - InternalRange.m_start.m_value };
}

} // polar::markup

#endif // POLARPHP_MARKUP_SOURCELOC_H
