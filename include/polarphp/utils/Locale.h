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

#ifndef POLARPHP_UTILS_LOCALE_H
#define POLARPHP_UTILS_LOCALE_H

namespace polar {

// forward declare class with namespace
namespace basic {
class StringRef;
} // basic

namespace sys {
namespace locale {

using polar::basic::StringRef;

int column_width(StringRef str);
bool is_print(int c);

} // locale
} // sys
} // locale

#endif // POLARPHP_UTILS_LOCALE_H
