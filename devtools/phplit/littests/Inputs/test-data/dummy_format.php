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
class DummyFormat extends FileBasedTestFormat
{
   public function execute(TestCase $test)
   {
      // In this dummy format, expect that each test file is actually just a
      //  .ini format dump of the results to report.
      $sourceDir = $test->getConfig()->getTestSourceRoot();
      $cfgFilename = $sourceDir.DIRECTORY_SEPARATOR.'metrics.ini';
      
   }
}
