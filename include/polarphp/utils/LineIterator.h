// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

#ifndef POLARPHP_UTILS_LINE_ITERATOR_H
#define POLARPHP_UTILS_LINE_ITERATOR_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/global/DataTypes.h"
#include <iterator>

namespace polar {
namespace utils {

using polar::basic::StringRef;

class MemoryBuffer;

/// \brief A forward iterator which reads text lines from a buffer.
///
/// This class provides a forward iterator interface for reading one line at
/// a time from a buffer. When default constructed the iterator will be the
/// "end" iterator.
///
/// The iterator is aware of what line number it is currently processing. It
/// strips blank lines by default, and comment lines given a comment-starting
/// character.
///
/// Note that this iterator requires the buffer to be nul terminated.
class LineIterator
      : public std::iterator<std::forward_iterator_tag, StringRef>
{
   const MemoryBuffer *m_buffer;
   char m_commentMarker;
   bool m_skipBlanks;

   unsigned m_lineNumber;
   StringRef m_currentLine;

public:
   /// \brief Default construct an "end" iterator.
   LineIterator() : m_buffer(nullptr)
   {}

   /// \brief Construct a new iterator around some memory buffer.
   explicit LineIterator(const MemoryBuffer &buffer, bool skipBlanks = true,
                          char commentMarker = '\0');

   /// \brief Return true if we've reached EOF or are an "end" iterator.
   bool isAtEof() const
   {
      return !m_buffer;
   }

   /// \brief Return true if we're an "end" iterator or have reached EOF.
   bool isAtEnd() const
   {
      return isAtEof();
   }

   /// \brief Return the current line number. May return any number at EOF.
   int64_t getLineNumber() const
   {
      return m_lineNumber;
   }

   /// \brief Advance to the next (non-empty, non-comment) line.
   LineIterator &operator++()
   {
      advance();
      return *this;
   }

   LineIterator operator++(int)
   {
      LineIterator tmp(*this);
      advance();
      return tmp;
   }

   /// \brief Get the current line as a \c StringRef.
   StringRef operator*() const
   {
      return m_currentLine;
   }

   const StringRef *operator->() const
   {
      return &m_currentLine;
   }

   friend bool operator==(const LineIterator &lhs, const LineIterator &rhs)
   {
      return lhs.m_buffer == rhs.m_buffer &&
            lhs.m_currentLine.begin() == rhs.m_currentLine.begin();
   }

   friend bool operator!=(const LineIterator &lhs, const LineIterator &rhs)
   {
      return !(lhs == rhs);
   }

private:
   /// \brief Advance the iterator to the next line.
   void advance();
};

} // utils
} // polar

#endif // POLARPHP_UTILS_LINE_ITERATOR_H
