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

   public function setDisplay(TestingProgressDisplay $display): TestDispatcher
   {
      $this->display = $display;
      return $this;
   }

   private function consumeTestResult($poolResult)
   {

   }

   private function setupSemaphores()
   {

   }
}