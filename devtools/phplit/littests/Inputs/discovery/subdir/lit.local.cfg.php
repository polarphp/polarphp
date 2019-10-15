<?php

use Lit\Kernel\TestingConfig;
/**
 * @var TestingConfig $config
 */
$config->setSuffixes(['php']);

// Check that the arbitrary config values in our parent was inherited.
assert($config->hasExtraConfig('an_extra_variable'));