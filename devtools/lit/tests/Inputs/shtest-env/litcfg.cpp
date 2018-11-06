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
   config->setSuffixes({".txt"});
   config->setTestFormat(std::make_shared<ShTest>());
   config->setTestExecRoot(std::nullopt);
   config->setTestSourceRoot(std::nullopt);
   config->addSubstitution("print_env", LIT_TEST_PRINT_ENVIRONMENT_BIN);
}
}
