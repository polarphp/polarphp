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

#ifndef POLAR_DEVLTOOLS_FILECHECKER_CHECK_PATTERN_H
#define POLAR_DEVLTOOLS_FILECHECKER_CHECK_PATTERN_H

#include "Global.h"
#include "polarphp/utils/SourceMgr.h"
#include "polarphp/utils/SourceLocation.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/StringMap.h"

#include <map>

// forward declare class with namespace
namespace polar {
namespace basic {

} // basic

namespace utils {
class SourceMgr;
class SMLocation;
class SMRange;
} // utils
} // polar

namespace polar {
namespace filechecker {

using polar::utils::SourceMgr;
using polar::utils::SMLocation;
using polar::utils::SMRange;
using polar::basic::StringMap;
using polar::basic::StringRef;

class Pattern
{
public:
   explicit Pattern(CheckType checkType);

   /// Returns the location in source code.
   SMLocation getLoc() const
   {
      return m_patternLoc;
   }

   bool parsePattern(StringRef patternStr, StringRef prefix, SourceMgr &sourceMgr,
                     unsigned lineNumber);
   size_t match(StringRef buffer, size_t &matchLen,
                StringMap<std::string> &variableTable) const;
   void printVariableUses(const SourceMgr &sourceMgr, StringRef buffer,
                          const StringMap<std::string> &variableTable,
                          SMRange matchRange = {}) const;
   void printFuzzyMatch(const SourceMgr &sourceMgr, StringRef buffer,
                        const StringMap<std::string> &variableTable) const;

   bool hasVariable() const
   {
      return !(m_variableUses.empty() && m_variableDefs.empty());
   }

   CheckType getCheckType() const
   {
      return m_checkType;
   }

   const std::string &getRegExStr() const
   {
      return m_regExStr;
   }

private:
   bool addRegExToRegEx(StringRef rs, unsigned &curParen, SourceMgr &sourceMgr);
   void addBackrefToRegEx(unsigned backrefNum);
   unsigned
   computeMatchDistance(StringRef buffer,
                        const StringMap<std::string> &variableTable) const;
   bool evaluateExpression(StringRef expr, std::string &value) const;
   size_t findRegexVarEnd(StringRef str, SourceMgr &sourceMgr);

private:
   SMLocation m_patternLoc;

   /// A fixed string to match as the pattern or empty if this pattern requires
   /// a regex match.
   StringRef m_fixedStr;

   /// A regex string to match as the pattern or empty if this pattern requires
   /// a fixed string to match.
   std::string m_regExStr;

   /// Entries in this vector map to uses of a variable in the pattern, e.g.
   /// "foo[[bar]]baz".  In this case, the m_regExStr will contain "foobaz" and
   /// we'll get an entry in this vector that tells us to insert the value of
   /// bar at offset 3.
   std::vector<std::pair<StringRef, unsigned>> m_variableUses;

   /// Maps definitions of variables to their parenthesized capture numbers.
   ///
   /// E.g. for the pattern "foo[[bar:.*]]baz", m_variableDefs will map "bar" to
   /// 1.
   std::map<StringRef, unsigned> m_variableDefs;

   CheckType m_checkType;

   /// Contains the number of line this pattern is in.
   unsigned m_lineNumber;
   bool m_matchFullLines;
   bool m_noCanonicalizeWhiteSpace;
};

} // filechecker
} // polar


#endif // POLAR_DEVLTOOLS_FILECHECKER_CHECK_PATTERN_H
