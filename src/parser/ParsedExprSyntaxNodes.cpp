// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/16.

#include "polarphp/parser/parsedsyntaxnode/ParsedExprSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/ExprSyntaxNodes.h"

namespace polar::parser {

using namespace polar::syntax;

///
/// ParsedNullExprSyntax
///
ParsedTokenSyntax ParsedNullExprSyntax::getDeferredNullKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[NullExprSyntax::Cursor::NullKeyword]};
}

///
/// ParsedClassRefParentExprSyntax
///
ParsedTokenSyntax ParsedClassRefParentExprSyntax::getDeferredParentKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[ClassRefParentExprSyntax::Cursor::ParentKeyword]};
}
///
/// ParsedClassRefSelfExprSyntax
///
ParsedTokenSyntax ParsedClassRefSelfExprSyntax::getDeferredSelfKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[ClassRefSelfExprSyntax::Cursor::SelfKeyword]};
}
///
/// ParsedClassRefStaticExprSyntax
///
ParsedTokenSyntax ParsedClassRefStaticExprSyntax::getDeferredStaticKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[ClassRefStaticExprSyntax::Cursor::StaticKeyword]};
}
} // polar::parser
