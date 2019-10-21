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
use Lit\Shell\TestRunner;

class ShellTestFormat extends FileBasedTestFormat
{
   /**
    * @var bool $executeExternal
    */
   private $executeExternal;
   /**
    * @var TestRunner $testRunner
    */
   private $testRunner;

   public function __construct(LitConfig $litConfig, bool $executeExternal = false)
   {
      parent::__construct($litConfig);
      $this->executeExternal = $executeExternal;
      $this->testRunner = new TestRunner($litConfig, [], $this->executeExternal);
   }

   public function execute(TestCase $test)
   {
      return $this->testRunner->executeTest($test);
   }
}