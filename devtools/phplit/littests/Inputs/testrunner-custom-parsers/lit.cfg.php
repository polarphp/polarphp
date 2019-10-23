<?php

use Lit\Format\FileBasedTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestCase;
use Lit\Kernel\TestingConfig;
use Lit\Kernel\TestResultCode;

/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */

class TestParserFormat extends FileBasedTestFormat
{
   public function execute(TestCase $test)
   {
      return [TestResultCode::PASS(), ''];
   }
}

$config->setName('custom-parsers');
$config->setSuffixes(['txt']);
$config->setTestFormat(new TestParserFormat($litConfig));
$config->setTestExecRoot('');
$config->setTestSourceRoot('');
$config->setExtraConfig('target_triple', 'x86_64-unknown-unknown');
