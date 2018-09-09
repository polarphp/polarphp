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

#ifndef POLAR_DEVLTOOLS_LIT_SHELL_COMMANDS_H
#define POLAR_DEVLTOOLS_LIT_SHELL_COMMANDS_H

#include <list>
#include <string>
#include <any>
#include <memory>
#include <assert.h>

namespace polar {
namespace lit {

using TokenType = std::tuple<std::any, std::any>;
using TokenTypePointer = std::shared_ptr<TokenType>;

class Command
{
public:
   Command(const std::list<std::any> &args, const std::list<TokenType> &redirects)
      : m_args(args),
        m_redirects(redirects)
   {}

   operator std::string();
   bool operator ==(const Command &other);
   const std::list<std::any> &getArgs();
   const std::list<TokenType> &getRedirects();
protected:
   // GlobItem or std::tuple<std::string, int>
   std::list<std::any> m_args;
   std::list<TokenType> m_redirects;
};

class GlobItem
{
public:
   GlobItem(const std::string &pattern)
      : m_pattern(pattern)
   {}
   std::list<std::string> resolve(const std::string &cwd);
   operator std::string();
   bool operator ==(const GlobItem &other);
protected:
   std::string m_pattern;
};

class Pipeline
{
public:
   Pipeline(const std::list<Command> &commands, bool negate = false, bool pipeError = false)
      : m_commands(commands),
        m_negate(negate),
        m_pipeError(pipeError)
   {
   }

   operator std::string();
   bool operator ==(const Pipeline &other);
protected:
   std::list<Command> m_commands;
   bool m_negate;
   bool m_pipeError;
};

class Seq
{
public:
   Seq(const std::any &lhs, const std::string &op, const std::any &rhs)
      : m_op(op),
        m_lhs(lhs),
        m_rhs(rhs)
   {
      assert(op.find(';') != std::string::npos ||
            op.find('&') != std::string::npos ||
            op.find("||") != std::string::npos ||
            op.find("&&") != std::string::npos);
   }
   operator std::string();
   bool operator ==(const GlobItem &other);
protected:
   std::string m_op;
   std::any m_lhs;
   std::any m_rhs;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_SHELL_COMMANDS_H
