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

#include "polarphp/parser/parsedsyntaxnode/ParsedStmtSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/StmtSyntaxNodes.h"

namespace polar::parser {

using namespace polar::syntax;

ParsedSyntax ParsedConditionElementSyntax::getDeferredCondition()
{
   return ParsedSyntax{getRaw().getDeferredChildren()[ConditionElementSyntax::Cursor::Condition]};
}

std::optional<ParsedTokenSyntax> ParsedConditionElementSyntax::getDeferredTrailingComma()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[ConditionElementSyntax::Cursor::TrailingComma];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

///
/// ParsedContinueStmtSyntax
///
ParsedTokenSyntax ParsedContinueStmtSyntax::getDeferredContinueKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[ContinueStmtSyntax::Cursor::ContinueKeyword]};
}

std::optional<ParsedTokenSyntax> ParsedContinueStmtSyntax::getDeferredLNumberToken()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[ContinueStmtSyntax::Cursor::LNumberToken];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

///
/// ParsedBreakStmtSyntax
///
ParsedTokenSyntax ParsedBreakStmtSyntax::getDeferredBreakKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[BreakStmtSyntax::Cursor::BreakKeyword]};
}

std::optional<ParsedTokenSyntax> ParsedBreakStmtSyntax::getDeferredLNumberToken()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[BreakStmtSyntax::Cursor::LNumberToken];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

///
/// ParsedFallthroughStmtSyntax
///
ParsedTokenSyntax ParsedFallthroughStmtSyntax::getDeferredFallthroughKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[FallthroughStmtSyntax::Cursor::FallthroughKeyword]};
}

///
/// ParsedElseIfClauseSyntax
///
ParsedTokenSyntax ParsedElseIfClauseSyntax::getDeferredElseIfKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[ElseIfClauseSyntax::Cursor::ElseIfKeyword]};
}

ParsedTokenSyntax ParsedElseIfClauseSyntax::getDeferredLeftParen()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[ElseIfClauseSyntax::Cursor::LeftParen]};
}

ParsedExprSyntax ParsedElseIfClauseSyntax::getDeferredCondition()
{
   return ParsedExprSyntax{getRaw().getDeferredChildren()[ElseIfClauseSyntax::Cursor::Condition]};
}

ParsedTokenSyntax ParsedElseIfClauseSyntax::getDeferredRightParen()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[ElseIfClauseSyntax::Cursor::RightParen]};
}

ParsedCodeBlockSyntax ParsedElseIfClauseSyntax::getDeferredBody()
{
   return ParsedCodeBlockSyntax{getRaw().getDeferredChildren()[ElseIfClauseSyntax::Cursor::Body]};
}

} // polar::parser
