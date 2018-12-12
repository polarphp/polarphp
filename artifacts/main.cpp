// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/12.

#include "CLI/CLI.hpp"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/InitPolar.h"
#include "lib/Commands.h"

using polar::basic::StringRef;

void setup_command_opts(CLI::App &parser);

int main(int argc, char *argv[])
{
   polar::InitPolar polarInitializer(argc, argv);
   CLI::App cmdParser;
   polarInitializer.initNgOpts(cmdParser);
   setup_command_opts(cmdParser);
   CLI11_PARSE(cmdParser, argc, argv);
   return 0;
}

void setup_command_opts(CLI::App &parser)
{

}
