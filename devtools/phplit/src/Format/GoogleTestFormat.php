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
use Lit\Kernel\TestResultCode;
use Lit\Kernel\TestSuite;
use Lit\Exception\ExecuteCommandTimeoutException;
use Lit\Utils\TestLogger;
use function Lit\Utils\has_substr;
use function Lit\Utils\listdir_files;
use function Lit\Utils\execute_command;
use function Lit\Utils\str_end_with;
use function Lit\Utils\path_split;
use Symfony\Component\Process\Process;
use Symfony\Component\Process\Exception\ProcessFailedException;

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
   public function getGTestTests($path, TestingConfig $localConfig) : iterable
   {
      $listTestCmd = $this->maybeAddPhpToCmd([$path, '--gtest_list_tests']);
      $process = new Process($listTestCmd, null, $localConfig->getEnvironment());
      try {
         $process->mustRun();
      } catch (ProcessFailedException $e) {
         TestLogger::warning("unable to discover google-tests in %s: %s. Process output: %s", $path, $e->getMessage(), $process->getErrorOutput());
         throw $e;
      }
      $output = $process->getOutput();
      $lines = array_filter(explode("\n", $output), function ($value){
         return !empty(trim($value));
      });
      $nestedTests = array();
      foreach ($lines as $line) {
         // Upstream googletest prints this to stdout prior to running
         // tests. LLVM removed that print statement in r61540, but we
         // handle it here in case upstream googletest is being used.
         if (has_substr($line, "Running main() from gtest_main.cc")) {
            continue;
         }
         // The test name list includes trailing comments beginning with
         // a '#' on some lines, so skip those. We don't support test names
         // that use escaping to embed '#' into their name as the names come
         // from C++ class and method names where such things are hard and
         // uninteresting to support.
         $line = rtrim(explode('#', $line, 2)[0]);
         if (empty(ltrim($line))) {
            continue;
         }
         $index = 0;
         while (substr($line, $index * 2, $index * 2 + 2) == '  ') {
            ++$index;
         }
         while (count($nestedTests) > $index) {
            array_pop($nestedTests);
         }
         $line = substr($line, $index * 2);
         if ($line[-1] == '.') {
            $nestedTests[] = $line;
         } elseif ($this->anyTrue($nestedTests+[$line], function ($item) {
            return substr($item, 0, 9) == 'DISABLED_';
         })) {
            // Gtest will internally skip these tests. No need to launch a
            // child process for it.
            continue;
         } else {
            yield join('', $nestedTests) . $line;
         }
      }
   }

   private function anyTrue(array $items, $callback) : bool
   {
      foreach ($items as $item) {
         if ($callback($item)) {
            return true;
         }
      }
      return false;
   }

   public function collectTestsInDirectory(TestSuite $testSuite, array $pathInSuite, TestingConfig $localConfig): iterable
   {
      $sourcePath = $testSuite->getSourcePath($pathInSuite);
      foreach ($this->testSubDirs as $subDir) {
         $dirPath = $sourcePath . DIRECTORY_SEPARATOR . $subDir;
         if (!is_dir($dirPath)) {
            continue;
         }
         foreach (listdir_files($dirPath, $this->testSuffixes) as $fn) {
            // Discover the tests in this executable.
            $execPath = join(DIRECTORY_SEPARATOR, [$sourcePath, $subDir, $fn]);
            $testNames = $this->getGTestTests($execPath, $localConfig);
            foreach ($testNames as $testName) {
               $testPath = array_merge($pathInSuite, [$subDir, $fn, $testName]);
               yield new TestCase($testSuite, $testPath, $localConfig, $execPath);
            }
         }
      }
   }

   public function execute(TestCase $test)
   {
      list($testPath, $testName) = path_split($test->getSourcePath());
      while (!file_exists($testPath)) {
         // Handle GTest parametrized and typed tests, whose name includes
         // some '/'s.
         list($testPath, $namePrefix) = path_split($testPath);
         $testName = $namePrefix . '/' . $testName;
      }
      $cmd = [$testPath, '--gtest_filter=' . $testName];
      $cmd = $this->maybeAddPhpToCmd($cmd);
      if ($this->litConfig->isUseValgrind()) {
         $cmd = $this->litConfig->getValgrindArgs() + $cmd;
      }
      if ($this->litConfig->isNoExecute()) {
         return [TestResultCode::PASS(), ''];
      }
      try {
         list($outMsg, $errorMsg, $exitCode) = execute_command($cmd, null, $test->getConfig()->getEnvironment(), null,
            $this->litConfig->getMaxIndividualTestTime());
      } catch (ExecuteCommandTimeoutException $e) {
         return [TestResultCode::TIMEOUT(), sprintf("Reached timeout of %d seconds", $this->litConfig->getMaxIndividualTestTime())];
      }
      if ($exitCode) {
         return [TestResultCode::FAIL(), $outMsg . $errorMsg];
      }
      $passingTestLine = '[  PASSED  ] 1 test.';
      if (!has_substr($outMsg, $passingTestLine)) {
         $msg = sprintf("Unable to find %r in gtest output:\n\n%s%s", $passingTestLine, $outMsg, $errorMsg);
         return [TestResultCode::UNRESOLVED(), $msg];
      }
      return [TestResultCode::PASS(), ''];
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
   private function maybeAddPhpToCmd(array $cmd): array
   {
      if (str_end_with($cmd[0], '.php')) {
         return array_merge([PHP_BINARY], $cmd);
      }
      return $cmd;
   }
}