<?php
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */
$litConfig->loadConfig($config, dirname(__FILE__).'/../shtest-shell/lit.cfg.php');
$config->setTestSourceRoot(dirname(__FILE__) . '/../shtest-shell');
