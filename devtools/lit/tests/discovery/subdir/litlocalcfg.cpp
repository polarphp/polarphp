#include "LitConfig.h"
#include "TestingConfig.h"

using polar::lit::LitConfig;
using polar::lit::TestingConfig;

extern "C" {
void subdir_local_cfgsetter(TestingConfig *config, LitConfig *litConfig)
{
   config->setSuffixes({".php"});

}
}
