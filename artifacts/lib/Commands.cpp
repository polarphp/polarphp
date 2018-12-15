// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/12.

#include "Commands.h"
#include "polarphp/global/CompilerFeature.h"
#include "lib/PolarVersion.h"

POLAR_BEGIN_DISABLE_ZENDVM_WARNING()
#include "polarphp/vm/zend/zend.h"
POLAR_END_DISABLE_ZENDVM_WARNING()

#include <iostream>
#include <vector>

extern std::vector<std::string> sg_defines;

namespace polar {

void print_polar_version()
{
   std::cout << POLARPHP_PACKAGE_STRING << " (built: "<< BUILD_TIME <<  ") "<< std::endl
             << "Copyright (c) 2016-2018 The polarphp Foundation" << std::endl
             << get_zend_version();
}

void setup_init_entries_commands(std::string &iniEntries)
{

}

int dispatch_cli_command(App &cmdParser, int argc, char *argv[])
{

}

} // polar
