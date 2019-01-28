// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/18.


#include "LitConfig.h"
#include "TestingConfig.h"
#include "formats/ShellTest.h"
#include "polarphp/basic/adt/StringRef.h"
#include "CustomTestFormatter.h"

using polar::lit::LitConfig;
using polar::lit::TestingConfig;
using polar::lit::CustomTestFormatter;

extern "C" {
void root_cfgsetter(TestingConfig *config, LitConfig *litConfig)
{
   config->setName("custom-parsers");
   config->setSuffixes({".littest"});
   config->setTestFormat(std::make_shared<CustomTestFormatter>());
   config->setTestSourceRoot(std::nullopt);
   config->setTestExecRoot(std::nullopt);
   config->setExtraConfig("target_triple", "x86_64-unknown-unknown");
}
}
