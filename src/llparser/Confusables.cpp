//===--- Confusables.cpp - Swift Confusable Character Diagnostics ---------===//
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

#include "polarphp/llparser/Confusables.h"

char polar::llparser::confusable::tryConvertConfusableCharacterToASCII(uint32_t codepoint) {
   switch (codepoint) {
      #define CONFUSABLE(CONFUSABLE_POINT, BASEPOINT) \
   case CONFUSABLE_POINT: return BASEPOINT;
      #include "polarphp/llparser/ConfusablesDef.h"
      default: return 0;
   }
}
