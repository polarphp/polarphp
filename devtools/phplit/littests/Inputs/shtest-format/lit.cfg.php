<?php
use Lit\Format\ShellTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */
$config->setName('shtest-format');
$config->setSuffixes(['txt']);
$config->setTestFormat(new ShellTestFormat($litConfig));
$config->setTestSourceRoot('');
$config->setTestExecRoot('');
$config->setExtraConfig('target_triple', 'x86_64-unknown-unknown');
$config->addAvailableFeature('a-present-feature');
