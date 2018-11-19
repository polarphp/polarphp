// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/19.

#include "LitConfig.h"
#include "TestingConfig.h"
#include "formats/ShellTest.h"
#include "polarphp/basic/adt/StringRef.h"
#include <filesystem>

using polar::lit::LitConfig;
using polar::lit::TestingConfig;
using polar::lit::ShTest;

namespace fs = std::filesystem;

extern "C" {
void root_cfgsetter(TestingConfig *config, LitConfig *litConfig)
{
   config->setName("littests");
   config->setSuffixes({".littest"});
   config->setExcludes({"Inputs"});
   config->setTestFormat(std::make_shared<ShTest>(true));
   fs::path testSourceRoot = fs::path(__FILE__).parent_path();
   config->setTestSourceRoot(testSourceRoot);
   config->setTestExecRoot(testSourceRoot);
   config->setExtraConfig("target_triple", "(unused)");
   config->addSubstitution("%{inputs}", testSourceRoot / "Inputs");
   config->addSubstitution("%{lit}", LIT_TEST_BIN);
//   config->addEnvironment("PATH", POLAR_RUNTIME_OUTPUT_INTDIR);
}
}
