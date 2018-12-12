// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/25.

#include "CheckString.h"
#include "CheckFuncs.h"
#include "CLI/CLI.hpp"
#include "polarphp/basic/adt/Twine.h"
#include <list>

namespace polar {
namespace filechecker {

using polar::basic::Twine;

/// match check string and its "not strings" and/or "dag strings".
size_t CheckString::check(const SourceMgr &sourceMgr, StringRef buffer,
                          bool isLabelScanMode, size_t &matchLen,
                          StringMap<std::string> &variableTable) const
{
   size_t lastPos = 0;
   std::vector<const Pattern *> notStrings;
   // isLabelScanMode is true when we are scanning forward to find CHECK-LABEL
   // bounds; we have not processed variable definitions within the bounded block
   // yet so cannot handle any final CHECK-DAG yet; this is handled when going
   // over the block again (including the last CHECK-LABEL) in normal mode.
   if (!isLabelScanMode) {
      // match "dag strings" (with mixed "not strings" if any).
      lastPos = checkDag(sourceMgr, buffer, notStrings, variableTable);
      if (lastPos == StringRef::npos) {
         return StringRef::npos;
      }
   }

   // match itself from the last position after matching CHECK-DAG.
   StringRef matchBuffer = buffer.substr(lastPos);
   size_t matchPos = m_pattern.match(matchBuffer, matchLen, variableTable);
   if (matchPos == StringRef::npos) {
      print_no_match(true, sourceMgr, *this, matchBuffer, variableTable);
      return StringRef::npos;
   }
   print_match(true, sourceMgr, *this, matchBuffer, variableTable, matchPos, matchLen);

   // Similar to the above, in "label-scan mode" we can't yet handle CHECK-NEXT
   // or CHECK-NOT
   if (!isLabelScanMode) {
      StringRef skippedRegion = buffer.substr(lastPos, matchPos);

      // If this check is a "CHECK-NEXT", verify that the previous match was on
      // the previous line (i.e. that there is one newline between them).
      if (checkNext(sourceMgr, skippedRegion)) {
         return StringRef::npos;
      }

      // If this check is a "CHECK-SAME", verify that the previous match was on
      // the same line (i.e. that there is no newline between them).
      if (checkSame(sourceMgr, skippedRegion)) {
         return StringRef::npos;
      }

      // If this match had "not strings", verify that they don't exist in the
      // skipped region.
      if (checkNot(sourceMgr, skippedRegion, notStrings, variableTable)) {
         return StringRef::npos;
      }
   }

   return lastPos + matchPos;
}


/// Verify there is a single line in the given buffer.
bool CheckString::checkNext(const SourceMgr &sourceMgr, StringRef buffer) const
{
   if (m_pattern.getCheckType() != CheckType::CheckNext &&
       m_pattern.getCheckType() != CheckType::CheckEmpty) {
      return false;
   }
   Twine checkName =
         m_prefix +
         Twine(m_pattern.getCheckType() == CheckType::CheckEmpty ? "-EMPTY" : "-NEXT");

   // Count the number of newlines between the previous match and this one.
   assert(buffer.getData() !=
         sourceMgr.getMemoryBuffer(sourceMgr.findBufferContainingLoc(
                                      SMLocation::getFromPointer(buffer.getData())))
         ->getBufferStart() &&
         "CHECK-NEXT and CHECK-EMPTY can't be the first check in a file");

   const char *firstNewLine = nullptr;
   unsigned numNewLines = count_num_newlines_between(buffer, firstNewLine);

   if (numNewLines == 0) {
      sourceMgr.printMessage(m_location, SourceMgr::DK_Error,
                             checkName + ": is on the same line as previous match");
      sourceMgr.printMessage(SMLocation::getFromPointer(buffer.end()), SourceMgr::DK_Note,
                             "'next' match was here");
      sourceMgr.printMessage(SMLocation::getFromPointer(buffer.getData()), SourceMgr::DK_Note,
                             "previous match ended here");
      return true;
   }

   if (numNewLines != 1) {
      sourceMgr.printMessage(m_location, SourceMgr::DK_Error,
                             checkName +
                             ": is not on the line after the previous match");
      sourceMgr.printMessage(SMLocation::getFromPointer(buffer.end()), SourceMgr::DK_Note,
                             "'next' match was here");
      sourceMgr.printMessage(SMLocation::getFromPointer(buffer.getData()), SourceMgr::DK_Note,
                             "previous match ended here");
      sourceMgr.printMessage(SMLocation::getFromPointer(firstNewLine), SourceMgr::DK_Note,
                             "non-matching line after previous match is here");
      return true;
   }

   return false;
}

/// Verify there is no newline in the given buffer.
bool CheckString::checkSame(const SourceMgr &sourceMgr, StringRef buffer) const
{
   if (m_pattern.getCheckType() != CheckType::CheckSame) {
      return false;
   }
   // Count the number of newlines between the previous match and this one.
   assert(buffer.getData() !=
         sourceMgr.getMemoryBuffer(sourceMgr.findBufferContainingLoc(
                                      SMLocation::getFromPointer(buffer.getData())))
         ->getBufferStart() &&
         "CHECK-SAME can't be the first check in a file");

   const char *firstNewLine = nullptr;
   unsigned numNewLines = count_num_newlines_between(buffer, firstNewLine);

   if (numNewLines != 0) {
      sourceMgr.printMessage(m_location, SourceMgr::DK_Error,
                             m_prefix +
                             "-SAME: is not on the same line as the previous match");
      sourceMgr.printMessage(SMLocation::getFromPointer(buffer.end()), SourceMgr::DK_Note,
                             "'next' match was here");
      sourceMgr.printMessage(SMLocation::getFromPointer(buffer.getData()), SourceMgr::DK_Note,
                             "previous match ended here");
      return true;
   }

   return false;
}

/// Verify there's no "not strings" in the given buffer.
bool CheckString::checkNot(const SourceMgr &sourceMgr, StringRef buffer,
                           const std::vector<const Pattern *> &notStrings,
                           StringMap<std::string> &variableTable) const
{
   for (const Pattern *m_pattern : notStrings) {
      assert((m_pattern->getCheckType() == CheckType::CheckNot) && "Expect CHECK-NOT!");

      size_t matchLen = 0;
      size_t pos = m_pattern->match(buffer, matchLen, variableTable);

      if (pos == StringRef::npos) {
         print_no_match(false, sourceMgr, m_prefix, m_pattern->getLoc(), *m_pattern, buffer,
                        variableTable);
         continue;
      }

      print_match(false, sourceMgr, m_prefix, m_pattern->getLoc(), *m_pattern, buffer, variableTable,
                  pos, matchLen);

      return true;
   }

   return false;
}

/// match "dag strings" and their mixed "not strings".
size_t CheckString::checkDag(const SourceMgr &sourceMgr, StringRef buffer,
                             std::vector<const Pattern *> &notStrings,
                             StringMap<std::string> &variableTable) const
{
   CLI::App &parser = retrieve_command_parser();
   bool verboseVerbose = parser.get_option("-v")->count() > 1 ? true : false;
   bool allowDeprecatedDagOverlap = parser.get_option("--allow-deprecated-dag-overlap")->count() > 0 ? true : false;
   if (m_dagNotStrings.empty()) {
      return 0;
   }

   // The start of the search range.
   size_t startPos = 0;

   struct MatchRange {
      size_t pos;
      size_t end;
   };
   // A sorted list of ranges for non-overlapping CHECK-DAG matches.  match
   // ranges are erased from this list once they are no longer in the search
   // range.
   std::list<MatchRange> matchRanges;

   // We need patternIter and patternEnd later for detecting the end of a CHECK-DAG
   // group, so we don't use a range-based for loop here.
   for (auto patternIter = m_dagNotStrings.begin(), patternEnd = m_dagNotStrings.end();
        patternIter != patternEnd; ++patternIter) {
      const Pattern &pattern = *patternIter;
      assert((pattern.getCheckType() == CheckType::CheckDAG ||
              pattern.getCheckType() == CheckType::CheckNot) &&
             "Invalid CHECK-DAG or CHECK-NOT!");

      if (pattern.getCheckType() == CheckType::CheckNot) {
         notStrings.push_back(&pattern);
         continue;
      }

      assert((pattern.getCheckType() == CheckType::CheckDAG) && "Expect CHECK-DAG!");

      // CHECK-DAG always matches from the start.
      size_t matchLen = 0, matchPos = startPos;

      // Search for a match that doesn't overlap a previous match in this
      // CHECK-DAG group.
      for (auto miter = matchRanges.begin(), mend = matchRanges.end(); true; ++miter) {
         StringRef matchBuffer = buffer.substr(matchPos);
         size_t matchPosBuf = pattern.match(matchBuffer, matchLen, variableTable);
         // With a group of CHECK-DAGs, a single mismatching means the match on
         // that group of CHECK-DAGs fails immediately.
         if (matchPosBuf == StringRef::npos) {
            print_no_match(true, sourceMgr, m_prefix, pattern.getLoc(), pattern, matchBuffer,
                           variableTable);
            return StringRef::npos;
         }
         // Re-calc it as the offset relative to the start of the original string.
         matchPos += matchPosBuf;
         if (verboseVerbose) {
            print_match(true, sourceMgr, m_prefix, pattern.getLoc(), pattern, buffer, variableTable,
                        matchPos, matchLen);
         }

         MatchRange mrange{matchPos, matchPos + matchLen};
         if (allowDeprecatedDagOverlap) {
            // We don't need to track all matches in this mode, so we just maintain
            // one match range that encompasses the current CHECK-DAG group's
            // matches.
            if (matchRanges.empty()) {
               matchRanges.insert(matchRanges.end(), mrange);
            } else {
               auto block = matchRanges.begin();
               block->pos = std::min(block->pos, mrange.pos);
               block->end = std::max(block->end, mrange.end);
            }
            break;
         }
         // Iterate previous matches until overlapping match or insertion point.
         bool overlap = false;
         for (; miter != mend; ++miter) {
            if (mrange.pos < miter->end) {
               // !overlap => New match has no overlap and is before this old match.
               // overlap => New match overlaps this old match.
               overlap = miter->pos < mrange.end;
               break;
            }
         }
         if (!overlap) {
            // Insert non-overlapping match into list.
            matchRanges.insert(miter, mrange);
            break;
         }
         if (verboseVerbose) {
            SMLocation oldStart = SMLocation::getFromPointer(buffer.getData() + miter->pos);
            SMLocation oldEnd = SMLocation::getFromPointer(buffer.getData() + miter->end);
            SMRange oldRange(oldStart, oldEnd);
            sourceMgr.printMessage(oldStart, SourceMgr::DK_Note,
                                   "match discarded, overlaps earlier DAG match here",
            {oldRange});
         }
         matchPos = miter->end;
      }
      if (!verboseVerbose) {
         print_match(true, sourceMgr, m_prefix, pattern.getLoc(), pattern, buffer, variableTable,
                     matchPos, matchLen);
      }

      // Handle the end of a CHECK-DAG group.
      if (std::next(patternIter) == patternEnd ||
          std::next(patternIter)->getCheckType() == CheckType::CheckNot) {
         if (!notStrings.empty()) {
            // If there are CHECK-NOTs between two CHECK-DAGs or from CHECK to
            // CHECK-DAG, verify that there are no 'not' strings occurred in that
            // region.
            StringRef skippedRegion =
                  buffer.slice(startPos, matchRanges.begin()->pos);
            if (checkNot(sourceMgr, skippedRegion, notStrings, variableTable)) {
               return StringRef::npos;
            }
            // Clear "not strings".
            notStrings.clear();
         }
         // All subsequent CHECK-DAGs and CHECK-NOTs should be matched from the
         // end of this CHECK-DAG group's match range.
         startPos = matchRanges.rbegin()->end;
         // Don't waste time checking for (impossible) overlaps before that.
         matchRanges.clear();
      }
   }
   return startPos;
}

} // filechecker
} // polar
