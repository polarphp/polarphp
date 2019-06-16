//===--- SyntaxParserResult.h - Syntax Parser result Wrapper ---*- C++ -*-===//
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
// Created by polarboy on 2019/06/15.

#ifndef POLARPHP_PARSER_SYNTAX_PARSE_RESULT_H
#define POLARPHP_PARSER_SYNTAX_PARSE_RESULT_H

#include "polarphp/parser/ParserResult.h"
#include <optional>

namespace polar::parser {

template <typename Syntax, typename AST>
class SyntaxParserResult
{
public:
   SyntaxParserResult(std::nullptr_t = nullptr)
      : m_syntaxNode(None),
        m_astResult(nullptr)
   {}

   SyntaxParserResult(ParserStatus status)
      : m_syntaxNode(None),
        m_astResult(status)
   {}

   SyntaxParserResult(std::optional<Syntax> syntaxNode, AST *astNode)
      : m_syntaxNode(syntaxNode),
        m_astResult(astNode)
   {}

   SyntaxParserResult(ParserStatus status, llvm::Optional<Syntax> m_syntaxNode,
                      AST *astNode)
      : m_syntaxNode(m_syntaxNode),
        m_astResult(make_parser_result(status, astNode))
   {}

   /// Convert from a different but compatible parser result.
   template <typename U, typename Enabler = typename std::enable_if<
                std::is_base_of<AST, U>::value>::type>
   SyntaxParserResult(SyntaxParserResult<Syntax, U> other)
      : m_syntaxNode(other.m_syntaxNode),
        m_astResult(other.m_astResult)
   {}

   bool isNull() const
   {
      return m_astResult.isNull();
   }

   bool isNonNull() const
   {
      return m_astResult.isNonNull();
   }

   bool isParseError() const
   {
      return m_astResult.isParseError();
   }

   bool hasCodeCompletion() const
   {
      return m_astResult.hasCodeCompletion();
   }

   void setIsParseError()
   {
      return m_astResult.setIsParserError();
   }

   void setHasCodeCompletion()
   {
      return m_astResult.setHasCodeCompletion();
   }

   const ParserResult<AST> &getAstResult()
   {
      return m_astResult;
   }

   AST *getAst() const
   {
      return m_astResult.get();
   }

   bool hasSyntax() const
   {
      return m_syntaxNode.hasValue();
   }

   Syntax getSyntax() const
   {
      assert(m_syntaxNode.hasValue() && "getSyntax from None value");
      return *m_syntaxNode;
   }

   SyntaxParserResult<Syntax, AST> &
   operator=(SyntaxParserResult<Syntax, AST> R)
   {
      std::swap(*this, R);
      return *this;
   }

private:
   std::optional<Syntax> m_syntaxNode;
   ParserResult<AST> m_astResult;
   template <typename T, typename U> friend class SyntaxParserResult;
};

namespace {
/// Create a successful parser result.
template <typename Syntax, typename AST>
static inline SyntaxParserResult<Syntax, AST>
make_syntax_result(std::optional<Syntax> syntaxNode, AST *astNode)
{
   return SyntaxParserResult<Syntax, AST>(syntaxNode, astNode);
}

/// Create a result with the specified status.
template <typename Syntax, typename AST>
static inline SyntaxParserResult<Syntax, AST>
make_syntax_result(ParserStatus status, std::optional<Syntax> syntaxNode,
                   AST *astNode)
{
   return SyntaxParserResult<Syntax, AST>(status, syntaxNode, astNode);
}

/// Create a result (null or non-null) with error and code completion bits set.
template <typename Syntax, typename AST>
static inline SyntaxParserResult<Syntax, AST>
make_syntax_code_completion_result(AST *result = nullptr)
{
   SyntaxParserResult<Syntax, AST> syntaxParseResult;
   if (result) {
      syntaxParseResult = SyntaxParserResult<Syntax, AST>(std::nullopt, result);
   }
   syntaxParseResult.setHasCodeCompletion();
   return syntaxParseResult;
}
} // anonymous namespace

} // polar::parser

#endif // POLARPHP_PARSER_SYNTAX_PARSE_RESULT_H
