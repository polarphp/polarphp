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

#include "BooleanExpression.h"
#include "LitGlobal.h"
#include <iostream>

namespace polar {
namespace lit {

#define LIT_BOOL_PARSE_END_MARK "END_PARSE_MARK"

boost::regex BooleanExpression::sm_pattern(R"(^\s*([()]|[-+=._a-zA-Z0-9]+|&&|\|\||!)\s*(.*)$)");

std::string BooleanExpression::quote(const std::string &token)
{
   if (token == LIT_BOOL_PARSE_END_MARK) {
      return "<end of expression>";
   } else {
      return "'"+token+"'";
   }
}

bool BooleanExpression::accept(const std::string &token)
{
   if (m_token == token) {
      if (m_tokenIterator != m_tokens.end()) {
         m_token = *m_tokenIterator++;
      }
      return true;
   }
   return false;
}

void BooleanExpression::expect(const std::string &token)
{
   if (m_token == token) {
      if (m_token != LIT_BOOL_PARSE_END_MARK) {
         if (m_tokenIterator != m_tokens.end()) {
            m_token = *m_tokenIterator++;
         }
      }
   } else {
      throw ValueError(std::string("expected: ") + quote(token) + "\nhave: "+ quote(m_token.value()));
   }
}

bool BooleanExpression::isIdentifier(const std::string &token)
{
   if (token == LIT_BOOL_PARSE_END_MARK || token == "&&" || token == "||" ||
       token == "!" || token == "(" || token == ")") {
      return false;
   }
   return true;
}

BooleanExpression &BooleanExpression::parseNOT()
{
   if (accept("!")) {
      parseNOT();
      m_value = !getParsedValue();
   } else if (accept("(")) {
      parseOR();
      expect(")");
   } else if (!isIdentifier(m_token.value())) {
      throw ValueError(std::string("expected: '!' or '(' or identifier\nhave: "+ quote(m_token.value())));
   } else {
      m_value = ( std::find(m_variables.begin(), m_variables.end(), m_token.value()) != m_variables.end()
            || m_triple.find(m_token.value()) != std::string::npos);
      if (m_tokenIterator == m_tokens.end()) {
         m_token = std::nullopt;
      } else {
         m_token = *m_tokenIterator++;
      }
   }
   return *this;
}

BooleanExpression &BooleanExpression::parseAND()
{
   parseNOT();
   while (accept("&&")) {
      bool left = getParsedValue();
      parseNOT();
      bool right = getParsedValue();
      // this is technically the wrong associativity, but it
      // doesn't matter for this limited expression grammar
      m_value = left && right;
   }
   return *this;
}

BooleanExpression &BooleanExpression::parseOR()
{
   parseAND();
   while (accept("||")) {
      bool left = getParsedValue();
      parseAND();
      bool right = getParsedValue();
      // this is technically the wrong associativity, but it
      // doesn't matter for this limited expression grammar
      m_value = left || right;
   }
   return *this;
}

std::optional<bool> BooleanExpression::evaluate(const std::string &str, const std::vector<std::string> variables,
                                                const std::string &triple)
{
   try {
      BooleanExpression parser(str, variables, triple);
      return parser.parseAll();
   } catch (ValueError &e) {
      throw ValueError(std::string(e.what()) + "\nin expression: " + quote(str));
   }
}

std::optional<bool> BooleanExpression::parseAll()
{
   if (m_tokenIterator != m_tokens.end()) {
      m_token = *m_tokenIterator++;
   } else {
      m_token = std::nullopt;
   }
   parseOR();
   expect(LIT_BOOL_PARSE_END_MARK);
   return m_value;
}

std::list<std::string> BooleanExpression::tokenize(std::string str)
{
   std::list<std::string> tokens;
   while (true) {
      boost::smatch match;
      if (boost::regex_match(str, match, sm_pattern)) {
         std::string token = match[1];
         str = match[2];
         tokens.push_back(token);
      } else {
         if (str.empty()) {
            tokens.push_back(LIT_BOOL_PARSE_END_MARK);
            break;
         } else {
            throw ValueError(std::string("couldn't parse text: ") + quote(str));
         }
      }
   }
   return tokens;
}

bool BooleanExpression::getParsedValue()
{
   return !m_value.has_value() ? false : m_value.value();
}

} // lit
} // polar
