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

use Lit\Kernel\NotImplementedException;
use Lit\Kernel\TestCase;
use Lit\Kernel\TestingConfig;
use Lit\Kernel\TestResultCode;
use Lit\Kernel\TestSuite;

class OneCommandPerFileTestFormat extends AbstractTestFormat
{
   /**
    * @var array $commands
    */
   private $commands;
   /**
    * @var string $dir
    */
   private $dir;
   /**
    * @var bool $recursive
    */
   private $recursive;
   /**
    * @var string $pattern
    */
   private $pattern;
   /**
    * @var bool $useTempInput
    */
   private $useTempInput;

   public function __construct($commands, string $dir, bool $recursive, string $pattern = '*', $useTempInput = false)
   {
      if (is_string($commands)) {
         $this->commands = [$commands];
      } else {
         $this->commands = $commands;
      }
      $this->dir = $dir;
      $this->recursive = $recursive;
      $this->pattern = $pattern;
      $this->useTempInput = $useTempInput;
   }

   public function collectTestsInDirectory(TestSuite $testSuite, array $pathInSuite, TestingConfig $localConfig): iterable
   {
      $dir = $this->dir;
      if (empty($dir)) {
         $dir = $testSuite->getSourcePath($pathInSuite);
      }
      $diter = new \RecursiveDirectoryIterator($dir,
         \FilesystemIterator::KEY_AS_PATHNAME | \FilesystemIterator::CURRENT_AS_FILEINFO | \FilesystemIterator::SKIP_DOTS);
      if ($this->recursive) {
         $diter = new \RecursiveIteratorIterator($diter);
      }
      foreach ($diter as $entry) {
         $filename = $diter->getFilename();
         if ($entry->isDir() && ($filename == '.svn' ||
                                 $filename == '.git' ||
                                 in_array($filename, $localConfig->getExcludes()))) {
            continue;
         }
         if ($entry->isFile() &&
             ($filename[0] == '.' || preg_match($this->pattern, $filename) != 1 || in_array($filename, $localConfig->getExcludes()))) {
            continue;
         }
         if ($entry->isFile()) {
            $pathname = $entry->getPathname();
            $suffix = substr($pathname, strlen($dir));
            if ($suffix[0] == DIRECTORY_SEPARATOR) {
               $suffix = substr($suffix, 1);
            }
            $test = new TestCase($testSuite, array_merge($pathInSuite, explode(DIRECTORY_SEPARATOR, $suffix)));
            $test->setManualSpecifiedSourcePath($pathname);
            yield $test;
         }
      }
   }

   public function execute(TestCase $test)
   {
      if ($test->isUnsupported()) {
         return [TestResultCode::UNSUPPORTED(), 'Test is unsupported'];
      }
      $commands = $this->commands;
      // If using temp input, create a temporary file and hand it to the
      // subclass.
      if ($this->isUseTempInput()) {
         // ignore error
         $tempname = tempnam(sys_get_temp_dir(), 'lit_cmd_');
         $tfile = fopen($tempname, 'w');
         $this->createTempInput($tfile, $test);
         fflush($tfile);
         $commands[] = $tempname;
      } elseif(!empty($test->getManualSpecifiedSourcePath())) {
         $commands[] = $test->getManualSpecifiedSourcePath();
      } else {
         $commands[] = $test->getSourcePath();
      }
      list($outMsg, $errMsg, $exitCode) = execute_shell_command($commands);
      $diags = $outMsg . $errMsg;
      if (!$exitCode && empty(trim($diags))) {
         fclose($tfile);
         return [TestResultCode::PASS(), ''];
      }
      // Try to include some useful information.
      $cmdStrs = array();
      foreach ($commands as $command) {
         $cmdStrs[] = sprintf("'%s'", $command);
      }
      $report = sprintf("Command: %s\n", join(' ', $cmdStrs));
      if ($this->useTempInput) {
         $report .= sprintf("Temporary File: %s\n", $tempname);
         $report .= sprintf("--\n%s--\n", file_get_contents($tempname));
      }
      $report .= sprintf("Output:\n--\n%s--", $diags);
      fclose($tfile);
      return [TestResultCode::FAIL(), $report];
   }

   public function createTempInput($tfile, TestCase $test)
   {
      throw new NotImplementedException('This is an abstract method.');
   }

   /**
    * @return array
    */
   public function getCommands(): array
   {
      return $this->commands;
   }

   /**
    * @return string
    */
   public function getDir(): string
   {
      return $this->dir;
   }

   /**
    * @return bool
    */
   public function isRecursive(): bool
   {
      return $this->recursive;
   }

   /**
    * @return string
    */
   public function getPattern(): string
   {
      return $this->pattern;
   }

   /**
    * @return bool
    */
   public function isUseTempInput(): bool
   {
      return $this->useTempInput;
   }
}