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

#include "ShellCommands.h"
#include "ShellUtil.h"
#include "Utils.h"

namespace polar {
namespace lit {

Command::operator std::string()
{
   return format_string("Command(%d, %d)", m_args.size(), m_redirects.size());
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
   } else if (lhs.type() == typeid(ShellTokenType)) {
      const ShellTokenType &lhsValue = std::any_cast<const ShellTokenType &>(lhs);
      const ShellTokenType &rhsValue = std::any_cast<const ShellTokenType &>(rhs);
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
         const TokenType &argAny = *aiter;
         const TokenType &oArgAny = *aoiter;
         if (!compareTokenAny(std::get<0>(argAny), std::get<0>(oArgAny)) ||
             !compareTokenAny(std::get<1>(argAny), std::get<1>(oArgAny))) {
            return false;
         }
      }
   }
   return true;
}

const std::list<std::any> &Command::getArgs()
{
   return m_args;
}

const std::list<TokenType> &Command::getRedirects()
{
   return m_redirects;
}

void Command::toShell(std::string &str, bool) const
{
   for (const std::any &argAny : m_args) {
      if (argAny.type() == typeid(ShellTokenType)) {
         const ShellTokenType &token = std::any_cast<const ShellTokenType &>(argAny);
         const std::string &arg = std::get<0>(token);
         std::string quotedArg;
         if (arg.find("'") == std::string::npos) {
            quotedArg = "\'" + arg + "\'";
         } else if (arg.find("\"") == std::string::npos &&
                    arg.find("$") == std::string::npos){
            quotedArg = "\"" + arg + "\"";
         } else {
            throw std::runtime_error(format_string("Unable to quote %s", arg));
         }
         str += quotedArg;
         // For debugging / validation.
         std::list<std::any> dequoted = ShLexer(quotedArg).lex();
         if (!compareTokenAny(dequoted.front(), argAny)) {
            throw std::runtime_error(format_string("Unable to quote %s", arg));
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
            str += format_string("%s '%s'", std::get<0>(op).c_str(), std::get<0>(arg));
         } else {
            str += format_string("%d%s '%s'", std::get<1>(op), std::get<0>(op).c_str(),
                                 std::get<0>(arg));
         }
      }
   }
}

std::list<std::string> GlobItem::resolve(const std::string &cwd)
{
   fs::path path(m_pattern);
   if (!path.is_absolute()) {
      path = fs::path(cwd) / path;
   }
   std::list<std::string> files;
   for(auto& entry: fs::recursive_directory_iterator(path)) {
      files.push_back(entry.path());
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
   return format_string("Pipeline(commands, negate, pipeError)");
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
   for (const std::shared_ptr<ShellAble> &cmd : m_commands) {
      cmd->toShell(str);
      ++cur;
      if (cur != size - 1) {
         str += "|\n  ";
      }
   }
}

void Seq::toShell(std::string &str, bool pipeFail) const
{

}

} // lit
} // polar
