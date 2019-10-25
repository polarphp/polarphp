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
use Symfony\Component\Console\Output\OutputInterface;

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
    * @var OutputInterface $output
    */
   private $output;

   /**
    * TestingProgressDisplay constructor.
    * @param array $opts
    * @param int $numTests
    * @param ProgressBar $progressBar
    */
   public function __construct(array $opts, int $numTests, $progressBar, OutputInterface $output)
   {
      $this->opts = $opts;
      $this->numTests = $numTests;
      $this->progressBar = $progressBar;
      $this->output = $output;
      $this->setupOpts();
   }

   private function setupOpts()
   {
      $this->opts += array(
         'show-all' => false
      );
   }

   public function finish()
   {
      if ($this->progressBar) {
         $this->progressBar->clear();
      }
      if ($this->opts['succinct']) {
         $this->output->writeln('');
      }
   }

   public function update(TestCase $test)
   {
      ++$this->completed;
      if ($this->opts['incremental']) {
         $this->updateIncrementalCache($test);
      }
      if ($this->progressBar) {
         $this->progressBar->advance(1);
      }
      $result = $test->getResult();
      $resultCode = $result->getCode();
      $shouldShow = $resultCode->isFailure() || $this->opts['show-all'] ||
         (!$this->opts['quiet'] && !$this->opts['succinct']);
      if (!$shouldShow) {
         return;
      }
      if ($this->progressBar) {
         $this->progressBar->clear();
      }
      // Show the test result line.
      $testName = $test->getFullName();
      $this->output->writeln(sprintf("%s: %s (%d of %d)", $resultCode->getName(), $testName,
         $this->completed, $this->numTests));
      // Show the test failure output, if requested.
      if (($resultCode->isFailure() && $this->opts['verbose']) || $this->opts['show-all']) {
         if ($resultCode->isFailure()) {
            $this->output->writeln(sprintf("%s TEST '%s' FAILED %s", str_repeat('*', 20), $test->getFullName(),
               str_repeat('*', 20)));
         }
         $this->output->writeln($test->getResult()->getOutput());
         $this->output->writeln(str_repeat('*', 20));
      }
      // Report test metrics, if present.
      if (!empty($result->getMetrics())) {
         $this->output->writeln(sprintf("%s TEST '%s' RESULTS %s", str_repeat('*', 10), $test->getFullName(),
            str_repeat('*', 10)));
         $items = $result->getMetrics();
         ksort($items);
         foreach ($items as $metricName => $value) {
            $this->output->writeln(sprintf('%s: %s ', $metricName, $value->format()));
         }
         $this->output->writeln(sprintf(str_repeat('*', 10)));
      }
      // Report micro-tests, if present
      if (!empty($result->getMicroResults())) {
         $items = $result->getMicroResults();
         ksort($items);
         foreach ($items as $microTestName => $microTest) {
            $this->output->writeln(sprintf("%s MICRO-TEST: %s", str_repeat('*', 3), $microTestName));
            if(!empty($microTest->getMetrics())) {
               $sortedMetrics = $microTest->getMetrics();
               ksort($sortedMetrics);
               foreach ($sortedMetrics as $metricName => $value) {
                  $this->output->writeln(sprintf('    %s:  %s ', $metricName, $value->format()));
               }
            }
         }
      }
      flush();
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