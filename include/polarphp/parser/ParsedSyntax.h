//===--- ParsedSyntax.h - Base class for ParsedSyntax hierarchy -*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
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

#ifndef POLARPHP_PARSER_PARSED_SYNTAX_H
#define POLARPHP_PARSER_PARSED_SYNTAX_H

#include "polarphp/parser/ParsedRawSyntaxNode.h"
#include "polarphp/syntax/SyntaxKind.h"

#include <optional>

namespace polar::parser {

using polar::syntax::SyntaxKind;

class ParsedSyntax
{
public:
   explicit ParsedSyntax(ParsedRawSyntaxNode rawNode)
      : m_rawNode(std::move(rawNode))
   {}

   const ParsedRawSyntaxNode &getRaw() const
   {
      return m_rawNode;
   }

   syntax::SyntaxKind getKind() const
   {
      return m_rawNode.getKind();
   }

   /// Returns true if the syntax node is of the given type.
   template <typename T>
   bool is() const
   {
      return T::classOf(this);
   }

   /// Cast this Syntax node to a more specific type, asserting it's of the
   /// right kind.
   template <typename T>
   T castTo() const
   {
      assert(is<T>() && "castTo<T>() node of incompatible type!");
      return T { m_rawNode };
   }

   /// If this Syntax node is of the right kind, cast and return it,
   /// otherwise return None.
   template <typename T>
   std::optional<T> getAs() const
   {
      if (is<T>()) {
         return castTo<T>();
      }
      return std::nullopt;
   }

   static bool kindOf(syntax::SyntaxKind kind)
   {
      return true;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      // Trivially true.
      return true;
   }

private:
   ParsedRawSyntaxNode m_rawNode;
};

class ParsedTokenSyntax final : public ParsedSyntax
{
public:
   explicit ParsedTokenSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedSyntax(rawNode)
   {}

   TokenKindType getTokenKind() const
   {
      return getRaw().getTokenKind();
   }

   static bool kindOf(SyntaxKind kind)
   {
      return polar::syntax::is_token_kind(kind);
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

/// A generic unbounded collection of syntax nodes
template <SyntaxKind CollectionKind>
class ParsedSyntaxCollection : public ParsedSyntax
{

public:
   explicit ParsedSyntaxCollection(ParsedRawSyntaxNode rawNode)
      : ParsedSyntax(std::move(rawNode))
   {}

   static bool kindOf(syntax::SyntaxKind kind)
   {
      return kind == CollectionKind;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSED_SYNTAX_H
