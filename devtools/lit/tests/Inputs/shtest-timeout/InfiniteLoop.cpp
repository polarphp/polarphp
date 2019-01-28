// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/16.

#include "CLI/CLI.hpp"
#include <string>
#include <iostream>

int main(int, char *[])
{
   std::cout << "Running infinite loop" << std::endl;
   std::cout.flush();
   while(true){}
   return 0;
}
