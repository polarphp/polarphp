// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/15.

#include "LitConfig.h"
#include "TestingConfig.h"
#include "formats/ShellTest.h"
#include "polarphp/basic/adt/StringRef.h"

using polar::lit::LitConfig;
using polar::lit::TestingConfig;
using polar::lit::ShTest;

// We intentionally don't set the source root or exec root directories here,
// because this suite gets reused for testing the exec root behavior (in
// ../exec-discovery).
//
// config.test_source_root = None
// config.test_exec_root = None

// Check that arbitrary config values are copied (tested by subdir/litlocalcfg.cpp).

extern "C" {
void root_cfgsetter(TestingConfig *config, LitConfig *litConfig)
{
   config->setName("shtest-env");
   config->setSuffixes({".littest"});
   config->setTestFormat(std::make_shared<ShTest>());
   config->setTestExecRoot(std::nullopt);
   config->setTestSourceRoot(std::nullopt);
   config->addEnvironment("FOO", "1");
   config->addEnvironment("BAR", "2");
   config->addSubstitution("%{print_env}", LIT_TEST_PRINT_ENVIRONMENT_BIN);
}
}
