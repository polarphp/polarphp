//===--- JSONSerialization.cpp - JSON serialization support ---------------===//
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
// Created by polarboy on 2019/11/30.

#include "polarphp/basic/JSONSerialization.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Format.h"

namespace polar::json {

using namespace llvm;

unsigned Output::beginArray()
{
   m_stateStack.push_back(ArrayFirstValue);
   m_stream << '[';
   return 0;
}

bool Output::preflightElement(unsigned, void *&)
{
   if (m_stateStack.back() != ArrayFirstValue) {
      assert(m_stateStack.back() == ArrayOtherValue && "We must be in a sequence!");
      m_stream << ',';
   }
   if (m_prettyPrint) {
      m_stream << '\n';
      indent();
   }
   return true;
}

void Output::postflightElement(void*)
{
   if (m_stateStack.back() == ArrayFirstValue) {
      m_stateStack.pop_back();
      m_stateStack.push_back(ArrayOtherValue);
   }
}

void Output::endArray()
{
   bool HadContent = m_stateStack.back() != ArrayFirstValue;
   m_stateStack.pop_back();
   if (m_prettyPrint && HadContent) {
      m_stream << '\n';
      indent();
   }
   m_stream << ']';
}

bool Output::canElideEmptyArray()
{
   if (m_stateStack.size() < 2) {
      return true;
   }

   if (m_stateStack.back() != ObjectFirstKey) {
      return true;
   }

   State checkedState = m_stateStack[m_stateStack.size() - 2];
   return (checkedState != ArrayFirstValue && checkedState != ArrayOtherValue);
}

void Output::beginObject()
{
   m_stateStack.push_back(ObjectFirstKey);
   m_stream << "{";
}

void Output::endObject()
{
   bool HadContent = m_stateStack.back() != ObjectFirstKey;
   m_stateStack.pop_back();
   if (m_prettyPrint && HadContent) {
      m_stream << '\n';
      indent();
   }
   m_stream << "}";
}

bool Output::preflightKey(StringRef Key, bool Required, bool SameAsDefault,
                          bool &UseDefault, void *&)
{
   UseDefault = false;
   if (Required || !SameAsDefault) {
      if (m_stateStack.back() != ObjectFirstKey) {
         assert(m_stateStack.back() == ObjectOtherKey && "We must be in an object!");
         m_stream << ',';
      }
      if (m_prettyPrint) {
         m_stream << '\n';
         indent();
      }
      m_stream << '"' << Key << "\":";
      if (m_prettyPrint) {
         m_stream << ' ';
      }
      return true;
   }
   return false;
}

void Output::postflightKey(void*)
{
   if (m_stateStack.back() == ObjectFirstKey) {
      m_stateStack.pop_back();
      m_stateStack.push_back(ObjectOtherKey);
   }
}

void Output::beginEnumScalar()
{
   m_enumerationMatchFound = false;
}

bool Output::matchEnumScalar(const char *Str, bool match)
{
   if (match && !m_enumerationMatchFound) {
      StringRef StrRef(Str);
      scalarString(StrRef, true);
      m_enumerationMatchFound = true;
   }
   return false;
}

void Output::endEnumScalar()
{
   if (!m_enumerationMatchFound) {
      llvm_unreachable("bad runtime enum value");
   }
}

bool Output::beginBitSetScalar(bool &doClear)
{
   m_stream << '[';
   if (m_prettyPrint) {
      m_stream << ' ';
   }
   m_needBitValueComma = false;
   doClear = false;
   return true;
}

bool Output::bitSetMatch(const char *Str, bool matches)
{
   if (matches) {
      if (m_needBitValueComma) {
         m_stream << ',';
         if (m_prettyPrint)
            m_stream << ' ';
      }
      StringRef StrRef(Str);
      scalarString(StrRef, true);
   }
   return false;
}

void Output::endBitSetScalar()
{
   if (m_prettyPrint) {
      m_stream << ' ';
   }
   m_stream << ']';
}

void Output::scalarString(StringRef &str, bool mustQuote)
{
   if (mustQuote) {
      m_stream << '"';
      for (unsigned char c : str) {
         // According to the JSON standard, the following characters must be
         // escaped:
         //   - Quotation mark (U+0022)
         //   - Reverse solidus (U+005C)
         //   - Control characters (U+0000 to U+001F)
         // We need to check for these and escape them if present.
         //
         // Since these are represented by a single byte in UTF8 (and will not be
         // present in any multi-byte UTF8 representations), we can just switch on
         // the value of the current byte.
         //
         // Any other bytes present in the string should therefore be emitted
         // as-is, without any escaping.
         switch (c) {
         // First, check for characters for which JSON has custom escape sequences.
         case '"':
            m_stream << '\\' << '"';
            break;
         case '\\':
            m_stream << '\\' << '\\';
            break;
         case '/':
            m_stream << '\\' << '/';
            break;
         case '\b':
            m_stream << '\\' << 'b';
            break;
         case '\f':
            m_stream << '\\' << 'f';
            break;
         case '\n':
            m_stream << '\\' << 'n';
            break;
         case '\r':
            m_stream << '\\' << 'r';
            break;
         case '\t':
            m_stream << '\\' << 't';
            break;
         default:
            // Otherwise, check to see if the current byte is a control character.
            if (c <= '\x1F') {
               // Since we have a control character, we need to escape it using
               // JSON's only valid escape sequence: \uxxxx (where x is a hex digit).

               // The upper two digits for control characters are always 00.
               m_stream << "\\u00";

               // Convert the current character into hexadecimal digits.
               m_stream << llvm::hexdigit((c >> 4) & 0xF);
               m_stream << llvm::hexdigit((c >> 0) & 0xF);
            } else {
               // This isn't a control character, so we don't need to escape it.
               // As a result, emit it directly; if it's part of a multi-byte UTF8
               // representation, all bytes will be emitted in this fashion.
               m_stream << c;
            }
            break;
         }
      }
      m_stream << '"';
   } else {
      m_stream << str;
   }
}

void Output::null()
{
   m_stream << "null";
}

void Output::indent()
{
   m_stream.indent(m_stateStack.size() * 2);
}

//===----------------------------------------------------------------------===//
//  traits for built-in types
//===----------------------------------------------------------------------===//

StringRef ScalarReferenceTraits<bool>::stringRef(const bool &value)
{
   return (value ? "true" : "false");
}

StringRef ScalarReferenceTraits<StringRef>::stringRef(const StringRef &value)
{
   return value;
}

StringRef
ScalarReferenceTraits<std::string>::stringRef(const std::string &value)
{
   return value;
}

void ScalarTraits<uint8_t>::output(const uint8_t &value,
                                   raw_ostream &out)
{
   // use temp uin32_t because ostream thinks uint8_t is a character
   uint32_t num = value;
   out << num;
}

void ScalarTraits<uint16_t>::output(const uint16_t &value,
                                    raw_ostream &out)
{
   out << value;
}

void ScalarTraits<uint32_t>::output(const uint32_t &value,
                                    raw_ostream &out)
{
   out << value;
}

#if defined(_MSC_VER)
void ScalarTraits<unsigned long>::output(const unsigned long &value,
                                         raw_ostream &out)
{
   out << value;
}
#endif

void ScalarTraits<uint64_t>::output(const uint64_t &value,
                                    raw_ostream &out)
{
   out << value;
}

void ScalarTraits<int8_t>::output(const int8_t &value, raw_ostream &out)
{
   // use temp in32_t because ostream thinks int8_t is a character
   int32_t num = value;
   out << num;
}

void ScalarTraits<int16_t>::output(const int16_t &value,
                                   raw_ostream &out)
{
   out << value;
}

void ScalarTraits<int32_t>::output(const int32_t &value,
                                   raw_ostream &out)
{
   out << value;
}

void ScalarTraits<int64_t>::output(const int64_t &value,
                                   raw_ostream &out)
{
   out << value;
}

void ScalarTraits<double>::output(const double &value, raw_ostream &out)
{
   out << llvm::format("%g", value);
}

void ScalarTraits<float>::output(const float &value, raw_ostream &out)
{
   out << llvm::format("%g", value);
}

} // polar::json
