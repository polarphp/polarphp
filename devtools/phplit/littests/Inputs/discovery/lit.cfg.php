<?php
use Lit\Format\ShellTestFormat;
use Lit\Kernel\TestingConfig;

// We intentionally don't set the source root or exec root directories here,
// because this suite gets reused for testing the exec root behavior (in
// ../exec-discovery).
//
// config.test_source_root = None
// config.test_exec_root = None
// Check that arbitrary config values are copied (tested by subdir/lit.local.cfg).
/**
 * @var TestingConfig $config
 */
$config->setName('top-level-suite');
$config->setSuffixes(['txt']);
$config->setExtraConfig('an_extra_variable', false);
$config->setTestFormat(new ShellTestFormat($litConfig));
