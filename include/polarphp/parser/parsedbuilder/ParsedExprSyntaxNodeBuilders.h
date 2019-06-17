// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/17.

#ifndef POLARPHP_PARSER_PARSED_EXPR_SYNTAX_NODE_BUILDERS_H
#define POLARPHP_PARSER_PARSED_EXPR_SYNTAX_NODE_BUILDERS_H

#include "polarphp/parser/ParsedRawSyntaxNode.h"
#include "polarphp/parser/ParsedSyntaxNodes.h"
#include "polarphp/parser/SyntaxParsingContext.h"
#include "polarphp/syntax/syntaxnode/ExprSyntaxNodes.h"

namespace polar::parser {

using namespace polar::syntax;
using polar::basic::SmallVector;

class ParsedRawSyntaxRecorder;
class SyntaxParsingContext;

class ParsedNullExprSyntaxBuilder
{
public:
   using Cursor = NullExprSyntax::Cursor;
public:
   explicit ParsedNullExprSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}

   ParsedNullExprSyntaxBuilder &useNullKeyword(ParsedTokenSyntax nullKeyword);

   ParsedNullExprSyntax build();
   ParsedNullExprSyntax makeDeferred();
private:
   ParsedNullExprSyntax record();
   void finishLayout(bool deferred);

   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[NullExprSyntax::CHILDREN_COUNT];
};

class ParsedClassRefParentExprSyntaxBuilder
{
public:
   using Cursor = ClassRefParentExprSyntax::Cursor;
public:
   explicit ParsedClassRefParentExprSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}

   ParsedClassRefParentExprSyntaxBuilder &useParentKeyword(ParsedTokenSyntax parentKeyword);

   ParsedClassRefParentExprSyntax build();
   ParsedClassRefParentExprSyntax makeDeferred();
private:
   ParsedClassRefParentExprSyntax record();
   void finishLayout(bool deferred);

   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[ClassRefParentExprSyntax::CHILDREN_COUNT];
};

class ParsedClassRefSelfExprSyntaxBuilder
{
public:
   using Cursor = ClassRefSelfExprSyntax::Cursor;
public:
   ParsedClassRefSelfExprSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}

   ParsedClassRefSelfExprSyntaxBuilder &useSelfKeyword(ParsedTokenSyntax selfKeyword);

   ParsedClassRefSelfExprSyntax build();
   ParsedClassRefSelfExprSyntax makeDeferred();

private:
   ParsedClassRefSelfExprSyntax record();
   void finishLayout(bool deferred);
   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[ClassRefSelfExprSyntax::CHILDREN_COUNT];
};

class ParsedClassRefStaticExprSyntaxBuilder
{
public:
   using Cursor = ClassRefStaticExprSyntax::Cursor;
public:
   ParsedClassRefStaticExprSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}

   ParsedClassRefStaticExprSyntaxBuilder &useStaticKeyword(ParsedTokenSyntax staticKeyword);

   ParsedClassRefStaticExprSyntax build();
   ParsedClassRefStaticExprSyntax makeDeferred();

private:
   ParsedClassRefStaticExprSyntax record();
   void finishLayout(bool deferred);
   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[ClassRefStaticExprSyntax::CHILDREN_COUNT];
};

class ParsedIntegerLiteralExprSyntaxBuilder
{
public:
   using Cursor = IntegerLiteralExprSyntax::Cursor;
public:
   ParsedIntegerLiteralExprSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}
   ParsedIntegerLiteralExprSyntaxBuilder &useDigits(ParsedTokenSyntax digits);
   ParsedIntegerLiteralExprSyntax build();
   ParsedIntegerLiteralExprSyntax makeDeferred();
private:
   ParsedIntegerLiteralExprSyntax record();
   void finishLayout(bool deferred);
   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[IntegerLiteralExprSyntax::CHILDREN_COUNT];
};

class ParsedFloatLiteralExprSyntaxBuilder
{
public:
   using Cursor = FloatLiteralExprSyntax::Cursor;
public:
   ParsedFloatLiteralExprSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}
   ParsedFloatLiteralExprSyntaxBuilder &useDigits(ParsedTokenSyntax floatDigits);
   ParsedFloatLiteralExprSyntax build();
   ParsedFloatLiteralExprSyntax makeDeferred();
private:
   ParsedFloatLiteralExprSyntax record();
   void finishLayout(bool deferred);
   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[FloatLiteralExprSyntax::CHILDREN_COUNT];
};

class ParsedStringLiteralExprSyntaxBuilder
{
public:
   using Cursor = StringLiteralExprSyntax::Cursor;
public:
   ParsedStringLiteralExprSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}
   ParsedStringLiteralExprSyntaxBuilder &useString(ParsedTokenSyntax str);
   ParsedStringLiteralExprSyntax build();
   ParsedStringLiteralExprSyntax makeDeferred();
private:
   ParsedStringLiteralExprSyntax record();
   void finishLayout(bool deferred);
   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[StringLiteralExprSyntax::CHILDREN_COUNT];
};

class ParsedBooleanLiteralExprSyntaxBuilder
{
public:
   using Cursor = BooleanLiteralExprSyntax::Cursor;
public:
   ParsedBooleanLiteralExprSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}
   ParsedBooleanLiteralExprSyntaxBuilder &useBoolean(ParsedTokenSyntax booleanToken);
   ParsedBooleanLiteralExprSyntax build();
   ParsedBooleanLiteralExprSyntax makeDeferred();
private:
   ParsedBooleanLiteralExprSyntax record();
   void finishLayout(bool deferred);
   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[BooleanLiteralExprSyntax::CHILDREN_COUNT];
};

class ParsedTernaryExprSyntaxBuilder
{
public:
   using Cursor = TernaryExprSyntax::Cursor;
public:
   ParsedTernaryExprSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}

   ParsedTernaryExprSyntaxBuilder &useConditionExpr(ParsedExprSyntax conditionExpr);
   ParsedTernaryExprSyntaxBuilder &useQuestionMark(ParsedTokenSyntax questionMark);
   ParsedTernaryExprSyntaxBuilder &useFirstChoice(ParsedExprSyntax firstChoice);
   ParsedTernaryExprSyntaxBuilder &useColonMark(ParsedTokenSyntax colonMark);
   ParsedTernaryExprSyntaxBuilder &useSecondChoice(ParsedExprSyntax secondChoice);

   ParsedTernaryExprSyntax build();
   ParsedTernaryExprSyntax makeDeferred();
private:
   ParsedTernaryExprSyntax record();
   void finishLayout(bool deferred);
   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[TernaryExprSyntax::CHILDREN_COUNT];
};



} // polar::parser

#endif // POLARPHP_PARSER_PARSED_EXPR_SYNTAX_NODE_BUILDERS_H
