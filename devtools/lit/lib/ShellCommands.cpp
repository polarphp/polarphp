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

#include "ShellCommands.h"
#include "ShellUtil.h"
#include "Utils.h"
#include "ForwardDefs.h"
#include "polarphp/basic/adt/Twine.h"
#include <glob.h>

namespace polar {
namespace lit {

using polar::basic::Twine;

Command::operator std::string()
{
   std::string argMsg = "";
   int argSize = m_args.size();
   int i = 0;
   for (std::any &argAny: m_args) {
      if (argAny.type() == typeid(std::string)) {
         if (i < argSize - 1) {
            argMsg += "\"" + std::any_cast<std::string>(argAny) + "\", ";
         } else {
            argMsg += "\"" + std::any_cast<std::string>(argAny) + "\"";
         }
      }
      ++i;
   }
   std::string redirectMsg = "";
   i = 0;
   int redirectSize = m_redirects.size();
   for (RedirectTokenType &redirect: m_redirects) {
      if (i < redirectSize - 1) {
         redirectMsg += format_string("((%s, %d), %s)", std::get<0>(std::get<0>(redirect)).c_str(),
                                      std::get<1>(std::get<0>(redirect)),
                                      std::get<1>(redirect).c_str()) + ", ";
      } else {
         redirectMsg += format_string("((%s, %d), %s)", std::get<0>(std::get<0>(redirect)).c_str(),
                                      std::get<1>(std::get<0>(redirect)),
                                      std::get<1>(redirect).c_str());
      }
      ++i;
   }
   return format_string("Command([%s], [%s])", argMsg.c_str(), redirectMsg.c_str());
}

bool Command::compareTokenAny(const std::any &lhs, const std::any &rhs) const
{
   if (lhs.type() != rhs.type()) {
      return false;
   }
   if (lhs.type() == typeid(GlobItem)) {
      const GlobItem &lhsValue = std::any_cast<const GlobItem &>(lhs);
      const GlobItem &rhsValue = std::any_cast<const GlobItem &>(rhs);
      if (lhsValue != rhsValue) {
         return false;
      }
   } else if (lhs.type() == typeid(std::string)) {
      const std::string &lhsValue = std::any_cast<std::string>(lhs);
      const std::string &rhsValue = std::any_cast<std::string>(rhs);
      if (lhsValue != rhsValue) {
         return false;
      }
   }
   return true;
}

bool Command::operator ==(const Command &other) const
{
   if (m_args.size() != other.m_args.size()) {
      return false;
   }
   {
      auto aiter = m_args.begin();
      auto endaMark = m_args.end();
      auto aoiter = other.m_args.begin();
      while (aiter != endaMark) {
         const std::any &argAny = *aiter;
         const std::any &oArgAny = *aoiter;
         if (!compareTokenAny(argAny, oArgAny)) {
            return false;
         }
         ++aiter;
         ++aoiter;
      }
   }
   {
      auto aiter = m_redirects.begin();
      auto endaMark = m_redirects.end();
      auto aoiter = other.m_redirects.begin();
      while (aiter != endaMark) {
         const RedirectTokenType &argAny = *aiter;
         const RedirectTokenType &oArgAny = *aoiter;
         if (!compareTokenAny(std::get<0>(argAny), std::get<0>(oArgAny)) ||
             !compareTokenAny(std::get<1>(argAny), std::get<1>(oArgAny))) {
            return false;
         }
      }
   }
   return true;
}

std::list<std::any> &Command::getArgs()
{
   return m_args;
}

const std::list<RedirectTokenType> &Command::getRedirects()
{
   return m_redirects;
}

void Command::toShell(std::string &str, bool) const
{
   for (const std::any &argAny : m_args) {
      if (argAny.type() == typeid(std::string)) {
         const std::string &arg = std::any_cast<std::string>(argAny);
         std::string quotedArg;
         if (arg.find("'") == std::string::npos) {
            quotedArg = "\'" + arg + "\'";
         } else if (arg.find("\"") == std::string::npos &&
                    arg.find("$") == std::string::npos){
            quotedArg = "\"" + arg + "\"";
         } else {
            throw std::runtime_error(format_string("Unable to quote %s", arg.c_str()));
         }
         str += quotedArg;
         // For debugging / validation.
         std::list<std::any> dequoted = ShLexer(quotedArg).lex();
         if (!compareTokenAny(dequoted.front(), argAny)) {
            throw std::runtime_error(format_string("Unable to quote %s", arg.c_str()));
         }
      }
   }
   for (const auto &redirectTuple : m_redirects) {
      const std::any &opAny = std::get<0>(redirectTuple);
      const std::any &argAny = std::get<0>(redirectTuple);
      if (opAny.type() == typeid(ShellTokenType) &&
          argAny.type() == typeid(ShellTokenType)) {
         const ShellTokenType &op = std::any_cast<const ShellTokenType &>(opAny);
         const ShellTokenType &arg = std::any_cast<const ShellTokenType &>(argAny);
         if (std::get<1>(op) == -1) {
            str += format_string("%s '%s'", std::get<0>(op).c_str(), std::get<0>(arg).c_str());
         } else {
            str += format_string("%d%s '%s'", std::get<1>(op), std::get<0>(op).c_str(),
                                 std::get<0>(arg).c_str());
         }
      }
   }
}

namespace {
class GlobCleaner
{
public:
   GlobCleaner(glob_t *globResult)
      : m_globResult(globResult)
   {
   }
   ~GlobCleaner()
   {
      globfree(m_globResult);
   }
protected:
   glob_t *m_globResult;
};
} // anonymous namespace

std::list<std::string> GlobItem::resolve(const std::string &cwd) const
{
   stdfs::path path(m_pattern);
   if (!path.is_absolute()) {
      path = stdfs::path(cwd) / path;
   }
   std::list<std::string> files;
   glob_t globResult;
   GlobCleaner cleaner(&globResult);
   if (0 == glob(path.string().c_str(), GLOB_TILDE, NULL, &globResult)) {
      for(unsigned int i=0; i < globResult.gl_pathc; ++i) {
         files.push_back(std::string(globResult.gl_pathv[i]));
      }
   }
   if (files.empty()){
      files.push_back(path);
   }
   return files;
}

GlobItem::operator std::string()
{
   return m_pattern;
}

bool GlobItem::operator ==(const GlobItem &other) const
{
   return m_pattern == other.m_pattern;
}

Pipeline::operator std::string()
{
   size_t i = 0;
   size_t cmdSize = m_commands.size();
   std::string commands = "[";
   for (AbstractCommandPointer command : m_commands) {
      if (i < cmdSize - 1) {
         commands += command->operator std::string() + ", ";
      } else {
         commands += command->operator std::string();
      }
      ++i;
   }
   commands += "]";
   return Twine("Pipeline(").concat(commands).concat(", negate: ").concat(m_negate ? "true" : "false")
         .concat(", pipeError:").concat(m_pipeError ? "true" : "false").concat(")").getStr();
}

bool Pipeline::isNegate()
{
   return m_negate;
}

bool Pipeline::isPipeError()
{
   return m_pipeError;
}

bool Pipeline::operator ==(const Pipeline &other) const
{
   return std::equal(m_commands.begin(), m_commands.end(),
                     other.m_commands.begin()) &&
         m_negate == other.m_negate &&
         m_pipeError == other.m_pipeError;
}

void Pipeline::toShell(std::string &str, bool pipeFail) const
{
   if (pipeFail != m_pipeError) {
      throw ValueError("Inconsistent \"pipeFail\" attribute!");
   }
   if (m_negate) {
      str += "! ";
   }
   int cur = 0;
   int size = m_commands.size();
   for (const AbstractCommandPointer &cmd : m_commands) {
      cmd->toShell(str);
      ++cur;
      if (cur != size - 1) {
         str += "|\n  ";
      }
   }
}

const CommandList &Pipeline::getCommands() const
{
   return m_commands;
}

Seq::operator std::string()
{
   return Twine("Seq(").concat(m_lhs->operator std::string())
         .concat(", \"").concat(m_op).concat("\", ")
         .concat(m_rhs->operator std::string().c_str()).getStr();
}

bool Seq::operator ==(const Seq &other) const
{
   return m_lhs == other.m_lhs && m_rhs == other.m_rhs &&
         m_op == other.m_op;
}

const std::string &Seq::getOp() const
{
   return m_op;
}

AbstractCommandPointer Seq::getLhs() const
{
   return m_lhs;
}

AbstractCommandPointer Seq::getRhs() const
{
   return m_rhs;
}

void Seq::toShell(std::string &str, bool pipeFail) const
{
   m_lhs->toShell(str, pipeFail);
   str += Twine(" ", m_op).concat("\n").getStr();
   m_rhs->toShell(str, pipeFail);
}

} // lit
} // polar
