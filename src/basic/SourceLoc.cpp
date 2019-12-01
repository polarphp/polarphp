//===--- SourceLoc.cpp - SourceLoc and SourceRange implementations --------===//
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
// Created by polarboy on 2019/04/25.

#include "polarphp/basic/SourceLoc.h"
#include "polarphp/basic/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>

namespace polar::basic {

using llvm::errs;
using LLVMSourceMgr = llvm::SourceMgr;

void SourceRange::widen(SourceRange other)
{
   if (other.start.m_value.getPointer() < start.m_value.getPointer()) {
      start = other.start;
   }
   if (other.end.m_value.getPointer() > end.m_value.getPointer()) {
      end = other.end;
   }
}

void SourceLoc::printLineAndColumn(raw_ostream &ostream, const SourceManager &sourceMgr,
                                   unsigned bufferId) const
{
   if (isInvalid()) {
      ostream << "<invalid loc>";
      return;
   }
   auto lineAndCol = sourceMgr.getLineAndColumn(*this, bufferId);
   ostream << "line:" << lineAndCol.first << ':' << lineAndCol.second;
}

void SourceLoc::print(raw_ostream &ostream, const SourceManager &sourceMgr,
                      unsigned &lastBufferId) const
{
   if (isInvalid()) {
      ostream << "<invalid loc>";
      return;
   }

   unsigned bufferId = sourceMgr.findBufferContainingLoc(*this);
   if (bufferId != lastBufferId) {
      ostream << sourceMgr.getIdentifierForBuffer(bufferId);
      lastBufferId = bufferId;
   } else {
      ostream << "line";
   }
   auto lineAndCol = sourceMgr.getLineAndColumn(*this, bufferId);
   ostream << ':' << lineAndCol.first << ':' << lineAndCol.second;
}

void SourceLoc::dump(const SourceManager &sourceMgr) const
{
   print(llvm::errs(), sourceMgr);
}

void SourceRange::print(raw_ostream &ostream, const SourceManager &sourceMgr,
                        unsigned &lastBufferId, bool printText) const
{
   // FIXME: CharSourceRange is a half-open character-based range, while
   // SourceRange is a closed token-based range, so this conversion omits the
   // last token in the range. Unfortunately, we can't actually get to the end
   // of the token without using the Lex library, which would be a layering
   // violation. This is still better than nothing.
   CharSourceRange(sourceMgr, start, end).print(ostream, sourceMgr, lastBufferId, printText);
}

void SourceRange::dump(const SourceManager &sourceMgr) const
{
   print(llvm::errs(), sourceMgr);
}

CharSourceRange::CharSourceRange(const SourceManager &sourceMgr, SourceLoc start,
                                 SourceLoc end)
   : start(start)
{
   assert(start.isValid() == end.isValid() &&
          "start and end should either both be valid or both be invalid!");
   if (start.isValid()) {
      byteLength = sourceMgr.getByteDistance(start, end);
   }
}

void CharSourceRange::print(raw_ostream &ostream, const SourceManager &sourceMgr,
                            unsigned &lastBufferId, bool printText) const
{
   ostream << '[';
   start.print(ostream, sourceMgr, lastBufferId);
   ostream << " - ";
   getEnd().print(ostream, sourceMgr, lastBufferId);
   ostream << ']';
   if (start.isInvalid() || getEnd().isInvalid()) {
      return;
   }
   if (printText) {
      ostream << " RangeText=\"" << sourceMgr.extractText(*this) << '"';
   }
}

void CharSourceRange::dump(const SourceManager &sourceMgr) const
{
   print(llvm::errs(), sourceMgr);
}

} // polar::basic
