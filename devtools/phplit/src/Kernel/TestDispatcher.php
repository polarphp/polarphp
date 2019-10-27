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

use http\Exception\RuntimeException;
use Lit\Application;
use Lit\ProcessPool\Events\ProcessFinished;
use Lit\ProcessPool\ProcessPool;
use Lit\Utils\TestingProgressDisplay;
use Symfony\Component\Console\Output\OutputInterface;
use Symfony\Component\Process\Exception\ProcessTimedOutException;
use Symfony\Component\Process\Process;

class TestDispatcher
{
   /**
    * @var LitConfig $litConfig
    */
   private $litConfig;
   /**
    * @var TestCase[] $tests
    */
   private $tests;

   /**
    * @var TestingProgressDisplay $display
    */
   private $display;

   /**
    * array (
    *    'SemaphoreName' => 'count',
    *    ...
    * )
    *
    * @var array $parallelismSemaphores
    */
   private $parallelismSemaphores;

   /**
    * @var int $failureCount
    */
   private $failureCount = 0;
   /**
    * @var bool $hitMaxFailures
    */
   private $hitMaxFailures = false;

   public function __construct(LitConfig $litConfig, array $tests)
   {
      $this->litConfig = $litConfig;
      $this->tests = $tests;
      // Set up semaphores to limit parallelism of certain classes of tests.
      $this->parallelismSemaphores = $litConfig->getParallelismGroups();
   }

   public function executeTestsInPool(int $jobs, $maxTime)
   {
      // We need to issue many wait calls, so compute the final deadline and
      // subtract time.time() from that as we go along.
      $deadline = null;
      if ($maxTime) {
         $deadline = time() + (int)$maxTime;
      }
      // Start a process pool. Copy over the data shared between all test runs.
      // FIXME: Find a way to capture the worker process stderr. If the user
      // interrupts the workers before we make it into our task callback, they
      // will each raise a KeyboardInterrupt exception and print to stderr at
      // the same time.
      $processPool = new ProcessPool(new TestRunnerInitializer(), [
         'concurrency' => $jobs,
         'maxTimeout' => $deadline,
         'throwExceptions' => true
      ]);
      $self = $this;
      $processTestMap = [];
      $processPool->setProcessFinishedCallback(function (ProcessFinished $event) use ($self, &$processTestMap, $processPool) {
         list($testIndex, $test) = $self->handleTaskProcessResult($event->getProcess(), $processTestMap);
         $self->setTest($testIndex, $test);
         $this->consumeTestResult($testIndex, $test);
         if ($this->hitMaxFailures) {
            $processPool->terminate();
         }
      });
      try {
         foreach ($this->tests as $index => $test) {
            $process = $processPool->postTask(new TestRunnerPoolTask($index, $test, $this->litConfig));
            $processTestMap[] = [$process, $index, $test];
         }
         $processPool->wait();
      } catch (ProcessTimedOutException $e) {
         throw new \RuntimeException(sprintf("maxTime timeout of %s seconds.", $maxTime));
      } finally {
         $processPool->terminate();
      }
   }

   public function handleTaskProcessResult(Process $process, array $processTestMap): array
   {
      $origTestIndex = null;
      /**
       * @var TestCase $origTest
       */
      $origTest = null;
      foreach ($processTestMap as $entry) {
         if ($process === $entry[0]) {
            $origTestIndex = $entry[1];
            $origTest = $entry[2];
            break;
         }
      }
      assert($origTestIndex !== null);
      assert($origTest !== null);
      $exitCode = $process->getExitCode();
      if ($exitCode != 0) {
         $result = new TestResult(TestResultCode::UNRESOLVED(), $process->getErrorOutput());
         $origTest->setResult($result);
         return [$origTestIndex, $origTest];
      }
      $output = $process->getOutput();
      assert(!empty($output));
      $returnData = unserialize($output);
      assert(false !== $returnData && is_array($returnData) && count($returnData) == 2);
      return $returnData;
   }

   /**
    * Execute each of the tests in the run, using up to jobs number of
    * parallel tasks, and inform the display of each individual result. The
    * provided tests should be a subset of the tests available in this run
    * object.
    *
    * If max_time is non-None, it should be a time in seconds after which to
    * stop executing tests.
    *
    * The display object will have its update method called with each test as
    * it is completed. The calls are guaranteed to be locked with respect to
    * one another, but are *not* guaranteed to be called on the same thread as
    * this method was invoked on.
    *
    * Upon completion, each test in the run will have its result
    * computed. Tests which were not actually executed (for any reason) will
    * be given an UNRESOLVED result.
    *
    * @param int $jobs
    * @param null $maxTime
    */
   public function executeTests(int $jobs, $maxTime = null)
   {
      $tests = $this->tests;
      if (empty($tests)) {
         return;
      }
      $litConfig = $this->litConfig;
      if ($jobs == 1) {
         foreach ($this->tests as $index => $test) {
            $testTask = new TestRunnerTask($index, $test, $litConfig);
            $testTask->exec();
            $this->consumeTestResult($index, $test);
            if ($this->hitMaxFailures) {
               break;
            }
         }
      } else {
         $this->executeTestsInPool($jobs, $maxTime);
      }
      // Mark any tests that weren't run as UNRESOLVED.
      foreach ($tests as $test) {
         if (!$test->hasResult()) {
            $test->setResult(new TestResult(TestResultCode::UNRESOLVED(), '', 0.0));
         }
      }
   }

   public function writeTestResults(float $testingTime, string $outputPath)
   {
      // Construct the data we will write.
      $data = array();
      // Encode the current lit version as a schema version.
      $data['__version__'] = Application::VERSION;
      $data['elapsed'] = $testingTime;
      // FIXME: Record some information on the lit configuration used?
      // FIXME: Record information from the individual test suites?
      // Encode the tests.
      $data['tests'] = [];
      $testsData = &$data['tests'];
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
            $testData['metrics'] = [];
            $metricsData = &$testData['metrics'];
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
                  'code' => $microTest->getCode()->getName(),
                  'output' => $microTest->getOutput(),
                  'elapsed' => $microTest->getElapsed(),
               );
               if ($microTest->getMetrics()) {
                  $microTestData['metrics'] = [];
                  $microMetricsData = &$microTestData['metrics'];
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
      file_put_contents($outputPath, json_encode($data, JSON_PRETTY_PRINT)."\n");
   }

   public function setDisplay(TestingProgressDisplay $display): TestDispatcher
   {
      $this->display = $display;
      return $this;
   }

   /**
    * @return array
    */
   public function getTests(): array
   {
      return $this->tests;
   }

   public function setTests(array $tests): TestDispatcher
   {
      $this->tests = $tests;
      return $this;
   }

   public function setTest(int $index, TestCase $test): void
   {
      $this->tests[$index] = $test;
   }

   /**
    * Test completion callback
    * Updates the test result status in the parent process. Each task in the
    * pool returns the test index and the result, and we use the index to look
    * up the original test object. Also updates the progress bar as tasks
    * complete.
    *
    * @param int $testIndex
    * @param TestCase $test
    */
   private function consumeTestResult(int $testIndex, TestCase $test)
   {
      // Don't add any more test results after we've hit the maximum failure
      // count.  Otherwise we're racing with the main thread, which is going
      // to terminate the process pool soon.
      if ($this->hitMaxFailures) {
         return;
      }
      // Update the parent process copy of the test. This includes the result,
      // XFAILS, REQUIRES, and UNSUPPORTED statuses.
      assert($this->tests[$testIndex]->getFilePath() == $test->getFilePath(),
            'parent and child disagree on test path');
      $this->tests[$testIndex] = $test;
      $this->display->update($test);
      // If we've finished all the tests or too many tests have failed, notify
      // the main thread that we've stopped testing.
      $this->failureCount += $test->getResult()->getCode() == TestResultCode::FAIL();
      $maxFailures = $this->litConfig->getMaxFailures();
      if ($maxFailures && $this->failureCount == $maxFailures) {
         $this->hitMaxFailures = true;
      }
   }

   /**
    * @return int
    */
   public function getFailureCount(): int
   {
      return $this->failureCount;
   }

   /**
    * @return bool
    */
   public function isHitMaxFailures(): bool
   {
      return $this->hitMaxFailures;
   }
}