// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/03.

#include "BooleanExpression.h"

namespace polar {
namespace lit {

std::optional<bool> BooleanExpression::evaluate(const std::string &str, const std::set<std::string> variables,
                                                const std::string &triple)
{
   try {
      BooleanExpression parser(str, variables, triple);
      return parser.parseAll();
   } catch (ValueError &e) {
      throw ValueError(std::string(e.what()) + " \nin expression: " + str);
   }
}

std::optional<bool> BooleanExpression::parseAll()
{

}

std::list<std::string> BooleanExpression::tokenize(std::string str)
{
   std::list<std::string> tokens;
   while (true) {
      std::smatch match;
      if (std::regex_match(str, match, sm_pattern)) {
         if (match.empty()) {
            if (str.empty()) {
               tokens.push_back("END_PARSE_MARK");
               break;
            } else {
               throw ValueError(std::string("couldn't parse text: ") + str);
            }
         }
         std::string token = match[1];
         str = match[2];
         tokens.push_back(token);
      }
   }
   return tokens;
}

std::regex BooleanExpression::sm_pattern(R"(\A\s*([()]|[-+=._a-zA-Z0-9]+|&&|\|\||!)\s*(.*)\Z)");

} // lit
} // polar
