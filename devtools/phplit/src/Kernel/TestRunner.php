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

use Lit\Utils\TestingProgressDisplay;

class TestRunner
{
   /**
    * @var TestingProgressDisplay $display
    */
   private $display;
   /**
    * @var LitConfig $litConfig
    */
   private $litConfig;
   /**
    * @var array $tests
    */
   private $tests;
   /**
    * array (
    *    'SemaphoreName' => 'count',
    *    ...
    * )
    *
    * @var array $parallelismSemaphores
    */
   private $parallelismSemaphores;

   public function __construct(LitConfig $litConfig, array $tests)
   {
      $this->litConfig = $litConfig;
      $this->tests = $tests;
      // Set up semaphores to limit parallelism of certain classes of tests.
      $this->parallelismSemaphores = $litConfig->getParallelismGroups();
   }

   public function executeTestsInPool(array $jobs, int $maxTime)
   {
      // We need to issue many wait calls, so compute the final deadline and
      // subtract time.time() from that as we go along.
      $deadline = null;
      if ($maxTime) {
         $deadline = time() + $maxTime;
      }
      // Start a process pool. Copy over the data shared between all test runs.
      // FIXME: Find a way to capture the worker process stderr. If the user
      // interrupts the workers before we make it into our task callback, they
      // will each raise a KeyboardInterrupt exception and print to stderr at
      // the same time.
      # Install a console-control signal handler on Windows.
      // TODO: stop pool on console-control received signal on Windows
   }

   /**
    * @return TestingProgressDisplay
    */
   public function getDisplay(): TestingProgressDisplay
   {
      return $this->display;
   }

   /**
    * @return LitConfig
    */
   public function getLitConfig(): LitConfig
   {
      return $this->litConfig;
   }

   /**
    * @return array
    */
   public function getTests(): array
   {
      return $this->tests;
   }
}