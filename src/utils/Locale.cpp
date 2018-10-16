// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by softboy on 2018/07/04.

#include "polarphp/utils/Locale.h"
#include "polarphp/utils/Unicode.h"
#include "polarphp/basic/adt/StringRef.h"

namespace polar {
namespace sys {
namespace locale {

int column_width(StringRef text) {
#if polar_ON_WIN32
   return text.size();
#else
   return polar::sys::unicode::column_width_utf8(text);
#endif
}

bool is_print(int ucs) {
#if POLAR_ON_WIN32
   // Restrict characters that we'll try to print to the lower part of ASCII
   // except for the control characters (0x20 - 0x7E). In general one can not
   // reliably output code points U+0080 and higher using narrow character C/C++
   // output functions in Windows, because the meaning of the upper 128 codes is
   // determined by the active code page in the console.
   return ' ' <= ucs && ucs <= '~';
#else
   return polar::sys::unicode::is_printable(ucs);
#endif
}

} // locale
} // sys
} // locale
