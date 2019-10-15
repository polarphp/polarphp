<?php

use Lit\Format\ShellTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
use Lit\Utils\TestLogger;

/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */

// Verify that the site configuration was loaded.
if (empty($config->getTestSourceRoot()) || empty($config->getTestExecRoot())) {
   TestLogger::fatal('No site specific configuration');
}
$config->setName('exec-discovery-in-tree-suite');
$config->setSuffixes(['txt']);
$config->setTestFormat(new ShellTestFormat($litConfig));
