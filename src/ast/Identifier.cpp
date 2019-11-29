//===--- Identifier.cpp - Uniqued Identifier ------------------------------===//
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
// Created by polarboy on 2019/05/09.
//===----------------------------------------------------------------------===//
//
// This file implements the Identifier interface.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/Identifier.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ConvertUTF.h"
#include "clang/Basic/CharInfo.h"

namespace polar::ast {

void *DeclBaseName::sm_subscriptIdentifierData = &DeclBaseName::sm_subscriptIdentifierData;
void *DeclBaseName::sm_constructorIdentifierData = &DeclBaseName::sm_constructorIdentifierData;
void *DeclBaseName::sm_destructorIdentifierData = &DeclBaseName::sm_destructorIdentifierData;

} // polar::ast

namespace llvm {

using polar::ast::Identifier;
using polar::ast::DeclName;
using polar::ast::DeclBaseName;

raw_ostream &operator<<(raw_ostream &ostream, Identifier identifier)
{
   if (identifier.get() == nullptr) {
      return ostream << "_";
   }
   return ostream << identifier.get();
}

raw_ostream &operator<<(raw_ostream &ostream, DeclBaseName decl)
{
   return ostream << decl.userFacingName();
}

raw_ostream &operator<<(raw_ostream &ostream, DeclName decl)
{
   if (decl.isSimpleName()) {
      return ostream << decl.getBaseName();
   }
   ostream << decl.getBaseName() << "(";
   for (auto c : decl.getArgumentNames()) {
      ostream << c << ':';
   }
   ostream << ")";
   return ostream;
}

} // llvm

namespace polar::ast {

bool Identifier::isOperatorSlow() const
{
   StringRef data = str();
   auto *s = reinterpret_cast<llvm::UTF8 const *>(data.begin()),
         *end = reinterpret_cast<llvm::UTF8 const *>(data.end());
   llvm::UTF32 codePoint;
   llvm::ConversionResult res =
         llvm::convertUTF8Sequence(&s, end, &codePoint, llvm::strictConversion);
   assert(res == llvm::conversionOK && "invalid UTF-8 in identifier?!");
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
   llvm::errs() << *this << "\n";
}

StringRef DeclName::getString(llvm::SmallVectorImpl<char> &scratch,
                              bool skipEmptyArgumentNames) const
{
   {
      llvm::raw_svector_ostream out(scratch);
      print(out, skipEmptyArgumentNames);
   }

   return StringRef(scratch.data(), scratch.size());
}

llvm::raw_ostream &DeclName::print(llvm::raw_ostream &os,
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

llvm::raw_ostream &DeclName::printPretty(llvm::raw_ostream &os) const
{
   return print(os, /*skipEmptyArgumentNames=*/!isSpecial());
}

} // polar::ast
