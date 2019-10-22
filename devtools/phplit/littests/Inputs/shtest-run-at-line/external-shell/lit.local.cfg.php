<?php
/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */

use Lit\Format\ShellTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestingConfig;
$config->setTestFormat(new ShellTestFormat($litConfig, true));
