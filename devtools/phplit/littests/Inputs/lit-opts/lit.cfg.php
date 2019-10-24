<?php

use Lit\Format\ShellTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */
$config->setName('lit-opts');
$config->setSuffixes(['txt']);
$config->setTestFormat(new ShellTestFormat($litConfig));
$config->setTestExecRoot('');
$config->setTestSourceRoot('');
$config->addSubstitution('%var', $litConfig->getParam('var', ''));

