// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/12/26.

#include "polarphp/global/Global.h"
#include <string>
#include <iostream>

namespace polar {
void polar_assert(const char *assertion, const char *file,
                  int line) noexcept
{
   std::string errMsg(assertion);
   errMsg += " is fail";
   std::cerr << errMsg << std::endl;
   POLAR_UNUSED(line);
   POLAR_UNUSED(file);
}

void polar_assert_x(const char *where, const char *what,
                    const char *file, int line) noexcept
{
   std::cerr << what << std::endl;
   POLAR_UNUSED(where);
   POLAR_UNUSED(file);
   POLAR_UNUSED(line);
}
} // polar
