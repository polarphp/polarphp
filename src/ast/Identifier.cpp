//===--- Identifier.cpp - Uniqued Identifier ------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/05/05.
//===----------------------------------------------------------------------===//
//
// This file implements the Identifier interface.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/Identifier.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/utils/RawOutStream.h"
#include "polarphp/utils/ConvertUtf.h"

namespace polar::ast {

using polar::utils::RawOutStream;
using polar::utils::ConversionResult;
using polar::utils::ConversionFlags;
using polar::utils::RawSvectorOutStream;

void *DeclBaseName::sm_subscriptIdentifierData =
      &DeclBaseName::sm_subscriptIdentifierData;
void *DeclBaseName::sm_constructorIdentifierData =
      &DeclBaseName::sm_constructorIdentifierData;
void *DeclBaseName::sm_destructorIdentifierData =
      &DeclBaseName::sm_destructorIdentifierData;

RawOutStream &operator<<(RawOutStream &outStream, Identifier identifier)
{
   if (identifier.get() == nullptr) {
      return outStream << "_";
   }
   return outStream << identifier.get();
}

RawOutStream &operator<<(RawOutStream &outStream, DeclBaseName name)
{
   return outStream << name.userFacingName();
}

RawOutStream &operator<<(RawOutStream &outStream, DeclName name)
{
   if (name.isSimpleName()) {
      return outStream << name.getBaseName();
   }
   outStream << name.getBaseName() << "(";
   for (auto c : name.getArgumentNames()) {
      outStream << c << ':';
   }
   outStream << ")";
   return outStream;
}

bool Identifier::isOperatorSlow() const
{
   StringRef data = str();
   auto *s = reinterpret_cast<polar::utils::Utf8 const *>(data.begin()),
         *end = reinterpret_cast<polar::utils::Utf8 const *>(data.end());
   polar::utils::Utf32 codePoint;
   polar::utils::ConversionResult res =
         polar::utils::convert_utf8_sequence(&s, end, &codePoint, ConversionFlags::StrictConversion);
   assert(res == ConversionResult::ConversionOK && "invalid UTF-8 in identifier?!");
   (void)res;
   return !empty() && isOperatorStartCodePoint(codePoint);
}

int Identifier::compare(Identifier other) const
{
   // Handle empty identifiers.
   if (empty() || other.empty()) {
      if (empty() != other.empty()) {
         return other.empty() ? -1 : 1;
      }
      return 0;
   }
   return str().compare(other.str());
}

int DeclName::compare(DeclName other) const
{
   // Compare base names.
   if (int result = getBaseName().compare(other.getBaseName())) {
      return result;
   }
   // Compare argument names.
   auto argNames = getArgumentNames();
   auto otherArgNames = other.getArgumentNames();
   for (unsigned i = 0, n = std::min(argNames.size(), otherArgNames.size());
        i != n; ++i) {
      if (int result = argNames[i].compare(otherArgNames[i])) {
         return result;
      }
   }
   if (argNames.size() == otherArgNames.size()) {
      return 0;
   }
   return argNames.size() < otherArgNames.size() ? -1 : 1;
}

static bool equals(ArrayRef<Identifier> idents, ArrayRef<StringRef> strings)
{
   if (idents.size() != strings.size()) {
      return false;
   }
   for (size_t i = 0, e = idents.size(); i != e; ++i) {
      if (!idents[i].is(strings[i])) {
         return false;
      }
   }
   return true;
}

bool DeclName::isCompoundName(DeclBaseName baseName,
                              ArrayRef<StringRef> argNames) const
{
   return (isCompoundName() &&
           getBaseName() == baseName &&
           equals(getArgumentNames(), argNames));
}

bool DeclName::isCompoundName(StringRef baseName,
                              ArrayRef<StringRef> argNames) const
{
   return (isCompoundName() &&
           getBaseName() == baseName &&
           equals(getArgumentNames(), argNames));
}

void DeclName::dump() const
{
   polar::utils::error_stream() << *this << "\n";
}

StringRef DeclName::getString(SmallVectorImpl<char> &scratch,
                              bool skipEmptyArgumentNames) const
{
   {
      RawSvectorOutStream out(scratch);
      print(out, skipEmptyArgumentNames);
   }

   return StringRef(scratch.data(), scratch.size());
}

RawOutStream &DeclName::print(RawOutStream &os,
                              bool skipEmptyArgumentNames) const
{
   // Print the base name.
   os << getBaseName();

   // If this is a simple name, we're done.
   if (isSimpleName()) {
      return os;
   }
   if (skipEmptyArgumentNames) {
      // If there is more than one argument yet none of them have names,
      // we're done.
      if (!getArgumentNames().empty()) {
         bool anyNonEmptyNames = false;
         for (auto c : getArgumentNames()) {
            if (!c.empty()) {
               anyNonEmptyNames = true;
               break;
            }
         }
         if (!anyNonEmptyNames) {
            return os;
         }
      }
   }

   // Print the argument names.
   os << "(";
   for (auto c : getArgumentNames()) {
      os << c << ':';
   }
   os << ")";
   return os;

}

RawOutStream &DeclName::printPretty(RawOutStream &os) const
{
   return print(os, /*skipEmptyArgumentNames=*/!isSpecial());
}

} // polar::ast
