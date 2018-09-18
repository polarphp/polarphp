// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/18.

#include "CLI/CLI.hpp"
#include "Config.h"
#include <iostream>
#include <thread>
#include <assert.h>
#include <filesystem>
#include <list>

int main(int argc, char *argv[])
{
   CLI::App catApp;
   bool showNonprinting = false;
   std::vector<std::string> filenames;
   catApp.add_option("-v, --show-nonprinting", showNonprinting, "show all non printable char", false);
   catApp.add_option("filenames", filenames, "Filenames to been print")->required();
   CLI11_PARSE(catApp, argc, argv);
   return 0;
}
