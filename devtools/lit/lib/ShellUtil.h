// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/04.

#ifndef POLAR_DEVLTOOLS_LIT_SHELL_UTILS_H
#define POLAR_DEVLTOOLS_LIT_SHELL_UTILS_H

#include <vector>
#include <string>
#include <list>

namespace polar {
namespace lit {

class ShLexer
{
public:
   ShLexer(const std::vector<std::string> &data, bool win32Escapes = false);
   void eat();
   void look();
   bool maybeEat(char c);
   void lexArgFast();
   void lexArgSlow();
   void lexArgQuoted();
   void lexArgChecked();
   void lexArg();
   void lexOneToken();
   void lex();
protected:
   std::vector<char> m_data;
   int m_pos;
   int m_end;
   bool m_win32Escapes;
};

class ShParser
{
public:
   ShParser(const std::list<std::string> &data, bool win32Escapes = false);
   void lex();
   void look();
   void parseCommand();
   void parsePipeline();
   void parse();
protected:
   std::vector<std::string> m_data;
   bool m_win32Escapes;
   bool m_pipefail;
   std::list<std::string> m_tokens;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_SHELL_UTILS_H
