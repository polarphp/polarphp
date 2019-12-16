//===--- QuotedString.h - Print a string in double-quotes -------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2018/06/29.
//===----------------------------------------------------------------------===//
//
/// \file Declares QuotedString, a convenient type for printing a
/// string as a string literal.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_QUOTEDSTRING_H
#define POLARPHP_BASIC_QUOTEDSTRING_H

#include "llvm/ADT/StringRef.h"

/// forward declare class with namespace
namespace llvm {
class raw_ostream;
}

namespace polar {

using llvm::StringRef;
using llvm::raw_ostream;

/// Print the given string as if it were a quoted string.
void print_as_quoted_string(raw_ostream &out, StringRef text);

/// A class designed to make it easy to write a string to a stream
/// as a quoted string.
class QuotedString
{
public:
   explicit QuotedString(StringRef text)
      : m_text(text)
   {}

   friend raw_ostream &operator<<(raw_ostream &out, QuotedString string)
   {
      print_as_quoted_string(out, string.m_text);
      return out;
   }

private:
   StringRef m_text;
};

} // polar

#endif // POLARPHP_BASIC_QUOTEDSTRING_H
