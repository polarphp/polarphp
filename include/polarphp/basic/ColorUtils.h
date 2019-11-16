//===--- ColorUtils.h - -----------------------------------------*- C++ -*-===//
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
// Created by polarboy on 2019/04/29.
//===----------------------------------------------------------------------===//
//
// This file defines an 'OsColor' class for helping printing colorful outputs
// to the terminal.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_COLORUTILS_H
#define POLARPHP_BASIC_COLORUTILS_H

#include "llvm/Support/raw_ostream.h"

namespace polar::basic {

using llvm::raw_ostream;
using llvm::StringRef;

/// RAII class for setting a color for a raw_ostream and resetting when it goes
/// out-of-scope.
class OsColor
{
public:
   OsColor(raw_ostream &outStream, raw_ostream::Colors color)
      : m_outStream(outStream)
   {
      m_hasColors = m_outStream.has_colors();
      if (m_hasColors) {
         m_outStream.changeColor(color);
      }
   }

   ~OsColor()
   {
      if (m_hasColors) {
         m_outStream.resetColor();
      }
   }

   OsColor &operator<<(char c)
   {
      m_outStream << c; return *this;
   }

   OsColor &operator<<(StringRef str)
   {
      m_outStream << str;
      return *this;
   }

private:
   raw_ostream &m_outStream;
   bool m_hasColors;
};

} // polar::basic

#endif // POLARPHP_BASIC_COLORUTILS_H
