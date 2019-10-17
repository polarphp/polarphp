<?php
use Lit\Format\ShellTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */
$config->setTestFormat(new ShellTestFormat($litConfig, true));
