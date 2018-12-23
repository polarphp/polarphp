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

#ifndef POLARPHP_UTILS_STRING_UTILS_H
#define POLARPHP_UTILS_STRING_UTILS_H

#include <string>

/// forward declare class with namespace
namespace polar {
namespace basic {
class StringRef;
} // basic
} // polar

namespace polar {
namespace utils {

using polar::basic::StringRef;

std::string regex_escape(StringRef str);

} // utils
} // polar

#endif // POLARPHP_UTILS_STRING_UTILS_H
