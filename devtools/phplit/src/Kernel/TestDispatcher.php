<?php
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/10/11.
namespace Lit\Kernel;

use Lit\Application;
use Lit\Utils\TestingProgressDisplay;

class TestDispatcher
{
   /**
    * @var LitConfig $litConfig
    */
   private $litConfig;
   /**
    * @var iterable $tests
    */
   private $tests;

   /**
    * @var TestingProgressDisplay $display
    */
   private $display;

   /**
    * @var array $parallelismSemaphores
    */
   private $parallelismSemaphores;

   public function __construct(LitConfig $litConfig, iterable $tests)
   {
      $this->litConfig = $litConfig;
      $this->tests = $tests;
      // Set up semaphores to limit parallelism of certain classes of tests.
      $this->setupSemaphores();
   }

   public function executeTestsInPool(array $jobs, int $maxTime)
   {

   }

   public function executeTests(array $jobs, $maxTime = null)
   {

   }

   public function writeTestResults(int $testingTime, string $outputPath)
   {
      // Construct the data we will write.
      $data = array();
      // Encode the current lit version as a schema version.
      $data['__version__'] = Application::VERSION;
      $data['elapsed'] = $testingTime;
      // FIXME: Record some information on the lit configuration used?
      // FIXME: Record information from the individual test suites?
      // Encode the tests.
      $data['tests'] = $testsData = [];
      foreach ($this->tests as $test) {
         $result = $test->getResult();
         $testData = array(
            'name' => $test->getFullName(),
            'code' => $result->getCode()->getName(),
            'output' => $result->getOutput(),
            'elapsed' => $result->getElapsed()
         );
         // Add test metrics, if present.
         if (!empty($result->getMetrics())) {
            $testData['metrics'] = $metricsData = [];
            foreach ($result->getMetrics() as $key => $value) {
               $metricsData[$key] = $value->toData();
            }
         }
         // Report micro-tests separately, if present
         if (!empty($result->getMicroResults())) {
            foreach ($result->getMicroResults() as $key => $microTest) {
               // Expand parent test name with micro test name
               $parentName = $test->getFullName();
               $microFullName = $parentName . ':' . $key;
               $microTestData = array(
                  'name' => $microFullName,
                  'code' => $microTest->getCode->getName(),
                  'output' => $microTest->getOutput(),
                  'elapsed' => $microTest->getElapsed(),
               );
               if ($microTest->getMetrics()) {
                  $microTestData['metrics'] = $microMetricsData = array();
                  foreach ($microTest->getMetrics() as $key => $value) {
                     $microMetricsData[$key] = $value->toData();
                  }
               }
               $testsData[] = $microTestData;
            }
         }
         $testsData[] = $testData;
      }
      // Write the output.
      file_put_contents($outputPath, json_encode($data)."\n");
   }

   public function setDisplay(TestingProgressDisplay $display): TestDispatcher
   {
      $this->display = $display;
      return $this;
   }

   /**
    * @return iterable
    */
   public function getTests(): iterable
   {
      return $this->tests;
   }

   public function setTests(array $tests): TestDispatcher
   {
      $this->tests = $tests;
      return $this;
   }

   private function consumeTestResult($poolResult)
   {

   }

   private function setupSemaphores()
   {

   }

}