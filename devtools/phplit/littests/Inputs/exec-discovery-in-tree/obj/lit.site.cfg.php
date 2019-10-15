<?php
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 */
$config->setTestExecRoot(dirname(__FILE__));
$config->setTestSourceRoot(dirname($config->getTestExecRoot()));
/**
 * @var LitConfig $litConfig
 */
$litConfig->loadConfig($config, $config->getTestSourceRoot().DIRECTORY_SEPARATOR.'lit.cfg.php');