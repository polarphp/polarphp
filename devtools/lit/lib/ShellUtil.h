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
#include "ShellCommands.h"
#include <any>

namespace polar {
namespace lit {

class ShLexer
{
public:
   ShLexer(const std::string &data, bool win32Escapes = false);
   char eat();
   char look();
   bool maybeEat(char c);
   std::any lexArgFast(char c);
   std::any lexArgSlow(char c);
   std::string lexArgQuoted(char delim);
   void lexArgChecked();
   std::any lexArg(char c);
   std::any lexOneToken();
   std::list<std::any> lex();
protected:
   std::string m_data;
   int m_pos;
   int m_end;
   bool m_win32Escapes;
};

class ShParser
{
public:
   ShParser(const std::string &data, bool win32Escapes = false, bool pipeFail = false);
   std::any lex();
   std::any look();
   Command parseCommand();
   Pipeline parsePipeline();
   Seq parse();
protected:
   std::string m_data;
   bool m_win32Escapes;
   bool m_pipeFail;
   std::list<std::any> m_tokens;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_SHELL_UTILS_H
