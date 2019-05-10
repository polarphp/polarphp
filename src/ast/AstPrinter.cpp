//===--- AstPrinter.cpp - Swift Language ast Printer ----------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
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
//  This file implements printing for the polarphp ASTs.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/AstPrinter.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/AstVisitor.h"
#include "polarphp/ast/Attr.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/NameLookup.h"
#include "polarphp/ast/ParameterList.h"
#include "polarphp/ast/PrintOptions.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/ast/TypeVisitor.h"
#include "polarphp/ast/TypeWalker.h"
#include "polarphp/ast/Types.h"
#include "polarphp/basic/Defer.h"
#include "polarphp/Basic/QuotedString.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/basic/adt/StringSwitch.h"
#include "polarphp/utils/ConvertUtf.h"
#include "polarphp/utils/SaveAndRestore.h"
#include "polarphp/utils/RawOutStream.h"
#include <algorithm>
#include <queue>

namespace polar::ast {

void PrintOptions::setBaseType(Type T)
{
}

void AstPrinter::anchor() {}

void AstPrinter::printIndent()
{}

void AstPrinter::printTextImpl(StringRef Text)
{
}

void AstPrinter::printEscapedStringLiteral(StringRef str)
{

}

void AstPrinter::printTypeRef(Type T, const TypeDecl *RefTo, Identifier Name)
{

}


void AstPrinter::callPrintDeclPre(const Decl *D,
                                  std::optional<BracketOptions> Bracket)
{
}

AstPrinter &AstPrinter::operator<<(unsigned long long N)
{
   return *this;
}

AstPrinter &AstPrinter::operator<<(UUID UU)
{
   return *this;
}

AstPrinter &AstPrinter::operator<<(DeclName name)
{
   return *this;
}

void StreamPrinter::printText(StringRef Text)
{
}

void Decl::print(RawOutStream &outStream) const
{
}

void Decl::print(RawOutStream &outStream, const PrintOptions &opts) const
{
   //   StreamPrinter printer(outStream);
   //   print(printer, opts);
}

bool Decl::print(AstPrinter &printer, const PrintOptions &opts) const
{
}

bool ShouldPrintChecker::shouldPrint(const Decl *decl,
                                     const PrintOptions &options)
{
   return true;
}

} // polar::ast
