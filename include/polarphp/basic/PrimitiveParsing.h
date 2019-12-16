//===--- PrimitiveParsing.h - Primitive parsing routines --------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/02.
///
/// \file
/// Primitive parsing routines useful in various places in the compiler.
///
//===----------------------------------------------------------------------===//


#ifndef POLARPHP_BASIC_PRIMITIVE_PARSING_H
#define POLARPHP_BASIC_PRIMITIVE_PARSING_H

#include "llvm/ADT/StringRef.h"
#include "polarphp/basic/LLVM.h"

namespace polar {

unsigned measure_newline(const char *bufferPtr, const char *bufferEnd);

static inline unsigned measure_newline(StringRef str)
{
   return measure_newline(str.data(), str.data() + str.size());
}

static inline bool starts_with_newline(StringRef str)
{
   return str.startswith("\n") || str.startswith("\r\n");
}

/// Breaks a given string to lines and trims leading whitespace from them.
void trim_leading_whitespace_from_lines(StringRef text, unsigned whitespaceToTrim,
                                        SmallVectorImpl<StringRef> &lines);

static inline void split_into_lines(StringRef text,
                                    SmallVectorImpl<StringRef> &lines)
{
   trim_leading_whitespace_from_lines(text, 0, lines);
}

} // polar

#endif // POLARPHP_BASIC_PRIMITIVE_PARSING_H

