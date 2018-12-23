// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/06.

//===----------------------------------------------------------------------===//
//
// This file implements the SourceMgr class.  This class is used as a simple
// substrate for diagnostics, #include handling, and other low level things for
// simple parsers.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/SourceMgr.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/OptionalError.h"
#include "polarphp/utils/Locale.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/Path.h"
#include "polarphp/utils/SourceLocation.h"
#include "polarphp/utils/WithColor.h"
#include "polarphp/utils/RawOutStream.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace polar {
namespace utils {

using polar::basic::make_array_ref;
using polar::basic::find_if;

static const size_t sg_tabStop = 8;

unsigned SourceMgr::addIncludeFile(const std::string &filename,
                                   SMLocation includeLoc,
                                   std::string &includedFile)
{
   includedFile = filename;
   OptionalError<std::unique_ptr<MemoryBuffer>> newBufOrErr =
         MemoryBuffer::getFile(includedFile);

   // If the file didn't exist directly, see if it's in an include path.
   for (unsigned i = 0, e = m_includeDirectories.size(); i != e && !newBufOrErr;
        ++i) {
      includedFile =
            m_includeDirectories[i] + fs::path::get_separator().getData() + filename;
      newBufOrErr = MemoryBuffer::getFile(includedFile);
   }

   if (!newBufOrErr) {
      return 0;
   }
   return addNewSourceBuffer(std::move(*newBufOrErr), includeLoc);
}

unsigned SourceMgr::findBufferContainingLoc(SMLocation loc) const
{
   for (unsigned i = 0, e = m_buffers.size(); i != e; ++i) {
      if (loc.getPointer() >= m_buffers[i].m_buffer->getBufferStart() &&
          // Use <= here so that a pointer to the null at the end of the buffer
          // is included as part of the buffer.
          loc.getPointer() <= m_buffers[i].m_buffer->getBufferEnd()) {
         return i + 1;
      }
   }
   return 0;
}

template <typename T>
unsigned SourceMgr::SrcBuffer::getLineNumber(const char *ptr) const
{

   // Ensure m_offsetCache is allocated and populated with offsets of all the
   // '\n' bytes.
   std::vector<T> *offsets = nullptr;
   if (m_offsetCache.isNull()) {
      offsets = new std::vector<T>();
      m_offsetCache = offsets;
      size_t size = m_buffer->getBufferSize();
      assert(size <= std::numeric_limits<T>::max());
      StringRef str = m_buffer->getBuffer();
      for (size_t N = 0; N < size; ++N) {
         if (str[N] == '\n') {
            offsets->push_back(static_cast<T>(N));
         }
      }
   } else {
      offsets = m_offsetCache.get<std::vector<T> *>();
   }

   const char *bufStart = m_buffer->getBufferStart();
   assert(ptr >= bufStart && ptr <= m_buffer->getBufferEnd());
   ptrdiff_t ptrDiff = ptr - bufStart;
   assert(ptrDiff >= 0 && static_cast<size_t>(ptrDiff) <= std::numeric_limits<T>::max());
   T ptrOffset = static_cast<T>(ptrDiff);

   // std::lower_bound returns the first eol offset that's not-less-than
   // ptrOffset, meaning the eol that _ends the line_ that ptrOffset is on
   // (including if ptrOffset refers to the eol itself). If there's no such
   // eol, returns end().
   auto eol = std::lower_bound(offsets->begin(), offsets->end(), ptrOffset);

   // Lines count from 1, so add 1 to the distance from the 0th line.
   return (1 + (eol - offsets->begin()));
}

SourceMgr::SrcBuffer::SrcBuffer(SourceMgr::SrcBuffer &&other)
   : m_buffer(std::move(other.m_buffer)),
     m_offsetCache(other.m_offsetCache),
     m_includeLoc(other.m_includeLoc)
{
   other.m_offsetCache = nullptr;
}

SourceMgr::SrcBuffer::~SrcBuffer()
{
   if (!m_offsetCache.isNull()) {
      if (m_offsetCache.is<std::vector<uint8_t>*>()) {
         delete m_offsetCache.get<std::vector<uint8_t>*>();
      } else if (m_offsetCache.is<std::vector<uint16_t>*>()) {
         delete m_offsetCache.get<std::vector<uint16_t>*>();
      } else if (m_offsetCache.is<std::vector<uint32_t>*>()) {
         delete m_offsetCache.get<std::vector<uint32_t>*>();
      } else {
         delete m_offsetCache.get<std::vector<uint64_t>*>();
      }
      m_offsetCache = nullptr;
   }
}

std::pair<unsigned, unsigned>
SourceMgr::getLineAndColumn(SMLocation loc, unsigned bufferID) const
{
   if (!bufferID) {
      bufferID = findBufferContainingLoc(loc);
   }
   assert(bufferID && "Invalid Location!");
   auto &sb = getBufferInfo(bufferID);
   const char *ptr = loc.getPointer();

   size_t size = sb.m_buffer->getBufferSize();
   unsigned lineNo;
   if (size <= std::numeric_limits<uint8_t>::max()) {
      lineNo = sb.getLineNumber<uint8_t>(ptr);
   } else if (size <= std::numeric_limits<uint16_t>::max()) {
      lineNo = sb.getLineNumber<uint16_t>(ptr);
   } else if (size <= std::numeric_limits<uint32_t>::max()) {
      lineNo = sb.getLineNumber<uint32_t>(ptr);
   } else {
      lineNo = sb.getLineNumber<uint64_t>(ptr);
   }
   const char *bufStart = sb.m_buffer->getBufferStart();
   size_t newlineOffs = StringRef(bufStart, ptr-bufStart).findLastOf("\n\r");
   if (newlineOffs == StringRef::npos) {
      newlineOffs = ~(size_t)0;
   }
   return std::make_pair(lineNo, ptr-bufStart-newlineOffs);
}

void SourceMgr::printIncludeStack(SMLocation includeLoc, RawOutStream &outstream) const
{
   if (includeLoc == SMLocation()) {
      return;  // Top of stack.
   }
   unsigned curBuffer = findBufferContainingLoc(includeLoc);
   assert(curBuffer && "Invalid or unspecified location!");
   printIncludeStack(getBufferInfo(curBuffer).m_includeLoc, outstream);
   outstream << "Included from "
             << getBufferInfo(curBuffer).m_buffer->getBufferIdentifier()
             << ":" << findLineNumber(includeLoc, curBuffer) << ":\n";
}

SMDiagnostic SourceMgr::getMessage(SMLocation loc, SourceMgr::DiagKind m_kind,
                                   const Twine &msg,
                                   ArrayRef<SMRange> ranges,
                                   ArrayRef<SMFixIt> fixIts) const
{
   // First thing to do: find the current buffer containing the specified
   // location to pull out the source line.
   SmallVector<std::pair<unsigned, unsigned>, 4> colRanges;
   std::pair<unsigned, unsigned> lineAndCol;
   StringRef bufferID = "<unknown>";
   std::string lineStr;
   if (loc.isValid()) {
      unsigned curBuffer = findBufferContainingLoc(loc);
      assert(curBuffer && "Invalid or unspecified location!");

      const MemoryBuffer *curMB = getMemoryBuffer(curBuffer);
      bufferID = curMB->getBufferIdentifier();

      // Scan backward to find the start of the line.
      const char *lineStart = loc.getPointer();
      const char *bufStart = curMB->getBufferStart();
      while (lineStart != bufStart && lineStart[-1] != '\n' &&
             lineStart[-1] != '\r') {
         --lineStart;
      }
      // Get the end of the line.
      const char *lineEnd = loc.getPointer();
      const char *bufEnd = curMB->getBufferEnd();
      while (lineEnd != bufEnd && lineEnd[0] != '\n' && lineEnd[0] != '\r') {
         ++lineEnd;
      }
      lineStr = std::string(lineStart, lineEnd);
      // Convert any ranges to column ranges that only intersect the line of the
      // location.
      for (unsigned i = 0, e = ranges.getSize(); i != e; ++i) {
         SMRange range = ranges[i];
         if (!range.isValid()) {
            continue;
         }
         // If the line doesn't contain any part of the range, then ignore it.
         if (range.m_start.getPointer() > lineEnd || range.m_end.getPointer() < lineStart) {
            continue;
         }
         // Ignore pieces of the range that go onto other lines.
         if (range.m_start.getPointer() < lineStart) {
            range.m_start = SMLocation::getFromPointer(lineStart);
         }
         if (range.m_end.getPointer() > lineEnd) {
            range.m_end = SMLocation::getFromPointer(lineEnd);
         }
         // Translate from SMLocation ranges to column ranges.
         // FIXME: Handle multibyte characters.
         colRanges.push_back(std::make_pair(range.m_start.getPointer()-lineStart,
                                            range.m_end.getPointer()-lineStart));
      }

      lineAndCol = getLineAndColumn(loc, curBuffer);
   }

   return SMDiagnostic(*this, loc, bufferID, lineAndCol.first,
                       lineAndCol.second - 1, m_kind, msg.getStr(),
                       lineStr, colRanges, fixIts);
}

void SourceMgr::printMessage(RawOutStream &outstream, const SMDiagnostic &diagnostic,
                             bool showColors) const
{
   // Report the message with the diagnostic handler if present.
   if (m_diagHandler) {
      m_diagHandler(diagnostic, m_diagContext);
      return;
   }

   if (diagnostic.getLocation().isValid()) {
      unsigned curBuffer = findBufferContainingLoc(diagnostic.getLocation());
      assert(curBuffer && "Invalid or unspecified location!");
      printIncludeStack(getBufferInfo(curBuffer).m_includeLoc, outstream);
   }

   diagnostic.print(nullptr, outstream, showColors);
}

void SourceMgr::printMessage(RawOutStream &outstream, SMLocation loc,
                             SourceMgr::DiagKind m_kind,
                             const Twine &msg, ArrayRef<SMRange> ranges,
                             ArrayRef<SMFixIt> m_fixIts, bool showColors) const
{
   printMessage(outstream, getMessage(loc, m_kind, msg, ranges, m_fixIts), showColors);
}

void SourceMgr::printMessage(SMLocation loc, SourceMgr::DiagKind m_kind,
                             const Twine &msg, ArrayRef<SMRange> ranges,
                             ArrayRef<SMFixIt> m_fixIts, bool showColors) const
{
   printMessage(error_stream(), loc, m_kind, msg, ranges, m_fixIts, showColors);
}

//===----------------------------------------------------------------------===//
// SMDiagnostic Implementation
//===----------------------------------------------------------------------===//

SMDiagnostic::SMDiagnostic(const SourceMgr &sm, SMLocation location, StringRef filename,
                           int line, int column, SourceMgr::DiagKind m_kind,
                           StringRef msg, StringRef lineStr,
                           ArrayRef<std::pair<unsigned,unsigned>> ranges,
                           ArrayRef<SMFixIt> hints)
   : m_sourceMgr(&sm),
     m_location(location),
     m_filename(filename),
     m_lineNo(line),
     m_columnNo(column),
     m_kind(m_kind),
     m_message(msg),
     m_lineContents(lineStr),
     m_ranges(ranges.getVector()),
     m_fixIts(hints.begin(), hints.end())
{
   polar::basic::sort(m_fixIts);
}

namespace {
void build_fixit_line(std::string &caretLine, std::string &fixItLine,
                      ArrayRef<SMFixIt> fixIts, ArrayRef<char> sourceLine)
{
   if (fixIts.empty()) {
      return;
   }
   const char *lineStart = sourceLine.begin();
   const char *lineEnd = sourceLine.end();

   size_t prevHintEndCol = 0;

   for (ArrayRef<SMFixIt>::iterator iter = fixIts.begin(), endMark = fixIts.end();
        iter != endMark; ++iter) {
      // If the fixit contains a newline or tab, ignore it.
      if (iter->getText().findFirstOf("\n\r\t") != StringRef::npos) {
         continue;
      }
      SMRange range = iter->getRange();
      // If the line doesn't contain any part of the range, then ignore it.
      if (range.m_start.getPointer() > lineEnd || range.m_end.getPointer() < lineStart) {
         continue;
      }
      // Translate from SMLocation to column.
      // Ignore pieces of the range that go onto other lines.
      // FIXME: Handle multibyte characters in the source line.
      unsigned firstCol;
      if (range.m_start.getPointer() < lineStart) {
         firstCol = 0;
      } else {
         firstCol = range.m_start.getPointer() - lineStart;
      }
      // If we inserted a long previous hint, push this one forwards, and add
      // an extra space to show that this is not part of the previous
      // completion. This is sort of the best we can do when two hints appear
      // to overlap.
      //
      // Note that if this hint is located immediately after the previous
      // hint, no space will be added, since the location is more important.
      unsigned hintCol = firstCol;
      if (hintCol < prevHintEndCol) {
         hintCol = prevHintEndCol + 1;
      }

      // FIXME: This assertion is intended to catch unintended use of multibyte
      // characters in fixits. If we decide to do this, we'll have to track
      // separate byte widths for the source and fixit lines.
      assert((size_t)sys::locale::column_width(iter->getText()) ==
             iter->getText().getSize());

      // This relies on one byte per column in our fixit hints.
      unsigned lastColumnModified = hintCol + iter->getText().getSize();
      if (lastColumnModified > fixItLine.size()) {
         fixItLine.resize(lastColumnModified, ' ');
      }
      std::copy(iter->getText().begin(), iter->getText().end(),
                fixItLine.begin() + hintCol);

      prevHintEndCol = lastColumnModified;

      // For replacements, mark the removal range with '~'.
      // FIXME: Handle multibyte characters in the source line.
      unsigned lastCol;
      if (range.m_end.getPointer() >= lineEnd) {
         lastCol = lineEnd - lineStart;
      } else{
         lastCol = range.m_end.getPointer() - lineStart;
      }
      std::fill(&caretLine[firstCol], &caretLine[lastCol], '~');
   }
}

void print_source_line(RawOutStream &stream, StringRef m_lineContents)
{
   // Print out the source line one character at a time, so we can expand tabs.
   for (unsigned i = 0, e = m_lineContents.getSize(), outCol = 0; i != e; ++i) {
      size_t nextTab = m_lineContents.find('\t', i);
      // If there were no tabs left, print the rest, we are done.
      if (nextTab == StringRef::npos) {
         stream << m_lineContents.dropFront(i);
         break;
      }

      // Otherwise, print from i to nextTab.
      stream << m_lineContents.slice(i, nextTab);
      outCol += nextTab - i;
      i = nextTab;
      // If we have a tab, emit at least one space, then round up to 8 columns.
      do {
         stream << ' ';
         ++outCol;
      } while ((outCol % sg_tabStop) != 0);
   }
   stream << '\n';
}

bool is_non_ascii(char c)
{
   return c & 0x80;
}

} // anonymous namespace

void SMDiagnostic::print(const char *progName, RawOutStream &stream,
                         bool showColors, bool showKindLabel) const
{
   {
      WithColor colorStream(stream, RawOutStream::Colors::SAVEDCOLOR, true, false, !showColors);

      if (progName && progName[0]) {
         colorStream << progName << ": ";
      }
      if (!m_filename.empty()) {
         if (m_filename == "-") {
            colorStream << "<stdin>";
         } else {
            colorStream << m_filename;
         }
         if (m_lineNo != -1) {
            colorStream << ':' << m_lineNo;
            if (m_columnNo != -1) {
               colorStream << ':' << (m_columnNo + 1);
            }
         }
         colorStream << ": ";
      }
   }

   if (showKindLabel) {
      switch (m_kind) {
      case SourceMgr::DK_Error:
         WithColor::error(stream, "", !showColors);
         break;
      case SourceMgr::DK_Warning:
         WithColor::warning(stream, "", !showColors);
         break;
      case SourceMgr::DK_Note:
         WithColor::note(stream, "", !showColors);
         break;
      case SourceMgr::DK_Remark:
         WithColor::remark(stream, "", !showColors);
         break;
      }
   }

   WithColor(stream, RawOutStream::Colors::SAVEDCOLOR, true, false, !showColors)
         << m_message << '\n';

   if (m_lineNo == -1 || m_columnNo == -1) {
      return;
   }
   // FIXME: If there are multibyte or multi-column characters in the source, all
   // our ranges will be wrong. To do this properly, we'll need a byte-to-column
   // map like Clang's TextDiagnostic. For now, we'll just handle tabs by
   // expanding them later, and bail out rather than show incorrect ranges and
   // misaligned fixits for any other odd characters.
   if (find_if(m_lineContents, is_non_ascii) != m_lineContents.end()) {
      print_source_line(stream, m_lineContents);
      return;
   }
   size_t numColumns = m_lineContents.size();

   // Build the line with the caret and ranges.
   std::string caretLine(numColumns+1, ' ');

   // Expand any ranges.
   for (unsigned r = 0, e = m_ranges.size(); r != e; ++r) {
      std::pair<unsigned, unsigned> range = m_ranges[r];
      std::fill(&caretLine[range.first],
            &caretLine[std::min((size_t)range.second, caretLine.size())],
            '~');
   }

   // Add any fix-its.
   // FIXME: Find the beginning of the line properly for multibyte characters.
   std::string fixItInsertionLine;
   build_fixit_line(caretLine, fixItInsertionLine, m_fixIts,
                    make_array_ref(m_location.getPointer() - m_columnNo,
                                   m_lineContents.size()));

   // Finally, plop on the caret.
   if (unsigned(m_columnNo) <= numColumns) {
      caretLine[m_columnNo] = '^';
   } else {
      caretLine[numColumns] = '^';
   }

   // ... and remove trailing whitespace so the output doesn't wrap for it.  We
   // know that the line isn't completely empty because it has the caret in it at
   // least.
   caretLine.erase(caretLine.find_last_not_of(' ')+1);

   print_source_line(stream, m_lineContents);

   {
      WithColor colorStream(stream, RawOutStream::Colors::GREEN, true, false, !showColors);

      // Print out the caret line, matching tabs in the source line.
      for (unsigned i = 0, e = caretLine.size(), outCol = 0; i != e; ++i) {
         if (i >= m_lineContents.size() || m_lineContents[i] != '\t') {
            colorStream << caretLine[i];
            ++outCol;
            continue;
         }

         // Okay, we have a tab.  Insert the appropriate number of characters.
         do {
            colorStream << caretLine[i];
            ++outCol;
         } while ((outCol % sg_tabStop) != 0);
      }
      colorStream << '\n';
   }

   // Print out the replacement line, matching tabs in the source line.
   if (fixItInsertionLine.empty())
      return;

   for (size_t i = 0, e = fixItInsertionLine.size(), outCol = 0; i < e; ++i) {
      if (i >= m_lineContents.size() || m_lineContents[i] != '\t') {
         stream << fixItInsertionLine[i];
         ++outCol;
         continue;
      }

      // Okay, we have a tab.  Insert the appropriate number of characters.
      do {
         stream << fixItInsertionLine[i];
         // FIXME: This is trying not to break up replacements, but then to re-sync
         // with the tabs between replacements. This will fail, though, if two
         // fix-it replacements are exactly adjacent, or if a fix-it contains a
         // space. Really we should be precomputing column widths, which we'll
         // need anyway for multibyte chars.
         if (fixItInsertionLine[i] != ' ') {
            ++i;
         }
         ++outCol;
      } while (((outCol % sg_tabStop) != 0) && i != e);
   }
   stream << '\n';
}

} // utils
} // polar
