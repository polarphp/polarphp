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

#include "StdlibExports.h"

#include "polarphp/vm/lang/Module.h"
#include "ModuleCycleHooks.h"
#include "Namespaces.h"
#include "Constants.h"
#include "Interfaces.h"
#include "Functions.h"
#include "Classes.h"
#include "Inis.h"

namespace php {

using polar::vmapi::Module;

bool export_stdlib_to_zendvm()
{
   static Module stdlibModule("stdlib");

   register_module_cycle_hooks(stdlibModule);
   register_inis_hook(stdlibModule);
   register_namepsace_hook(stdlibModule);
   register_constants_hook(stdlibModule);
   register_interfaces_hook(stdlibModule);
   register_functions_hook(stdlibModule);
   register_classes_hook(stdlibModule);

   return stdlibModule.registerToVM();
}

} // php
