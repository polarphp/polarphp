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
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2019/04/24.

#ifndef POLARPHP_PARSER_SOURCE_LOC_H
#define POLARPHP_PARSER_SOURCE_LOC_H

#include "polarphp/basic/adt/DenseMapInfo.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/SourceLocation.h"
#include <functional>

/// forwrad declare with namespace
namespace polar::utils {
class RawOutStream;
}

namespace polar {
namespace parser {

/// forward declare class
class SourceManager;
using polar::utils::RawOutStream;
using BasicSMLoc = polar::utils::SMLocation;

/// SourceLoc in parser namespace is just an polar::utils::SMLocation.
/// We define it as a different type
/// (instead of as a typedef) just to remove the "getFromPointer" methods and
/// enforce purity in the polarphp codebase.
///
class SourceLoc
{
public:
   SourceLoc()
   {}

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
   void print(RawOutStream &outStream, const SourceManager &sourceMgr,
              unsigned &lastBufferID) const;

   void printLineAndColumn(RawOutStream &outStream, const SourceManager &sourceMgr,
                           unsigned bufferID = 0) const;

   void print(RawOutStream &outStream, const SourceManager &sourceMgr) const
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

} // parser
} // polar

#endif // POLARPHP_PARSER_SOURCE_LOC_H



