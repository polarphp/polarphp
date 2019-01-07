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

#include "Inis.h"
#include "polarphp/vm/lang/Ini.h"
#include "polarphp/vm/lang/Module.h"

namespace php {

using polar::vmapi::Ini;

void register_inis_hook(Module &module)
{
   // this have no effect we write this in php.ini
   module.registerIni(Ini("polarphp_author", "polarboy"));
   // rewrite in php.ini
   module.registerIni(Ini("polarphp_team_address", "beijing"));
   // register but empty value
   module.registerIni(Ini("polarphp_product", ""));
   module.registerIni(Ini("polarphp_enable_gpu", "off"));
   module.registerIni(Ini("polarphp_name", "polarphp"));
   module.registerIni(Ini("polarphp_age", 1));
   // int type
   module.registerIni(Ini("polarphp_string_value", "stringvalue"));
   module.registerIni(Ini("polarphp_int_value", 2019));
   module.registerIni(Ini("polarphp_bool_value", true));
   module.registerIni(Ini("polarphp_double_value", 3.14));
}

} // php
