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

#ifndef POLARPHP_UTILS_SCOPED_PRINTER_H
#define POLARPHP_UTILS_SCOPED_PRINTER_H

#include "polarphp/global/DataTypes.h"
#include "polarphp/basic/adt/ApSInt.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/Endian.h"
#include "polarphp/utils/RawOutStream.h"
#include <algorithm>

namespace polar {

// forward declare class with namespace
namespace basic {
class ApSInt;
template<typename T>
class ArrayRef;
} // basic

namespace utils {

using polar::basic::ArrayRef;
using basic::ApSInt;
using polar::basic::make_array_ref;

template <typename T>
struct EnumEntry
{
   StringRef m_name;
   // While Name suffices in most of the cases, in certain cases
   // GNU style and LLVM style of ELFDumper do not
   // display same string for same enum. The AltName if initialized appropriately
   // will hold the string that GNU style emits.
   // Example:
   // "EM_X86_64" string on LLVM style for Elf_Ehdr->e_machine corresponds to
   // "Advanced Micro Devices X86-64" on GNU style
   StringRef m_altName;
   T m_value;
   EnumEntry(StringRef name, StringRef altName, T value)
      : m_name(name),
        m_altName(altName),
        m_value(value)
   {}

   EnumEntry(StringRef name, T value)
      : m_name(name),
        m_altName(name),
        m_value(value)
   {}
};

struct HexNumber
{
   // To avoid sign-extension we have to explicitly cast to the appropriate
   // unsigned type. The overloads are here so that every type that is implicitly
   // convertible to an integer (including enums and endian helpers) can be used
   // without requiring type traits or call-site changes.
   HexNumber(char value) : m_value(static_cast<unsigned char>(value)) {}
   HexNumber(signed char value) : m_value(static_cast<unsigned char>(value)) {}
   HexNumber(signed short value) : m_value(static_cast<unsigned short>(value)) {}
   HexNumber(signed int value) : m_value(static_cast<unsigned int>(value)) {}
   HexNumber(signed long value) : m_value(static_cast<unsigned long>(value)) {}
   HexNumber(signed long long value)
      : m_value(static_cast<unsigned long long>(value)) {}
   HexNumber(unsigned char value) : m_value(value) {}
   HexNumber(unsigned short value) : m_value(value) {}
   HexNumber(unsigned int value) : m_value(value) {}
   HexNumber(unsigned long value) : m_value(value) {}
   HexNumber(unsigned long long value) : m_value(value) {}
   uint64_t m_value;
};

RawOutStream &operator<<(RawOutStream &outstream, const HexNumber &value);

const std::string to_hexString(uint64_t value, bool upperCase = true);

template <class T>
const std::string to_string(const T &value)
{
   std::string number;
   RawStringOutStream stream(number);
   stream << value;
   return stream.getStr();
}

class ScopedPrinter
{
public:
   ScopedPrinter(RawOutStream &outstream)
      : m_outstream(outstream), m_indentLevel(0)
   {}

   void flush()
   {
      m_outstream.flush();
   }

   void indent(int levels = 1)
   {
      m_indentLevel += levels;
   }

   void unindent(int levels = 1)
   {
      m_indentLevel = std::max(0, m_indentLevel - levels);
   }

   void resetIndent()
   {
      m_indentLevel = 0;
   }

   void setPrefix(StringRef prefix)
   {
      m_prefix = prefix;
   }

   void printIndent()
   {
      m_outstream << m_prefix;
      for (int i = 0; i < m_indentLevel; ++i) {
         m_outstream << "  ";
      }
   }

   template <typename T>
   HexNumber hex(T value)
   {
      return HexNumber(value);
   }

   template <typename T, typename TEnum>
   void printEnum(StringRef label, T value,
                  ArrayRef<EnumEntry<TEnum>> enumvalues)
   {
      StringRef name;
      bool found = false;
      for (const auto &enumItem : enumvalues) {
         if (enumItem.m_value == value) {
            name = enumItem.m_name;
            found = true;
            break;
         }
      }
      if (found) {
         startLine() << label << ": " << name << " (" << hex(value) << ")\n";
      } else {
         startLine() << label << ": " << hex(value) << "\n";
      }
   }

   template <typename T, typename TFlag>
   void printFlags(StringRef label, T value, ArrayRef<EnumEntry<TFlag>> flags,
                   TFlag enumMask1 = {}, TFlag enumMask2 = {},
                   TFlag enumMask3 = {})
   {
      typedef EnumEntry<TFlag> FlagEntry;
      typedef SmallVector<FlagEntry, 10> FlagVector;
      FlagVector setFlags;
      for (const auto &flag : flags) {
         if (flag.m_value == 0) {
            continue;
         }
         TFlag enumMask{};
         if (flag.m_value & enumMask1) {
            enumMask = enumMask1;
         } else if (flag.m_value & enumMask2) {
            enumMask = enumMask2;
         } else if (flag.m_value & enumMask3) {
            enumMask = enumMask3;
         }
         bool isEnum = (flag.m_value & enumMask) != 0;
         if ((!isEnum && (value & flag.m_value) == flag.m_value) ||
             (isEnum && (value & enumMask) == flag.m_value)) {
            setFlags.push_back(flag);
         }
      }

      std::sort(setFlags, &flagName<TFlag>);

      startLine() << label << " [ (" << hex(value) << ")\n";
      for (const auto &flag : setFlags) {
         startLine() << "  " << flag.m_name << " (" << hex(flag.m_value) << ")\n";
      }
      startLine() << "]\n";
   }

   template <typename T>
   void printFlags(StringRef label, T value)
   {
      startLine() << label << " [ (" << hex(value) << ")\n";
      uint64_t flag = 1;
      uint64_t curr = value;
      while (curr > 0) {
         if (curr & 1) {
            startLine() << "  " << hex(flag) << "\n";
         }
         curr >>= 1;
         flag <<= 1;
      }
      startLine() << "]\n";
   }

   void printNumber(StringRef label, uint64_t value)
   {
      startLine() << label << ": " << value << "\n";
   }

   void printNumber(StringRef label, uint32_t value)
   {
      startLine() << label << ": " << value << "\n";
   }

   void printNumber(StringRef label, uint16_t value)
   {
      startLine() << label << ": " << value << "\n";
   }

   void printNumber(StringRef label, uint8_t value)
   {
      startLine() << label << ": " << unsigned(value) << "\n";
   }

   void printNumber(StringRef label, int64_t value)
   {
      startLine() << label << ": " << value << "\n";
   }

   void printNumber(StringRef label, int32_t value)
   {
      startLine() << label << ": " << value << "\n";
   }

   void printNumber(StringRef label, int16_t value)
   {
      startLine() << label << ": " << value << "\n";
   }

   void printNumber(StringRef label, int8_t value)
   {
      startLine() << label << ": " << int(value) << "\n";
   }

   void printNumber(StringRef label, const ApSInt &value)
   {
      startLine() << label << ": " << value << "\n";
   }

   void printBoolean(StringRef label, bool value)
   {
      startLine() << label << ": " << (value ? "Yes" : "No") << '\n';
   }

   template <typename... T>
   void printVersion(StringRef label, T... version)
   {
      startLine() << label << ": ";
      printVersionInternal(version...);
      getOutStream() << "\n";
   }

   template <typename T>
   void printList(StringRef label, const T &list)
   {
      startLine() << label << ": [";
      bool comma = false;
      for (const auto &item : list) {
         if (comma) {
            m_outstream << ", ";
         }
         m_outstream << item;
         comma = true;
      }
      m_outstream << "]\n";
   }

   template <typename T, typename U>
   void printList(StringRef label, const T &list, const U &printer)
   {
      startLine() << label << ": [";
      bool comma = false;
      for (const auto &item : list) {
         if (comma) {
            m_outstream << ", ";
         }
         printer(m_outstream, item);
         comma = true;
      }
      m_outstream << "]\n";
   }

   template <typename T>
   void printHexList(StringRef label, const T &list)
   {
      startLine() << label << ": [";
      bool comma = false;
      for (const auto &item : list) {
         if (comma) {
            m_outstream << ", ";
         }
         m_outstream << hex(item);
         comma = true;
      }
      m_outstream << "]\n";
   }

   template <typename T>
   void printHex(StringRef label, T value)
   {
      startLine() << label << ": " << hex(value) << "\n";
   }

   template <typename T>
   void printHex(StringRef label, StringRef str, T value)
   {
      startLine() << label << ": " << str << " (" << hex(value) << ")\n";
   }

   template <typename T>
   void printSymbolOffset(StringRef label, StringRef symbol, T value)
   {
      startLine() << label << ": " << symbol << '+' << hex(value) << '\n';
   }

   void printString(StringRef value)
   {
      startLine() << value << "\n";
   }

   void printString(StringRef label, StringRef value)
   {
      startLine() << label << ": " << value << "\n";
   }

   void printString(StringRef label, const std::string &value)
   {
      startLine() << label << ": " << value << "\n";
   }

   void printString(StringRef label, const char *value)
   {
      printString(label, StringRef(value));
   }

   template <typename T>
   void printNumber(StringRef label, StringRef str, T value)
   {
      startLine() << label << ": " << str << " (" << value << ")\n";
   }

   void printBinary(StringRef label, StringRef str, ArrayRef<uint8_t> value)
   {
      printBinaryImpl(label, str, value, false);
   }

   void printBinary(StringRef label, StringRef str, ArrayRef<char> value)
   {
      auto ret = make_array_ref(reinterpret_cast<const uint8_t *>(value.getData()),
                                value.getSize());
      printBinaryImpl(label, str, ret, false);
   }

   void printBinary(StringRef label, ArrayRef<uint8_t> value)
   {
      printBinaryImpl(label, StringRef(), value, false);
   }

   void printBinary(StringRef label, ArrayRef<char> value)
   {
      auto ret = make_array_ref(reinterpret_cast<const uint8_t *>(value.getData()),
                                value.getSize());
      printBinaryImpl(label, StringRef(), ret, false);
   }

   void printBinary(StringRef label, StringRef value)
   {
      auto ret = make_array_ref(reinterpret_cast<const uint8_t *>(value.getData()),
                                value.getSize());
      printBinaryImpl(label, StringRef(), ret, false);
   }

   void printBinaryBlock(StringRef label, ArrayRef<uint8_t> value,
                         uint32_t startOffset)
   {
      printBinaryImpl(label, StringRef(), value, true, startOffset);
   }

   void printBinaryBlock(StringRef label, ArrayRef<uint8_t> value)
   {
      printBinaryImpl(label, StringRef(), value, true);
   }

   void printBinaryBlock(StringRef label, StringRef value)
   {
      auto ret = make_array_ref(reinterpret_cast<const uint8_t *>(value.getData()),
                                value.getSize());
      printBinaryImpl(label, StringRef(), ret, true);
   }

   template <typename T>
   void printObject(StringRef label, const T &value)
   {
      startLine() << label << ": " << value << "\n";
   }

   RawOutStream &startLine()
   {
      printIndent();
      return m_outstream;
   }

   RawOutStream &getOutStream()
   {
      return m_outstream;
   }

private:
   template <typename T>
   void printVersionInternal(T value)
   {
      getOutStream() << value;
   }

   template <typename S, typename T, typename... TArgs>
   void printVersionInternal(S value, T value2, TArgs... args)
   {
      getOutStream() << value << ".";
      printVersionInternal(value2, args...);
   }

   template <typename T>
   static bool flagName(const EnumEntry<T> &lhs, const EnumEntry<T> &rhs)
   {
      return lhs.m_name < rhs.m_name;
   }

   void printBinaryImpl(StringRef label, StringRef str, ArrayRef<uint8_t> value,
                        bool block, uint32_t startOffset = 0);

   RawOutStream &m_outstream;
   int m_indentLevel;
   StringRef m_prefix;
};

template <>
inline void
ScopedPrinter::printHex<ulittle16_t>(StringRef label,
                                     ulittle16_t value)
{
   startLine() << label << ": " << hex(value) << "\n";
}

template<char Open, char Close>
struct DelimitedScope
{
   explicit DelimitedScope(ScopedPrinter &printer) : m_printer(printer)
   {
      m_printer.startLine() << Open << '\n';
      m_printer.indent();
   }

   DelimitedScope(ScopedPrinter &printer, StringRef str) : m_printer(printer)
   {
      m_printer.startLine() << str;
      if (!str.empty()) {
         m_printer.getOutStream() << ' ';
      }
      m_printer.getOutStream() << Open << '\n';
      m_printer.indent();
   }

   ~DelimitedScope()
   {
      m_printer.unindent();
      m_printer.startLine() << Close << '\n';
   }

   ScopedPrinter &m_printer;
};

using DictScope = DelimitedScope<'{', '}'>;
using ListScope = DelimitedScope<'[', ']'>;

} // utils
} // polar

#endif // POLARPHP_UTILS_SCOPED_PRINTER_H
