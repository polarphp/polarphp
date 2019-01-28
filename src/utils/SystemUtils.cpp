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

//===----------------------------------------------------------------------===//
//
// This file contains functions used to do a variety of low-level, often
// system-specific, tasks.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/SystemUtils.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace utils {

bool check_bitcode_output_to_console(RawOutStream &streamToCheck,
                                     bool printWarning)
{
   if (streamToCheck.isDisplayed()) {
      if (printWarning) {
         error_stream() << "WARNING: You're attempting to print out a bitcode file.\n"
                           "This is inadvisable as it may cause display problems. If\n"
                           "you REALLY want to taste polarVM bitcode first-hand, you\n"
                           "can force output with the `-f' option.\n\n";
      }
      return true;
   }
   return false;
}

} // utils
} // polar
