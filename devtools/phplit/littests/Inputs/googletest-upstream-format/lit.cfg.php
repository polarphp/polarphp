<?php
use Lit\Format\GoogleTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */
$config->setName('googletest-upstream-format');
$config->setTestFormat(new GoogleTestFormat('DummySubDir', 'Test', $litConfig));
