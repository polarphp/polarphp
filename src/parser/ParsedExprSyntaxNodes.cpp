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

///
/// ParsedIntegerLiteralExprSyntax
///
ParsedTokenSyntax ParsedIntegerLiteralExprSyntax::getDeferredDigits()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[IntegerLiteralExprSyntax::Cursor::Digits]};
}

///
/// ParsedFloatLiteralExprSyntax
///
ParsedTokenSyntax ParsedFloatLiteralExprSyntax::getDeferredFloatDigits()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[FloatLiteralExprSyntax::Cursor::FloatDigits]};
}

///
/// ParsedStringLiteralExprSyntax
///
ParsedTokenSyntax ParsedStringLiteralExprSyntax::getDeferredString()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[StringLiteralExprSyntax::Cursor::String]};
}

///
/// ParsedBooleanLiteralExprSyntax
///
ParsedTokenSyntax ParsedBooleanLiteralExprSyntax::getDeferredBoolean()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[BooleanLiteralExprSyntax::Cursor::Boolean]};
}

///
/// ParsedTernaryExprSyntax
///
ParsedTokenSyntax ParsedTernaryExprSyntax::getDeferredConditionExpr()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[TernaryExprSyntax::Cursor::ConditionExpr]};
}

ParsedTokenSyntax ParsedTernaryExprSyntax::getDeferredQuestionMark()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[TernaryExprSyntax::Cursor::QuestionMark]};
}

ParsedTokenSyntax ParsedTernaryExprSyntax::getDeferredFirstChoice()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[TernaryExprSyntax::Cursor::FirstChoice]};
}

ParsedTokenSyntax ParsedTernaryExprSyntax::getDeferredColonMark()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[TernaryExprSyntax::Cursor::ColonMark]};
}

ParsedTokenSyntax ParsedTernaryExprSyntax::getDeferredSecondChoice()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[TernaryExprSyntax::Cursor::SecondChoice]};
}

///
/// ParsedAssignmentExprSyntax
///
ParsedTokenSyntax ParsedAssignmentExprSyntax::getDeferredAssignToken()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[AssignmentExprSyntax::Cursor::AssignToken]};
}

///
/// ParsedSequenceExprSyntax
///
ParsedTokenSyntax ParsedSequenceExprSyntax::getDeferredElements()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[SequenceExprSyntax::Cursor::Elements]};
}

///
/// ParsedPrefixOperatorExprSyntax
///
std::optional<ParsedTokenSyntax> ParsedPrefixOperatorExprSyntax::getDeferredOperatorToken()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[PrefixOperatorExprSyntax::Cursor::OperatorToken];
   if (!rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

ParsedSyntax ParsedPrefixOperatorExprSyntax::getDeferredExpr()
{
   return ParsedSyntax{getRaw().getDeferredChildren()[PrefixOperatorExprSyntax::Cursor::Expr]};
}

///
/// ParsedPostfixOperatorExprSyntax
///
ParsedSyntax ParsedPostfixOperatorExprSyntax::getDeferredExpr()
{
   return ParsedSyntax{getRaw().getDeferredChildren()[PostfixOperatorExprSyntax::Cursor::Expr]};
}

ParsedTokenSyntax ParsedPostfixOperatorExprSyntax::getDeferredOperatorToken()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[PostfixOperatorExprSyntax::Cursor::OperatorToken]};
}

///
/// ParsedBinaryOperatorExprSyntax
///
ParsedTokenSyntax ParsedBinaryOperatorExprSyntax::getDeferredOperatorToken()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[BinaryOperatorExprSyntax::Cursor::OperatorToken]};
}

} // polar::parser
