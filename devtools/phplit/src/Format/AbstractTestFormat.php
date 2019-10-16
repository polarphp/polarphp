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
namespace Lit\Format;

use Lit\Kernel\LitConfig;
use Lit\Kernel\TestCase;
use Lit\Kernel\TestingConfig;
use Lit\Kernel\TestSuite;

class AbstractTestFormat implements TestFormatInterface
{
   /**
    * @var LitConfig $litConfig
    */
   protected $litConfig;

   public function __construct(LitConfig $litConfig)
   {
      $this->litConfig = $litConfig;
   }

   public function collectTestsInDirectory(TestSuite $testSuite, array $pathInSuite, TestingConfig $localConfig): iterable
   {
      return null;
   }

   public function execute(TestCase $test)
   {}

   /**
    * @return LitConfig
    */
   public function getLitConfig(): LitConfig
   {
      return $this->litConfig;
   }
}