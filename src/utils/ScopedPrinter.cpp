// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/18.

#include "polarphp/utils/ScopedPrinter.h"
#include "polarphp/utils/Format.h"
#include <cctype>

namespace polar {
namespace utils {

RawOutStream &operator<<(RawOutStream &outstream, const HexNumber &value)
{
   outstream << "0x" << to_hexString(value.m_value);
   return outstream;
}

const std::string to_hexString(uint64_t value, bool upperCase)
{
   std::string number;
   RawStringOutStream stream(number);
   stream << format_hex_no_prefix(value, 1, upperCase);
   return stream.getStr();
}

void ScopedPrinter::printBinaryImpl(StringRef label, StringRef str,
                                    ArrayRef<uint8_t> data, bool block,
                                    uint32_t startOffset)
{
   if (data.getSize() > 16) {
      block = true;
   }
   if (block) {
      startLine() << label;
      if (!str.empty()) {
         m_outstream << ": " << str;
      }
      m_outstream << " (\n";
      if (!data.empty()) {
         m_outstream << format_bytes_with_ascii(data, startOffset, 16, 4,
                                                (m_indentLevel + 1) * 2, true)
                     << "\n";
      }
      startLine() << ")\n";
   } else {
      startLine() << label << ":";
      if (!str.empty()) {
         m_outstream << " " << str;
      }
      m_outstream << " (" << format_bytes(data, std::nullopt, data.getSize(), 1, 0, true) << ")\n";
   }
}

} // utils
} // polar
