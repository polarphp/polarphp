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

#ifndef POLAR_DEVLTOOLS_FILECHECKER_CHECK_STRING_H
#define POLAR_DEVLTOOLS_FILECHECKER_CHECK_STRING_H

#include "CheckPattern.h"

namespace polar {
namespace filechecker {

/// A check that we found in the input file.
class CheckString
{
public:
   /// The pattern to match.
   Pattern m_pattern;

   /// Which prefix name this check matched.
   StringRef m_prefix;

   /// The location in the match file that the check string was specified.
   SMLocation m_location;

   /// All of the strings that are disallowed from occurring between this match
   /// string and the previous one (or start of file).
   std::vector<Pattern> m_dagNotStrings;

   CheckString(const Pattern &pattern, StringRef str, SMLocation location)
      : m_pattern(pattern),
        m_prefix(str),
        m_location(location)
   {}

   size_t check(const SourceMgr &sourceMgr, StringRef buffer, bool isLabelScanMode,
                size_t &matchLen, StringMap<std::string> &variableTable) const;

   bool checkNext(const SourceMgr &sourceMgr, StringRef buffer) const;
   bool checkSame(const SourceMgr &sourceMgr, StringRef buffer) const;
   bool checkNot(const SourceMgr &sourceMgr, StringRef buffer,
                 const std::vector<const Pattern *> &notStrings,
                 StringMap<std::string> &variableTable) const;
   size_t checkDag(const SourceMgr &sourceMgr, StringRef buffer,
                   std::vector<const Pattern *> &notStrings,
                   StringMap<std::string> &variableTable) const;
};

} // filechecker
} // polar

#endif // POLAR_DEVLTOOLS_FILECHECKER_CHECK_STRING_H
