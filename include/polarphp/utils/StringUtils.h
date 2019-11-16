// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/25.

#ifndef POLARPHP_UTILS_STRING_UTILS_H
#define POLARPHP_UTILS_STRING_UTILS_H

#include <string>

/// forward declare class with namespace
namespace llvm {
class StringRef;
} // llvm

namespace polar::utils {

using llvm::StringRef;

std::string regex_escape(StringRef str);

} // polar::utils

#endif // POLARPHP_UTILS_STRING_UTILS_H
