<?php
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */
$config->setName('test-data');
$config->setSuffixes(['ini']);
include dirname(__FILE__).DIRECTORY_SEPARATOR.'dummy_format.php';
$config->setTestFormat(new \XuintOutput\DummyFormat($litConfig));
$config->setTestSourceRoot('');
$config->setTestExecRoot('');
$config->setExtraConfig('target_triple', null);

