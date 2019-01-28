// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/06.

#include "LitConfig.h"
#include "TestingConfig.h"
#include "formats/ShellTest.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Twine.h"

using polar::lit::LitConfig;
using polar::lit::TestingConfig;
using polar::lit::ShTest;
using polar::basic::StringRef;
using polar::basic::Twine;

extern "C" {
void root_cfgsetter(TestingConfig *config, LitConfig *litConfig)
{
   config->setName("shtest-run-at-line");
   config->setSuffixes({".littest"});
   config->addSubstitution("%{lit}", LIT_TEST_BIN);
   config->addEnvironment("PATH", Twine(POLAR_RUNTIME_OUTPUT_INTDIR, StringRef(":")).concat(std::getenv("PATH")).getStr());
}
}
