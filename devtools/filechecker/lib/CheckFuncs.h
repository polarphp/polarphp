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

#ifndef POLAR_DEVLTOOLS_FILECHECKER_CHECK_FUNCS_H
#define POLAR_DEVLTOOLS_FILECHECKER_CHECK_FUNCS_H

#include "Global.h"
#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include <string>
#include <boost/regex.hpp>

namespace polar {
namespace utils {
class MemoryBuffer;
class SourceMgr;
class SMLocation;
} // utils

namespace basic {
class StringRef;
template <typename T>
class SmallVectorImpl;
} // basic
} // polar

namespace polar {
namespace filechecker {

using polar::basic::SmallVectorImpl;
using polar::basic::ArrayRef;
using polar::basic::StringRef;
using polar::basic::StringMap;
using polar::utils::MemoryBuffer;
using polar::utils::SourceMgr;
using polar::utils::SMLocation;
class CheckString;
class Pattern;

/// Canonicalize whitespaces in the file. Line endings are replaced with
/// UNIX-style '\n'.
StringRef canonicalize_file(MemoryBuffer &memoryBuffer,
                            SmallVectorImpl<char> &outputBuffer);
bool is_part_of_word(char c);
size_t check_type_size(CheckType checkType);
std::string check_type_name(StringRef prefix, CheckType checkType);
CheckType find_check_type(StringRef buffer, StringRef prefix);
// From the given position, find the next character after the word.
size_t skip_word(StringRef str);
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
StringRef find_first_matching_prefix(boost::regex &prefixRegex, StringRef &buffer,
                                     unsigned &lineNumber,
                                     CheckType &checkType);

/// Read the check file, which specifies the sequence of expected strings.
///
/// The strings are added to the checkStrings vector. Returns true in case of
/// an error, false otherwise.
bool read_check_file(SourceMgr &sourceMgr, StringRef buffer, boost::regex &prefixRegex,
                     std::vector<CheckString> &checkStrings);
void print_match(bool expectedMatch, const SourceMgr &sourceMgr,
                 StringRef prefix, SMLocation loc, const Pattern &pattern,
                 StringRef buffer, StringMap<std::string> &variableTable,
                 size_t matchPos, size_t matchLen);
void print_match(bool expectedMatch, const SourceMgr &sourceMgr,
                 const CheckString &checkStr, StringRef buffer,
                 StringMap<std::string> &variableTable, size_t matchPos,
                 size_t matchLen);
void print_no_match(bool expectedMatch, const SourceMgr &sourceMgr,
                    StringRef prefix, SMLocation loc, const Pattern &Pat,
                    StringRef buffer,
                    StringMap<std::string> &variableTable);
void print_no_match(bool expectedMatch, const SourceMgr &sourceMgr,
                    const CheckString &checkStr, StringRef buffer,
                    StringMap<std::string> &variableTable);
unsigned count_num_newlines_between(StringRef range,
                                    const char *&firstNewLine);

bool validate_check_prefix(StringRef checkPrefix);
bool validate_check_prefixes();
bool build_check_prefix_regex(boost::regex &regex, std::string &errorMsg);
void dump_command_line(int argc, char **argv);
void clear_local_vars(StringMap<std::string> &variableTable);
bool check_input(SourceMgr &sourceMgr, StringRef buffer,
                 ArrayRef<CheckString> checkStrings);

} // filechecker
} // polar

#endif // POLAR_DEVLTOOLS_FILECHECKER_CHECK_FUNCS_H

