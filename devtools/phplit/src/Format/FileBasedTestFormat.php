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

use Lit\kernel\TestCase;
use Lit\Kernel\TestingConfig;
use Lit\Kernel\TestSuite;

class FileBasedTestFormat extends AbstractTestFormat
{
   public function collectTestsInDirectory(TestSuite $testSuite, array $pathInSuite, TestingConfig $localConfig)
   {
      $sourcePath = $testSuite->getSourcePath($pathInSuite);
      $diter = new \DirectoryIterator($sourcePath);
      $excludes = $localConfig->getExcludes();
      $suffixes = $localConfig->getSuffixes();
      foreach ($diter as $entry) {
         // Ignore dot files and excluded tests.
         $filename = $entry->getFilename();
         if ($entry->isDot() || in_array($filename, $excludes)) {
            continue;
         }
         if ($entry->isFile()) {
            $ext = $entry->getExtension();
            if (in_array($ext, $suffixes)) {
               yield new TestCase($testSuite, $pathInSuite + [$filename], $localConfig);
            }
         }
      }
   }
}