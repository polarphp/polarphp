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

#include "Global.h"
#include "CheckFuncs.h"
#include "CheckPattern.h"
#include "CheckString.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/basic/adt/StringSet.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/Debug.h"
#include "CLI/CLI.hpp"
#include <boost/regex.hpp>

namespace polar {
namespace filechecker {

using polar::basic::Twine;
using polar::basic::StringSet;
using polar::basic::SmallString;
using polar::basic::SmallVector;
using polar::utils::MemoryBuffer;

/// Canonicalize whitespaces in the file. Line endings are replaced with
/// UNIX-style '\n'.
StringRef canonicalize_file(MemoryBuffer &memorybuffer,
                            SmallVectorImpl<char> &outputbuffer)
{
   CLI::App &parser = retrieve_command_parser();
   bool noCanonicalizeWhiteSpace = parser.get_option("--strict-whitespace")->count() > 0 ? true : false;
   outputbuffer.reserve(memorybuffer.getBufferSize());
   for (const char *ptr = memorybuffer.getBufferStart(), *end = memorybuffer.getBufferEnd();
        ptr != end; ++ptr) {
      // Eliminate trailing dosish \r.
      if (ptr <= end - 2 && ptr[0] == '\r' && ptr[1] == '\n') {
         continue;
      }
      // If current char is not a horizontal whitespace or if horizontal
      // whitespace canonicalization is disabled, dump it to output as is.
      if (noCanonicalizeWhiteSpace || (*ptr != ' ' && *ptr != '\t')) {
         outputbuffer.push_back(*ptr);
         continue;
      }
      // Otherwise, add one space and advance over neighboring space.
      outputbuffer.push_back(' ');
      while (ptr + 1 != end && (ptr[1] == ' ' || ptr[1] == '\t')) {
         ++ptr;
      }
   }
   // Add a null byte and then return all but that byte.
   outputbuffer.push_back('\0');
   return StringRef(outputbuffer.getData(), outputbuffer.size() - 1);
}

bool is_part_of_word(char c)
{
   return (isalnum(c) || c == '-' || c == '_');
}

// Get the size of the prefix extension.
size_t check_type_size(CheckType checkType)
{
   switch (checkType) {
   case CheckType::CheckNone:
   case CheckType::CheckBadNot:
      return 0;

   case CheckType::CheckPlain:
      return sizeof(":") - 1;

   case CheckType::CheckNext:
      return sizeof("-NEXT:") - 1;

   case CheckType::CheckSame:
      return sizeof("-SAME:") - 1;

   case CheckType::CheckNot:
      return sizeof("-NOT:") - 1;

   case CheckType::CheckDAG:
      return sizeof("-DAG:") - 1;

   case CheckType::CheckLabel:
      return sizeof("-LABEL:") - 1;

   case CheckType::CheckEmpty:
      return sizeof("-EMPTY:") - 1;

   case CheckType::CheckEOF:
      polar_unreachable("Should not be using EOF size");
   }
   polar_unreachable("Bad check type");
}

// Get a description of the type.
std::string check_type_name(StringRef prefix, CheckType checkType)
{
   switch (checkType) {
   case CheckType::CheckNone:
      return "invalid";
   case CheckType::CheckPlain:
      return prefix;
   case CheckType::CheckNext:
      return prefix.getStr() + "-NEXT";
   case CheckType::CheckSame:
      return prefix.getStr() + "-SAME";
   case CheckType::CheckNot:
      return prefix.getStr() + "-NOT";
   case CheckType::CheckDAG:
      return prefix.getStr() + "-DAG";
   case CheckType::CheckLabel:
      return prefix.getStr() + "-LABEL";
   case CheckType::CheckEmpty:
      return prefix.getStr() + "-EMPTY";
   case CheckType::CheckEOF:
      return "implicit EOF";
   case CheckType::CheckBadNot:
      return "bad NOT";
   }
   polar_unreachable("unknown CheckType");
}

CheckType find_check_type(StringRef buffer, StringRef prefix)
{
   if (buffer.size() <= prefix.size()) {
      return CheckType::CheckNone;
   }
   char nextChar = buffer[prefix.size()];
   // Verify that the : is present after the prefix.
   if (nextChar == ':') {
      return CheckType::CheckPlain;
   }
   if (nextChar != '-') {
      return CheckType::CheckNone;
   }
   StringRef rest = buffer.dropFront(prefix.size() + 1);
   if (rest.startsWith("NEXT:")) {
      return CheckType::CheckNext;
   }
   if (rest.startsWith("SAME:")) {
      return CheckType::CheckSame;
   }
   if (rest.startsWith("NOT:")) {
      return CheckType::CheckNot;
   }
   if (rest.startsWith("DAG:")) {
      return CheckType::CheckDAG;
   }
   if (rest.startsWith("LABEL:")) {
      return CheckType::CheckLabel;
   }
   if (rest.startsWith("EMPTY:")) {
      return CheckType::CheckEmpty;
   }
   // You can't combine -NOT with another suffix.
   if (rest.startsWith("DAG-NOT:") || rest.startsWith("NOT-DAG:") ||
       rest.startsWith("NEXT-NOT:") || rest.startsWith("NOT-NEXT:") ||
       rest.startsWith("SAME-NOT:") || rest.startsWith("NOT-SAME:") ||
       rest.startsWith("EMPTY-NOT:") || rest.startsWith("NOT-EMPTY:")) {
      return CheckType::CheckBadNot;
   }
   return CheckType::CheckNone;
}

// From the given position, find the next character after the word.
size_t skip_word(StringRef str)
{
   size_t i = 0;
   for (;i < str.size() && is_part_of_word(str[i]); ++i);
   return i;
}

/// Search the buffer for the first prefix in the prefix regular expression.
///
/// This searches the buffer using the provided regular expression, however it
/// enforces constraints beyond that:
/// 1) The found prefix must not be a suffix of something that looks like
///    a valid prefix.
/// 2) The found prefix must be followed by a valid check type suffix using \c
///    FindCheckTypepe above.
///
/// The first match of the regular expression to satisfy these two is returned,
/// otherwise an empty StringRef is returned to indicate failure.
///
/// If this routine returns a valid prefix, it will also shrink \p buffer to
/// start at the beginning of the returned prefix, increment \p lineNumber for
/// each new line consumed from \p buffer, and set \p CheckType to the type of
/// check found by examining the suffix.
///
/// If no valid prefix is found, the state of buffer, lineNumber, and CheckType
/// is unspecified.
StringRef find_first_matching_prefix(boost::regex &prefixRegex, StringRef &buffer,
                                     unsigned &lineNumber,
                                     CheckType &checkType)
{
   while (!buffer.empty()) {
      boost::cmatch matches;
      // Find the first (longest) match using the RE.
      if (!boost::regex_search(buffer.begin(), buffer.end(), matches, prefixRegex,
                               boost::match_posix)) {
         // No match at all, bail.
         return StringRef();
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
         checkType = find_check_type(buffer, prefix);
         // If we've found a valid check type for this prefix, we're done.
         if (checkType != CheckType::CheckNone) {
            return prefix;
         }
      }

      // If we didn't successfully find a prefix, we need to skip this invalid
      // prefix and continue scanning. We directly skip the prefix that was
      // matched and any additional parts of that check-like word.
      buffer = buffer.dropFront(skip_word(buffer));
   }

   // We ran out of buffer while skipping partial matches so give up.
   return StringRef();
}

/// Read the check file, which specifies the sequence of expected strings.
///
/// The strings are added to the checkStrings vector. Returns true in case of
/// an error, false otherwise.
bool read_check_file(SourceMgr &sourceMgr, StringRef buffer, boost::regex &prefixRegex,
                     std::vector<CheckString> &checkStrings)
{
   CLI::App &parser = retrieve_command_parser();
   std::string cmdName = "--implicit-check-not";
   bool noCanonicalizeWhiteSpace = parser.get_option("--strict-whitespace")->count() > 0 ? true : false;
   bool matchFullLines = parser.get_option("--match-full-lines")->count() > 0 ? true : false;
   std::vector<Pattern> implicitNegativeChecks;
   for (const auto &patternstring : sg_implicitCheckNot) {
      // Create a buffer with fake command line content in order to display the
      // command line option responsible for the specific implicit CHECK-NOT.
      std::string prefix = Twine(cmdName, " '").getStr();
      std::string suffix = "'";
      std::unique_ptr<MemoryBuffer> cmdLine = MemoryBuffer::getMemBufferCopy(
               prefix + patternstring + suffix, "command line");

      StringRef patternInbuffer =
            cmdLine->getBuffer().substr(prefix.size(), patternstring.size());
      sourceMgr.addNewSourceBuffer(std::move(cmdLine), SMLocation());

      implicitNegativeChecks.push_back(Pattern(CheckType::CheckNot));
      implicitNegativeChecks.back().parsePattern(patternInbuffer,
                                                 "IMPLICIT-CHECK", sourceMgr, 0);
   }

   std::vector<Pattern> dagNotMatches = implicitNegativeChecks;

   // lineNumber keeps track of the line on which Checkprefix instances are
   // found.
   unsigned lineNumber = 1;
   while (true) {
      CheckType checkType;
      // See if a prefix occurs in the memory buffer.
      StringRef usedPrefix = find_first_matching_prefix(prefixRegex, buffer, lineNumber,
                                                        checkType);
      if (usedPrefix.empty()) {
         break;
      }
      assert(usedPrefix.getData() == buffer.getData() &&
             "Failed to move buffer's start forward, or pointed prefix outside "
             "of the buffer!");

      // location to use for error messages.
      const char *usedprefixStart = usedPrefix.getData();

      // Skip the buffer to the end.
      buffer = buffer.dropFront(usedPrefix.size() + check_type_size(checkType));

      // Complain about useful-looking but unsupported suffixes.
      if (checkType == CheckType::CheckBadNot) {
         sourceMgr.printMessage(SMLocation::getFromPointer(buffer.getData()), SourceMgr::DK_Error,
                                "unsupported -NOT combo on prefix '" + usedPrefix + "'");
         return true;
      }

      // Okay, we found the prefix, yay. Remember the rest of the line, but ignore
      // leading whitespace.
      if (!(noCanonicalizeWhiteSpace && matchFullLines)) {
         buffer = buffer.substr(buffer.findFirstNotOf(" \t"));
      }
      // Scan ahead to the end of line.
      size_t eol = buffer.findFirstOf("\n\r");

      // Remember the location of the start of the pattern, for diagnostics.
      SMLocation patternloc = SMLocation::getFromPointer(buffer.getData());

      // Parse the pattern.
      Pattern pattern(checkType);
      if (pattern.parsePattern(buffer.substr(0, eol), usedPrefix, sourceMgr, lineNumber)) {
         return true;
      }
      // Verify that CHECK-LABEL lines do not define or use variables
      if ((checkType == CheckType::CheckLabel) && pattern.hasVariable()) {
         sourceMgr.printMessage(
                  SMLocation::getFromPointer(usedprefixStart), SourceMgr::DK_Error,
                  "found '" + usedPrefix + "-LABEL:'"
                                           " with variable definition or use");
         return true;
      }

      buffer = buffer.substr(eol);

      // Verify that CHECK-NEXT/SAME/EMPTY lines have at least one CHECK line before them.
      if ((checkType == CheckType::CheckNext || checkType == CheckType::CheckSame ||
           checkType == CheckType::CheckEmpty) &&
          checkStrings.empty()) {
         StringRef type = checkType == CheckType::CheckNext
               ? "NEXT"
               : checkType == CheckType::CheckEmpty ? "EMPTY" : "SAME";
         sourceMgr.printMessage(SMLocation::getFromPointer(usedprefixStart),
                                SourceMgr::DK_Error,
                                "found '" + usedPrefix + "-" + type +
                                "' without previous '" + usedPrefix + ": line");
         return true;
      }

      // Handle CHECK-DAG/-NOT.
      if (checkType == CheckType::CheckDAG || checkType == CheckType::CheckNot) {
         dagNotMatches.push_back(pattern);
         continue;
      }

      // Okay, add the string we captured to the output vector and move on.
      checkStrings.emplace_back(pattern, usedPrefix, patternloc);
      std::swap(dagNotMatches, checkStrings.back().m_dagNotStrings);
      dagNotMatches = implicitNegativeChecks;
   }

   // Add an EOF pattern for any trailing CHECK-DAG/-NOTs, and use the first
   // prefix as a filler for the error message.
   if (!dagNotMatches.empty()) {
      checkStrings.emplace_back(Pattern(CheckType::CheckEOF), *sg_checkPrefixes.begin(),
                                SMLocation::getFromPointer(buffer.getData()));
      std::swap(dagNotMatches, checkStrings.back().m_dagNotStrings);
   }
   if (checkStrings.empty()) {
      polar::utils::error_stream() << "error: no check strings found with prefix"
                                   << (sg_checkPrefixes.size() > 1 ? "es " : " ");
      auto iter = sg_checkPrefixes.begin();
      auto endMark = sg_checkPrefixes.end();
      if (iter != endMark) {
         polar::utils::error_stream() << "\'" << *iter << ":'";
         ++iter;
      }
      for (; iter != endMark; ++iter) {
         polar::utils::error_stream() << ", \'" << *iter << ":'";
      }

      polar::utils::error_stream() << '\n';
      return true;
   }

   return false;
}

void print_match(bool expectedMatch, const SourceMgr &sourceMgr,
                 StringRef prefix, SMLocation loc, const Pattern &pattern,
                 StringRef buffer, StringMap<std::string> &variableTable,
                 size_t matchPos, size_t matchLen)
{
   CLI::App &parser = retrieve_command_parser();
   bool verbose = parser.get_option("-v")->count() >= 1 ? true : false;
   bool verboseVerbose = parser.get_option("-v")->count() > 1 ? true : false;
   if (expectedMatch) {
      if (!verbose) {
         return;
      }
      if (!verboseVerbose && pattern.getCheckType() == CheckType::CheckEOF) {
         return;
      }
   }
   SMLocation matchStart = SMLocation::getFromPointer(buffer.getData() + matchPos);
   SMLocation matchEnd = SMLocation::getFromPointer(buffer.getData() + matchPos + matchLen);
   SMRange matchRange(matchStart, matchEnd);
   sourceMgr.printMessage(
            loc, expectedMatch ? SourceMgr::DK_Remark : SourceMgr::DK_Error,
            check_type_name(prefix, pattern.getCheckType()) + ": " +
            (expectedMatch ? "expected" : "excluded") +
            " string found in input");
   sourceMgr.printMessage(matchStart, SourceMgr::DK_Note, "found here", {matchRange});
   pattern.printVariableUses(sourceMgr, buffer, variableTable, matchRange);
}

void print_match(bool expectedMatch, const SourceMgr &sourceMgr,
                 const CheckString &Checkstr, StringRef buffer,
                 StringMap<std::string> &variableTable, size_t matchPos,
                 size_t matchLen)
{
   print_match(expectedMatch, sourceMgr, Checkstr.m_prefix, Checkstr.m_location, Checkstr.m_pattern,
               buffer, variableTable, matchPos, matchLen);
}

void print_no_match(bool expectedMatch, const SourceMgr &sourceMgr,
                    StringRef prefix, SMLocation loc, const Pattern &pattern,
                    StringRef buffer,
                    StringMap<std::string> &variableTable)
{
   CLI::App &parser = retrieve_command_parser();
   bool verboseVerbose = parser.get_option("-v")->count() > 1 ? true : false;
   if (!expectedMatch && !verboseVerbose) {
      return;
   }

   // Otherwise, we have an error, emit an error message.
   sourceMgr.printMessage(loc,
                          expectedMatch ? SourceMgr::DK_Error : SourceMgr::DK_Remark,
                          check_type_name(prefix, pattern.getCheckType()) + ": " +
                          (expectedMatch ? "expected" : "excluded") +
                          " string not found in input");

   // Print the "scanning from here" line.  If the current position is at the
   // end of a line, advance to the start of the next line.
   buffer = buffer.substr(buffer.findFirstNotOf(" \t\n\r"));

   sourceMgr.printMessage(SMLocation::getFromPointer(buffer.getData()), SourceMgr::DK_Note,
                          "scanning from here");

   // Allow the pattern to print additional information if desired.
   pattern.printVariableUses(sourceMgr, buffer, variableTable);
   if (expectedMatch) {
      pattern.printFuzzyMatch(sourceMgr, buffer, variableTable);
   }
}

void print_no_match(bool expectedMatch, const SourceMgr &sourceMgr,
                    const CheckString &Checkstr, StringRef buffer,
                    StringMap<std::string> &variableTable)
{
   print_no_match(expectedMatch, sourceMgr, Checkstr.m_prefix, Checkstr.m_location, Checkstr.m_pattern,
                  buffer, variableTable);
}

/// Count the number of newlines in the specified range.
unsigned count_num_newlines_between(StringRef range,
                                    const char *&firstNewLine)
{
   unsigned numNewLines = 0;
   while (true) {
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

// A check prefix must contain only alphanumeric, hyphens and underscores.
bool validate_check_prefix(StringRef checkPrefix)
{
   return boost::regex_match(checkPrefix.getStr(), boost::regex("^[a-zA-Z0-9_-]*$"));
}

bool validate_check_prefixes()
{
   StringSet<> prefixSet;

   for (StringRef prefix : sg_checkPrefixes) {
      // Reject empty prefixes.
      if (prefix.trim() == "") {
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
bool build_check_prefix_regex(boost::regex &regex, std::string &errorMsg)
{
   // I don't think there's a way to specify an initial value for cl::list,
   // so if nothing was specified, add the default
   if (sg_checkPrefixes.empty()) {
      sg_checkPrefixes.push_back("CHECK");
   }
   // We already validated the contents of checkPrefixes so just concatenate
   // them as alternatives.
   SmallString<32> prefixRegexStr;
   for (StringRef prefix : sg_checkPrefixes) {
      if (prefix != sg_checkPrefixes.front()) {
         prefixRegexStr.push_back('|');
      }
      prefixRegexStr.append(prefix);
   }
   try {
      regex = boost::regex(prefixRegexStr.getStr().getStr());
      return true;
   } catch (std::exception &e) {
      errorMsg = e.what();
      return false;
   }
}

void dump_command_line(int argc, char **argv)
{
   polar::utils::error_stream() << "filechecker command line: ";
   for (int index = 0; index < argc; index++) {
      polar::utils::error_stream() << " " << argv[index];
   }
   polar::utils::error_stream() << "\n";
}

// Remove local variables from \p variableTable. Global variables
// (start with '$') are preserved.
void clear_local_vars(StringMap<std::string> &variableTable)
{
   SmallVector<StringRef, 16> localVars;
   for (const auto &var : variableTable) {
      if (var.getFirst()[0] != '$') {
         localVars.push_back(var.getFirst());
      }
   }
   for (const auto &var : localVars) {
      variableTable.erase(var);
   }
}

/// Check the input to filechecker provided in the \p buffer against the \p
/// checkStrings read from the check file.
///
/// Returns false if the input fails to satisfy the checks.
bool check_input(SourceMgr &sourceMgr, StringRef buffer,
                 ArrayRef<CheckString> checkStrings)
{
   CLI::App &parser = retrieve_command_parser();
   bool enableVarScope = parser.get_option("--enable-var-scope")->count() > 0 ? true : false;
   bool checksFailed = false;
   /// variableTable - This holds all the current filecheck variables.
   StringMap<std::string> variableTable;
   for (const auto& def : sg_defines) {
      variableTable.insert(StringRef(def).split('='));
   }
   unsigned i = 0, j = 0, e = checkStrings.getSize();
   while (true) {
      StringRef checkRegion;
      if (j == e) {
         checkRegion = buffer;
      } else {
         const CheckString &checkLabelStr = checkStrings[j];
         if (checkLabelStr.m_pattern.getCheckType() != CheckType::CheckLabel) {
            ++j;
            continue;
         }

         // Scan to next CHECK-LABEL match, ignoring CHECK-NOT and CHECK-DAG
         size_t matchLabelLen = 0;
         size_t matchLabelPos =
               checkLabelStr.check(sourceMgr, buffer, true, matchLabelLen, variableTable);
         if (matchLabelPos == StringRef::npos) {
            // Immediately bail of CHECK-LABEL fails, nothing else we can do.
            return false;
         }
         checkRegion = buffer.substr(0, matchLabelPos + matchLabelLen);
         buffer = buffer.substr(matchLabelPos + matchLabelLen);
         ++j;
      }
      if (enableVarScope) {
         clear_local_vars(variableTable);
      }
      for (; i != j; ++i) {
         const CheckString &checkStr = checkStrings[i];
         // Check each string within the scanned region, including a second check
         // of any final CHECK-LABEL (to verify CHECK-NOT and CHECK-DAG)
         size_t matchLen = 0;
         size_t matchPos =
               checkStr.check(sourceMgr, checkRegion, false, matchLen, variableTable);

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
