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
#include "polarphp/basic/Debug.h"
#include <functional>

/// forwrad declare with namespace
namespace llvm {
class raw_ostream;
}
namespace polar {
class DiagnosticConsumer;
}

namespace polar {
class SourceManager;
using llvm::raw_ostream;
using llvm::StringRef;
using polar::DiagnosticConsumer;
/// SourceLoc in swift is just an SMLoc.  We define it as a different type
/// (instead of as a typedef) just to remove the "getFromPointer" methods and
/// enforce purity in the Swift codebase.
class SourceLoc {
   friend class SourceManager;
   friend class SourceRange;
   friend class CharSourceRange;
   friend class DiagnosticConsumer;

   llvm::SMLoc m_value;

public:
   SourceLoc() {}
   explicit SourceLoc(llvm::SMLoc value) : m_value(value) {}

   bool isValid() const { return m_value.isValid(); }
   bool isInvalid() const { return !isValid(); }

   bool operator==(const SourceLoc &RHS) const { return RHS.m_value == m_value; }
   bool operator!=(const SourceLoc &RHS) const { return !operator==(RHS); }

   /// Return a source location advanced a specified number of bytes.
   SourceLoc getAdvancedLoc(int ByteOffset) const {
      assert(isValid() && "Can't advance an invalid location");
      return SourceLoc(
               llvm::SMLoc::getFromPointer(m_value.getPointer() + ByteOffset));
   }

   SourceLoc getAdvancedLocOrInvalid(int ByteOffset) const {
      if (isValid())
         return getAdvancedLoc(ByteOffset);
      return SourceLoc();
   }

   const void *getOpaquePointerValue() const { return m_value.getPointer(); }

   /// Print out the SourceLoc.  If this location is in the same buffer
   /// as specified by \c LastBufferID, then we don't print the filename.  If
   /// not, we do print the filename, and then update \c LastBufferID with the
   /// BufferID printed.
   void print(raw_ostream &OS, const SourceManager &SM,
              unsigned &LastBufferID) const;

   void printLineAndColumn(raw_ostream &OS, const SourceManager &SM,
                           unsigned BufferID = 0) const;

   void print(raw_ostream &OS, const SourceManager &SM) const {
      unsigned Tmp = ~0U;
      print(OS, SM, Tmp);
   }

   POLAR_DEBUG_DUMPER(dump(const SourceManager &SM));

   friend size_t hash_value(SourceLoc loc) {
      return reinterpret_cast<uintptr_t>(loc.getOpaquePointerValue());
   }

   friend void simple_display(raw_ostream &OS, const SourceLoc &loc) {
      // Nothing meaningful to print.
   }
};

/// SourceRange in swift is a pair of locations.  However, note that the end
/// location is the start of the last token in the range, not the last character
/// in the range.  This is unlike SMRange, so we use a distinct type to make
/// sure that proper conversions happen where important.
class SourceRange {
public:
     SourceLoc start, end;
   SourceRange() {}
   SourceRange(SourceLoc Loc) : start(Loc), end(Loc) {}
   SourceRange(SourceLoc start, SourceLoc end) : start(start), end(end) {
      assert(start.isValid() == end.isValid() &&
             "start and end should either both be valid or both be invalid!");
   }

   bool isValid() const { return start.isValid(); }
   bool isInvalid() const { return !isValid(); }
   const SourceLoc &getStart() const {return start;}
  const SourceLoc &getEnd() const {
      return end;
   }
   /// Extend this SourceRange to the smallest continuous SourceRange that
   /// includes both this range and the other one.
   void widen(SourceRange Other);

   bool operator==(const SourceRange &other) const {
      return start == other.start && end == other.end;
   }
   bool operator!=(const SourceRange &other) const { return !operator==(other); }

   /// Print out the SourceRange.  If the locations are in the same buffer
   /// as specified by LastBufferID, then we don't print the filename.  If not,
   /// we do print the filename, and then update LastBufferID with the BufferID
   /// printed.
   void print(raw_ostream &OS, const SourceManager &SM,
              unsigned &LastBufferID, bool PrintText = true) const;

   void print(raw_ostream &OS, const SourceManager &SM,
              bool PrintText = true) const {
      unsigned Tmp = ~0U;
      print(OS, SM, Tmp, PrintText);
   }

   POLAR_DEBUG_DUMPER(dump(const SourceManager &SM));

};

/// A half-open character-based source range.
class CharSourceRange {
   SourceLoc start;
   unsigned ByteLength;

public:
   /// Constructs an invalid range.
   CharSourceRange() = default;

   CharSourceRange(SourceLoc start, unsigned ByteLength)
      : start(start), ByteLength(ByteLength) {}

   /// Constructs a character range which starts and ends at the
   /// specified character locations.
   CharSourceRange(const SourceManager &SM, SourceLoc start, SourceLoc end);

   /// Use Lexer::getCharSourceRangeFromSourceRange() instead.
   CharSourceRange(const SourceManager &SM, SourceRange Range) = delete;

   bool isValid() const { return start.isValid(); }
   bool isInvalid() const { return !isValid(); }

   bool operator==(const CharSourceRange &other) const {
      return start == other.start && ByteLength == other.ByteLength;
   }
   bool operator!=(const CharSourceRange &other) const {
      return !operator==(other);
   }

   SourceLoc getStart() const { return start; }
   SourceLoc getEnd() const { return start.getAdvancedLocOrInvalid(ByteLength); }

   /// Returns true if the given source location is contained in the range.
   bool contains(SourceLoc loc) const {
      auto less = std::less<const char *>();
      auto less_equal = std::less_equal<const char *>();
      return less_equal(getStart().m_value.getPointer(), loc.m_value.getPointer()) &&
            less(loc.m_value.getPointer(), getEnd().m_value.getPointer());
   }

   bool contains(CharSourceRange Other) const {
      auto less_equal = std::less_equal<const char *>();
      return contains(Other.getStart()) &&
            less_equal(Other.getEnd().m_value.getPointer(), getEnd().m_value.getPointer());
   }

   /// expands *this to cover Other
   void widen(CharSourceRange Other) {
      auto Diff = Other.getEnd().m_value.getPointer() - getEnd().m_value.getPointer();
      if (Diff > 0) {
         ByteLength += Diff;
      }
      const auto MyStartPtr = getStart().m_value.getPointer();
      Diff = MyStartPtr - Other.getStart().m_value.getPointer();
      if (Diff > 0) {
         ByteLength += Diff;
         start = SourceLoc(llvm::SMLoc::getFromPointer(MyStartPtr - Diff));
      }
   }

   bool overlaps(CharSourceRange Other) const {
      if (getByteLength() == 0 || Other.getByteLength() == 0) return false;
      return contains(Other.getStart()) || Other.contains(getStart());
   }

   StringRef str() const {
      return StringRef(start.m_value.getPointer(), ByteLength);
   }

   /// Return the length of this valid range in bytes.  Can be zero.
   unsigned getByteLength() const {
      assert(isValid() && "length does not make sense for an invalid range");
      return ByteLength;
   }

   /// Print out the CharSourceRange.  If the locations are in the same buffer
   /// as specified by LastBufferID, then we don't print the filename.  If not,
   /// we do print the filename, and then update LastBufferID with the BufferID
   /// printed.
   void print(raw_ostream &OS, const SourceManager &SM,
              unsigned &LastBufferID, bool PrintText = true) const;

   void print(raw_ostream &OS, const SourceManager &SM,
              bool PrintText = true) const {
      unsigned Tmp = ~0U;
      print(OS, SM, Tmp, PrintText);
   }

   POLAR_DEBUG_DUMPER(dump(const SourceManager &SM));
};

} // polar

namespace llvm {
template <typename T> struct DenseMapInfo;

template <> struct DenseMapInfo<polar::SourceLoc> {
   static polar::SourceLoc getEmptyKey() {
      return polar::SourceLoc(
               SMLoc::getFromPointer(DenseMapInfo<const char *>::getEmptyKey()));
   }

   static polar::SourceLoc getTombstoneKey() {
      // Make this different from empty key. See for context:
      // http://lists.llvm.org/pipermail/llvm-dev/2015-July/088744.html
      return polar::SourceLoc(
               SMLoc::getFromPointer(DenseMapInfo<const char *>::getTombstoneKey()));
   }

   static unsigned getHashValue(const polar::SourceLoc &Val) {
      return DenseMapInfo<const void *>::getHashValue(
               Val.getOpaquePointerValue());
   }

   static bool isEqual(const polar::SourceLoc &LHS,
                       const polar::SourceLoc &RHS) {
      return LHS == RHS;
   }
};

template <> struct DenseMapInfo<polar::SourceRange> {
   static polar::SourceRange getEmptyKey() {
      return polar::SourceRange(polar::SourceLoc(
                                          SMLoc::getFromPointer(DenseMapInfo<const char *>::getEmptyKey())));
   }

   static polar::SourceRange getTombstoneKey() {
      // Make this different from empty key. See for context:
      // http://lists.llvm.org/pipermail/llvm-dev/2015-July/088744.html
      return polar::SourceRange(polar::SourceLoc(
                                          SMLoc::getFromPointer(DenseMapInfo<const char *>::getTombstoneKey())));
   }

   static unsigned getHashValue(const polar::SourceRange &Val) {
      return hash_combine(Val.start.getOpaquePointerValue(),
                          Val.end.getOpaquePointerValue());
   }

   static bool isEqual(const polar::SourceRange &LHS,
                       const polar::SourceRange &RHS) {
      return LHS == RHS;
   }
};

} // namespace llvm

#endif // POLARPHP_BASIC_SOURCE_LOC_H
