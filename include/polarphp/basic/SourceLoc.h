//===--- SourceLoc.h - Source Locations and Ranges --------------*- C++ -*-===//
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
//
//  This file defines types used to reason about source locations and ranges.
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
// Created by polarboy on 2019/04/24.

#ifndef POLARPHP_BASIC_SOURCE_LOC_H
#define POLARPHP_BASIC_SOURCE_LOC_H

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/SourceMgr.h"
#include <functional>

/// forwrad declare with namespace
namespace llvm {
class raw_ostream;
}
namespace polar::ast {
class DiagnosticConsumer;
}

namespace polar::basic {

/// forward declare class
class SourceManager;
using llvm::raw_ostream;
using BasicSMLoc = llvm::SMLoc;
using llvm::StringRef;
using polar::ast::DiagnosticConsumer;

/// SourceLoc in swift is just an SMLoc.  We define it as a different type
/// (instead of as a typedef) just to remove the "getFromPointer" methods and
/// enforce purity in the Swift codebase.
class SourceLoc
{
   friend class SourceManager;
   friend class SourceRange;
   friend class CharSourceRange;
   friend class DiagnosticConsumer;
   llvm::SMLoc m_value;
public:
   SourceLoc() {}
   explicit SourceLoc(llvm::SMLoc value)
      : m_value(value)
   {}

   bool isValid() const
   {
      return m_value.isValid();
   }

   bool isInvalid() const
   {
      return !isValid();
   }

   bool operator==(const SourceLoc &other) const
   {
      return other.m_value == m_value;
   }

   bool operator!=(const SourceLoc &other) const
   {
      return !operator==(other);
   }

   /// Return a source location advanced a specified number of bytes.
   SourceLoc getAdvancedLoc(int byteOffset) const
   {
      assert(isValid() && "Can't advance an invalid location");
      return SourceLoc(
               llvm::SMLoc::getFromPointer(m_value.getPointer() + byteOffset));
   }

   SourceLoc getAdvancedLocOrInvalid(int byteOffset) const
   {
      if (isValid()) {
         return getAdvancedLoc(byteOffset);
      }
      return SourceLoc();
   }

   const void *getOpaquePointerValue() const
   {
      return m_value.getPointer();
   }

   /// Print out the SourceLoc.  If this location is in the same buffer
   /// as specified by \c lastBufferId, then we don't print the filename.  If
   /// not, we do print the filename, and then update \c lastBufferId with the
   /// bufferID printed.
   void print(raw_ostream &ostream, const SourceManager &sourceMgr,
              unsigned &lastBufferId) const;

   void printLineAndColumn(raw_ostream &ostream, const SourceManager &sourceMgr,
                           unsigned bufferID = 0) const;

   void print(raw_ostream &ostream, const SourceManager &sourceMgr) const
   {
      unsigned temp = ~0U;
      print(ostream, sourceMgr, temp);
   }

   void dump(const SourceManager &sourceMgr) const;
};

/// SourceRange in swift is a pair of locations.  However, note that the end
/// location is the start of the last token in the range, not the last character
/// in the range.  This is unlike SMRange, so we use a distinct type to make
/// sure that proper conversions happen where important.
class SourceRange
{
public:
   SourceLoc start;
   SourceLoc end;

   SourceRange() {}

   SourceRange(SourceLoc loc)
      : start(loc),
        end(loc)
   {}

   SourceRange(SourceLoc start, SourceLoc end)
      : start(start),
        end(end)
   {
      assert(start.isValid() == end.isValid() &&
             "start and end should either both be valid or both be invalid!");
   }

   bool isValid() const
   {
      return start.isValid();
   }

   bool isInvalid() const
   {
      return !isValid();
   }

   const SourceLoc &getStart() const
   {
      return start;
   }

   const SourceLoc &getEnd() const
   {
      return end;
   }

   /// Extend this SourceRange to the smallest continuous SourceRange that
   /// includes both this range and the other one.
   void widen(SourceRange other);

   bool operator==(const SourceRange &other) const
   {
      return start == other.start && end == other.end;
   }

   bool operator!=(const SourceRange &other) const
   {
      return !operator==(other);
   }

   /// Print out the SourceRange.  If the locations are in the same buffer
   /// as specified by lastBufferId, then we don't print the filename.  If not,
   /// we do print the filename, and then update lastBufferId with the bufferID
   /// printed.
   void print(raw_ostream &ostream, const SourceManager &sourceMgr,
              unsigned &lastBufferId, bool printText = true) const;

   void print(raw_ostream &ostream, const SourceManager &sourceMgr,
              bool printText = true) const
   {
      unsigned temp = ~0U;
      print(ostream, sourceMgr, temp, printText);
   }

   void dump(const SourceManager &sourceMgr) const;
};

/// A half-open character-based source range.
class CharSourceRange
{
   SourceLoc start;
   unsigned byteLength;

public:
   /// Constructs an invalid range.
   CharSourceRange() = default;

   CharSourceRange(SourceLoc start, unsigned byteLength)
      : start(start),
        byteLength(byteLength)
   {}

   /// Constructs a character range which starts and ends at the
   /// specified character locations.
   CharSourceRange(const SourceManager &sourceMgr, SourceLoc start, SourceLoc end);

   /// Use Lexer::getCharSourceRangeFromSourceRange() instead.
   CharSourceRange(const SourceManager &sourceMgr, SourceRange range) = delete;

   bool isValid() const
   {
      return start.isValid();
   }

   bool isInvalid() const
   {
      return !isValid();
   }

   bool operator==(const CharSourceRange &other) const
   {
      return start == other.start && byteLength == other.byteLength;
   }

   bool operator!=(const CharSourceRange &other) const
   {
      return !operator==(other);
   }

   SourceLoc getStart() const
   {
      return start;
   }

   SourceLoc getEnd() const
   {
      return start.getAdvancedLocOrInvalid(byteLength);
   }

   /// Returns true if the given source location is contained in the range.
   bool contains(SourceLoc loc) const
   {
      auto less = std::less<const char *>();
      auto less_equal = std::less_equal<const char *>();
      return less_equal(getStart().m_value.getPointer(), loc.m_value.getPointer()) &&
            less(loc.m_value.getPointer(), getEnd().m_value.getPointer());
   }

   bool contains(CharSourceRange other) const
   {
      auto less_equal = std::less_equal<const char *>();
      return contains(other.getStart()) &&
            less_equal(other.getEnd().m_value.getPointer(), getEnd().m_value.getPointer());
   }

   /// expands *this to cover other
   void widen(CharSourceRange other)
   {
      auto diff = other.getEnd().m_value.getPointer() - getEnd().m_value.getPointer();
      if (diff > 0) {
         byteLength += diff;
      }
      const auto myStartPtr = getStart().m_value.getPointer();
      diff = myStartPtr - other.getStart().m_value.getPointer();
      if (diff > 0) {
         byteLength += diff;
         start = SourceLoc(llvm::SMLoc::getFromPointer(myStartPtr - diff));
      }
   }

   bool overlaps(CharSourceRange other) const
   {
      if (getByteLength() == 0 || other.getByteLength() == 0) {
         return false;
      }
      return contains(other.getStart()) || other.contains(getStart());
   }

   StringRef str() const
   {
      return StringRef(start.m_value.getPointer(), byteLength);
   }

   /// Return the length of this valid range in bytes.  Can be zero.
   unsigned getByteLength() const
   {
      assert(isValid() && "length does not make sense for an invalid range");
      return byteLength;
   }

   /// Print out the CharSourceRange.  If the locations are in the same buffer
   /// as specified by lastBufferId, then we don't print the filename.  If not,
   /// we do print the filename, and then update lastBufferId with the bufferID
   /// printed.
   void print(raw_ostream &ostream, const SourceManager &sourceMgr,
              unsigned &lastBufferId, bool printText = true) const;

   void print(raw_ostream &ostream, const SourceManager &sourceMgr,
              bool printText = true) const
   {
      unsigned temp = ~0U;
      print(ostream, sourceMgr, temp, printText);
   }

   void dump(const SourceManager &sourceMgr) const;
};

} // polar::basic

namespace llvm {
template <typename T>
struct DenseMapInfo;

template <>
struct DenseMapInfo<polar::basic::SourceLoc>
{
   static polar::basic::SourceLoc getEmptyKey()
   {
      return polar::basic::SourceLoc(
               SMLoc::getFromPointer(DenseMapInfo<const char *>::getEmptyKey()));
   }

   static polar::basic::SourceLoc getTombstoneKey()
   {
      // Make this different from empty key. See for context:
      // http://lists.llvm.org/pipermail/llvm-dev/2015-July/088744.html
      return polar::basic::SourceLoc(
               SMLoc::getFromPointer(DenseMapInfo<const char *>::getTombstoneKey()));
   }

   static unsigned getHashValue(const polar::basic::SourceLoc &value)
   {
      return DenseMapInfo<const void *>::getHashValue(
               value.getOpaquePointerValue());
   }

   static bool isEqual(const polar::basic::SourceLoc &lhs,
                       const polar::basic::SourceLoc &rhs)
   {
      return lhs == rhs;
   }
};

template <>
struct DenseMapInfo<polar::basic::SourceRange>
{
   static polar::basic::SourceRange getEmptyKey()
   {
      return polar::basic::SourceRange(polar::basic::SourceLoc(
                                          SMLoc::getFromPointer(DenseMapInfo<const char *>::getEmptyKey())));
   }

   static polar::basic::SourceRange getTombstoneKey()
   {
      // Make this different from empty key. See for context:
      // http://lists.llvm.org/pipermail/llvm-dev/2015-July/088744.html
      return polar::basic::SourceRange(polar::basic::SourceLoc(
                                          SMLoc::getFromPointer(DenseMapInfo<const char *>::getTombstoneKey())));
   }

   static unsigned getHashValue(const polar::basic::SourceRange &value)
   {
      return hash_combine(DenseMapInfo<const void *>::getHashValue(
                             value.start.getOpaquePointerValue()),
                          DenseMapInfo<const void *>::getHashValue(
                             value.end.getOpaquePointerValue()));
   }

   static bool isEqual(const polar::basic::SourceRange &lhs,
                       const polar::basic::SourceRange &rhs)
   {
      return lhs == rhs;
   }
};

} // namespace llvm

#endif // POLARPHP_BASIC_SOURCE_LOC_H
