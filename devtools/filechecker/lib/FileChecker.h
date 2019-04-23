//==-- llvm/Support/FileCheck.h ---------------------------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
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

#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/SourceMgr.h"
#include "FileCheckerConfig.h"
#include <boost/regex.hpp>
#include <vector>
#include <map>

namespace polar {
namespace filechecker {

using polar::basic::StringRef;
using polar::basic::StringMap;
using polar::basic::SmallVectorImpl;
using polar::basic::ArrayRef;
using polar::utils::SourceMgr;
using polar::utils::SMLocation;
using polar::utils::SMRange;
using polar::utils::MemoryBuffer;

/// Contains info about various FileCheck options.
struct FileCheckRequest
{
   std::vector<std::string> checkPrefixes;
   bool noCanonicalizeWhiteSpace = false;
   std::vector<std::string> implicitCheckNot;
   std::vector<std::string> globalDefines;
   bool allowEmptyInput = false;
   bool matchFullLines = false;
   bool enableVarScope = false;
   bool allowDeprecatedDagOverlap = false;
   bool verbose = false;
   bool verboseVerbose = false;
};

//===----------------------------------------------------------------------===//
// Pattern Handling Code.
//===----------------------------------------------------------------------===//

enum FileCheckKind
{
   CheckNone = 0,
   CheckPlain,
   CheckNext,
   CheckSame,
   CheckNot,
   CheckDAG,
   CheckLabel,
   CheckEmpty,

   /// Indicates the pattern only matches the end of file. This is used for
   /// trailing CHECK-NOTs.
   CheckEOF,

   /// Marks when parsing found a -NOT check combined with another CHECK suffix.
   CheckBadNot,

   /// Marks when parsing found a -COUNT directive with invalid count value.
   CheckBadCount
};

class FileCheckType
{
   FileCheckKind m_kind;
   int m_count; ///< optional Count for some checks

public:
   FileCheckType(FileCheckKind kind = CheckNone)
      : m_kind(kind),
        m_count(1)
   {}

   FileCheckType(const FileCheckType &) = default;

   operator FileCheckKind() const
   {
      return m_kind;
   }

   int getCount() const
   {
      return m_count;
   }

   FileCheckType &setCount(int count);

   std::string getDescription(StringRef prefix) const;
};

struct FileCheckDiag;

class FileCheckPattern
{
   SMLocation m_patternLoc;

   /// A fixed string to match as the pattern or empty if this pattern requires
   /// a regex match.
   StringRef m_fixedStr;

   /// A regex string to match as the pattern or empty if this pattern requires
   /// a fixed string to match.
   std::string m_regExStr;

   /// Entries in this vector map to uses of a variable in the pattern, e.g.
   /// "foo[[bar]]baz".  In this case, the regExStr will contain "foobaz" and
   /// we'll get an entry in this vector that tells us to insert the value of
   /// bar at offset 3.
   std::vector<std::pair<StringRef, unsigned>> m_variableUses;

   /// Maps definitions of variables to their parenthesized capture numbers.
   ///
   /// E.g. for the pattern "foo[[bar:.*]]baz", variableDefs will map "bar" to
   /// 1.
   std::map<StringRef, unsigned> m_variableDefs;

   FileCheckType m_checkType;

   /// Contains the number of line this pattern is in.
   unsigned m_lineNumber;

public:
   explicit FileCheckPattern(FileCheckType type)
      : m_checkType(type)
   {}

   /// Returns the location in source code.
   SMLocation getLoc() const
   {
      return m_patternLoc;
   }

   bool parsePattern(StringRef patternStr, StringRef prefix, SourceMgr &sourceMgr,
                     unsigned lineNumber, const FileCheckRequest &req);
   size_t match(StringRef buffer, size_t &matchLen,
                StringMap<std::string> &variableTable) const;
   void printVariableUses(const SourceMgr &sourceMgr, StringRef buffer,
                          const StringMap<std::string> &variableTable,
                          SMRange matchRange = std::nullopt) const;
   void printFuzzyMatch(const SourceMgr &sourceMgr, StringRef buffer,
                        const StringMap<std::string> &variableTable,
                        std::vector<FileCheckDiag> *diags) const;

   bool hasVariable() const
   {
      return !(m_variableUses.empty() && m_variableDefs.empty());
   }

   FileCheckType getCheckType() const
   {
      return m_checkType;
   }

   int getCount() const
   {
      return m_checkType.getCount();
   }

private:
   bool addRegExToRegEx(StringRef regexStr, unsigned &curParen, SourceMgr &sourceMgr);
   void addBackrefToRegEx(unsigned backrefNum);
   unsigned
   computeMatchDistance(StringRef buffer,
                        const StringMap<std::string> &variableTable) const;
   bool evaluateExpression(StringRef expr, std::string &value) const;
   size_t findRegexVarEnd(StringRef str, SourceMgr &sourceMgr);
};

//===----------------------------------------------------------------------===//
/// Summary of a FileCheck diagnostic.
//===----------------------------------------------------------------------===//

struct FileCheckDiag
{
   /// What is the FileCheck directive for this diagnostic?
   FileCheckType m_checkType;
   /// Where is the FileCheck directive for this diagnostic?
   unsigned m_checkLine;
   unsigned m_checkCol;
   /// What type of match result does this diagnostic describe?
   ///
   /// A directive's supplied pattern is said to be either expected or excluded
   /// depending on whether the pattern must have or must not have a match in
   /// order for the directive to succeed.  For example, a CHECK directive's
   /// pattern is expected, and a CHECK-NOT directive's pattern is excluded.
   /// All match result types whose names end with "Excluded" are for excluded
   /// patterns, and all others are for expected patterns.
   ///
   /// There might be more than one match result for a single pattern.  For
   /// example, there might be several discarded matches
   /// (MatchFoundButDiscarded) before either a good match
   /// (MatchFoundAndExpected) or a failure to match (MatchNoneButExpected),
   /// and there might be a fuzzy match (MatchFuzzy) after the latter.
   enum MatchType
   {
      /// Indicates a good match for an expected pattern.
      MatchFoundAndExpected,
      /// Indicates a match for an excluded pattern.
      MatchFoundButExcluded,
      /// Indicates a match for an expected pattern, but the match is on the
      /// wrong line.
      MatchFoundButWrongLine,
      /// Indicates a discarded match for an expected pattern.
      MatchFoundButDiscarded,
      /// Indicates no match for an excluded pattern.
      MatchNoneAndExcluded,
      /// Indicates no match for an expected pattern, but this might follow good
      /// matches when multiple matches are expected for the pattern, or it might
      /// follow discarded matches for the pattern.
      MatchNoneButExpected,
      /// Indicates a fuzzy match that serves as a suggestion for the next
      /// intended match for an expected pattern with too few or no good matches.
      MatchFuzzy,
   } m_matchType;
   /// The search range if MatchTy is MatchNoneAndExcluded or
   /// MatchNoneButExpected, or the match range otherwise.
   unsigned m_inputStartLine;
   unsigned m_inputStartCol;
   unsigned m_inputEndLine;
   unsigned m_inputEndCol;

   FileCheckDiag(const SourceMgr &sourceMgr, const FileCheckType &checkType,
                 SMLocation checkLoc, MatchType matchType, SMRange inputRange);
};

//===----------------------------------------------------------------------===//
// Check Strings.
//===----------------------------------------------------------------------===//

/// A check that we found in the input file.
struct FileCheckString
{
   /// The pattern to match.
   FileCheckPattern m_pattern;

   /// Which prefix name this check matched.
   StringRef m_prefix;

   /// The location in the match file that the check string was specified.
   SMLocation m_loc;

   /// All of the strings that are disallowed from occurring between this match
   /// string and the previous one (or start of file).
   std::vector<FileCheckPattern> m_dagNotStrings;

   FileCheckString(const FileCheckPattern &pattern, StringRef str, SMLocation loc)
      : m_pattern(pattern),
        m_prefix(str),
        m_loc(loc)
   {}

   size_t check(const SourceMgr &sourceMgr, StringRef buffer, bool isLabelScanMode,
                size_t &matchLen, StringMap<std::string> &variableTable,
                FileCheckRequest &req, std::vector<FileCheckDiag> *diags) const;

   bool checkNext(const SourceMgr &sourceMgr, StringRef buffer) const;
   bool checkSame(const SourceMgr &sourceMgr, StringRef buffer) const;
   bool checkNot(const SourceMgr &sourceMgr, StringRef buffer,
                 const std::vector<const FileCheckPattern *> &notStrings,
                 StringMap<std::string> &variableTable,
                 const FileCheckRequest &req,
                 std::vector<FileCheckDiag> *diags) const;
   size_t checkDag(const SourceMgr &sourceMgr, StringRef buffer,
                   std::vector<const FileCheckPattern *> &notStrings,
                   StringMap<std::string> &variableTable,
                   const FileCheckRequest &req,
                   std::vector<FileCheckDiag> *diags) const;
};

/// FileCheck class takes the request and exposes various methods that
/// use information from the request.
class FileCheck
{
   FileCheckRequest m_req;

public:
   FileCheck(FileCheckRequest req)
      : m_req(req)
   {}

   // Combines the check prefixes into a single regex so that we can efficiently
   // scan for any of the set.
   //
   // The semantics are that the longest-match wins which matches our regex
   // library.
   boost::regex buildCheckPrefixRegex();

   /// Read the check file, which specifies the sequence of expected strings.
   ///
   /// The strings are added to the checkStrings vector. Returns true in case of
   /// an error, false otherwise.
   bool readCheckFile(SourceMgr &sourceMgr, StringRef buffer, boost::regex &prefixRE,
                      std::vector<FileCheckString> &checkStrings);

   bool validateCheckPrefixes();

   /// Canonicalize whitespaces in the file. Line endings are replaced with
   /// UNIX-style '\n'.
   StringRef canonicalizeFile(MemoryBuffer &memoryBuffer,
                              SmallVectorImpl<char> &outputBuffer);

   /// Check the input to FileCheck provided in the \p buffer against the \p
   /// checkStrings read from the check file.
   ///
   /// Returns false if the input fails to satisfy the checks.
   bool checkInput(SourceMgr &sourceMgr, StringRef buffer,
                   ArrayRef<FileCheckString> checkStrings,
                   std::vector<FileCheckDiag> *diags = nullptr);
};

} // filechecker
} // polar
