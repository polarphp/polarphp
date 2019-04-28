//===--- RawComment.h - Extraction of raw comments --------------*- C++ -*-===//
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

#ifndef POLARPHP_AST_RAW_COMMENT_H
#define POLARPHP_AST_RAW_COMMENT_H

#include "polarphp/parser/SourceLoc.h"
#include "polarphp/parser/SourceMgr.h"

namespace polar::ast {

using polar::parser::SourceLoc;
using polar::parser::SourceManager;
using polar::parser::CharSourceRange;

using polar::basic::ArrayRef;
using polar::basic::StringRef;

struct SingleRawComment
{
   enum class CommentKind
   {
      OrdinaryLine,  ///< Any normal // comments
      OrdinaryBlock, ///< Any normal /* */ comment
      LineDoc,       ///< \code /// stuff \endcode
      BlockDoc,      ///< \code /** stuff */ \endcode
   };

   CharSourceRange m_range;
   StringRef m_rawText;
   unsigned m_kind : 8;
   unsigned m_startColumn : 16;
   unsigned m_startLine;
   unsigned m_endLine;

   SingleRawComment(CharSourceRange range, const SourceManager &sourceMgr);
   SingleRawComment(StringRef rawText, unsigned startColumn);

   SingleRawComment(const SingleRawComment &) = default;
   SingleRawComment &operator=(const SingleRawComment &) = default;

   CommentKind getKind() const POLAR_READONLY
   {
      return static_cast<CommentKind>(m_kind);
   }

   bool isOrdinary() const POLAR_READONLY
   {
      return getKind() == CommentKind::OrdinaryLine ||
            getKind() == CommentKind::OrdinaryBlock;
   }

   bool isLine() const POLAR_READONLY
   {
      return getKind() == CommentKind::OrdinaryLine ||
            getKind() == CommentKind::LineDoc;
   }
};

struct RawComment
{
   ArrayRef<SingleRawComment> m_comments;

   RawComment()
   {}

   RawComment(ArrayRef<SingleRawComment> comments)
      : m_comments(comments)
   {}

   RawComment(const RawComment &) = default;
   RawComment &operator=(const RawComment &) = default;

   bool isEmpty() const
   {
      return m_comments.empty();
   }

   CharSourceRange getCharSourceRange();
};

struct CommentInfo
{
   StringRef brief;
   RawComment raw;
   uint32_t group;
   uint32_t sourceOrder;
};

} // polar::ast

#endif // POLARPHP_AST_RAW_COMMENT_H
