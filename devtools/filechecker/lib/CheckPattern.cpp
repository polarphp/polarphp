// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/25.

#include "CheckPattern.h"
#include "polarphp/utils/StringUtils.h"
#include "CLI/CLI.hpp"

namespace polar {
namespace filechecker {

using namespace polar::utils;

Pattern::Pattern(CheckType checkType)
   : m_checkType(checkType)
{
   CLI::App &parser = retrieve_command_parser();
   m_matchFullLines = parser.get_option("--match-full-lines")->count() > 0 ? true : false;
   m_noCanonicalizeWhiteSpace = parser.get_option("--strict-whitespace")->count() > 0 ? true : false;
}


/// Parses the given string into the Pattern.
///
/// \p Prefix provides which prefix is being matched, \p SM provides the
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

   // Check to see if this is a fixed string, or if it has regex pieces.
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
      size_t m_fixedMatchEnd = patternStr.find("{{");
      m_fixedMatchEnd = std::min(m_fixedMatchEnd, patternStr.find("[["));
      m_regExStr += regex_escape(patternStr.substr(0, m_fixedMatchEnd));
      patternStr = patternStr.substr(m_fixedMatchEnd);
   }

   if (matchFullLinesHere) {
      if (!m_noCanonicalizeWhiteSpace) {
         m_regExStr += " *";
      }
      m_regExStr += '$';
   }

   return false;
}

} // filechecker
} // polar
