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

namespace polar {
namespace lit {

Command::operator std::string()
{

}

bool Command::operator ==(const Command &other)
{
   if (m_args.size() != other.m_args.size()) {
      return false;
   }
   auto aiter = m_args.begin();
   auto endaMark = m_args.end();
   auto aoiter = other.m_args.begin();
   while (aiter != endaMark) {
//      std::any &argAny = *aiter;
//      std::any &oArgAny = *aoiter;
//      if (argAny.type() != oArgAny.type()) {
//         return false;
//      }
      ++aiter;
      ++aoiter;
   }
}

const std::list<std::any> &Command::getArgs()
{

}

const std::list<TokenType> &Command::getRedirects()
{

}

} // lit
} // polar
