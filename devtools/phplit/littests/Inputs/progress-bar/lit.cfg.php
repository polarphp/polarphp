<?php

use Lit\Format\ShellTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */
$config->setName('progress-bar');
$config->setSuffixes(['txt']);
$config->setTestFormat(new ShellTestFormat($litConfig));
$config->setTestSourceRoot('');
$config->setTestExecRoot('');

