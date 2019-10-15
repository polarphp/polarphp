<?php
use Lit\Format\ShellTestFormat;
use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 */
$config->setName('sub-suite');
$config->setSuffixes(['txt']);
$config->setTestFormat(new ShellTestFormat($litConfig));
$config->setTestSourceRoot('');
$config->setTestExecRoot('');
