//===--- PrimitiveParsing.cpp - Primitive parsing routines ----------------===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/02.
///
/// \file
/// Primitive parsing routines useful in various places in the compiler.
///
//===----------------------------------------------------------------------===//

#include "polarphp/basic/PrimitiveParsing.h"
#include "llvm/ADT/SmallVector.h"

namespace polar {

unsigned measure_newline(const char *bufferPtr, const char *bufferEnd)
{
   if (bufferPtr == bufferEnd) {
      return 0;
   }
   if (*bufferPtr == '\n') {
      return 1;
   }
   assert(*bufferPtr == '\r');
   unsigned bytes = 1;
   if (bufferPtr != bufferEnd && *bufferPtr == '\n') {
      bytes++;
   }
   return bytes;
}

void trim_leading_whitespace_from_lines(StringRef rawText,
                                        unsigned whitespaceToTrim,
                                        SmallVectorImpl<StringRef> &outLines)
{
   SmallVector<StringRef, 8> lines;

   bool isFirstLine = true;

   while (!rawText.empty()) {
      size_t pos = rawText.find_first_of("\n\r");
      if (pos == StringRef::npos) {
         pos = rawText.size();
      }
      StringRef line = rawText.substr(0, pos);
      lines.push_back(line);
      if (!isFirstLine) {
         size_t NonWhitespacePos = rawText.find_first_not_of(' ');
         if (NonWhitespacePos != StringRef::npos) {
            whitespaceToTrim =
                  std::min(whitespaceToTrim,
                           static_cast<unsigned>(NonWhitespacePos));
         }
      }
      isFirstLine = false;
      rawText = rawText.drop_front(pos);
      unsigned newlineBytes = measure_newline(rawText);
      rawText = rawText.drop_front(newlineBytes);
   }

   isFirstLine = true;
   for (auto &line : lines) {
      if (!isFirstLine) {
         line = line.drop_front(whitespaceToTrim);
      }
      isFirstLine = false;
   }
   outLines.append(lines.begin(), lines.end());
}

} //  polar
