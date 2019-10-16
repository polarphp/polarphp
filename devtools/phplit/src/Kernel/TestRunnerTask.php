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

class TestRunnerTask implements TaskInterface
{
   /**
    * @var int $index
    */
   private $index;
   /**
    * @var TestCase $test
    */
   private $test;

   /**
    * TestRunnerTask constructor.
    * @param int $index
    * @param TestCase $test
    */
   public function __construct(int $index, TestCase $test)
   {
      $this->index = $index;
      $this->test = $test;
   }

   public function exec(array $data = array())
   {
   }
}