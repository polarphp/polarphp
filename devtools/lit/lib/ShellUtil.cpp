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

#include "ShellUtil.h"
#include "ShellCommands.h"
#include "ForwardDefs.h"
#include "LitConfig.h"
#include "Utils.h"
#include <iostream>
#include <cctype>
#include <assert.h>

namespace polar {
namespace lit {

extern LitConfig *sg_litCfg;

ShLexer::ShLexer(const std::string &data, bool win32Escapes)
   : m_data(data),
     m_pos(0),
     m_end(data.size()),
     m_win32Escapes(win32Escapes)
{}

char ShLexer::eat()
{
   char c = m_data[m_pos];
   ++m_pos;
   return c;
}

char ShLexer::look()
{
   return m_data[m_pos];
}

bool ShLexer::maybeEat(char c)
{
   if (m_data[m_pos] == c){
      ++m_pos;
      return true;
   }
   return false;
}

std::any ShLexer::lexArgFast(char)
{
   std::string chunk = m_data.substr(m_pos - 1);
   chunk = split_string(chunk, ' ', 1).front();
   if (chunk.find('|') != std::string::npos ||
       chunk.find('&') != std::string::npos ||
       chunk.find('>') != std::string::npos ||
       chunk.find('<') != std::string::npos ||
       chunk.find('\'') != std::string::npos ||
       chunk.find('"') != std::string::npos ||
       chunk.find(';') != std::string::npos ||
       chunk.find('\\') != std::string::npos) {
      return std::any{};
   }
   m_pos = m_pos - 1 + chunk.size();
   if (chunk.find('*') != std::string::npos ||
       chunk.find('?') != std::string::npos){
      return GlobItem(chunk);
   } else {
      return ShellTokenType{chunk, SHELL_CMD_NORMAL_TOKEN};
   }
}

std::any ShLexer::lexArgSlow(char c)
{
   std::string str;
   if (c == '\'' || c == '"') {
      str = lexArgQuoted(c);
   } else {
      str = c;
   }
   bool unquotedGlobChar = false;
   bool quotedGlobChar = false;
   while (m_pos != m_end) {
      c = look();
      if (std::isspace(c) || c == '|' || c == '&' || c == ';') {
         break;
      } else if (c == '>' || c == '<') {
         // This is an annoying case; we treat '2>' as a single token so
         // we don't have to track whitespace tokens.
         // If the parse string isn't an integer, do the usual thing.
         try {
            int num = std::stoi(str);
            // Otherwise, lex the operator and convert to a redirection
            // token.
            ShellTokenType token = std::any_cast<ShellTokenType>(lexOneToken());
            return ShellTokenType{std::get<0>(token), num};
         } catch (...) {
            break;
         }
      } else if (c == '"' || c == '\'') {
         eat();
         std::string quotedArg = lexArgQuoted(c);
         if (quotedArg.find('*') != std::string::npos ||
             quotedArg.find('?') != std::string::npos) {
            quotedGlobChar = true;
         }
         str += quotedArg;
      } else if (!m_win32Escapes && c == '\\') {
         // Outside of a string, '\\' escapes everything.
         eat();
         if (m_pos == m_end) {
            sg_litCfg->warning(format_string("escape at end of quoted argument in: %s\n", m_data.c_str()), __FILE__, __LINE__);
            return std::any(str);
         }
         str += eat();
      } else if (c == '*' || c == '?') {
         unquotedGlobChar = true;
         str += eat();
      } else {
         str += eat();
      }
   }
   assert(!(quotedGlobChar && unquotedGlobChar));
   if (unquotedGlobChar) {
      return GlobItem(str);
   }
   return ShellTokenType{str, SHELL_CMD_NORMAL_TOKEN};
}

std::string ShLexer::lexArgQuoted(char delim)
{
   std::string str;
   char c;
   while (m_pos != m_end) {
      c = eat();
      if (c == delim) {
         return str;
      } else if (c == '\\' && delim == '"') {
         // Inside a '"' quoted string, '\\' only escapes the quote
         // character and backslash, otherwise it is preserved.
         if (m_pos == m_end) {
            sg_litCfg->warning(format_string("escape at end of quoted argument in: %s\n", m_data.c_str()), __FILE__, __LINE__);
            return str;
         }
         c = eat();
         if (c == '"') {
            str += '"';
         } else if (c == '\\') {
            str += '\\';
         } else {
            str += '\\';
            str += c;
         }
      } else {
         str += c;
      }
   }
   std::string warning(format_string("missing quote character in: %s\n", m_data.c_str()));
   sg_litCfg->warning(warning, __FILE__, __LINE__);
   throw ValueError(warning);
   return str;
}

std::any ShLexer::lexArg(char c)
{
   std::any fastResult = lexArgFast(c);
   if (fastResult.has_value()) {
      return fastResult;
   }
   return lexArgSlow(c);
}

std::any ShLexer::lexOneToken()
{
   char c = eat();
   if (c == ';') {
      return ShellTokenType{std::string(1, c), SHELL_CMD_REDIRECT_TOKEN};
   }
   if (c == '|') {
      if (maybeEat('|')) {
         return ShellTokenType("||", SHELL_CMD_REDIRECT_TOKEN);
      }
      return ShellTokenType{std::string(1, c), SHELL_CMD_REDIRECT_TOKEN};
   }
   if (c == '&') {
      if (maybeEat('&')) {
         return ShellTokenType{"&&", SHELL_CMD_REDIRECT_TOKEN};
      }
      if (maybeEat('>')) {
         return ShellTokenType{"&>", SHELL_CMD_REDIRECT_TOKEN};
      }
      return ShellTokenType{std::string(1, c), SHELL_CMD_REDIRECT_TOKEN};
   }
   if (c == '>') {
      if (maybeEat('&')) {
         return ShellTokenType{">&", SHELL_CMD_REDIRECT_TOKEN};
      }
      if (maybeEat('>')) {
         return ShellTokenType{">>", SHELL_CMD_REDIRECT_TOKEN};
      }
      return ShellTokenType{std::string(1, c), SHELL_CMD_REDIRECT_TOKEN};
   }
   if (c == '<') {
      if (maybeEat('&')) {
         return ShellTokenType{"<&", SHELL_CMD_REDIRECT_TOKEN};
      }
      if (maybeEat('<')) {
         return ShellTokenType{"<<", SHELL_CMD_REDIRECT_TOKEN};
      }
      return ShellTokenType{std::string(1, c), SHELL_CMD_REDIRECT_TOKEN};
   }
   return lexArg(c);
}

std::list<std::any> ShLexer::lex()
{
   std::list<std::any> result;
   while (m_pos != m_end) {
      if (std::isspace(look())) {
         eat();
      } else {
         result.push_back(lexOneToken());
      }
   }
   return result;
}

ShParser::ShParser(const std::string &data, bool win32Escapes, bool pipeFail)
   : m_data(data),
     m_win32Escapes(win32Escapes),
     m_pipeFail(pipeFail)
{
   m_tokens = ShLexer(data, m_win32Escapes).lex();
   if (!m_tokens.empty()) {
      m_curToken = m_tokens.begin();
   } else {
      m_curToken = m_tokens.end();
   }
}

std::any &ShParser::lex()
{
   if (m_curToken != m_tokens.end()) {
      return *m_curToken++;
   }
   return m_emptyToken;
}

std::any &ShParser::look()
{
   if (m_curToken == m_tokens.end()){
      return m_emptyToken;
   }
   std::any &token = *m_curToken;
   return token;
}

std::shared_ptr<AbstractCommand> ShParser::parseCommand()
{
   std::any &tokenAny = lex();
   if (!tokenAny.has_value()) {
      throw ValueError("empty command!");
   }
   assert(tokenAny.type() == typeid(ShellTokenType));
   std::list<std::any> args{std::get<0>(std::any_cast<ShellTokenType &>(tokenAny))};
   std::list<RedirectTokenType> redirects;
   while (true) {
      tokenAny = look();
      // EOF?
      if (!tokenAny.has_value()) {
         break;
      }
      // If this is an argument, just add it to the current command.
      auto &tokenType = tokenAny.type();
      if ((tokenType == typeid(ShellTokenType) &&
           std::get<1>(std::any_cast<ShellTokenType &>(tokenAny)) == SHELL_CMD_NORMAL_TOKEN) ||
          tokenType == typeid(GlobItem)) {
         if (tokenType == typeid(GlobItem)) {
            args.push_back(lex());
         } else {
            args.push_back(std::get<0>(std::any_cast<ShellTokenType &>(lex())));
         }
         continue;
      }
      ShellTokenType token = std::any_cast<ShellTokenType>(tokenAny);
      assert(tokenType == typeid(ShellTokenType));
      std::string &tokenStr = std::get<0>(token);
      if (tokenStr == "|" ||
          tokenStr == ";" ||
          tokenStr == "&" ||
          tokenStr == "||" ||
          tokenStr == "&&") {
         break;
      }
      std::any &op = lex();
      std::any &arg = lex();
      if (!arg.has_value()) {
         ShellTokenType opTuple = std::any_cast<ShellTokenType>(op);
         throw ValueError("syntax error near token "+ std::get<0>(opTuple));
      }
      redirects.push_back(RedirectTokenType{
                             std::any_cast<ShellTokenType &>(op),
                             std::get<0>(std::any_cast<ShellTokenType &>(arg))});
   }
   return std::shared_ptr<AbstractCommand>(new Command(args, redirects));
}

std::shared_ptr<AbstractCommand> ShParser::parsePipeline()
{
   bool negate = false;
   std::list<std::shared_ptr<AbstractCommand>> commands{parseCommand()};
   while (true) {
      try{
         std::any &tokenAny = look();
         if (!tokenAny.has_value()) {
            break;
         }
         ShellTokenType token = std::any_cast<ShellTokenType>(tokenAny);
         if (token == ShellTokenType{"|", SHELL_CMD_REDIRECT_TOKEN}) {
            lex();
            commands.push_back(parseCommand());
            continue;
         }
      } catch (std::bad_any_cast &) {
      }
      break;
   }
   return std::shared_ptr<AbstractCommand>(new Pipeline(commands, negate, m_pipeFail));
}

std::shared_ptr<AbstractCommand> ShParser::parse()
{
   std::shared_ptr<AbstractCommand> lhs = parsePipeline();
   while (look().has_value()) {
      std::any &operatorAny = lex();
      if (!operatorAny.has_value()) {
         break;
      }
      ShellTokenType operatorToken = std::any_cast<ShellTokenType>(operatorAny);
      std::any &tokenAny = look();
      if (!tokenAny.has_value()) {
         throw ValueError(std::string("missing argument to operator ") + std::get<0>(operatorToken));
      }
      lhs = std::shared_ptr<AbstractCommand>(new Seq(lhs, std::get<0>(operatorToken), parsePipeline()));
   }
   return lhs;
}

} // lit
} // polar
