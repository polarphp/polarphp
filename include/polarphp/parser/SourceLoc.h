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

#ifndef POLARPHP_PARSER_SOURCE_LOC_H
#define POLARPHP_PARSER_SOURCE_LOC_H

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

namespace polar::parser {

/// forward declare class
class SourceManager;
using llvm::raw_ostream;
using BasicSMLoc = llvm::SMLoc;
using llvm::StringRef;
using polar::ast::DiagnosticConsumer;

/// SourceLoc in parser namespace is just an polar::utils::SMLocation.
/// We define it as a different type
/// (instead of as a typedef) just to remove the "getFromPointer" methods and
/// enforce purity in the polarphp codebase.
///
class SourceLoc
{
public:
   SourceLoc() {}

   explicit SourceLoc(BasicSMLoc loc)
      : m_loc(loc)
   {}

   bool isValid() const
   {
      return m_loc.isValid();
   }

   bool isInvalid() const
   {
      return !isValid();
   }

   bool operator==(const SourceLoc &other) const
   {
      return m_loc == other.m_loc;
   }

   bool operator!=(const SourceLoc &other) const
   {
      return !operator==(other);
   }

   /// Return a source location advanced a specified number of bytes.
   SourceLoc getAdvancedLoc(int byteOffset) const
   {
      assert(isValid() && "Can't advance an invalid location");
      return SourceLoc(BasicSMLoc::getFromPointer(m_loc.getPointer() + byteOffset));
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
      return m_loc.getPointer();
   }

   /// Print out the SourceLoc.  If this location is in the same buffer
   /// as specified by \c LastBufferID, then we don't print the filename.  If
   /// not, we do print the filename, and then update \c LastBufferID with the
   /// BufferID printed.
   void print(raw_ostream &outStream, const SourceManager &sourceMgr,
              unsigned &lastBufferID) const;

   void printLineAndColumn(raw_ostream &outStream, const SourceManager &sourceMgr,
                           unsigned bufferID = 0) const;

   void print(raw_ostream &outStream, const SourceManager &sourceMgr) const
   {
      unsigned temp = ~0U;
      print(outStream, sourceMgr, temp);
   }

   void dump(const SourceManager &sourceMgr) const;

private:
   BasicSMLoc m_loc;

private:
   friend class SourceManager;
   friend class SourceRange;
   friend class CharSourceRange;
   friend class DiagnosticConsumer;
};

/// SourceRange in swift is a pair of locations.  However, note that the end
/// location is the start of the last token in the range, not the last character
/// in the range.  This is unlike SMRange, so we use a distinct type to make
/// sure that proper conversions happen where important.
class SourceRange
{
public:
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
      return !isValid();
   }

   const SourceLoc &getStart() const
   {
      return m_start;
   }

   const SourceLoc &getEnd() const
   {
      return m_end;
   }

   SourceLoc &getStart()
   {
      return m_start;
   }

   SourceLoc &getEnd()
   {
      return m_end;
   }

   /// Extend this SourceRange to the smallest continuous SourceRange that
   /// includes both this range and the other one.
   void widen(SourceRange other);

   bool operator==(const SourceRange &other) const
   {
      return m_start == other.m_start && m_end == other.m_end;
   }

   bool operator!=(const SourceRange &other) const
   {
      return !operator==(other);
   }

   /// Print out the SourceRange.  If the locations are in the same buffer
   /// as specified by LastBufferID, then we don't print the filename.  If not,
   /// we do print the filename, and then update LastBufferID with the BufferID
   /// printed.
   void print(raw_ostream &outStream, const SourceManager &sourceMgr,
              unsigned &lastBufferID, bool PrintText = true) const;

   void print(raw_ostream &outStream, const SourceManager &sourceMgr,
              bool printText = true) const
   {
      unsigned temp = ~0U;
      print(outStream, sourceMgr, temp, printText);
   }

   void dump(const SourceManager &sourceMgr) const;

private:
   SourceLoc m_start;
   SourceLoc m_end;
};

/// A half-open character-based source range.
class CharSourceRange
{
public:
   /// Constructs an invalid range.
   CharSourceRange() = default;

   CharSourceRange(SourceLoc start, std::size_t byteLength)
      : m_start(start),
        m_byteLength(byteLength)
   {}

   /// Constructs a character range which starts and ends at the
   /// specified character locations.
   CharSourceRange(const SourceManager &sourceMgr, SourceLoc start, SourceLoc end);

   /// Use Lexer::getCharSourceRangeFromSourceRange() instead.
   CharSourceRange(const SourceManager &sourceMgr, SourceRange range) = delete;

   bool isValid() const
   {
      return m_start.isValid();
   }

   bool isInvalid() const
   {
      return !isValid();
   }

   bool operator==(const CharSourceRange &other) const
   {
      return m_start == other.m_start && m_byteLength == other.m_byteLength;
   }
   bool operator!=(const CharSourceRange &other) const
   {
      return !operator==(other);
   }

   SourceLoc getStart() const
   {
      return m_start;
   }

   SourceLoc getEnd() const
   {
      return m_start.getAdvancedLocOrInvalid(m_byteLength);
   }

   /// Returns true if the given source location is contained in the range.
   bool contains(SourceLoc loc) const
   {
      auto less = std::less<const char *>();
      auto less_equal = std::less_equal<const char *>();
      return less_equal(getStart().m_loc.getPointer(), loc.m_loc.getPointer()) &&
            less(loc.m_loc.getPointer(), getEnd().m_loc.getPointer());
   }

   bool contains(CharSourceRange other) const
   {
      auto less_equal = std::less_equal<const char *>();
      return contains(other.getStart()) &&
            less_equal(other.getEnd().m_loc.getPointer(), getEnd().m_loc.getPointer());
   }

   /// expands *this to cover other
   void widen(CharSourceRange other)
   {
      auto diff = other.getEnd().m_loc.getPointer() - getEnd().m_loc.getPointer();
      if (diff > 0) {
         m_byteLength += diff;
      }
      const auto myStartPtr = getStart().m_loc.getPointer();
      diff = myStartPtr - other.getStart().m_loc.getPointer();
      if (diff > 0) {
         m_byteLength += diff;
         m_start = SourceLoc(BasicSMLoc::getFromPointer(myStartPtr - diff));
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
      return StringRef(m_start.m_loc.getPointer(), m_byteLength);
   }

   /// Return the length of this valid range in bytes.  Can be zero.
   unsigned getByteLength() const
   {
      assert(isValid() && "length does not make sense for an invalid range");
      return m_byteLength;
   }

   /// Print out the CharSourceRange.  If the locations are in the same buffer
   /// as specified by LastBufferID, then we don't print the filename.  If not,
   /// we do print the filename, and then update LastBufferID with the BufferID
   /// printed.
   void print(raw_ostream &outStream, const SourceManager &sourceMgr,
              unsigned &lastBufferID, bool printText = true) const;

   void print(raw_ostream &outStream, const SourceManager &sourceMgr,
              bool printText = true) const
   {
      unsigned temp = ~0U;
      print(outStream, sourceMgr, temp, printText);
   }

   void dump(const SourceManager &sourceMgr) const;

private:
   SourceLoc m_start;
   std::size_t m_byteLength;
};

} // polar::parser

namespace llvm {

using polar::parser::SourceLoc;
using polar::parser::BasicSMLoc;
using polar::parser::SourceRange;

template <typename T>
struct DenseMapInfo;

template <>
struct DenseMapInfo<SourceLoc>
{
   static SourceLoc getEmptyKey()
   {
      return SourceLoc(
               BasicSMLoc::getFromPointer(DenseMapInfo<const char *>::getEmptyKey()));
   }

   static SourceLoc getTombstoneKey()
   {
      // Make this different from empty key. See for context:
      // http://lists.llvm.org/pipermail/llvm-dev/2015-July/088744.html
      return SourceLoc(
               BasicSMLoc::getFromPointer(DenseMapInfo<const char *>::getTombstoneKey()));
   }

   static unsigned getHashValue(const SourceLoc &loc)
   {
      return DenseMapInfo<const void *>::getHashValue(
               loc.getOpaquePointerValue());
   }

   static bool isEqual(const SourceLoc &lhs,
                       const SourceLoc &rhs)
   {
      return lhs == rhs;
   }
};

template <>
struct DenseMapInfo<SourceRange>
{
   static SourceRange getEmptyKey()
   {
      return SourceRange(SourceLoc(
                            BasicSMLoc::getFromPointer(DenseMapInfo<const char *>::getEmptyKey())));
   }

   static SourceRange getTombstoneKey()
   {
      // Make this different from empty key. See for context:
      // http://lists.llvm.org/pipermail/llvm-dev/2015-July/088744.html
      return SourceRange(SourceLoc(
                            BasicSMLoc::getFromPointer(DenseMapInfo<const char *>::getTombstoneKey())));
   }

   static unsigned getHashValue(const SourceRange &loc)
   {
      return hash_combine(DenseMapInfo<const void *>::getHashValue(
                             loc.getStart().getOpaquePointerValue()),
                          DenseMapInfo<const void *>::getHashValue(
                             loc.getEnd().getOpaquePointerValue()));
   }

   static bool isEqual(const SourceRange &lhs,
                       const SourceRange &rhs)
   {
      return lhs == rhs;
   }
};

} // llvm

#endif // POLARPHP_PARSER_SOURCE_LOC_H
