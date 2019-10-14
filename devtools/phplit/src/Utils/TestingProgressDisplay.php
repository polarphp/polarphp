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
// Created by polarboy on 2019/10/12.
namespace Lit\Utils;
use Lit\Kernel\TestCase;
use Symfony\Component\Console\Helper\ProgressBar;

class TestingProgressDisplay
{
   /**
    * @var array $opts
    */
   private $opts;
   /**
    * @var int $numTests
    */
   private $numTests;
   /**
    * @var ProgressBar $progressBar
    */
   private $progressBar;
   /**
    * @var int $completed
    */
   private $completed = 0;

   /**
    * TestingProgressDisplay constructor.
    * @param array $opts
    * @param int $numTests
    */
   public function __construct(array $opts, int $numTests)
   {
      $this->opts = $opts;
      $this->numTests = $numTests;
      $this->setupProgressBar();
   }

   public function finish()
   {
      $this->progressBar->clear();
      if ($this->opts['succinct']) {
         echo "\n";
      }
   }

   public function update(TestCase $test)
   {
      ++$this->completed;
      if ($this->opts['incremental']) {
         $this->updateIncrementalCache($test);
      }
      // TODO update progress bar
      $result = $test->getResult();
      $resultCode = $result->getCode();
      $shouldShow = $resultCode->isFailure() || $this->opts['showAllOutput'] ||
         (!$this->opts['quiet'] && !$this->opts['succinct']);
      if (!$shouldShow) {
         return;
      }
      $this->progressBar->clear();
      // Show the test result line.
      $testName = $test->getFullName();
      printf('%s: %s (%d of %d)' % $resultCode->getName(), $testName,
         $this->completed, $this->numTests);
      // Show the test failure output, if requested.
      if (($resultCode->isFailure() && $this->opts['showOutput']) || $this->opts['showAllOutput']) {
         if ($resultCode->isFailure()) {
            printf("%s TEST '%s' FAILED %s", str_repeat('*', 20), $test->getFullName(),
               str_repeat('*', 20));
         }
         printf($test->getResult()->getOutput());
         printf(str_repeat('*', 20));
      }
      // Report test metrics, if present.
      if (!empty($result->getMetrics())) {
         printf("%s TEST '%s' RESULTS %s", str_repeat('*', 10), $test->getFullName(),
            str_repeat('*', 10));
         $items = $result->getMetrics();
         sort($items);
         foreach ($items as $metricName => $value) {
            printf('%s: %s ', $metricName, $value->format());
         }
         printf(str_repeat('*', 10));
      }
      // Report micro-tests, if present
      if (!empty($result->getMicroResults())) {
         $items = $result->getMicroResults();
         sort($items);
         foreach ($items as $microTestName => $microTest) {
            printf("%s MICRO-TEST: %s", str_repeat('*', 3), $microTestName);
            if(!empty($microTest->getMetrics())) {
               $sortedMetrics = $microTest->getMetrics();
               sort($sortedMetrics);
               foreach ($sortedMetrics as $metricName => $value) {
                  printf('    %s:  %s ', $metricName, $value->format());
               }
            }
         }
      }
      flush();
   }

   private function setupProgressBar()
   {

   }

   private function updateIncrementalCache(TestCase $test)
   {
      if (!$test->getResult()->getCode()->isFailure()) {
         return;
      }
      $fname = $test->getFilePath();
      touch($fname);
   }
}