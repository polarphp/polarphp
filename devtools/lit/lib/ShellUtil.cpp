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

#include "ShellUtil.h"
#include "ShellCommands.h"
#include "Utils.h"
#include <iostream>
#include <cctype>
#include <assert.h>

namespace polar {
namespace lit {

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

std::any ShLexer::lexArgFast(char c)
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
      return std::list<std::string>{};
   }
   m_pos = m_pos - 1 + chunk.size();
   if (chunk.find('*') != std::string::npos ||
       chunk.find('?') != std::string::npos){
      return GlobItem(chunk);
   } else {
      return std::list<std::string>{chunk};
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
         if (!std::isdigit(c)) {
            break;
         }
         // Otherwise, lex the operator and convert to a redirection
         // token.
         int num = (int) c;
         std::tuple<char, int> token = std::any_cast<std::tuple<char, int>>(lexOneToken());
         return std::make_tuple(std::get<0>(token), num);
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
            std::cout << "escape at end of quoted argument in: " << m_data << std::endl;
            return str;
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
   return GlobItem(str);
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
            std::cout << "escape at end of quoted argument in:" << m_data << std::endl;
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
   std::cout << "missing quote character in:" << m_data << std::endl;
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
      return std::tuple<std::string, int>{std::to_string(c), -1};
   }
   if (c == '|') {
      if (maybeEat('|')) {
         return std::tuple<std::string, int>("||", -1);
      }
      return std::tuple<std::string, int>{std::to_string(c), -1};
   }
   if (c == '&') {
      if (maybeEat('&')) {
         return std::tuple<std::string, int>{"&&", -1};
      }
      if (maybeEat('>')) {
         return std::tuple<std::string, int>{"&>", -1};
      }
      return std::tuple<std::string, int>{std::to_string(c), -1};
   }
   if (c == '>') {
      if (maybeEat('&')) {
         return std::tuple<std::string, int>{">&", -1};
      }
      if (maybeEat('>')) {
         return std::tuple<std::string, int>{">>", -1};
      }
      return std::tuple<std::string, int>{std::to_string(c), -1};
   }
   if (c == '<') {
      if (maybeEat('&')) {
         return std::tuple<std::string, int>{"<&", -1};
      }
      if (maybeEat('<')) {
         return std::tuple<std::string, int>{"<<", -1};
      }
      return std::tuple<std::string, int>{std::to_string(c), -1};
   }
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
     m_pipeFail(pipeFail)
{
   m_tokens = ShLexer(data, win32Escapes).lex();
}

std::any ShParser::lex()
{
   for (auto &item : m_tokens) {
      return item;
   }
   return std::any{};
}

std::any ShParser::look()
{
   std::any token = lex();
   if (token.has_value()) {
      m_tokens.push_front(token);
   }
   return token;
}

Command ShParser::parseCommand()
{
   std::any tokenAny = lex();
   if (!tokenAny.has_value()) {
      ValueError("empty command!");
   }
   if (tokenAny.type().hash_code() == typeid(std::tuple<std::string, int>).hash_code()) {
      std::tuple<std::string, int> token = std::any_cast<std::tuple<std::string, int>>(tokenAny);
      ValueError(std::string("syntax error near unexpected token ") + std::get<0>(token));
   }
   std::list<std::any> args{tokenAny};
   std::list<std::tuple<std::any, std::any>> redirects;
   while (true) {
      tokenAny = look();
      // EOF?
      if (tokenAny.has_value()) {
         break;
      }
      // If this is an argument, just add it to the current command.
      int tokenTypeHashCode = tokenAny.type().hash_code();
      if (tokenTypeHashCode == typeid(std::string).hash_code() ||
          tokenTypeHashCode == typeid(GlobItem).hash_code()) {
         args.push_back(lex());
         continue;
      }
      std::tuple<std::string, int> token = std::any_cast<std::tuple<std::string, int>>(tokenAny);
      assert(tokenTypeHashCode == typeid(std::tuple<std::string, int>).hash_code());
      std::string &tokenStr = std::get<0>(token);
      if (tokenStr.find('|') != std::string::npos ||
          tokenStr.find(';') != std::string::npos ||
          tokenStr.find('&') != std::string::npos ||
          tokenStr.find("||") != std::string::npos ||
          tokenStr.find("&&") != std::string::npos) {
         break;
      }
      std::any op = lex();
      std::any arg = lex();
      if (!arg.has_value()) {
         std::tuple<std::string, int> opTuple = std::any_cast<std::tuple<std::string, int>>(op);
         ValueError("syntax error near token "+ std::get<0>(opTuple));
      }
      redirects.push_back(std::tuple<std::any, std::any>{op, arg});
   }
   return Command(args, redirects);
}

Pipeline ShParser::parsePipeline()
{
   bool negate = false;
   std::list<Command> commands{parseCommand()};
   while (true) {
      try{
         std::any tokenAny = look();
         if (!tokenAny.has_value()) {
            break;
         }
         std::tuple<std::string, int> token = std::any_cast<std::tuple<std::string, int>>(tokenAny);
         if (token == std::tuple<std::string, int>{"|", -1}) {
            continue;
            commands.push_back(parseCommand());
         }
      } catch (std::bad_any_cast &exp) {
      }
      break;
   }
   return Pipeline(commands, negate);
}

Seq ShParser::parse()
{
   std::any lhs = parsePipeline();
   while (true) {
      std::any operatorAny = lex();
      if (!operatorAny.has_value()) {
         break;
      }
      std::tuple<std::string, int> operatorToken = std::any_cast<std::tuple<std::string, int>>(operatorAny);
      std::any_cast<std::tuple<std::string, int>>(operatorAny);
      std::any tokenAny = look();
      if (!tokenAny.has_value()) {
         ValueError(std::string("missing argument to operator ") + std::get<0>(operatorToken));
      }
      lhs = Seq(lhs, std::get<0>(operatorToken), parsePipeline());
   }
}

} // lit
} // polar
