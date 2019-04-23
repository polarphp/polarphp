//===- FileCheck.cpp - Check that File's Contents match what is expected --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/23.

#include "FileChecker.h"

#include "polarphp/basic/adt/StringSet.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/utils/FormatVariadic.h"
#include "polarphp/utils/StringUtils.h"
#include "polarphp/utils/RawOutStream.h"

#include <cstdint>
#include <list>
#include <map>
#include <tuple>
#include <utility>

namespace polar {
namespace filechecker {

using polar::basic::SmallString;
using polar::basic::Twine;
using polar::basic::StringSet;
using polar::basic::SmallVector;
using polar::utils::regex_escape;
using polar::utils::error_stream;
using polar::utils::RawSvectorOutStream;

/// Parses the given string into the Pattern.
///
/// \p prefix provides which prefix is being matched, \p sourceMgr provides the
/// SourceMgr used for error reports, and \p lineNumber is the line number in
/// the input file from which the pattern string was read. Returns true in
/// case of an error, false otherwise.
bool FileCheckPattern::parsePattern(StringRef patternStr, StringRef prefix,
                                    SourceMgr &sourceMgr, unsigned lineNumber,
                                    const FileCheckRequest &req) {
   bool matchFullLinesHere = req.matchFullLines && m_checkType != FileCheckKind::CheckNot;

   this->m_lineNumber = lineNumber;
   m_patternLoc = SMLocation::getFromPointer(patternStr.getData());

   if (!(req.noCanonicalizeWhiteSpace && req.matchFullLines))
      // Ignore trailing whitespace.
      while (!patternStr.empty() &&
             (patternStr.back() == ' ' || patternStr.back() == '\t'))
         patternStr = patternStr.substr(0, patternStr.size() - 1);

   // Check that there is something on the line.
   if (patternStr.empty() && m_checkType != FileCheckKind::CheckEmpty) {
      sourceMgr.printMessage(m_patternLoc, SourceMgr::DK_Error,
                             "found empty check string with prefix '" + prefix + ":'");
      return true;
   }

   if (!patternStr.empty() && m_checkType == FileCheckKind::CheckEmpty) {
      sourceMgr.printMessage(
               m_patternLoc, SourceMgr::DK_Error,
               "found non-empty check string for empty check with prefix '" + prefix +
               ":'");
      return true;
   }

   if (m_checkType == FileCheckKind::CheckEmpty) {
      m_regExStr = "(\n$)";
      return false;
   }

   // Check to see if this is a fixed string, or if it has regex pieces.
   if (!matchFullLinesHere &&
       (patternStr.size() < 2 || (patternStr.find("{{") == StringRef::npos &&
                                  patternStr.find("[[") == StringRef::npos))) {
      m_fixedStr = patternStr;
      return false;
   }

   if (matchFullLinesHere) {
      m_regExStr += '^';
      if (!req.noCanonicalizeWhiteSpace)
         m_regExStr += " *";
   }

   // Paren value #0 is for the fully matched string.  Any new parenthesized
   // values add from there.
   unsigned curParen = 1;

   // Otherwise, there is at least one regex piece.  Build up the regex pattern
   // by escaping scary characters in fixed strings, building up one big regex.
   while (!patternStr.empty()) {
      // RegEx matches.
      if (patternStr.startsWith("{{")) {
         // This is the start of a regex match.  Scan for the }}.
         size_t end = patternStr.find("}}");
         if (end == StringRef::npos) {
            sourceMgr.printMessage(SMLocation::getFromPointer(patternStr.getData()),
                                   SourceMgr::DK_Error,
                                   "found start of regex string with no end '}}'");
            return true;
         }

         // Enclose {{}} patterns in parens just like [[]] even though we're not
         // capturing the result for any purpose.  This is required in case the
         // expression contains an alternation like: CHECK:  abc{{x|z}}def.  We
         // want this to turn into: "abc(x|z)def" not "abcx|zdef".
         m_regExStr += '(';
         ++curParen;

         if (addRegExToRegEx(patternStr.substr(2, end - 2), curParen, sourceMgr)) {
            return true;
         }

         m_regExStr += ')';

         patternStr = patternStr.substr(end + 2);
         continue;
      }

      // Named RegEx matches.  These are of two forms: [[foo:.*]] which matches .*
      // (or some other regex) and assigns it to the FileCheck variable 'foo'. The
      // second form is [[foo]] which is a reference to foo.  The variable name
      // itself must be of the form "[a-zA-Z_][0-9a-zA-Z_]*", otherwise we reject
      // it.  This is to catch some common errors.
      if (patternStr.startsWith("[[")) {
         // Find the closing bracket pair ending the match.  end is going to be an
         // offset relative to the beginning of the match string.
         size_t end = findRegexVarEnd(patternStr.substr(2), sourceMgr);

         if (end == StringRef::npos) {
            sourceMgr.printMessage(SMLocation::getFromPointer(patternStr.getData()),
                                   SourceMgr::DK_Error,
                                   "invalid named regex reference, no ]] found");
            return true;
         }

         StringRef matchStr = patternStr.substr(2, end);
         patternStr = patternStr.substr(end + 4);

         // Get the regex name (e.g. "foo").
         size_t nameEnd = matchStr.find(':');
         StringRef name = matchStr.substr(0, nameEnd);

         if (name.empty()) {
            sourceMgr.printMessage(SMLocation::getFromPointer(name.getData()), SourceMgr::DK_Error,
                                   "invalid name in named regex: empty name");
            return true;
         }

         // Verify that the name/expression is well formed. FileCheck currently
         // supports @LINE, @LINE+number, @LINE-number expressions. The check here
         // is relaxed, more strict check is performed in \c EvaluateExpression.
         bool IsExpression = false;
         for (unsigned i = 0, e = name.size(); i != e; ++i) {
            if (i == 0) {
               if (name[i] == '$')  // Global vars start with '$'
                  continue;
               if (name[i] == '@') {
                  if (nameEnd != StringRef::npos) {
                     sourceMgr.printMessage(SMLocation::getFromPointer(name.getData()),
                                            SourceMgr::DK_Error,
                                            "invalid name in named regex definition");
                     return true;
                  }
                  IsExpression = true;
                  continue;
               }
            }
            if (name[i] != '_' && !isalnum(name[i]) &&
                (!IsExpression || (name[i] != '+' && name[i] != '-'))) {
               sourceMgr.printMessage(SMLocation::getFromPointer(name.getData() + i),
                                      SourceMgr::DK_Error, "invalid name in named regex");
               return true;
            }
         }

         // name can't start with a digit.
         if (isdigit(static_cast<unsigned char>(name[0]))) {
            sourceMgr.printMessage(SMLocation::getFromPointer(name.getData()), SourceMgr::DK_Error,
                                   "invalid name in named regex");
            return true;
         }

         // Handle [[foo]].
         if (nameEnd == StringRef::npos) {
            // Handle variables that were defined earlier on the same line by
            // emitting a backreference.
            if (m_variableDefs.find(name) != m_variableDefs.end()) {
               unsigned VarParenNum = m_variableDefs[name];
               if (VarParenNum < 1 || VarParenNum > 9) {
                  sourceMgr.printMessage(SMLocation::getFromPointer(name.getData()),
                                         SourceMgr::DK_Error,
                                         "Can't back-reference more than 9 variables");
                  return true;
               }
               addBackrefToRegEx(VarParenNum);
            } else {
               m_variableUses.push_back(std::make_pair(name, m_regExStr.size()));
            }
            continue;
         }

         // Handle [[foo:.*]].
         m_variableDefs[name] = curParen;
         m_regExStr += '(';
         ++curParen;

         if (addRegExToRegEx(matchStr.substr(nameEnd + 1), curParen, sourceMgr)) {
            return true;
         }
         m_regExStr += ')';
      }

      // Handle fixed string matches.
      // Find the end, which is the start of the next regex.
      size_t fixedMatchEnd = patternStr.find("{{");
      fixedMatchEnd = std::min(fixedMatchEnd, patternStr.find("[["));
      m_regExStr += regex_escape(patternStr.substr(0, fixedMatchEnd));
      patternStr = patternStr.substr(fixedMatchEnd);
   }

   if (matchFullLinesHere) {
      if (!req.noCanonicalizeWhiteSpace) {
         m_regExStr += " *";
      }
      m_regExStr += '$';
   }

   return false;
}

bool FileCheckPattern::addRegExToRegEx(StringRef regexStr, unsigned &curParen, SourceMgr &sourceMgr)
{
   try {
      std::string stdRegexStr = regexStr.getStr();
      boost::regex regex(stdRegexStr);
      m_regExStr += stdRegexStr;
      curParen += regex.mark_count();
      return false;
   } catch (boost::regex_error &e) {
      sourceMgr.printMessage(SMLocation::getFromPointer(regexStr.getData()), SourceMgr::DK_Error,
                             "invalid regex: " + StringRef(e.what()));
      return true;
   }
}

void FileCheckPattern::addBackrefToRegEx(unsigned backrefNum)
{
   assert(backrefNum >= 1 && backrefNum <= 9 && "Invalid backref number");
   std::string backref = std::string("\\") + std::string(1, '0' + backrefNum);
   m_regExStr += backref;
}

/// Evaluates expression and stores the result to \p value.
///
/// Returns true on success and false when the expression has invalid syntax.
bool FileCheckPattern::evaluateExpression(StringRef expr, std::string &value) const
{
   // The only supported expression is @LINE([\+-]\d+)?
   if (!expr.startsWith("@LINE")) {
      return false;
   }
   expr = expr.substr(StringRef("@LINE").size());
   int offset = 0;
   if (!expr.empty()) {
      if (expr[0] == '+') {
         expr = expr.substr(1);
      } else if (expr[0] != '-') {
         return false;
      }
      if (expr.getAsInteger(10, offset)) {
         return false;
      }
   }
   value = polar::basic::itostr(m_lineNumber + offset);
   return true;
}

/// Matches the pattern string against the input buffer \p buffer
///
/// This returns the position that is matched or npos if there is no match. If
/// there is a match, the size of the matched string is returned in \p
/// matchLen.
///
/// The \p variableTable StringMap provides the current values of filecheck
/// variables and is updated if this match defines new values.
size_t FileCheckPattern::match(StringRef buffer, size_t &matchLen,
                               StringMap<std::string> &variableTable) const
{
   // If this is the EOF pattern, match it immediately.
   if (m_checkType == FileCheckKind::CheckEOF) {
      matchLen = 0;
      return buffer.size();
   }

   // If this is a fixed string pattern, just match it now.
   if (!m_fixedStr.empty()) {
      matchLen = m_fixedStr.size();
      return buffer.find(m_fixedStr);
   }

   // Regex match.

   // If there are variable uses, we need to create a temporary string with the
   // actual value.
   StringRef regExToMatch = m_regExStr;
   std::string tmpStr;
   if (!m_variableUses.empty()) {
      tmpStr = m_regExStr;

      unsigned insertOffset = 0;
      for (const auto &variableUse : m_variableUses) {
         std::string value;

         if (variableUse.first[0] == '@') {
            if (!evaluateExpression(variableUse.first, value))
               return StringRef::npos;
         } else {
            StringMap<std::string>::iterator it =
                  variableTable.find(variableUse.first);
            // If the variable is undefined, return an error.
            if (it == variableTable.end()) {
               return StringRef::npos;
            }

            // Look up the value and escape it so that we can put it into the regex.
            value += regex_escape(it->m_second);
         }

         // Plop it into the regex at the adjusted offset.
         tmpStr.insert(tmpStr.begin() + variableUse.second + insertOffset,
                       value.begin(), value.end());
         insertOffset += value.size();
      }

      // Match the newly constructed regex.
      regExToMatch = tmpStr;
   }

   boost::cmatch matchInfo;
   if (!boost::regex_search(buffer.begin(), buffer.end(), matchInfo, boost::regex(regExToMatch.getStr()), boost::match_not_dot_newline)) {
      return StringRef::npos;
   }

   // Successful regex match.
   assert(!matchInfo.empty() && "Didn't get any match");
   StringRef fullMatch(buffer.getData() + matchInfo.position(), matchInfo[0].length());

   // If this defines any variables, remember their values.
   for (const auto &variableDef : m_variableDefs) {
      assert(variableDef.second < matchInfo.size() && "Internal paren error");
      variableTable[variableDef.first] = matchInfo[variableDef.second].str();
   }

   // Like CHECK-NEXT, CHECK-EMPTY's match range is considered to start after
   // the required preceding newline, which is consumed by the pattern in the
   // case of CHECK-EMPTY but not CHECK-NEXT.
   size_t matchStartSkip = m_checkType == FileCheckKind::CheckEmpty;
   matchLen = fullMatch.size() - matchStartSkip;
   return fullMatch.getData() - buffer.getData() + matchStartSkip;
}


/// Computes an arbitrary estimate for the quality of matching this pattern at
/// the start of \p buffer; a distance of zero should correspond to a perfect
/// match.
unsigned
FileCheckPattern::computeMatchDistance(StringRef buffer,
                                       const StringMap<std::string> &variableTable) const {
   // Just compute the number of matching characters. For regular expressions, we
   // just compare against the regex itself and hope for the best.
   //
   // FIXME: One easy improvement here is have the regex lib generate a single
   // example regular expression which matches, and use that as the example
   // string.
   StringRef exampleString(m_fixedStr);
   if (exampleString.empty()) {
      exampleString = m_regExStr;
   }

   // Only compare up to the first line in the buffer, or the string size.
   StringRef bufferPrefix = buffer.substr(0, exampleString.size());
   bufferPrefix = bufferPrefix.split('\n').first;
   return bufferPrefix.editDistance(exampleString);
}

void FileCheckPattern::printVariableUses(const SourceMgr &sourceMgr, StringRef buffer,
                                         const StringMap<std::string> &variableTable,
                                         SMRange m_matchRange) const {
   // If this was a regular expression using variables, print the current
   // variable values.
   if (!m_variableUses.empty()) {
      for (const auto &variableUse : m_variableUses) {
         SmallString<256> msg;
         RawSvectorOutStream outstream(msg);
         StringRef var = variableUse.first;
         if (var[0] == '@') {
            std::string value;
            if (evaluateExpression(var, value)) {
               outstream << "with expression \"";
               outstream.writeEscaped(var) << "\" equal to \"";
               outstream.writeEscaped(value) << "\"";
            } else {
               outstream << "uses incorrect expression \"";
               outstream.writeEscaped(var) << "\"";
            }
         } else {
            StringMap<std::string>::const_iterator it = variableTable.find(var);

            // Check for undefined variable references.
            if (it == variableTable.end()) {
               outstream << "uses undefined variable \"";
               outstream.writeEscaped(var) << "\"";
            } else {
               outstream << "with variable \"";
               outstream.writeEscaped(var) << "\" equal to \"";
               outstream.writeEscaped(it->m_second) << "\"";
            }
         }

         if (m_matchRange.isValid()) {
            sourceMgr.printMessage(m_matchRange.m_start, SourceMgr::DK_Note, outstream.getStr(),
            {m_matchRange});
         } else {
            sourceMgr.printMessage(SMLocation::getFromPointer(buffer.getData()),
                                   SourceMgr::DK_Note, outstream.getStr());
         }
      }
   }
}

static SMRange process_match_result(FileCheckDiag::MatchType matchType,
                                    const SourceMgr &sourceMgr, SMLocation loc,
                                    FileCheckType checkType,
                                    StringRef buffer, size_t pos, size_t len,
                                    std::vector<FileCheckDiag> *diags,
                                    bool adjustPrevDiag = false)
{
   SMLocation start = SMLocation::getFromPointer(buffer.getData() + pos);
   SMLocation end = SMLocation::getFromPointer(buffer.getData() + pos + len);
   SMRange range(start, end);
   if (diags) {
      if (adjustPrevDiag) {
         diags->rbegin()->m_matchType = matchType;
      } else {
         diags->emplace_back(sourceMgr, checkType, loc, matchType, range);
      }
   }
   return range;
}

void FileCheckPattern::printFuzzyMatch(
      const SourceMgr &sourceMgr, StringRef buffer,
      const StringMap<std::string> &variableTable,
      std::vector<FileCheckDiag> *diags) const
{
   // Attempt to find the closest/best fuzzy match.  Usually an error happens
   // because some string in the output didn't exactly match. In these cases, we
   // would like to show the user a best guess at what "should have" matched, to
   // save them having to actually check the input manually.
   size_t numLinesForward = 0;
   size_t best = StringRef::npos;
   double bestQuality = 0;

   // Use an arbitrary 4k limit on how far we will search.
   for (size_t i = 0, e = std::min(size_t(4096), buffer.size()); i != e; ++i) {
      if (buffer[i] == '\n')
         ++numLinesForward;

      // Patterns have leading whitespace stripped, so skip whitespace when
      // looking for something which looks like a pattern.
      if (buffer[i] == ' ' || buffer[i] == '\t')
         continue;

      // Compute the "quality" of this match as an arbitrary combination of the
      // match distance and the number of lines skipped to get to this match.
      unsigned distance = computeMatchDistance(buffer.substr(i), variableTable);
      double quality = distance + (numLinesForward / 100.);

      if (quality < bestQuality || best == StringRef::npos) {
         best = i;
         bestQuality = quality;
      }
   }

   // Print the "possible intended match here" line if we found something
   // reasonable and not equal to what we showed in the "scanning from here"
   // line.
   if (best && best != StringRef::npos && bestQuality < 50) {
      SMRange m_matchRange =
            process_match_result(FileCheckDiag::MatchFuzzy, sourceMgr, getLoc(),
                                 getCheckType(), buffer, best, 0, diags);
      sourceMgr.printMessage(m_matchRange.m_start, SourceMgr::DK_Note,
                             "possible intended match here");

      // FIXME: If we wanted to be really friendly we would show why the match
      // failed, as it can be hard to spot simple one character differences.
   }
}

/// Finds the closing sequence of a regex variable usage or definition.
///
/// \p str has to point in the beginning of the definition (right after the
/// opening sequence). Returns the offset of the closing sequence within str,
/// or npos if it was not found.
size_t FileCheckPattern::findRegexVarEnd(StringRef str, SourceMgr &sourceMgr)
{
   // offset keeps track of the current offset within the input str
   size_t offset = 0;
   // [...] Nesting depth
   size_t bracketDepth = 0;

   while (!str.empty()) {
      if (str.startsWith("]]") && bracketDepth == 0)
         return offset;
      if (str[0] == '\\') {
         // Backslash escapes the next char within regexes, so skip them both.
         str = str.substr(2);
         offset += 2;
      } else {
         switch (str[0]) {
         default:
            break;
         case '[':
            bracketDepth++;
            break;
         case ']':
            if (bracketDepth == 0) {
               sourceMgr.printMessage(SMLocation::getFromPointer(str.getData()),
                                      SourceMgr::DK_Error,
                                      "missing closing \"]\" for regex variable");
               exit(1);
            }
            bracketDepth--;
            break;
         }
         str = str.substr(1);
         offset++;
      }
   }
   return StringRef::npos;
}

/// Canonicalize whitespaces in the file. Line endings are replaced with
/// UNIX-style '\n'.
StringRef FileCheck::canonicalizeFile(MemoryBuffer &memoryBuffer,
                                      SmallVectorImpl<char> &outputBuffer)
{
   outputBuffer.reserve(memoryBuffer.getBufferSize());
   for (const char *ptr = memoryBuffer.getBufferStart(), *end = memoryBuffer.getBufferEnd();
        ptr != end; ++ptr) {
      // Eliminate trailing dosish \r.
      if (ptr <= end - 2 && ptr[0] == '\r' && ptr[1] == '\n') {
         continue;
      }

      // If current char is not a horizontal whitespace or if horizontal
      // whitespace canonicalization is disabled, dump it to output as is.
      if (m_req.noCanonicalizeWhiteSpace || (*ptr != ' ' && *ptr != '\t')) {
         outputBuffer.push_back(*ptr);
         continue;
      }

      // Otherwise, add one space and advance over neighboring space.
      outputBuffer.push_back(' ');
      while (ptr + 1 != end && (ptr[1] == ' ' || ptr[1] == '\t')) {
         ++ptr;
      }
   }

   // Add a null byte and then return all but that byte.
   outputBuffer.push_back('\0');
   return StringRef(outputBuffer.getData(), outputBuffer.size() - 1);
}

FileCheckDiag::FileCheckDiag(const SourceMgr &sourceMgr,
                             const FileCheckType &checkType,
                             SMLocation checkLoc, MatchType matchType,
                             SMRange inputRange)
   : m_checkType(checkType),
     m_matchType(matchType)
{
   auto start = sourceMgr.getLineAndColumn(inputRange.m_start);
   auto end = sourceMgr.getLineAndColumn(inputRange.m_end);
   m_inputStartLine = start.first;
   m_inputStartCol = start.second;
   m_inputEndLine = end.first;
   m_inputEndCol = end.second;
   start = sourceMgr.getLineAndColumn(checkLoc);
   m_checkLine = start.first;
   m_checkCol = start.second;
}

static bool is_part_of_word(char c)
{
   return (isalnum(c) || c == '-' || c == '_');
}

FileCheckType &FileCheckType::setCount(int count)
{
   assert(count > 0 && "zero and negative counts are not supported");
   assert((count == 1 || m_kind == CheckPlain) &&
          "count supported only for plain CHECK directives");
   m_count = count;
   return *this;
}

// Get a description of the type.
std::string FileCheckType::getDescription(StringRef prefix) const
{
   switch (m_kind) {
   case FileCheckKind::CheckNone:
      return "invalid";
   case FileCheckKind::CheckPlain:
      if (m_count > 1) {
         return prefix.getStr() + "-COUNT";
      }
      return prefix;
   case FileCheckKind::CheckNext:
      return prefix.getStr() + "-NEXT";
   case FileCheckKind::CheckSame:
      return prefix.getStr() + "-SAME";
   case FileCheckKind::CheckNot:
      return prefix.getStr() + "-NOT";
   case FileCheckKind::CheckDAG:
      return prefix.getStr() + "-DAG";
   case FileCheckKind::CheckLabel:
      return prefix.getStr() + "-LABEL";
   case FileCheckKind::CheckEmpty:
      return prefix.getStr() + "-EMPTY";
   case FileCheckKind::CheckEOF:
      return "implicit EOF";
   case FileCheckKind::CheckBadNot:
      return "bad NOT";
   case FileCheckKind::CheckBadCount:
      return "bad COUNT";
   }
   polar_unreachable("unknown FileCheckType");
}

static std::pair<FileCheckType, StringRef>
find_check_type(StringRef buffer, StringRef prefix)
{
   if (buffer.size() <= prefix.size()) {
      return {FileCheckKind::CheckNone, StringRef()};
   }
   char nextChar = buffer[prefix.size()];

   StringRef rest = buffer.dropFront(prefix.size() + 1);
   // Verify that the : is present after the prefix.
   if (nextChar == ':') {
      return {FileCheckKind::CheckPlain, rest};
   }
   if (nextChar != '-') {
      return {FileCheckKind::CheckNone, StringRef()};
   }

   if (rest.consumeFront("COUNT-")) {
      int64_t count;
      if (rest.consumeInteger(10, count)) {
         // Error happened in parsing integer.
         return {FileCheckKind::CheckBadCount, rest};
      }
      if (count <= 0 || count > INT32_MAX) {
         return {FileCheckKind::CheckBadCount, rest};
      }

      if (!rest.consumeFront(":")) {
         return {FileCheckKind::CheckBadCount, rest};
      }
      return {FileCheckType(FileCheckKind::CheckPlain).setCount(count), rest};
   }

   if (rest.consumeFront("NEXT:"))
      return {FileCheckKind::CheckNext, rest};

   if (rest.consumeFront("SAME:"))
      return {FileCheckKind::CheckSame, rest};

   if (rest.consumeFront("NOT:"))
      return {FileCheckKind::CheckNot, rest};

   if (rest.consumeFront("DAG:"))
      return {FileCheckKind::CheckDAG, rest};

   if (rest.consumeFront("LABEL:"))
      return {FileCheckKind::CheckLabel, rest};

   if (rest.consumeFront("EMPTY:"))
      return {FileCheckKind::CheckEmpty, rest};

   // You can't combine -NOT with another suffix.
   if (rest.startsWith("DAG-NOT:") || rest.startsWith("NOT-DAG:") ||
       rest.startsWith("NEXT-NOT:") || rest.startsWith("NOT-NEXT:") ||
       rest.startsWith("SAME-NOT:") || rest.startsWith("NOT-SAME:") ||
       rest.startsWith("EMPTY-NOT:") || rest.startsWith("NOT-EMPTY:")) {
      return {FileCheckKind::CheckBadNot, rest};
   }

   return {FileCheckKind::CheckNone, rest};
}

// From the given position, find the next character after the word.
static size_t skip_word(StringRef str, size_t loc)
{
   while (loc < str.size() && is_part_of_word(str[loc])) {
      ++loc;
   }
   return loc;
}

/// Search the buffer for the first prefix in the prefix regular expression.
///
/// This searches the buffer using the provided regular expression, however it
/// enforces constraints beyond that:
/// 1) The found prefix must not be a suffix of something that looks like
///    a valid prefix.
/// 2) The found prefix must be followed by a valid check type suffix using \c
///    find_check_type above.
///
/// Returns a pair of StringRefs into the buffer, which combines:
///   - the first match of the regular expression to satisfy these two is
///   returned,
///     otherwise an empty StringRef is returned to indicate failure.
///   - buffer rewound to the location right after parsed suffix, for parsing
///     to continue from
///
/// If this routine returns a valid prefix, it will also shrink \p buffer to
/// start at the beginning of the returned prefix, increment \p lineNumber for
/// each new line consumed from \p buffer, and set \p m_checkType to the type of
/// check found by examining the suffix.
///
/// If no valid prefix is found, the state of buffer, lineNumber, and checkType
/// is unspecified.

static std::pair<StringRef, StringRef>
find_first_matching_prefix(boost::regex &prefixRegex, StringRef &buffer,
                           unsigned &lineNumber,
                           FileCheckType &checkType)
{
   while (!buffer.empty()) {
      boost::cmatch matches;
      // Find the first (longest) match using the RE.
      if (!boost::regex_search(buffer.begin(), buffer.end(), matches, prefixRegex,
                               boost::match_posix)) {
         // No match at all, bail.
         return {StringRef(), StringRef()};
      }
      StringRef prefix(buffer.getData() + matches.position(), matches[0].length());
      assert(prefix.getData() >= buffer.getData() &&
             prefix.getData() < buffer.getData() + buffer.size() &&
             "Prefix doesn't start inside of buffer!");

      size_t loc = matches.position();
      StringRef skipped = buffer.substr(0, loc);
      buffer = buffer.dropFront(loc);
      lineNumber += skipped.count('\n');

      // Check that the matched prefix isn't a suffix of some other check-like
      // word.
      // FIXME: This is a very ad-hoc check. it would be better handled in some
      // other way. Among other things it seems hard to distinguish between
      // intentional and unintentional uses of this feature.
      if (skipped.empty() || !is_part_of_word(skipped.back())) {
         // Now extract the type.
         StringRef afterSuffix;
         std::tie(checkType, afterSuffix) = find_check_type(buffer, prefix);

         // If we've found a valid check type for this prefix, we're done.
         if (checkType != FileCheckKind::CheckNone) {
            return {prefix, afterSuffix};
         }
      }

      // If we didn't successfully find a prefix, we need to skip this invalid
      // prefix and continue scanning. We directly skip the prefix that was
      // matched and any additional parts of that check-like word.
      buffer = buffer.dropFront(skip_word(buffer, prefix.size()));
   }

   // We ran out of buffer while skipping partial matches so give up.
   return {StringRef(), StringRef()};
}

/// Read the check file, which specifies the sequence of expected strings.
///
/// The strings are added to the checkStrings vector. Returns true in case of
/// an error, false otherwise.
bool FileCheck::readCheckFile(SourceMgr &sourceMgr, StringRef buffer,
                              boost::regex &prefixRE,
                              std::vector<FileCheckString> &checkStrings)
{
   std::vector<FileCheckPattern> implicitNegativeChecks;
   for (const auto &PatternString : m_req.implicitCheckNot) {
      // Create a buffer with fake command line content in order to display the
      // command line option responsible for the specific implicit CHECK-NOT.
      std::string prefix = "-implicit-check-not='";
      std::string Suffix = "'";
      std::unique_ptr<MemoryBuffer> CmdLine = MemoryBuffer::getMemBufferCopy(
               prefix + PatternString + Suffix, "command line");

      StringRef patternInBuffer =
            CmdLine->getBuffer().substr(prefix.size(), PatternString.size());
      sourceMgr.addNewSourceBuffer(std::move(CmdLine), SMLocation());

      implicitNegativeChecks.push_back(FileCheckPattern(FileCheckKind::CheckNot));
      implicitNegativeChecks.back().parsePattern(patternInBuffer,
                                                 "IMPLICIT-CHECK", sourceMgr, 0, m_req);
   }

   std::vector<FileCheckPattern> dagNotMatches = implicitNegativeChecks;

   // lineNumber keeps track of the line on which checkPrefix instances are
   // found.
   unsigned lineNumber = 1;

   while (1) {
      FileCheckType checkType;

      // See if a prefix occurs in the memory buffer.
      StringRef usedPrefix;
      StringRef afterSuffix;
      std::tie(usedPrefix, afterSuffix) =
            find_first_matching_prefix(prefixRE, buffer, lineNumber, checkType);
      if (usedPrefix.empty())
         break;
      assert(usedPrefix.getData() == buffer.getData() &&
             "Failed to move buffer's start forward, or pointed prefix outside "
             "of the buffer!");
      assert(afterSuffix.getData() >= buffer.getData() &&
             afterSuffix.getData() < buffer.getData() + buffer.size() &&
             "Parsing after suffix doesn't start inside of buffer!");

      // Location to use for error messages.
      const char *usedPrefixStart = usedPrefix.getData();

      // Skip the buffer to the end of parsed suffix (or just prefix, if no good
      // suffix was processed).
      buffer = afterSuffix.empty() ? buffer.dropFront(usedPrefix.size())
                                   : afterSuffix;

      // Complain about useful-looking but unsupported suffixes.
      if (checkType == FileCheckKind::CheckBadNot) {
         sourceMgr.printMessage(SMLocation::getFromPointer(buffer.getData()), SourceMgr::DK_Error,
                                "unsupported -NOT combo on prefix '" + usedPrefix + "'");
         return true;
      }

      // Complain about invalid count specification.
      if (checkType == FileCheckKind::CheckBadCount) {
         sourceMgr.printMessage(SMLocation::getFromPointer(buffer.getData()), SourceMgr::DK_Error,
                                "invalid count in -COUNT specification on prefix '" +
                                usedPrefix + "'");
         return true;
      }

      // Okay, we found the prefix, yay. Remember the rest of the line, but ignore
      // leading whitespace.
      if (!(m_req.noCanonicalizeWhiteSpace && m_req.matchFullLines)) {
         buffer = buffer.substr(buffer.findFirstNotOf(" \t"));
      }

      // Scan ahead to the end of line.
      size_t eol = buffer.findFirstOf("\n\r");

      // Remember the location of the start of the pattern, for diagnostics.
      SMLocation patternLoc = SMLocation::getFromPointer(buffer.getData());

      // Parse the pattern.
      FileCheckPattern pattern(checkType);
      if (pattern.parsePattern(buffer.substr(0, eol), usedPrefix, sourceMgr, lineNumber, m_req)) {
         return true;
      }

      // Verify that CHECK-LABEL lines do not define or use variables
      if ((checkType == FileCheckKind::CheckLabel) && pattern.hasVariable()) {
         sourceMgr.printMessage(
                  SMLocation::getFromPointer(usedPrefixStart), SourceMgr::DK_Error,
                  "found '" + usedPrefix + "-LABEL:'"
                                           " with variable definition or use");
         return true;
      }

      buffer = buffer.substr(eol);

      // Verify that CHECK-NEXT/SAME/EMPTY lines have at least one CHECK line before them.
      if ((checkType == FileCheckKind::CheckNext || checkType == FileCheckKind::CheckSame ||
           checkType == FileCheckKind::CheckEmpty) &&
          checkStrings.empty()) {
         StringRef type = checkType == FileCheckKind::CheckNext
               ? "NEXT"
               : checkType == FileCheckKind::CheckEmpty ? "EMPTY" : "SAME";
         sourceMgr.printMessage(SMLocation::getFromPointer(usedPrefixStart),
                                SourceMgr::DK_Error,
                                "found '" + usedPrefix + "-" + type +
                                "' without previous '" + usedPrefix + ": line");
         return true;
      }

      // Handle CHECK-DAG/-NOT.
      if (checkType == FileCheckKind::CheckDAG || checkType == FileCheckKind::CheckNot) {
         dagNotMatches.push_back(pattern);
         continue;
      }

      // Okay, add the string we captured to the output vector and move on.
      checkStrings.emplace_back(pattern, usedPrefix, patternLoc);
      std::swap(dagNotMatches, checkStrings.back().m_dagNotStrings);
      dagNotMatches = implicitNegativeChecks;
   }

   // Add an EOF pattern for any trailing CHECK-DAG/-NOTs, and use the first
   // prefix as a filler for the error message.
   if (!dagNotMatches.empty()) {
      checkStrings.emplace_back(FileCheckPattern(FileCheckKind::CheckEOF), *m_req.checkPrefixes.begin(),
                                SMLocation::getFromPointer(buffer.getData()));
      std::swap(dagNotMatches, checkStrings.back().m_dagNotStrings);
   }

   if (checkStrings.empty()) {
      error_stream() << "error: no check strings found with prefix"
                     << (m_req.checkPrefixes.size() > 1 ? "es " : " ");
      auto iter = m_req.checkPrefixes.begin();
      auto end = m_req.checkPrefixes.end();
      if (iter != end) {
         error_stream() << "\'" << *iter << ":'";
         ++iter;
      }
      for (; iter != end; ++iter) {
         error_stream() << ", \'" << *iter << ":'";
      }
      error_stream() << '\n';
      return true;
   }

   return false;
}

static void print_match(bool expectedMatch, const SourceMgr &sourceMgr,
                        StringRef prefix, SMLocation loc, const FileCheckPattern &pattern,
                        int matchedCount, StringRef buffer,
                        StringMap<std::string> &variableTable, size_t matchPos,
                        size_t matchLen, const FileCheckRequest &req,
                        std::vector<FileCheckDiag> *diags)
{
   if (expectedMatch) {
      if (!req.verbose) {
         return;
      }

      if (!req.verboseVerbose && pattern.getCheckType() == FileCheckKind::CheckEOF) {
         return;
      }

   }
   SMRange matchRange = process_match_result(
            expectedMatch ? FileCheckDiag::MatchFoundAndExpected
                          : FileCheckDiag::MatchFoundButExcluded,
            sourceMgr, loc, pattern.getCheckType(), buffer, matchPos, matchLen, diags);
   std::string message = polar::utils::formatv("{0}: {1} string found in input",
                                               pattern.getCheckType().getDescription(prefix),
                                               (expectedMatch ? "expected" : "excluded"))
         .getStr();
   if (pattern.getCount() > 1) {
      message += polar::utils::formatv(" ({0} out of {1})", matchedCount, pattern.getCount()).getStr();
   }

   sourceMgr.printMessage(
            loc, expectedMatch ? SourceMgr::DK_Remark : SourceMgr::DK_Error, message);
   sourceMgr.printMessage(matchRange.m_start, SourceMgr::DK_Note, "found here",
   {matchRange});
   pattern.printVariableUses(sourceMgr, buffer, variableTable, matchRange);
}

static void print_match(bool expectedMatch, const SourceMgr &sourceMgr,
                        const FileCheckString &checkStr, int matchedCount,
                        StringRef buffer, StringMap<std::string> &variableTable,
                        size_t matchPos, size_t matchLen, FileCheckRequest &req,
                        std::vector<FileCheckDiag> *diags)
{
   print_match(expectedMatch, sourceMgr, checkStr.m_prefix, checkStr.m_loc, checkStr.m_pattern,
               matchedCount, buffer, variableTable, matchPos, matchLen, req,
               diags);
}

static void print_no_match(bool expectedMatch, const SourceMgr &sourceMgr,
                           StringRef prefix, SMLocation loc,
                           const FileCheckPattern &pattern, int matchedCount,
                           StringRef buffer, StringMap<std::string> &variableTable,
                           bool verboseVerbose,
                           std::vector<FileCheckDiag> *diags) {
   if (!expectedMatch && !verboseVerbose) {
      return;
   }

   // Otherwise, we have an error, emit an error message.
   std::string message = polar::utils::formatv("{0}: {1} string not found in input",
                                               pattern.getCheckType().getDescription(prefix),
                                               (expectedMatch ? "expected" : "excluded"))
         .getStr();
   if (pattern.getCount() > 1) {
      message += polar::utils::formatv(" ({0} out of {1})", matchedCount, pattern.getCount()).getStr();
   }
   sourceMgr.printMessage(
            loc, expectedMatch ? SourceMgr::DK_Error : SourceMgr::DK_Remark, message);

   // Print the "scanning from here" line.  If the current position is at the
   // end of a line, advance to the start of the next line.
   buffer = buffer.substr(buffer.findFirstNotOf(" \t\n\r"));
   SMRange searchRange = process_match_result(
            expectedMatch ? FileCheckDiag::MatchNoneButExpected
                          : FileCheckDiag::MatchNoneAndExcluded,
            sourceMgr, loc, pattern.getCheckType(), buffer, 0, buffer.size(), diags);
   sourceMgr.printMessage(searchRange.m_start, SourceMgr::DK_Note, "scanning from here");

   // Allow the pattern to print additional information if desired.
   pattern.printVariableUses(sourceMgr, buffer, variableTable);
   if (expectedMatch) {
      pattern.printFuzzyMatch(sourceMgr, buffer, variableTable, diags);
   }

}

static void print_no_match(bool expectedMatch, const SourceMgr &sourceMgr,
                           const FileCheckString &checkStr, int matchedCount,
                           StringRef buffer, StringMap<std::string> &variableTable,
                           bool verboseVerbose,
                           std::vector<FileCheckDiag> *diags)
{
   print_no_match(expectedMatch, sourceMgr, checkStr.m_prefix, checkStr.m_loc, checkStr.m_pattern,
                  matchedCount, buffer, variableTable, verboseVerbose, diags);
}

/// count the number of newlines in the specified range.
static unsigned count_num_newlines_between(StringRef range,
                                           const char *&firstNewLine)
{
   unsigned numNewLines = 0;
   while (1) {
      // Scan for newline.
      range = range.substr(range.findFirstOf("\n\r"));
      if (range.empty()) {
         return numNewLines;
      }

      ++numNewLines;

      // Handle \n\r and \r\n as a single newline.
      if (range.size() > 1 && (range[1] == '\n' || range[1] == '\r') &&
          (range[0] != range[1])) {
         range = range.substr(1);
      }
      range = range.substr(1);
      if (numNewLines == 1) {
         firstNewLine = range.begin();
      }
   }
}

/// Match check string and its "not strings" and/or "dag strings".
size_t FileCheckString::check(const SourceMgr &sourceMgr, StringRef buffer,
                              bool isLabelScanMode, size_t &matchLen,
                              StringMap<std::string> &variableTable,
                              FileCheckRequest &req,
                              std::vector<FileCheckDiag> *diags) const
{
   size_t lastPos = 0;
   std::vector<const FileCheckPattern *> notStrings;

   // isLabelScanMode is true when we are scanning forward to find CHECK-LABEL
   // bounds; we have not processed variable definitions within the bounded block
   // yet so cannot handle any final CHECK-DAG yet; this is handled when going
   // over the block again (including the last CHECK-LABEL) in normal mode.
   if (!isLabelScanMode) {
      // Match "dag strings" (with mixed "not strings" if any).
      lastPos = checkDag(sourceMgr, buffer, notStrings, variableTable, req, diags);
      if (lastPos == StringRef::npos) {
         return StringRef::npos;
      }
   }

   // Match itself from the last position after matching CHECK-DAG.
   size_t lastMatchEnd = lastPos;
   size_t firstMatchPos = 0;
   // Go match the pattern count times. Majority of patterns only match with
   // count 1 though.
   assert(m_pattern.getCount() != 0 && "pattern count can not be zero");
   for (int i = 1; i <= m_pattern.getCount(); i++) {
      StringRef matchBuffer = buffer.substr(lastMatchEnd);
      size_t currentMatchLen;
      // get a match at current start point
      size_t matchPos = m_pattern.match(matchBuffer, currentMatchLen, variableTable);
      if (i == 1) {
         firstMatchPos = lastPos + matchPos;
      }

      // report
      if (matchPos == StringRef::npos) {
         print_no_match(true, sourceMgr, *this, i, matchBuffer, variableTable,
                        req.verboseVerbose, diags);
         return StringRef::npos;
      }
      print_match(true, sourceMgr, *this, i, matchBuffer, variableTable, matchPos,
                  currentMatchLen, req, diags);

      // move start point after the match
      lastMatchEnd += matchPos + currentMatchLen;
   }
   // Full match len counts from first match pos.
   matchLen = lastMatchEnd - firstMatchPos;

   // Similar to the above, in "label-scan mode" we can't yet handle CHECK-NEXT
   // or CHECK-NOT
   if (!isLabelScanMode) {
      size_t matchPos = firstMatchPos - lastPos;
      StringRef matchBuffer = buffer.substr(lastPos);
      StringRef skippedRegion = buffer.substr(lastPos, matchPos);

      // If this check is a "CHECK-NEXT", verify that the previous match was on
      // the previous line (i.e. that there is one newline between them).
      if (checkNext(sourceMgr, skippedRegion)) {
         process_match_result(FileCheckDiag::MatchFoundButWrongLine, sourceMgr, m_loc,
                              m_pattern.getCheckType(), matchBuffer, matchPos, matchLen,
                              diags, req.verbose);
         return StringRef::npos;
      }

      // If this check is a "CHECK-SAME", verify that the previous match was on
      // the same line (i.e. that there is no newline between them).
      if (checkSame(sourceMgr, skippedRegion)) {
         process_match_result(FileCheckDiag::MatchFoundButWrongLine, sourceMgr, m_loc,
                              m_pattern.getCheckType(), matchBuffer, matchPos, matchLen,
                              diags, req.verbose);
         return StringRef::npos;
      }

      // If this match had "not strings", verify that they don't exist in the
      // skipped region.
      if (checkNot(sourceMgr, skippedRegion, notStrings, variableTable, req, diags)) {
         return StringRef::npos;
      }
   }

   return firstMatchPos;
}

/// Verify there is a single line in the given buffer.
bool FileCheckString::checkNext(const SourceMgr &sourceMgr, StringRef buffer) const
{
   if (m_pattern.getCheckType() != FileCheckKind::CheckNext &&
       m_pattern.getCheckType() != FileCheckKind::CheckEmpty) {
      return false;
   }
   Twine checkName =
         m_prefix +
         Twine(m_pattern.getCheckType() == FileCheckKind::CheckEmpty ? "-EMPTY" : "-NEXT");

   // count the number of newlines between the previous match and this one.
   assert(buffer.getData() !=
         sourceMgr.getMemoryBuffer(sourceMgr.findBufferContainingLoc(
                                      SMLocation::getFromPointer(buffer.getData())))
         ->getBufferStart() &&
         "CHECK-NEXT and CHECK-EMPTY can't be the first check in a file");

   const char *firstNewLine = nullptr;
   unsigned numNewLines = count_num_newlines_between(buffer, firstNewLine);

   if (numNewLines == 0) {
      sourceMgr.printMessage(m_loc, SourceMgr::DK_Error,
                             checkName + ": is on the same line as previous match");
      sourceMgr.printMessage(SMLocation::getFromPointer(buffer.end()), SourceMgr::DK_Note,
                             "'next' match was here");
      sourceMgr.printMessage(SMLocation::getFromPointer(buffer.getData()), SourceMgr::DK_Note,
                             "previous match ended here");
      return true;
   }

   if (numNewLines != 1) {
      sourceMgr.printMessage(m_loc, SourceMgr::DK_Error,
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
bool FileCheckString::checkSame(const SourceMgr &sourceMgr, StringRef buffer) const
{
   if (m_pattern.getCheckType() != FileCheckKind::CheckSame) {
      return false;
   }

   // count the number of newlines between the previous match and this one.
   assert(buffer.getData() !=
         sourceMgr.getMemoryBuffer(sourceMgr.findBufferContainingLoc(
                                      SMLocation::getFromPointer(buffer.getData())))
         ->getBufferStart() &&
         "CHECK-SAME can't be the first check in a file");

   const char *firstNewLine = nullptr;
   unsigned numNewLines = count_num_newlines_between(buffer, firstNewLine);

   if (numNewLines != 0) {
      sourceMgr.printMessage(m_loc, SourceMgr::DK_Error,
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
bool FileCheckString::checkNot(
      const SourceMgr &sourceMgr, StringRef buffer,
      const std::vector<const FileCheckPattern *> &notStrings,
      StringMap<std::string> &variableTable, const FileCheckRequest &req,
      std::vector<FileCheckDiag> *diags) const
{
   for (const FileCheckPattern *pattern : notStrings) {
      assert((pattern->getCheckType() == FileCheckKind::CheckNot) && "Expect CHECK-NOT!");
      size_t matchLen = 0;
      size_t pos = pattern->match(buffer, matchLen, variableTable);
      if (pos == StringRef::npos) {
         print_no_match(false, sourceMgr, m_prefix, pattern->getLoc(), *pattern, 1, buffer,
                        variableTable, req.verboseVerbose, diags);
         continue;
      }
      print_match(false, sourceMgr, m_prefix, pattern->getLoc(), *pattern, 1, buffer, variableTable,
                  pos, matchLen, req, diags);
      return true;
   }
   return false;
}

/// Match "dag strings" and their mixed "not strings".
size_t
FileCheckString::checkDag(const SourceMgr &sourceMgr, StringRef buffer,
                          std::vector<const FileCheckPattern *> &notStrings,
                          StringMap<std::string> &variableTable,
                          const FileCheckRequest &req,
                          std::vector<FileCheckDiag> *diags) const
{
   if (m_dagNotStrings.empty()) {
      return 0;
   }
   // The start of the search range.
   size_t startPos = 0;

   struct MatchRange {
      size_t pos;
      size_t end;
   };
   // A sorted list of ranges for non-overlapping CHECK-DAG matches.  Match
   // ranges are erased from this list once they are no longer in the search
   // range.
   std::list<MatchRange> matchRanges;
   // We need patternIter and patternEnd later for detecting the end of a CHECK-DAG
   // group, so we don't use a range-based for loop here.
   for (auto patternIter = m_dagNotStrings.begin(), patternEnd = m_dagNotStrings.end();
        patternIter != patternEnd; ++patternIter) {
      const FileCheckPattern &pattern = *patternIter;
      assert((pattern.getCheckType() == FileCheckKind::CheckDAG ||
              pattern.getCheckType() == FileCheckKind::CheckNot) &&
             "Invalid CHECK-DAG or CHECK-NOT!");

      if (pattern.getCheckType() == FileCheckKind::CheckNot) {
         notStrings.push_back(&pattern);
         continue;
      }

      assert((pattern.getCheckType() == FileCheckKind::CheckDAG) && "Expect CHECK-DAG!");
      // CHECK-DAG always matches from the start.
      size_t matchLen = 0;
      size_t matchPos = startPos;

      // Search for a match that doesn't overlap a previous match in this
      // CHECK-DAG group.
      for (auto miter = matchRanges.begin(), mend = matchRanges.end(); true; ++miter) {
         StringRef matchBuffer = buffer.substr(matchPos);
         size_t matchPosBuf = pattern.match(matchBuffer, matchLen, variableTable);
         // With a group of CHECK-DAGs, a single mismatching means the match on
         // that group of CHECK-DAGs fails immediately.
         if (matchPosBuf == StringRef::npos) {
            print_no_match(true, sourceMgr, m_prefix, pattern.getLoc(), pattern, 1, matchBuffer,
                           variableTable, req.verboseVerbose, diags);
            return StringRef::npos;
         }
         // Re-calc it as the offset relative to the start of the original string.
         matchPos += matchPosBuf;
         if (req.verboseVerbose) {
            print_match(true, sourceMgr, m_prefix, pattern.getLoc(), pattern, 1, buffer,
                        variableTable, matchPos, matchLen, req, diags);
         }

         MatchRange matchRange{matchPos, matchPos + matchLen};
         if (req.allowDeprecatedDagOverlap) {
            // We don't need to track all matches in this mode, so we just maintain
            // one match range that encompasses the current CHECK-DAG group's
            // matches.
            if (matchRanges.empty())
               matchRanges.insert(matchRanges.end(), matchRange);
            else {
               auto Block = matchRanges.begin();
               Block->pos = std::min(Block->pos, matchRange.pos);
               Block->end = std::max(Block->end, matchRange.end);
            }
            break;
         }

         // Iterate previous matches until overlapping match or insertion point.
         bool overlap = false;

         for (; miter != mend; ++miter) {
            if (matchRange.pos < miter->end) {
               // !overlap => New match has no overlap and is before this old match.
               // overlap => New match overlaps this old match.
               overlap = miter->pos < matchRange.end;
               break;
            }
         }

         if (!overlap) {
            // Insert non-overlapping match into list.
            matchRanges.insert(miter, matchRange);
            break;
         }

         if (req.verboseVerbose) {
            SMLocation oldStart = SMLocation::getFromPointer(buffer.getData() + miter->pos);
            SMLocation oldEnd = SMLocation::getFromPointer(buffer.getData() + miter->end);
            SMRange oldRange(oldStart, oldEnd);
            sourceMgr.printMessage(oldStart, SourceMgr::DK_Note,
                                   "match discarded, overlaps earlier DAG match here",
            {oldRange});
            if (diags) {
               diags->rbegin()->m_matchType = FileCheckDiag::MatchFoundButDiscarded;
            }
         }
         matchPos = miter->end;
      }
      if (!req.verboseVerbose) {
         print_match(true, sourceMgr, m_prefix, pattern.getLoc(), pattern, 1, buffer, variableTable,
                     matchPos, matchLen, req, diags);
      }

      // Handle the end of a CHECK-DAG group.
      if (std::next(patternIter) == patternEnd ||
          std::next(patternIter)->getCheckType() == FileCheckKind::CheckNot) {
         if (!notStrings.empty()) {
            // If there are CHECK-NOTs between two CHECK-DAGs or from CHECK to
            // CHECK-DAG, verify that there are no 'not' strings occurred in that
            // region.
            StringRef skippedRegion =
                  buffer.slice(startPos, matchRanges.begin()->pos);
            if (checkNot(sourceMgr, skippedRegion, notStrings, variableTable, req, diags)) {
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

// A check prefix must contain only alphanumeric, hyphens and underscores.
static bool validate_check_prefix(StringRef checkPrefix)
{
   return boost::regex_match(checkPrefix.getStr(), boost::regex("^[a-zA-Z0-9_-]*$"));
}

bool FileCheck::validateCheckPrefixes()
{
   StringSet<> prefixSet;

   for (StringRef prefix : m_req.checkPrefixes) {
      // Reject empty prefixes.
      if (prefix == "") {
         return false;
      }
      if (!prefixSet.insert(prefix).second) {
         return false;
      }
      if (!validate_check_prefix(prefix)) {
         return false;
      }
   }
   return true;
}

// Combines the check prefixes into a single regex so that we can efficiently
// scan for any of the set.
//
// The semantics are that the longest-match wins which matches our regex
// library.
boost::regex FileCheck::buildCheckPrefixRegex()
{
   // I don't think there's a way to specify an initial value for cl::list,
   // so if nothing was specified, add the default
   if (m_req.checkPrefixes.empty()) {
      m_req.checkPrefixes.push_back("CHECK");
   }

   // We already validated the contents of CheckPrefixes so just concatenate
   // them as alternatives.
   SmallString<32> prefixRegexStr;
   for (StringRef prefix : m_req.checkPrefixes) {
      if (prefix != m_req.checkPrefixes.front()) {
         prefixRegexStr.push_back('|');
      }
      prefixRegexStr.append(prefix);
   }
   return boost::regex(prefixRegexStr.getStr().getStr());
}

// Remove local variables from \p variableTable. Global variables
// (start with '$') are preserved.
static void clear_local_vars(StringMap<std::string> &variableTable)
{
   SmallVector<std::string, 16> localVars;
   for (const auto &var : variableTable) {
      if (var.getFirst()[0] != '$') {
         localVars.push_back(var.getFirst());
      }
   }
   for (const auto &var : localVars) {
      variableTable.erase(var);
   }
}

/// Check the input to FileCheck provided in the \p buffer against the \p
/// checkStrings read from the check file.
///
/// Returns false if the input fails to satisfy the checks.
bool FileCheck::checkInput(SourceMgr &sourceMgr, StringRef buffer,
                           ArrayRef<FileCheckString> checkStrings,
                           std::vector<FileCheckDiag> *diags)
{
   bool checksFailed = false;

   /// variableTable - This holds all the current filecheck variables.
   StringMap<std::string> variableTable;

   for (const auto& def : m_req.globalDefines) {
      variableTable.insert(StringRef(def).split('='));
   }
   unsigned i = 0, j = 0, e = checkStrings.getSize();
   while (true) {
      StringRef checkRegion;
      if (j == e) {
         checkRegion = buffer;
      } else {
         const FileCheckString &checkLabelStr = checkStrings[j];
         if (checkLabelStr.m_pattern.getCheckType() != FileCheckKind::CheckLabel) {
            ++j;
            continue;
         }

         // Scan to next CHECK-LABEL match, ignoring CHECK-NOT and CHECK-DAG
         size_t matchLabelLen = 0;
         size_t matchLabelPos = checkLabelStr.check(
                  sourceMgr, buffer, true, matchLabelLen, variableTable, m_req, diags);
         if (matchLabelPos == StringRef::npos) {
            // Immediately bail of CHECK-LABEL fails, nothing else we can do.
            return false;
         }
         checkRegion = buffer.substr(0, matchLabelPos + matchLabelLen);
         buffer = buffer.substr(matchLabelPos + matchLabelLen);
         ++j;
      }

      if (m_req.enableVarScope) {
         clear_local_vars(variableTable);
      }

      for (; i != j; ++i) {
         const FileCheckString &checkStr = checkStrings[i];
         // Check each string within the scanned region, including a second check
         // of any final CHECK-LABEL (to verify CHECK-NOT and CHECK-DAG)
         size_t matchLen = 0;
         size_t matchPos = checkStr.check(sourceMgr, checkRegion, false, matchLen,
                                          variableTable, m_req, diags);

         if (matchPos == StringRef::npos) {
            checksFailed = true;
            i = j;
            break;
         }

         checkRegion = checkRegion.substr(matchPos + matchLen);
      }
      if (j == e) {
         break;
      }
   }

   // Success if no checks failed.
   return !checksFailed;
}

} // filechecker
} // polar
