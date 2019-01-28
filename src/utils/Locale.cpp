// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/04.

#include "polarphp/utils/Locale.h"
#include "polarphp/utils/Unicode.h"
#include "polarphp/basic/adt/StringRef.h"

namespace polar {
namespace sys {
namespace locale {

int column_width(StringRef text)
{
   return polar::sys::unicode::column_width_utf8(text);
}

bool is_print(int ucs)
{
   return polar::sys::unicode::is_printable(ucs);
}

} // locale
} // sys
} // locale
