<?php

use Lit\Format\ShellTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */
$config->setName('lit');
$config->setTestFormat(new ShellTestFormat($litConfig, false));
$config->setSuffixes(['php']);
$config->setExcludes(['Inputs']);
$config->setTestSourceRoot(dirname(__FILE__));
$config->setTestExecRoot($config->getTestSourceRoot());
$config->setExtraConfig('target_triple', '(unused)');
$config->addSubstitution('%{phpuint}', LIT_ROOT_DIR.DIRECTORY_SEPARATOR.'bin'.DIRECTORY_SEPARATOR.'phpunit');
$config->addSubstitution('%{inputs}', $config->getTestSourceRoot().DIRECTORY_SEPARATOR.'Inputs');