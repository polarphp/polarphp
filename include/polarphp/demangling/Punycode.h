//===--- Punycode.h - UTF-8 to Punycode transcoding -------------*- C++ -*-===//
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
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/29.
//
//===----------------------------------------------------------------------===//
//
// These functions implement a variant of the Punycode algorithm from RFC3492,
// originally designed for encoding international domain names, for the purpose
// of encoding Swift identifiers into mangled symbol names. This version differs
// from RFC3492 in the following respects:
// - '_' is used as the encoding delimiter instead of '-'.
// - Encoding digits are represented using [a-zA-J] instead of [a-z0-9], because
//   symbol names are case-sensitive, and Swift mangled identifiers cannot begin
//   with a digit.
// - Optionally, non-symbol ASCII characters (characters except [$_a-zA-Z0-9])
//   are mapped to the code range 0xD800 - 0xD880 and are also encoded like
//   non-ASCII unicode characters.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_DEMANGLING_PUNYCODE_H
#define POLARPHP_DEMANGLING_PUNYCODE_H

#include "llvm/ADT/StringRef.h"
#include <vector>
#include <cstdint>

namespace polar::demangling {

using llvm::StringRef;

/// Encodes a sequence of code points into Punycode.
///
/// Returns false if input contains surrogate code points.
bool encode_punycode(const std::vector<uint32_t> &inputCodePoints,
                     std::string &outPunycode);

/// Decodes a Punycode string into a sequence of Unicode scalars.
///
/// Returns false if decoding failed.
bool decode_punycode(StringRef inputPunycode,
                     std::vector<uint32_t> &outCodePoints);

/// Encodes an UTF8 string into Punycode.
///
/// If \p mapNonSymbolChars is true, non-symbol ASCII characters (characters
/// except [$_a-zA-Z0-9]) are also encoded like non-ASCII unicode characters.
/// Returns false if \p InputUTF8 contains surrogate code points.
bool encode_punycode_utf8(StringRef InputUTF8, std::string &outPunycode,
                          bool mapNonSymbolChars = false);

bool decode_punycode_utf8(StringRef inputPunycode, std::string &outUTF8);

} // polar::demangling

#endif // POLARPHP_DEMANGLING_PUNYCODE_H
