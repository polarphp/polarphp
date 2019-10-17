<?php
use Lit\Format\GoogleTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */
$config->setName('googletest-timeout');
$config->setTestFormat(new GoogleTestFormat('DummySubDir', 'Test', $litConfig));
$configSetTimeout = $litConfig->getParam('set_timeout', 0);
if ($configSetTimeout == 1) {
    // Try setting the max individual test time in the configuration
   $litConfig->setMaxIndividualTestTime(1);
}
