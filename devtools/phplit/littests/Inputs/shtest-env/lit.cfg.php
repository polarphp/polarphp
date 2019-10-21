<?php

use Lit\Format\ShellTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */
$config->setName('shtest-env');
$config->setSuffixes(['txt']);
$config->setTestFormat(new ShellTestFormat($litConfig));
$config->setTestSourceRoot('');
$config->setTestExecRoot('');
$config->setEnvVar('FOO', 1);
$config->setEnvVar('BAR', 2);
