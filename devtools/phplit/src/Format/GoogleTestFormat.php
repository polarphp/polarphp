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
use Lit\kernel\TestCase;

class GoogleTestFormat extends AbstractTestFormat
{
   /**
    * @var array $testSubDirs
    */
   private $testSubDirs;
   /**
    * @var string $testSuffix
    */
   private $testSuffixes;

   public function __construct(string $testSubDirs, string $testSuffix, LitConfig $litConfig)
   {
      parent::__construct($litConfig);
      $this->testSubDirs = explode(PATH_SEPARATOR, $testSubDirs);
      // On Windows, assume tests will also end in '.exe'.
      $execSuffix = $testSuffix;
      if (PHP_OS_FAMILY == "Windows") {
         $execSuffix .= '.exe';
      }
      // Also check for .py files for testing purposes.
      $this->testSuffixes = [$execSuffix, $testSuffix . '.php'];
   }

   /**
    * Return the tests available in gtest executable.
    * Args:
    * path: String path to a gtest executable
    * localConfig: TestingConfig instance
    *
    * @param $path
    * @param $localConfig
    * @return iterable
    */
   public function getGTestTests($path, $localConfig) : iterable
   {

   }

   public function execute(TestCase $test) : array
   {

   }

   /**
    * Insert the python exe into the command if cmd[0] ends in .py
    * We cannot rely on the system to interpret shebang lines for us on
    * Windows, so add the python executable to the command if this is a .py
    * script.
    *
    * @param array $cmd
    * @return string
    */
   private function maybeAddPhpToCmd(array $cmd) : string
   {
      if (substr($cmd[0], -4) == '.php') {
         return [PHP_BINARY] + $cmd;
      }
      return $cmd;
   }
}