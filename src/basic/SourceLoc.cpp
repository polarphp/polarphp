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

namespace polar {

using llvm::errs;
using LLVMSourceMgr = llvm::SourceMgr;

void SourceRange::widen(SourceRange Other) {
  if (Other.start.m_value.getPointer() < start.m_value.getPointer())
    start = Other.start;
  if (Other.end.m_value.getPointer() > end.m_value.getPointer())
    end = Other.end;
}

void SourceLoc::printLineAndColumn(raw_ostream &OS, const SourceManager &SM,
                                   unsigned BufferID) const {
  if (isInvalid()) {
    OS << "<invalid loc>";
    return;
  }

  auto LineAndCol = SM.getLineAndColumn(*this, BufferID);
  OS << "line:" << LineAndCol.first << ':' << LineAndCol.second;
}

void SourceLoc::print(raw_ostream &OS, const SourceManager &SM,
                      unsigned &LastBufferID) const {
  if (isInvalid()) {
    OS << "<invalid loc>";
    return;
  }

  unsigned BufferID = SM.findBufferContainingLoc(*this);
  if (BufferID != LastBufferID) {
    OS << SM.getIdentifierForBuffer(BufferID);
    LastBufferID = BufferID;
  } else {
    OS << "line";
  }

  auto LineAndCol = SM.getLineAndColumn(*this, BufferID);
  OS << ':' << LineAndCol.first << ':' << LineAndCol.second;
}

void SourceLoc::dump(const SourceManager &SM) const {
  print(llvm::errs(), SM);
}

void SourceRange::print(raw_ostream &OS, const SourceManager &SM,
                        unsigned &LastBufferID, bool PrintText) const {
  // FIXME: CharSourceRange is a half-open character-based range, while
  // SourceRange is a closed token-based range, so this conversion omits the
  // last token in the range. Unfortunately, we can't actually get to the end
  // of the token without using the Lex library, which would be a layering
  // violation. This is still better than nothing.
  CharSourceRange(SM, start, end).print(OS, SM, LastBufferID, PrintText);
}

void SourceRange::dump(const SourceManager &SM) const {
  print(llvm::errs(), SM);
}

CharSourceRange::CharSourceRange(const SourceManager &SM, SourceLoc start,
                                 SourceLoc end)
    : start(start) {
  assert(start.isValid() == end.isValid() &&
         "start and end should either both be valid or both be invalid!");
  if (start.isValid())
    ByteLength = SM.getByteDistance(start, end);
}

void CharSourceRange::print(raw_ostream &OS, const SourceManager &SM,
                            unsigned &LastBufferID, bool PrintText) const {
  OS << '[';
  start.print(OS, SM, LastBufferID);
  OS << " - ";
  getEnd().print(OS, SM, LastBufferID);
  OS << ']';

  if (start.isInvalid() || getEnd().isInvalid())
    return;

  if (PrintText) {
    OS << " RangeText=\"" << SM.extractText(*this) << '"';
  }
}

void CharSourceRange::dump(const SourceManager &SM) const {
  print(llvm::errs(), SM);
}

} // polar
