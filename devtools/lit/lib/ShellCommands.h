// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/04.

#ifndef POLAR_DEVLTOOLS_LIT_SHELL_COMMANDS_H
#define POLAR_DEVLTOOLS_LIT_SHELL_COMMANDS_H

#include <list>
#include <string>
#include <any>
#include <memory>
#include <assert.h>
#include "LitGlobal.h"
#include "ForwardDefs.h"

namespace polar {
namespace lit {

using RedirectTokenType  = std::tuple<ShellTokenType, std::string>;

class AbstractCommand
{
public:
   enum class Type {
      Command,
      Pipeline,
      Seq
   };
public:
   virtual void toShell(std::string &str, bool pipeFail = false) const = 0;
   virtual operator std::string() = 0;
   virtual Type getCommandType() = 0;
};

class Command : public AbstractCommand
{
public:
   Command(const std::list<std::any> &args, const std::list<RedirectTokenType > &redirects)
      : m_args(args),
        m_redirects(redirects)
   {}

   operator std::string() override;
   bool operator ==(const Command &other) const;
   std::list<std::any> &getArgs();
   const std::list<RedirectTokenType > &getRedirects();
   void toShell(std::string &str, bool pipeFail = false) const override;
   Type getCommandType() override
   {
      return Type::Command;
   }
private:
   bool compareTokenAny(const std::any &lhs, const std::any &rhs) const;
protected:
   // GlobItem or std::tuple<std::string, int>
   std::list<std::any> m_args;
   std::list<RedirectTokenType> m_redirects;
};

class GlobItem
{
public:
   GlobItem(const std::string &pattern)
      : m_pattern(pattern)
   {}
   std::list<std::string> resolve(const std::string &cwd) const;
   operator std::string();
   bool operator ==(const GlobItem &other) const;
   bool operator !=(const GlobItem &other) const
   {
      return !operator ==(other);
   }
protected:
   std::string m_pattern;
};

class Pipeline : public AbstractCommand
{
public:
   Pipeline(const std::list<std::shared_ptr<AbstractCommand>> &commands, bool negate = false, bool pipeError = false)
      : m_commands(commands),
        m_negate(negate),
        m_pipeError(pipeError)
   {
   }

   operator std::string() override;
   bool isNegate();
   bool isPipeError();
   bool operator ==(const Pipeline &other) const;
   bool operator !=(const Pipeline &other) const
   {
      return !operator ==(other);
   }
   void toShell(std::string &str, bool pipeFail = false) const override;
   Type getCommandType() override
   {
      return Type::Pipeline;
   }
   const CommandList &getCommands() const;
protected:
   CommandList m_commands;
   bool m_negate;
   bool m_pipeError;
};

class Seq : public AbstractCommand
{
public:
   Seq(std::shared_ptr<AbstractCommand> lhs, const std::string &op, std::shared_ptr<AbstractCommand> rhs)
      : m_op(op),
        m_lhs(lhs),
        m_rhs(rhs)
   {
      assert(op.find(';') != std::string::npos ||
            op.find('&') != std::string::npos ||
            op.find("||") != std::string::npos ||
            op.find("&&") != std::string::npos);
   }
   operator std::string() override;
   bool operator ==(const Seq &other) const;
   bool operator !=(const Seq &other) const
   {
      return !operator ==(other);
   }
   Type getCommandType() override
   {
      return Type::Seq;
   }
   const std::string &getOp() const;
   AbstractCommandPointer getLhs() const;
   AbstractCommandPointer getRhs() const;
   void toShell(std::string &str, bool pipeFail = false) const override;
protected:
   std::string m_op;
   AbstractCommandPointer m_lhs;
   AbstractCommandPointer m_rhs;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_SHELL_COMMANDS_H
