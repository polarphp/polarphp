// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/16.


#include "LitConfig.h"
#include "TestingConfig.h"
#include "formats/ShellTest.h"
#include "polarphp/basic/adt/StringRef.h"
#include <filesystem>

using polar::lit::LitConfig;
using polar::lit::TestingConfig;
using polar::lit::ShTest;
using polar::basic::StringRef;

namespace fs = std::filesystem;

extern "C" {
void root_cfgsetter(TestingConfig *config, LitConfig *litConfig)
{
   std::string shellType;
   if (litConfig->hasParam("external")) {
      shellType = litConfig->getParam("external", "1");
   }
   bool externalShell;
   if (shellType == "0") {
      litConfig->note("Using internal shell");
      externalShell = false;
   } else {
      litConfig->note("Using external shell");
      externalShell = true;
   }
   std::string configSetTimeout = litConfig->getParam("set_timeout", "0");
   if (configSetTimeout != "0") {
      // Try setting the max individual test time in the configuration
      litConfig->setMaxIndividualTestTime(std::stoi(configSetTimeout));
   }
   litConfig->getParam("external");
   config->setName("per_test_timeout");
   config->setSuffixes({".littest"});
   config->setTestFormat(std::make_shared<ShTest>(externalShell));
   config->setTestSourceRoot(fs::path(__FILE__).parent_path());
   config->setTestExecRoot(config->getTestSourceRoot());
   config->setExtraConfig("target_triple", "(unused)");
   config->addSubstitution("%{short}", SHORT_GTEST_BIN);
   config->addSubstitution("%{infinite_loop}", INFINITE_LOOP_GTEST_BIN);
}
}
