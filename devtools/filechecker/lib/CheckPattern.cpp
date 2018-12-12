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

#include "CheckPattern.h"
#include "polarphp/utils/StringUtils.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/utils/RawOutStream.h"
#include "CLI/CLI.hpp"
#include <boost/regex.hpp>

namespace polar {
namespace filechecker {

using namespace polar::utils;
using namespace polar::basic;

Pattern::Pattern(CheckType checkType)
   : m_checkType(checkType)
{
   CLI::App &parser = retrieve_command_parser();
   m_matchFullLines = parser.get_option("--match-full-lines")->count() > 0 ? true : false;
   m_noCanonicalizeWhiteSpace = parser.get_option("--strict-whitespace")->count() > 0 ? true : false;
}


/// Parses the given string into the Pattern.
///
/// \p Prefix provides which prefix is being matched, \p sourceMgr provides the
/// SourceMgr used for error reports, and \p m_lineNumber is the line number in
/// the input file from which the pattern string was read. Returns true in
/// case of an error, false otherwise.
bool Pattern::parsePattern(basic::StringRef patternStr, basic::StringRef prefix,
                           utils::SourceMgr &sourceMgr, unsigned lineNumber)
{
   bool matchFullLinesHere = m_matchFullLines && m_checkType != CheckType::CheckNot;
   this->m_lineNumber = lineNumber;
   m_patternLoc = SMLocation::getFromPointer(patternStr.getData());

   if (!(m_noCanonicalizeWhiteSpace && m_matchFullLines)) {
      // Ignore trailing whitespace.
      while (!patternStr.empty() &&
             (patternStr.back() == ' ' || patternStr.back() == '\t')) {
         patternStr = patternStr.substr(0, patternStr.size() - 1);
      }
   }

   // Check that there is something on the line.
   if (patternStr.empty() && m_checkType != CheckType::CheckEmpty) {
      sourceMgr.printMessage(m_patternLoc, SourceMgr::DK_Error,
                             "found empty check string with prefix '" + prefix + ":'");
      return true;
   }

   if (!patternStr.empty() && m_checkType == CheckType::CheckEmpty) {
      sourceMgr.printMessage(
               m_patternLoc, SourceMgr::DK_Error,
               "found non-empty check string for empty check with prefix '" + prefix +
               ":'");
      return true;
   }

   if (m_checkType == CheckType::CheckEmpty) {
      m_regExStr = "(\n$)";
      return false;
   }

   // Check to see if this is a fixed string, or if iter has regex pieces.
   if (!matchFullLinesHere &&
       (patternStr.size() < 2 || (patternStr.find("{{") == StringRef::npos &&
                                  patternStr.find("[[") == StringRef::npos))) {
      m_fixedStr = patternStr;
      return false;
   }

   if (matchFullLinesHere) {
      m_regExStr += '^';
      if (!m_noCanonicalizeWhiteSpace) {
         m_regExStr += " *";
      }
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
      // (or some other regex) and assigns iter to the filechecker variable 'foo'. The
      // second form is [[foo]] which is a reference to foo.  The variable name
      // itself must be of the form "[a-zA-Z_][0-9a-zA-Z_]*", otherwise we reject
      // iter.  This is to catch some common errors.
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

         // Verify that the name/expression is well formed. filechecker currently
         // supports @LINE, @LINE+number, @LINE-number expressions. The check here
         // is relaxed, more strict check is performed in \c evaluateExpression.
         bool isExpression = false;
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
                  isExpression = true;
                  continue;
               }
            }
            if (name[i] != '_' && !isalnum(name[i]) &&
                (!isExpression || (name[i] != '+' && name[i] != '-'))) {
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
               unsigned varParenNum = m_variableDefs[name];
               if (varParenNum < 1 || varParenNum > 9) {
                  sourceMgr.printMessage(SMLocation::getFromPointer(name.getData()),
                                         SourceMgr::DK_Error,
                                         "Can't back-reference more than 9 variables");
                  return true;
               }
               addBackrefToRegEx(varParenNum);
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
      if (!m_noCanonicalizeWhiteSpace) {
         m_regExStr += " *";
      }
      m_regExStr += '$';
   }

   return false;
}

bool Pattern::addRegExToRegEx(StringRef regexStr, unsigned &curParen, SourceMgr &sourceMgr)
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

void Pattern::addBackrefToRegEx(unsigned backrefNum)
{
   assert(backrefNum >= 1 && backrefNum <= 9 && "Invalid backref number");
   std::string backref = std::string("\\") + std::string(1, '0' + backrefNum);
   m_regExStr += backref;
}

/// Evaluates expression and stores the result to \p value.
///
/// Returns true on success and false when the expression has invalid syntax.
bool Pattern::evaluateExpression(basic::StringRef expr, std::string &value) const
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
size_t Pattern::match(StringRef buffer, size_t &matchLen,
                      StringMap<std::string> &variableTable) const
{
   // If this is the EOF pattern, match iter immediately.
   if (m_checkType == CheckType::CheckEOF) {
      matchLen = 0;
      return buffer.size();
   }

   // If this is a fixed string pattern, just match iter now.
   if (!m_fixedStr.empty()) {
      matchLen = m_fixedStr.size();
      return buffer.find(m_fixedStr);
   }

   // Regex match.

   // If there are variable uses, we need to create a temporary string with the
   // actual value.
   StringRef regExToMatch = m_regExStr;
   std::string m_tmpStr;
   if (!m_variableUses.empty()) {
      m_tmpStr = m_regExStr;
      unsigned insertOffset = 0;
      for (const auto &m_variableUse : m_variableUses) {
         std::string value;
         if (m_variableUse.first[0] == '@') {
            if (!evaluateExpression(m_variableUse.first, value))
               return StringRef::npos;
         } else {
            StringMap<std::string>::iterator iter =
                  variableTable.find(m_variableUse.first);
            // If the variable is undefined, return an error.
            if (iter == variableTable.end())
               return StringRef::npos;

            // Look up the value and escape iter so that we can put iter into the regex.
            value += regex_escape(iter->m_second);
         }

         // Plop iter into the regex at the adjusted offset.
         m_tmpStr.insert(m_tmpStr.begin() + m_variableUse.second + insertOffset,
                         value.begin(), value.end());
         insertOffset += value.size();
      }

      // Match the newly constructed regex.
      regExToMatch = m_tmpStr;
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
   size_t matchStartSkip = m_checkType == CheckType::CheckEmpty;
   matchLen = fullMatch.size() - matchStartSkip;
   return fullMatch.getData() - buffer.getData() + matchStartSkip;
}

/// Computes an arbitrary estimate for the quality of matching this pattern at
/// the start of \p buffer; a distance of zero should correspond to a perfect
/// match.
unsigned Pattern::computeMatchDistance(StringRef buffer, const StringMap<std::string> &variableTable) const
{
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

void Pattern::printVariableUses(const SourceMgr &sourceMgr, StringRef buffer,
                                const StringMap<std::string> &variableTable, SMRange matchRange) const
{
   // If this was a regular expression using variables, print the current
   // variable values.
   if (!m_variableUses.empty()) {
      for (const auto &variableUse : m_variableUses) {
         SmallString<256> msg;
         RawSvectorOutStream out(msg);
         StringRef var = variableUse.first;
         if (var[0] == '@') {
            std::string value;
            if (evaluateExpression(var, value)) {
               out << "with expression \"";
               out.writeEscaped(var) << "\" equal to \"";
               out.writeEscaped(value) << "\"";
            } else {
               out << "uses incorrect expression \"";
               out.writeEscaped(var) << "\"";
            }
         } else {
            StringMap<std::string>::const_iterator iter = variableTable.find(var);
            // Check for undefined variable references.
            if (iter == variableTable.end()) {
               out << "uses undefined variable \"";
               out.writeEscaped(var) << "\"";
            } else {
               out << "with variable \"";
               out.writeEscaped(var) << "\" equal to \"";
               out.writeEscaped(iter->m_second) << "\"";
            }
         }

         if (matchRange.isValid()) {
            sourceMgr.printMessage(matchRange.m_start, SourceMgr::DK_Note, out.getStr(),
            {matchRange});
         } else {
            sourceMgr.printMessage(SMLocation::getFromPointer(buffer.getData()),
                                   SourceMgr::DK_Note, out.getStr());
         }
      }
   }
}

void Pattern::printFuzzyMatch(
      const SourceMgr &sourceMgr, StringRef buffer,
      const StringMap<std::string> &variableTable) const {
   // Attempt to find the closest/best fuzzy match.  Usually an error happens
   // because some string in the output didn't exactly match. In these cases, we
   // would like to show the user a best guess at what "should have" matched, to
   // save them having to actually check the input manually.
   size_t numLinesForward = 0;
   size_t best = StringRef::npos;
   double bestQuality = 0;

   // Use an arbitrary 4k limit on how far we will search.
   for (size_t i = 0, e = std::min(size_t(4096), buffer.size()); i != e; ++i) {
      if (buffer[i] == '\n') {
         ++numLinesForward;
      }
      // Patterns have leading whitespace stripped, so skip whitespace when
      // looking for something which looks like a pattern.
      if (buffer[i] == ' ' || buffer[i] == '\t') {
         continue;
      }
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
      sourceMgr.printMessage(SMLocation::getFromPointer(buffer.getData() + best),
                      SourceMgr::DK_Note, "possible intended match here");

      // FIXME: If we wanted to be really friendly we would show why the match
      // failed, as it can be hard to spot simple one character differences.
   }
}

/// Finds the closing sequence of a regex variable usage or definition.
///
/// \p str has to point in the beginning of the definition (right after the
/// opening sequence). Returns the offset of the closing sequence within str,
/// or npos if it was not found.
size_t Pattern::findRegexVarEnd(StringRef str, SourceMgr &sourceMgr)
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

} // filechecker
} // polar
