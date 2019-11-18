<?php
use Lit\Format\ShellTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;

/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */

$config->setName('parser');
$config->setTestFormat(new ShellTestFormat($litConfig, false));
$config->setSuffixes(['txt']);
$config->setExcludes(['Inputs', 'CMakeLists.txt', 'README.txt', 'LICENSE.txt']);
$config->setTestSourceRoot(dirname(__FILE__));
$config->setTestExecRoot($config->getTestSourceRoot());
$config->addSubstitution('%{inputs}', $config->getTestSourceRoot().DIRECTORY_SEPARATOR.'Inputs');
