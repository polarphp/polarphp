// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

#ifndef POLARPHP_UTILS_UNICODE_H
#define POLARPHP_UTILS_UNICODE_H

namespace polar {
// forward declare class with namespace
namespace basic {
class StringRef;
} // basic
namespace sys {
namespace unicode {

using polar::basic::StringRef;

enum ColumnWidthErrors
{
   ErrorInvalidUTF8 = -2,
   ErrorNonPrintableCharacter = -1
};

/// Determines if a character is likely to be displayed correctly on the
/// terminal. Exact implementation would have to depend on the specific
/// terminal, so we define the semantic that should be suitable for generic case
/// of a terminal capable to output Unicode characters.
///
/// All characters from the Unicode code point range are considered printable
/// except for:
///   * C0 and C1 control character ranges;
///   * default ignorable code points as per 5.21 of
///     http://www.unicode.org/versions/Unicode6.2.0/UnicodeStandard-6.2.pdf
///     except for U+00AD SOFT HYPHEN, as it's actually displayed on most
///     terminals;
///   * format characters (category = Cf);
///   * surrogates (category = Cs);
///   * unassigned characters (category = Cn).
/// \return true if the character is considered printable.
bool is_printable(int ucs);

/// Gets the number of positions the UTF8-encoded \p Text is likely to occupy
/// when output on a terminal ("character width"). This depends on the
/// implementation of the terminal, and there's no standard definition of
/// character width.
///
/// The implementation defines it in a way that is expected to be compatible
/// with a generic Unicode-capable terminal.
///
/// \return Character width:
///   * ErrorNonPrintableCharacter (-1) if \p Text contains non-printable
///     characters (as identified by is_printable);
///   * 0 for each non-spacing and enclosing combining mark;
///   * 2 for each CJK character excluding halfwidth forms;
///   * 1 for each of the remaining characters.
int column_width_utf8(StringRef text);

/// Fold input unicode character according the Simple unicode case folding
/// rules.
int fold_char_simple(int c);

} // namespace unicode
} // sys
} // polar

#endif // POLARPHP_UTILS_UNICODE_H
