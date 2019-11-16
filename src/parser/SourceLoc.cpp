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

#include "polarphp/parser/SourceLoc.h"
#include "polarphp/parser/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>

namespace polar::parser {

using llvm::errs;

void SourceRange::widen(SourceRange other)
{
   if (other.m_start.m_loc.getPointer() < m_start.m_loc.getPointer()) {
      m_start = other.m_start;
   }
   if (other.m_end.m_loc.getPointer() > m_end.m_loc.getPointer()) {
      m_end = other.m_end;
   }
}

void SourceLoc::printLineAndColumn(raw_ostream &outStream, const SourceManager &sourceMgr,
                                   unsigned bufferId) const
{
   if (isInvalid()) {
      outStream << "<invalid loc>";
      return;
   }

   auto lineAndCol = sourceMgr.getLineAndColumn(*this, bufferId);
   outStream << "line:" << lineAndCol.first << ':' << lineAndCol.second;
}

void SourceLoc::print(raw_ostream &outStream, const SourceManager &sourceMgr,
                      unsigned &lastbufferID) const
{
   if (isInvalid()) {
      outStream << "<invalid loc>";
      return;
   }

   unsigned bufferId = sourceMgr.findBufferContainingLoc(*this);
   if (bufferId != lastbufferID) {
      outStream << sourceMgr.getIdentifierForBuffer(bufferId);
      lastbufferID = bufferId;
   } else {
      outStream << "line";
   }

   auto lineAndCol = sourceMgr.getLineAndColumn(*this, bufferId);
   outStream << ':' << lineAndCol.first << ':' << lineAndCol.second;
}

void SourceLoc::dump(const SourceManager &sourceMgr) const
{
   print(llvm::errs(), sourceMgr);
}

void SourceRange::print(raw_ostream &outStream, const SourceManager &sourceMgr,
                        unsigned &lastbufferID, bool printText) const
{
   // FIXME: CharSourceRange is a half-open character-based range, while
   // SourceRange is a closed token-based range, so this conversion omits the
   // last token in the range. Unfortunately, we can't actually get to the end
   // of the token without using the Lex library, which would be a layering
   // violation. This is still better than nothing.
   CharSourceRange(sourceMgr, m_start, m_end).print(outStream, sourceMgr, lastbufferID, printText);
}

void SourceRange::dump(const SourceManager &sourceMgr) const
{
   print(llvm::errs(), sourceMgr);
}

CharSourceRange::CharSourceRange(const SourceManager &sourceMgr, SourceLoc start,
                                 SourceLoc end)
   : m_start(start)
{
   assert(m_start.isValid() == end.isValid() &&
          "Start and end should either both be valid or both be invalid!");
   if (m_start.isValid()) {
      m_byteLength = sourceMgr.getByteDistance(m_start, end);
   }
}

void CharSourceRange::print(raw_ostream &outStream, const SourceManager &sourceMgr,
                            unsigned &lastbufferID, bool printText) const
{
   outStream << '[';
   m_start.print(outStream, sourceMgr, lastbufferID);
   outStream << " - ";
   getEnd().print(outStream, sourceMgr, lastbufferID);
   outStream << ']';
   if (m_start.isInvalid() || getEnd().isInvalid()) {
      return;
   }
   if (printText) {
      outStream << " RangeText=\"" << sourceMgr.extractText(*this) << '"';
   }
}

void CharSourceRange::dump(const SourceManager &sourceMgr) const
{
   print(llvm::errs(), sourceMgr);
}

} // polar::parser
