<?php

use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */

include dirname(__FILE__).DIRECTORY_SEPARATOR.'dummy_format.php';
$config->setTestFormat(new \TestDataMicro\DummyFormat($litConfig));
$config->setName('test-data-micro');
$config->setSuffixes(['ini']);
$config->setTestSourceRoot('');
$config->setTestExecRoot('');
$config->setExtraConfig('target_triple', null);
