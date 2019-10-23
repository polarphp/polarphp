<?php

use Lit\Format\ShellTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
use Lit\Utils\TestLogger;

/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */
$config->setName('per_test_timeout');
$shellType = (int)$litConfig->getParam('external', '1');
if ($shellType == 0) {
   TestLogger::note('Using internal shell');
   $externalShell = false;
} else {
   TestLogger::note('Using external shell');
   $externalShell = true;
}
$configSetTimeout = (int)$litConfig->getParam('set_timeout', '0');
if ($configSetTimeout != 0) {
    // Try setting the max individual test time in the configuration
   $litConfig->setMaxIndividualTestTime($configSetTimeout);
}
$config->setTestFormat(new ShellTestFormat($litConfig, $externalShell));
$config->setSuffixes(['php']);
$config->setTestSourceRoot(dirname(__FILE__));
$config->setTestExecRoot($config->getTestSourceRoot());
$config->setExtraConfig('target_triple', '(unused)');
