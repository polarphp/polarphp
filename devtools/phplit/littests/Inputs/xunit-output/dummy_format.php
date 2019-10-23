<?php

namespace XuintOutput;

use Lit\Format\FileBasedTestFormat;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestCase;
use Lit\Kernel\TestingConfig;
use Lit\Kernel\TestResult;
use Lit\Kernel\TestResultCode;
use Lit\Shell\IntMetricValue;
use Lit\Shell\RealMetricValue;

/**
 * @var TestingConfig $config
 * @var LitConfig $litConfig
 */
class DummyFormat extends FileBasedTestFormat
{
   public function execute(TestCase $test)
   {
      $sourcePath = $test->getSourcePath();
      // In this dummy format, expect that each test file is actually just a
      // .ini format dump of the results to report.
      $config = parse_ini_file($sourcePath, true, INI_SCANNER_RAW);
      // Create the basic test result.
      $resultCode = $config['global']['result_code'];
      $resultOutput = $config['global']['result_output'];
      $methodName = 'Lit\Kernel\TestResultCode::'.$resultCode;
      $result = new TestResult($methodName(), $resultOutput);

      // Load additional metrics.
      foreach ($config['results'] as $key => $valueStr) {
         $value = null;
         eval("\$value = $valueStr;");
         if (is_int($value)) {
            $metric = new IntMetricValue($value);
         } elseif (is_double($value)) {
            $metric = new RealMetricValue($value);
         } else {
            throw new \RuntimeException('unsupported result type');
         }
         $result->addMetric($key, $metric);
      }
      return $result;
   }
}