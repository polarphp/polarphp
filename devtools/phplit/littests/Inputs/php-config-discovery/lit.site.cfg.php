<?php
/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */
# Load the discovery suite, but with a separate exec root.
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
$config->setTestExecRoot(dirname(__FILE__));
$config->setTestSourceRoot(dirname($config->getTestExecRoot()).DIRECTORY_SEPARATOR.'discovery');
$litConfig->loadConfig($config, $config->getTestSourceRoot().DIRECTORY_SEPARATOR.'lit.cfg.php');

