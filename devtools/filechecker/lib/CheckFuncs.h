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

#ifndef POLAR_DEVLTOOLS_FILECHECKER_CHECK_FUNCS_H
#define POLAR_DEVLTOOLS_FILECHECKER_CHECK_FUNCS_H

#include "Global.h"
#include <string>
#include <regex>

namespace polar {
namespace basic {
class StringRef;
class StringMap;
template <typename T>
class SmallVectorImpl;
} // basic

namespace utils {
class MemoryBuffer;
class SourceMgr;
class SMLocation;
} // utils
} // polar

namespace polar {
namespace filechecker {

using polar::basic::SmallVectorImpl;
using polar::basic::StringRef;
using polar::basic::StringMap;
using polar::utils::MemoryBuffer;
using polar::utils::SourceMgr;
using polar::utils::SMLocation;
class CheckString;
class CheckPattern;
/// Canonicalize whitespaces in the file. Line endings are replaced with
/// UNIX-style '\n'.
StringRef canonicalize_file(MemoryBuffer &memoryBuffer,
                            SmallVectorImpl<char> &outputBuffer);
bool is_part_of_word(char c);
size_t check_type_size(CheckType checkType);
std::string check_type_name(StringRef prefix, CheckType checkType);
CheckType find_check_type(StringRef buffer, StringRef prefix);
// From the given position, find the next character after the word.
size_t skip_word(StringRef str, size_t loc);
/// Search the buffer for the first prefix in the prefix regular expression.
///
/// This searches the buffer using the provided regular expression, however it
/// enforces constraints beyond that:
/// 1) The found prefix must not be a suffix of something that looks like
///    a valid prefix.
/// 2) The found prefix must be followed by a valid check type suffix using \c
///    FindCheckType above.
///
/// The first match of the regular expression to satisfy these two is returned,
/// otherwise an empty StringRef is returned to indicate failure.
///
/// If this routine returns a valid prefix, it will also shrink \p buffer to
/// start at the beginning of the returned prefix, increment \p LineNumber for
/// each new line consumed from \p buffer, and set \p CheckTy to the type of
/// check found by examining the suffix.
///
/// If no valid prefix is found, the state of buffer, LineNumber, and CheckTy
/// is unspecified.
StringRef find_first_matching_prefix(std::regex &prefixRegex, StringRef &buffer,
                                     unsigned &lineNumber,
                                     CheckType &checkType);

/// Read the check file, which specifies the sequence of expected strings.
///
/// The strings are added to the CheckStrings vector. Returns true in case of
/// an error, false otherwise.
bool read_check_file(SourceMgr &sourceMgr, StringRef buffer, std::regex &prefixRegex,
                     std::vector<CheckString> &checkStrings);
void print_match(bool expectedMatch, const SourceMgr &sourceMgr,
                 StringRef prefix, SMLocation loc, const CheckPattern &pattern,
                 StringRef buffer, StringMap<StringRef> &variableTable,
                 size_t matchPos, size_t matchLen);
void print_match(bool expectedMatch, const SourceMgr &sourceMgr,
                 const CheckString &checkStr, StringRef buffer,
                 StringMap<StringRef> &variableTable, size_t matchPos,
                 size_t matchLen);
void print_no_match(bool expectedMatch, const SourceMgr &sourceMgr,
                    StringRef prefix, SMLocation loc, const Pattern &Pat,
                    StringRef buffer,
                    StringMap<StringRef> &variableTable);
void print_no_match(bool expectedMatch, const SourceMgr &sourceMgr,
                    const CheckString &checkStr, StringRef buffer,
                    StringMap<StringRef> &variableTable);
unsigned count_num_newlines_between(StringRef range,
                                    const char *&firstNewLine);

} // filechecker
} // polar

#endif // POLAR_DEVLTOOLS_FILECHECKER_CHECK_FUNCS_H

