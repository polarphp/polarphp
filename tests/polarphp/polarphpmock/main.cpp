// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/02/07.

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/runtime/ExecEnv.h"
#include "polarphp/utils/InitPolar.h"
#include "polarphp/runtime/ExecEnv.h"
#include "polarphp/runtime/LifeCycle.h"

#include "libpdkmock/PdkMockDefs.h"

#include "CLI/CLI.hpp"

int main(int argc, char *argv[])
{
   std::string inputFilename;
   CLI::App cmdParser;
   polar::InitPolar polarInitializer(argc, argv);

   cmdParser.add_option("filename", inputFilename, "<filename>")
         ->required(true)
         ->check(CLI::ExistingFile);
   CLI11_PARSE(cmdParser, argc, argv);

   polar::runtime::ExecEnv &execEnv = polar::runtime::retrieve_global_execenv();
   polar::runtime::ExecEnvInfo &execEnvInfo = execEnv.getRuntimeInfo();
   execEnv.setContainerArgc(argc);
   execEnv.setContainerArgv(argv);
   execEnvInfo.iniDefaultInitHandler = polar::runtime::cli_ini_defaults;
   execEnvInfo.phpIniIgnoreCwd = true;
   execEnvInfo.phpIniIgnore = polar::runtime::HARDCODED_INI;
   polar::runtime::sg_vmExtensionInitHook = php::stdlib_init_entry;
   if (!execEnv.bootup()) {
      std::cerr << "polarphp initialize failed." << std::endl;
      return 1;
   }
   int exitStatus;
   execEnv.execScript(inputFilename, exitStatus);
   execEnv.shutdown();
   return exitStatus;
}
