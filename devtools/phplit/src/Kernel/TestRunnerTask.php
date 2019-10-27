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
// Created by polarboy on 2019/10/16.

namespace Lit\Kernel;

use Lit\ProcessPool\TaskInterface;
use Lit\Utils\ConsoleLogger;
use Lit\Utils\TestLogger;
use Symfony\Component\Process\Exception\ProcessSignaledException;
use Lit\Exception\ValueException;

class TestRunnerTask implements TaskInterface
{
   /**
    * @var int $index
    */
   protected $index;
   /**
    * @var TestCase $test
    */
   protected $test;

   /**
    * @var LitConfig $litConfig
    */
   private $litConfig;

   /**
    * @var ConsoleLogger $logger
    */
   private $logger;

   /**
    * TestRunnerTask constructor.
    * @param int $index
    * @param TestCase $test
    */
   public function __construct(int $index, TestCase $test, LitConfig $litConfig)
   {
      $this->index = $index;
      $this->test = $test;
      $this->litConfig = $litConfig;
   }

   public function setLogger(ConsoleLogger $logger)
   {
      $this->logger = $logger;
   }

   public function exec(array $data = array())
   {
      try {
         $startTime = microtime(true);
         $result = $this->test->getConfig()->getTestFormat()->execute($this->test);
         if (is_array($result)) {
            list($code, $output) = $result;
            $result = new TestResult($code, $output);
         } elseif (!$result instanceof TestResult) {
            throw new ValueException('unexpected result from test execution');
         }
         $result->setElapsed(microtime(true) - $startTime);
      } catch (ProcessSignaledException $e) {
         // TODO handle some type signal?
         throw $e;
      } catch (\Exception $e) {
         $output = "Exception during script execution:\n";
         $output .= $e->getMessage();
         $output .= "\n";
         $result = new TestResult(TestResultCode::UNRESOLVED(), $output);
         if ($this->litConfig->isDebug()) {
            TestLogger::errorWithoutCount($e->getTraceAsString());
         }
      }
      $this->test->setResult($result);
   }

   public function writeToStdout($format, ...$args)
   {
      fwrite(STDOUT, sprintf($format, ...$args));
   }

   public function writeToStderr($format, ...$args)
   {
      fwrite(STDERR, sprintf($format, ...$args));
   }
}