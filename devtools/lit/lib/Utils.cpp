// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/28.

#include "Utils.h"

namespace polar {
namespace lit {

std::list<std::string> split_string(const std::string &str, char separator)
{
   std::string buff;
   std::list<std::string> parts;
   for(auto n : str) {
      if(n != separator) {
         buff+=n;
      } else if(n == separator && buff != "") {
         parts.push_back(buff);
         buff = "";
      }
   }
   if(buff != "") {
      parts.push_back(buff);
   }
   return parts;
}

void show_version()
{

}

} // lit
} // polar
