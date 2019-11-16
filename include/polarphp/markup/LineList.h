//===--- LineList.h - Data structures for Markup parsing --------*- C++ -*-===//
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

#ifndef POLARPHP_MARKUP_LINELIST_H
#define POLARPHP_MARKUP_LINELIST_H

#include "llvm/ADTArrayRef.h"
#include "llvm/ADTStringRef.h"
#include "polarphp/parser/SourceLoc.h"

namespace polar::markup {

using polar::basic::StringRef;
using polar::basic::MutableArrayRef;
using polar::basic::ArrayRef;
using polar::parser::SourceRange;

class MarkupContext;

/// Returns the amount of indentation on the line and
/// the length of the line's length.
size_t measure_indentation(StringRef text);

/// Represents a substring of a single line of source text.
struct Line
{
   StringRef m_text;
   SourceRange m_range;
   size_t m_firstNonspaceOffset;
public:
   Line(StringRef text, SourceRange range)
      : m_text(text),
        m_range(range),
        m_firstNonspaceOffset(measure_indentation(text))
   {}

   void dropFront(size_t amount)
   {
      m_text = m_text.dropFront(amount);
   }

   void drop_front(size_t amount)
   {
      dropFront(amount);
   }
};

/// A possibly non-contiguous view into a source buffer.
class LineList
{
   MutableArrayRef<Line> m_lines;
public:
   LineList(MutableArrayRef<Line> lines)
      : m_lines(lines)
   {}

   LineList() = default;

   std::string str() const;

   ArrayRef<Line> getLines() const
   {
      return m_lines;
   }

   /// Creates a LineList from a box selection of text.
   ///
   /// \param mcontext MarkupContext used for allocations
   /// \param startLine 0-based start line
   /// \param endLine 0-based end line (1 off the end)
   /// \param startColumn 0-based start column
   /// \param endColumn 0-based end column (1 off the end)
   /// \returns a new LineList with the selected start and end lines/columns.
   LineList subListWithRange(MarkupContext &mcontext, size_t startLine, size_t endLine,
                             size_t startColumn, size_t endColumn) const;
};

class LineListBuilder
{
   std::vector<Line> m_lines;
   MarkupContext &m_context;
public:
   LineListBuilder(MarkupContext &context)
      : m_context(context)
   {}

   void addLine(StringRef text, SourceRange range);
   LineList takeLineList() const;
};

} // polar::markup

#endif // POLARPHP_MARKUP_LINELIST_H
