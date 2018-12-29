// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/28.

#include "PolarEmbed.h"

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/runtime/ExecEnv.h"
#include "polarphp/utils/InitPolar.h"
#include "polarphp/runtime/ExecEnv.h"
#include "polarphp/runtime/LifeCycle.h"

namespace polar {
namespace unittest {

bool begin_vm_context(int argc, char *argv[])
{
   polar::InitPolar polarInitializer(argc, argv);
   polar::runtime::ExecEnv &execEnv = polar::runtime::retrieve_global_execenv();
   polar::runtime::ExecEnvInfo &execEnvInfo = execEnv.getRuntimeInfo();
   execEnv.setContainerArgc(argc);
   execEnv.setContainerArgv(argv);
   execEnvInfo.iniDefaultInitHandler = polar::runtime::cli_ini_defaults;
   execEnvInfo.phpIniIgnoreCwd = true;
   execEnvInfo.phpIniIgnore = polar::runtime::HARDCODED_INI;
   if (!execEnv.bootup()) {
      return false;
   }
   return true;
}

void end_vm_context()
{
   polar::runtime::ExecEnv &execEnv = polar::runtime::retrieve_global_execenv();
   execEnv.shutdown();
}

} // unittest
} // polar
