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

namespace polar {
namespace lit {

class Command
{
public:
   Command(const std::list<std::string> &args, const std::list<std::string> &redirects)
      : m_args(args),
        m_redirects(redirects)
   {}
   operator std::string();
   operator =(const Command &other);
protected:
   std::list<std::string> m_args;
   std::list<std::string> m_redirects;
};

class GlobItem
{
public:
   GlobItem(const std::string &pattern)
      : m_pattern(pattern)
   {}
   std::list<std::string> resolve(const std::string &cwd);
   operator std::string();
   operator =(const GlobItem &other);
protected:
   std::string m_pattern;
};

class Pipeline
{
public:
   Pipeline(const std::list<Command> &commands, bool negate = false, bool pipeError = false);
   operator std::string();
   operator =(const Pipeline &other);
protected:
   std::list<Command> m_commands;
};

class Seq
{
public:
   Seq(const std::string &lhs, const std::string &op, const std::string &rhs);
   operator std::string();
   operator =(const GlobItem &other);
protected:
   std::string m_op;
   std::string m_lhs;
   std::string m_rhs;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_SHELL_COMMANDS_H
