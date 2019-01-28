// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/03.

#ifndef POLAR_DEVLTOOLS_LIT_BOOLEAN_EXPRESSION_H
#define POLAR_DEVLTOOLS_LIT_BOOLEAN_EXPRESSION_H

#include <string>
#include <boost/regex.hpp>
#include <list>
#include <optional>

namespace polar {
namespace lit {

// A simple evaluator of boolean expressions.
//
// Grammar:
//   expr       :: or_expr
//   or_expr    :: and_expr ('||' and_expr)*
//   and_expr   :: not_expr ('&&' not_expr)*
//   not_expr   :: '!' not_expr
//                 '(' or_expr ')'
//                 identifier
//   identifier :: [-+=._a-zA-Z0-9]+

// Evaluates `string` as a boolean expression.
// Returns True or False. Throws a ValueError on syntax error.
//
// Variables in `variables` are true.
// Substrings of `triple` are true.
// 'true' is true.
// All other identifiers are false.

class BooleanExpression
{
public:
   BooleanExpression(const std::string &str, const std::vector<std::string> &variables,
                     const std::string &triple = "")
      : m_variables(variables),
        m_triple (triple),
        m_token(std::nullopt),
        m_value(std::nullopt)
   {
      m_tokens = tokenize(str);
      m_variables.push_back("true");
      m_tokenIterator = m_tokens.begin();
   }

   static std::string quote(const std::string &token);
   bool accept(const std::string &token);
   void expect(const std::string &token);
   bool isIdentifier(const std::string &token);
   BooleanExpression &parseNOT();
   BooleanExpression &parseAND();
   BooleanExpression &parseOR();
   std::optional<bool> parseAll();
public:
   static std::optional<bool> evaluate(const std::string &str, const std::vector<std::string> variables,
                                       const std::string &triple = "");
   static std::list<std::string> tokenize(std::string str);
protected:
   bool getParsedValue();
protected:
   static boost::regex sm_pattern;
   std::list<std::string> m_tokens;
   std::list<std::string>::iterator m_tokenIterator;
   std::vector<std::string> m_variables;
   std::string m_triple;
   std::optional<std::string> m_token;
   std::optional<bool> m_value;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_BOOLEAN_EXPRESSION_H
