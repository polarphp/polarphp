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
// Created by polarboy on 2019/10/09.

namespace Lit\Kernel;
use Lit\Utils\TestLogger;
use phpDocumentor\Reflection\Types\Mixed_;
use function Lit\Utils\check_tools_path;
use function Lit\Utils\execute_command;
use function Lit\Utils\is_absolute_path;
use function Lit\Utils\which;
use function Lit\Utils\which_tools;

/**
 * LitConfig - Configuration data for a 'lit' test runner instance, shared
 * across all tests.
 * The LitConfig object is also used to communicate with client configuration
 * files, it is always passed in as the global variable 'lit' so that
 * configuration files can access common functionality and internal components
 * easily.
 *
 * @package Lit\Kernel
 */
class LitConfig
{
   /**
    * The name of the test runner.
    * @var string $programName
    */
   private $programName;
   /**
    * The items to add to the PATH environment variable.
    * @var array $paths
    */
   private $paths = array();

   /**
    * @var boolean $quite
    */
   private $quite;

   /**
    * @var boolean $useValgrind
    */
   private $useValgrind;

   /**
    * @var boolean $valgrindLeakCheck
    */
   private $valgrindLeakCheck;

   /**
    * @var array $valgrindUserArgs
    */
   private $valgrindUserArgs;

   /**
    * @var boolean $noExecute
    */
   private $noExecute;

   /**
    * @var boolean $debug
    */
   private $debug;

   /**
    * @var boolean $isWindows
    */
   private $isWindows;

   /**
    * @var array $params
    */
   private $params;

   /**
    * @var string $bashPath
    */
   private $bashPath;

   /**
    * @var string $configPrefix
    */
   private $configPrefix;

   /**
    * @var array $suffixes
    */
   private $suffixes;

   /**
    * @var array $configNames
    */
   private $configNames;

   /**
    * @var array $siteConfigNames
    */
   private $siteConfigNames = array();

   /**
    * @var array $localConfigNames
    */
   private $localConfigNames;

   /**
    * @var array $valgrindArgs
    */
   private $valgrindArgs = array();

   /**
    * @var int $maxIndividualTestTime
    */
   private $maxIndividualTestTime;

   /**
    * @var int $maxFailures
    */
   private $maxFailures;

   /**
    * @var array $parallelismGroups
    */
   private $parallelismGroups = array();

   /**
    * @var boolean $echoAllCommands
    */
   private $echoAllCommands;

   public function __construct(string $programName, array $paths, bool $quiet,
                               bool $useValgrind, bool $valgrindLeakCheck, array $valgrindArgs,
                               bool $noExecute, bool $debug, bool $isWindows,
                               array $params, string $configPrefix = null, int $maxIndividualTestTime = 0,
                               $maxFailures = null, array $parallelismGroups = [], bool $echoAllCommands = false)
   {
      $this->programName = $programName;
      foreach ($paths as $path) {
         $this->paths[] = strval($path);
      }
      $this->quite = $quiet;
      $this->useValgrind = $useValgrind;
      $this->valgrindLeakCheck = $valgrindLeakCheck;
      $this->valgrindUserArgs = $valgrindArgs;
      $this->noExecute = $noExecute;
      $this->debug = $debug;
      $this->isWindows = $isWindows;
      $this->params = $params;
      // Configuration files to look for when discovering test suites.
      $this->configPrefix = is_null($configPrefix) ? 'lit' : $configPrefix;
      $this->suffixes = ['cfg.php', 'cfg'];
      foreach ($this->suffixes as $suffix) {
         $this->configNames[] = sprintf('%s.site.%s', $this->configPrefix, $suffix);
      }
      foreach ($this->suffixes as $suffix) {
         $this->siteConfigNames[] = sprintf('%s.%s', $this->configPrefix, $suffix);
      }
      foreach ($this->suffixes as $suffix) {
         $this->localConfigNames[] = sprintf('%s.local.%s', $this->configPrefix, $suffix);
      }
      if ($this->useValgrind) {
         $this->valgrindArgs = [
            'valgrind', '-q', '--run-libc-freeres=no',
            '--tool=memcheck', '--trace-children=yes',
            '--error-exitcode=123'
         ];
         if ($this->valgrindLeakCheck) {
            $this->valgrindArgs[] = '--leak-check=full';
         } else {
            $this->valgrindArgs[] = '--leak-check=no';
         }
         $this->valgrindArgs += $this->valgrindUserArgs;
      }
      $this->maxIndividualTestTime = $maxIndividualTestTime;
      $this->maxFailures = $maxFailures;
      $this->parallelismGroups = $parallelismGroups;
      $this->echoAllCommands = $echoAllCommands;
   }

   /**
    * @return string
    */
   public function getProgramName(): string
   {
      return $this->programName;
   }

   /**
    * @param string $programName
    * @return LitConfig
    */
   public function setProgramName(string $programName): LitConfig
   {
      $this->programName = $programName;
      return $this;
   }

   /**
    * @return array
    */
   public function getPaths(): array
   {
      return $this->paths;
   }

   /**
    * @param array $paths
    * @return LitConfig
    */
   public function setPaths(array $paths): LitConfig
   {
      $this->paths = $paths;
      return $this;
   }

   /**
    * @return bool
    */
   public function isQuite(): bool
   {
      return $this->quite;
   }

   /**
    * @param bool $quite
    * @return LitConfig
    */
   public function setQuite(bool $quite): LitConfig
   {
      $this->quite = $quite;
      return $this;
   }

   /**
    * @return bool
    */
   public function isUseValgrind(): bool
   {
      return $this->useValgrind;
   }

   /**
    * @param bool $useValgrind
    * @return LitConfig
    */
   public function setUseValgrind(bool $useValgrind): LitConfig
   {
      $this->useValgrind = $useValgrind;
      return $this;
   }

   /**
    * @return bool
    */
   public function isValgrindLeakCheck(): bool
   {
      return $this->valgrindLeakCheck;
   }

   /**
    * @param bool $valgrindLeakCheck
    * @return LitConfig
    */
   public function setValgrindLeakCheck(bool $valgrindLeakCheck): LitConfig
   {
      $this->valgrindLeakCheck = $valgrindLeakCheck;
      return $this;
   }

   /**
    * @return array
    */
   public function getValgrindUserArgs(): array
   {
      return $this->valgrindUserArgs;
   }

   /**
    * @param array $valgrindUserArgs
    * @return LitConfig
    */
   public function setValgrindUserArgs(array $valgrindUserArgs): LitConfig
   {
      $this->valgrindUserArgs = $valgrindUserArgs;
      return $this;
   }

   /**
    * @return bool
    */
   public function isNoExecute(): bool
   {
      return $this->noExecute;
   }

   /**
    * @param bool $noExecute
    * @return LitConfig
    */
   public function setNoExecute(bool $noExecute): LitConfig
   {
      $this->noExecute = $noExecute;
      return $this;
   }

   /**
    * @return bool
    */
   public function isDebug(): bool
   {
      return $this->debug;
   }

   /**
    * @param bool $debug
    * @return LitConfig
    */
   public function setDebug(bool $debug): LitConfig
   {
      $this->debug = $debug;
      return $this;
   }

   /**
    * @return bool
    */
   public function isWindows(): bool
   {
      return $this->isWindows;
   }

   /**
    * @param bool $isWindows
    * @return LitConfig
    */
   public function setIsWindows(bool $isWindows): LitConfig
   {
      $this->isWindows = $isWindows;
      return $this;
   }

   /**
    * @return array
    */
   public function getParams(): array
   {
      return $this->params;
   }

   /**
    * @param array $params
    * @return LitConfig
    */
   public function setParams(array $params): LitConfig
   {
      $this->params = $params;
      return $this;
   }

   public function getParam(string $name, $defaultValue = null)
   {
      if (!array_key_exists($name, $this->params)) {
         return $defaultValue;
      }
      return $this->params[$name];
   }

   public function setParam(string $name, $value): LitConfig
   {
      $this->params[$name] = $value;
      return $this;
   }

   /**
    * @return string
    */
   public function getBashPath(): string
   {
      if ($this->bashPath) {
         return $this->bashPath;
      }
      $this->bashPath = which('bash', join(PATH_SEPARATOR, $this->paths));
      if ($this->bashPath == '') {
         $this->bashPath = which('bash');
      }
      // Check whether the found version of bash is able to cope with paths in
      // the host path format. If not, don't return it as it can't be used to
      // run scripts. For example, WSL's bash.exe requires '/mnt/c/foo' rather
      // than 'C:\\foo' or 'C:/foo'.
      if ($this->isWindows && $this->bashPath) {
         $command = [$this->bashPath, '-c', sprintf('[[ -f "%s" ]]', str_replace('\\', '\\\\', $this->bashPath))];
         list($out, $err, $exitCode) = execute_command($command);
         if ($exitCode) {
            TestLogger::note('bash command failed: %s', join('"%s"', $command));
            $this->bashPath = '';
         }
      }
      if (empty($this->bashPath)) {
         TestLogger::warning('Unable to find a usable version of bash.');
      }
      return $this->bashPath;
   }

   public function getToolsPath(string $dir, array $paths, array $tools): string
   {
      if (!empty($dir) && is_absolute_path($dir) && is_dir($dir)) {
         if (!check_tools_path($dir, $tools)) {
            return '';
         }
      } else {
         $dir = which_tools($tools, $paths);
      }
      // bash
      $this->bashPath = which('bash', $dir);
      if ($this->bashPath == null) {
         $this->bashPath = '';
      }
      return $dir;
   }

   /**
    * @return string
    */
   public function getConfigPrefix(): string
   {
      return $this->configPrefix;
   }

   /**
    * @param string $configPrefix
    * @return LitConfig
    */
   public function setConfigPrefix(string $configPrefix): LitConfig
   {
      $this->configPrefix = $configPrefix;
      return $this;
   }

   /**
    * @return array
    */
   public function getSuffixes(): array
   {
      return $this->suffixes;
   }

   /**
    * @param array $suffixes
    * @return LitConfig
    */
   public function setSuffixes(array $suffixes): LitConfig
   {
      $this->suffixes = $suffixes;
      return $this;
   }

   /**
    * @return array
    */
   public function getConfigNames(): array
   {
      return $this->configNames;
   }

   /**
    * @param array $configNames
    * @return LitConfig
    */
   public function setConfigNames(array $configNames): LitConfig
   {
      $this->configNames = $configNames;
      return $this;
   }

   /**
    * @return array
    */
   public function getSiteConfigNames(): array
   {
      return $this->siteConfigNames;
   }

   /**
    * @param array $siteConfigNames
    * @return LitConfig
    */
   public function setSiteConfigNames(array $siteConfigNames): LitConfig
   {
      $this->siteConfigNames = $siteConfigNames;
      return $this;
   }

   /**
    * @return array
    */
   public function getLocalConfigNames(): array
   {
      return $this->localConfigNames;
   }

   /**
    * @param array $localConfigNames
    * @return LitConfig
    */
   public function setLocalConfigNames(array $localConfigNames): LitConfig
   {
      $this->localConfigNames = $localConfigNames;
      return $this;
   }

   /**
    * @return array
    */
   public function getValgrindArgs(): array
   {
      return $this->valgrindArgs;
   }

   /**
    * @param array $valgrindArgs
    * @return LitConfig
    */
   public function setValgrindArgs(array $valgrindArgs): LitConfig
   {
      $this->valgrindArgs = $valgrindArgs;
      return $this;
   }

   /**
    * @return int
    */
   public function getMaxIndividualTestTime(): int
   {
      return $this->maxIndividualTestTime;
   }

   /**
    * @param int $maxIndividualTestTime
    * @return LitConfig
    */
   public function setMaxIndividualTestTime(int $maxIndividualTestTime): LitConfig
   {
      if ($maxIndividualTestTime < 0) {
         TestLogger::fatal("The timeout per test must be >= 0 seconds");
      }
      $this->maxIndividualTestTime = $maxIndividualTestTime;
      return $this;
   }

   /**
    * @return mixed
    */
   public function getMaxFailures()
   {
      return $this->maxFailures;
   }

   /**
    * @param int $maxFailures
    * @return LitConfig
    */
   public function setMaxFailures(int $maxFailures): LitConfig
   {
      $this->maxFailures = $maxFailures;
      return $this;
   }

   /**
    * @return array
    */
   public function getParallelismGroups(): array
   {
      return $this->parallelismGroups;
   }

   /**
    * @param array $parallelismGroups
    * @return LitConfig
    */
   public function setParallelismGroups(array $parallelismGroups): LitConfig
   {
      $this->parallelismGroups = $parallelismGroups;
      return $this;
   }

   /**
    * @return bool
    */
   public function isEchoAllCommands(): bool
   {
      return $this->echoAllCommands;
   }

   /**
    * @param bool $echoAllCommands
    * @return LitConfig
    */
   public function setEchoAllCommands(bool $echoAllCommands): LitConfig
   {
      $this->echoAllCommands = $echoAllCommands;
      return $this;
   }

   public function loadConfig($testConfig, string $path)
   {
      if ($this->debug) {
         TestLogger::note("load_config from '%s'", $path);
      }
      $testConfig->loadFromPath($path, $this);
      return $testConfig;
   }
}
