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
// Created by polarboy on 2019/10/10.

namespace Lit\Kernel;

use Lit\Format\AbstractTestFormat;
use Lit\Utils\TestLogger;
use function Lit\Utils\get_envvar;
use function Lit\Utils\is_os_win;
use function Lit\Utils\copy_array;

/**
 * TestingConfig - Information on the tests inside a suite.
 *
 * @package Lit\Kernel
 */
class TestingConfig
{
   /**
    * @var TestingConfig $parent
    */
   private $parent;
   /**
    * @var string $name
    */
   private $name;
   /**
    * @var array $suffixes
    */
   private $suffixes;
   /**
    * @var AbstractTestFormat $testFormat
    */
   private $testFormat;
   /**
    * @var array $environment
    */
   private $environment;
   /**
    * @var array $substitutions
    */
   private $substitutions;
   /**
    * @var bool $unsupported
    */
   private $unsupported;
   /**
    * @var string $testExecRoot
    */
   private $testExecRoot;
   /**
    * @var string $testSourceRoot
    */
   private $testSourceRoot;
   /**
    * @var array $excludes
    */
   private $excludes;
   /**
    * @var array $availableFeatures
    */
   private $availableFeatures;
   /**
    * @var bool $pipeFail
    */
   private $pipeFail;
   /**
    * @var array $limitToFeatures
    */
   private $limitToFeatures;
   /**
    * @var bool $isEarly
    */
   private $isEarly;
   /**
    * @var array $parallelismGroup
    */
   private $parallelismGroup;

   /**
    * @var array $extraConfig
    */
   private $extraConfig = array();

   public function __construct($parent, string $name, array $suffixes, $testFormat,
                               array $environment, array $substitutions, bool $unsupported,
                               string $testExecRoot, string $testSourceRoot, array $excludes,
                               array $availableFeatures, bool $pipeFail, array $limitToFeatures = [],
                               bool $isEarly = false, array $parallelismGroup = [])
   {
      $this->parent = $parent;
      $this->name = $name;
      $this->suffixes = $suffixes;
      $this->testFormat = $testFormat;
      $this->environment = $environment;
      $this->substitutions = $substitutions;
      $this->unsupported = $unsupported;
      $this->testExecRoot = $testExecRoot;
      $this->testSourceRoot = $testSourceRoot;
      $this->excludes = $excludes;
      $this->availableFeatures = $availableFeatures;
      $this->pipeFail = $pipeFail;
      // This list is used by TestRunner.py to restrict running only tests that
      // require one of the features in this list if this list is non-empty.
      // Configurations can set this list to restrict the set of tests to run.
      $this->limitToFeatures = $limitToFeatures;
      $this->isEarly = $isEarly;
      $this->parallelismGroup = $parallelismGroup;
   }

   /**
    * Create a TestingConfig object with default values.
    *
    * @param LitConfig $config
    * @return TestingConfig
    */
   public static function fromDefaults(LitConfig $config) : TestingConfig
   {
      // Set the environment based on the command line arguments.
      $environment = array(
         'PATH' => join(PATH_SEPARATOR, $config->getPaths() + [get_envvar('PATH', '')])
      );
      $passVars = array(
         'LIBRARY_PATH', 'LD_LIBRARY_PATH', 'SYSTEMROOT', 'TERM',
         'POLARPHP', 'LD_PRELOAD', 'ASAN_OPTIONS', 'UBSAN_OPTIONS',
         'LSAN_OPTIONS', 'ADB', 'ANDROID_SERIAL',
         'SANITIZER_IGNORE_CVE_2016_2143', 'TMPDIR', 'TMP', 'TEMP',
         'TEMPDIR', 'AVRLIT_BOARD', 'AVRLIT_PORT',
         'FILECHECK_DUMP_INPUT_ON_FAILURE', 'FILECHECK_OPTS',
         'VCINSTALLDIR', 'VCToolsinstallDir', 'VSINSTALLDIR',
         'WindowsSdkDir', 'WindowsSDKLibVersion'
      );
      if (is_os_win()) {
         array_push($passVars, 'INCLUDE', 'LIB', 'PATHEXT');
      }
      foreach ($passVars as $varname) {
         $var = get_envvar($varname, '');
         // Check for empty string as some variables such as LD_PRELOAD cannot be empty
         // ('') for OS's such as OpenBSD.
         if (!empty($envVar)) {
            $environment[$varname] = $var;
         }
      }
      // Set the default available features based on the LitConfig.
      $availableFeatures = array();
      if ($config->isUseValgrind()) {
         $availableFeatures[] = 'valgrind';
         if ($config->isValgrindLeakCheck()) {
            $availableFeatures[] = 'vg_leak';
         }
      }
      return new TestingConfig(
         null,
         '<unnamed>',
         [],
         null,
         $environment,
         [],
         false,
         '',
         '',
         [],
         $availableFeatures,
         true
      );
   }

   public function getCopyConfig() : TestingConfig
   {
      $config = new self(
         $this->parent,
         $this->name,
         copy_array($this->suffixes),
         $this->testFormat ? clone $this->testFormat : null,
         copy_array($this->environment),
         copy_array($this->substitutions),
         $this->unsupported,
         $this->testExecRoot,
         $this->testSourceRoot,
         copy_array($this->excludes),
         copy_array($this->availableFeatures),
         $this->pipeFail,
         copy_array($this->limitToFeatures),
         $this->isEarly,
         copy_array($this->parallelismGroup)
      );
      $config->extraConfig = copy_array($this->extraConfig);
      return $config;
   }

   /**
    * Load the configuration module at the provided path into the given config
    * object.
    *
    * @param string $path
    * @param LitConfig $config
    */
   public function loadFromPath(string $path, LitConfig $litConfig)
   {
      if (!file_exists($path)) {
         TestLogger::fatal('unable to load config file: %s', $path);
      }
      try {
         // set some closure variables used by included config script
         $config = $this;
         include $path;
         if ($litConfig->isDebug()){
            TestLogger::note("... loaded config '%s'", $path);
         }
         $this->loadFinish();
      } catch (\ParseError $error) {
         TestLogger::fatal("unable to parse config file %s\n error: %s", $path, $error->getMessage());
      }
   }

   private function loadFinish()
   {
      $this->name = (string)$this->name;
      $this->suffixes = (array)$this->suffixes;
      $this->environment = (array)$this->environment;
      if (isset($this->environment['PATH'])) {
         $path = $this->environment['PATH'];
         $path = POLARPHP_BIN_DIR.PATH_SEPARATOR.$path;
         $this->environment['PATH'] = $path;
      }
      $this->substitutions = (array)$this->substitutions;
      if (!$this->testExecRoot) {
         // FIXME: This should really only be suite in test suite config
         // files. Should we distinguish them?
         $this->testExecRoot = ($this->testExecRoot);
      }
      if (!$this->testSourceRoot) {
         // FIXME: This should really only be suite in test suite config
         // files. Should we distinguish them?
         $this->testSourceRoot = ($this->testSourceRoot);
      }
      $this->excludes = (array)$this->excludes;
   }

   /**
    * @return TestingConfig
    */
   public function getParent(): TestingConfig
   {
      $target = $this;
      while ($target->parent) {
         $target = $target->parent;
      }
      return $target;
   }

   /**
    * @param TestingConfig $parent
    * @return TestingConfig
    */
   public function setParent(TestingConfig $parent): TestingConfig
   {
      $this->parent = $parent;
      return $this;
   }

   /**
    * @return string
    */
   public function getName(): string
   {
      return $this->name;
   }

   /**
    * @param string $name
    * @return TestingConfig
    */
   public function setName(string $name): TestingConfig
   {
      $this->name = $name;
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
    * @return TestingConfig
    */
   public function setSuffixes(array $suffixes): TestingConfig
   {
      $this->suffixes = $suffixes;
      return $this;
   }

   /**
    * @return mixed
    */
   public function getTestFormat()
   {
      return $this->testFormat;
   }

   /**
    * @param AbstractTestFormat $testFormat
    * @return TestingConfig
    */
   public function setTestFormat(AbstractTestFormat $testFormat)
   {
      $this->testFormat = $testFormat;
      return $this;
   }

   /**
    * @return array
    */
   public function getEnvironment(): array
   {
      return $this->environment;
   }

   /**
    * @param array $environment
    * @return TestingConfig
    */
   public function setEnvironment(array $environment): TestingConfig
   {
      $this->environment = $environment;
      return $this;
   }

   public function setEnvVar(string $name, string $value): TestingConfig
   {
      $this->environment[$name] = $value;
      return $this;
   }

   /**
    * @param string $name
    * @return TestingConfig
    */
   public function unsetEnvVar(string $name): TestingConfig
   {
      if (array_key_exists($name, $this->environment)) {
         unset($this->environment[$name]);
      }
      return $this;
   }

   /**
    * @return array
    */
   public function getSubstitutions(): array
   {
      return $this->substitutions;
   }

   /**
    * @param array $substitutions
    * @return TestingConfig
    */
   public function setSubstitutions(array $substitutions): TestingConfig
   {
      $this->substitutions = $substitutions;
      return $this;
   }

   /**
    * @param string $name
    * @param string $value
    * @return TestingConfig
    */
   public function addSubstitution(string $name, string $value): TestingConfig
   {
      $this->substitutions[] = [$name, $value];
      return $this;
   }

   /**
    * @return mixed
    */
   public function getUnsupported()
   {
      return $this->unsupported;
   }

   /**
    * @param mixed $unsupported
    * @return TestingConfig
    */
   public function setUnsupported($unsupported)
   {
      $this->unsupported = $unsupported;
      return $this;
   }

   /**
    * @return string
    */
   public function getTestExecRoot(): string
   {
      return $this->testExecRoot;
   }

   /**
    * @param string $testExecRoot
    * @return TestingConfig
    */
   public function setTestExecRoot(string $testExecRoot): TestingConfig
   {
      $this->testExecRoot = $testExecRoot;
      return $this;
   }

   /**
    * @return string
    */
   public function getTestSourceRoot(): string
   {
      return $this->testSourceRoot;
   }

   /**
    * @param string $testSourceRoot
    * @return TestingConfig
    */
   public function setTestSourceRoot(string $testSourceRoot): TestingConfig
   {
      $this->testSourceRoot = $testSourceRoot;
      return $this;
   }

   /**
    * @return array
    */
   public function getExcludes(): array
   {
      return $this->excludes;
   }

   /**
    * @param array $excludes
    * @return TestingConfig
    */
   public function setExcludes(array $excludes): TestingConfig
   {
      $this->excludes = $excludes;
      return $this;
   }

   /**
    * @return array
    */
   public function getAvailableFeatures(): array
   {
      return $this->availableFeatures;
   }

   /**
    * @param array $availableFeatures
    * @return TestingConfig
    */
   public function setAvailableFeatures(array $availableFeatures): TestingConfig
   {
      $this->availableFeatures = $availableFeatures;
      return $this;
   }

   /**
    * @param $feature
    * @return TestingConfig
    */
   public function addAvailableFeature($feature): TestingConfig
   {
      $this->availableFeatures[] = $feature;
      return $this;
   }

   /**
    * @return bool
    */
   public function isPipeFail(): bool
   {
      return $this->pipeFail;
   }

   /**
    * @param bool $pipeFail
    * @return TestingConfig
    */
   public function setPipeFail(bool $pipeFail): TestingConfig
   {
      $this->pipeFail = $pipeFail;
      return $this;
   }

   /**
    * @return array
    */
   public function getLimitToFeatures(): array
   {
      return $this->limitToFeatures;
   }

   /**
    * @param array $limitToFeatures
    * @return TestingConfig
    */
   public function setLimitToFeatures(array $limitToFeatures): TestingConfig
   {
      $this->limitToFeatures = $limitToFeatures;
      return $this;
   }

   /**
    * @return bool
    */
   public function isEarly(): bool
   {
      return $this->isEarly;
   }

   /**
    * @param bool $isEarly
    * @return TestingConfig
    */
   public function setIsEarly(bool $isEarly): TestingConfig
   {
      $this->isEarly = $isEarly;
      return $this;
   }

   /**
    * @return array
    */
   public function getParallelismGroup(): array
   {
      return $this->parallelismGroup;
   }

   /**
    * @param array $parallelismGroup
    * @return TestingConfig
    */
   public function setParallelismGroup(array $parallelismGroup): TestingConfig
   {
      $this->parallelismGroup = $parallelismGroup;
      return $this;
   }

   public function setExtraConfig(string $name, $value) : TestingConfig
   {
      $this->extraConfig[$name] = $value;
      return $this;
   }

   public function getExtraConfig(string $name, $defaultValue)
   {
      if (!array_key_exists($name, $this->extraConfig)) {
         return $defaultValue;
      }
      return $this->extraConfig[$name];
   }

   public function hasExtraConfig(string $name): bool
   {
      return array_key_exists($name, $this->extraConfig);
   }
}