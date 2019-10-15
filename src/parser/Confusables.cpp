// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/30.

#include "polarphp/parser/Confusables.h"

namespace polar::parser {

char try_convert_confusable_character_to_ascii(uint32_t codepoint)
{
   switch (codepoint) {
#define CONFUSABLE(CONFUSABLE_POINT, BASEPOINT) \
   case CONFUSABLE_POINT: return BASEPOINT;
#include "polarphp/parser/ConfusableDefs.h"
   default: return 0;
   }
}

} // polar::parser
