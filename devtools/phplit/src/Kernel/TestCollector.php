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

use Lit\Utils\TestLogger;
use function Lit\Utils\is_absolute_path;

class TestCollector
{
   /**
    * @var LitConfig $litConfig
    */
   private $litConfig;
   /**
    * @var array $inputs
    */
   private $inputs;
   /**
    * @var array $tests
    */
   private $tests;

   public function __construct(LitConfig $litConfig, array $inputs)
   {
      $this->litConfig = $litConfig;
      $this->inputs = $inputs;
   }

   /**
    * Given a configuration object and a list of input specifiers, find all the
    * tests to execute.
    */
   public function resolve()
   {
      $actualInputs = array();
      // TODO support multi_byte char ?
      foreach ($this->inputs as $input) {
         if ($input[0] == '@') {
            $filename = substr($input, 1);
            if (file_exists($filename)) {
               $lines = file($filename, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
               foreach ($lines as $line) {
                  $actualInputs[] = trim($line);
               }
            }
         } else {
            $actualInputs[] = trim($input);
         }
      }
      // Load the tests from the inputs.
      $tests = array();
      $testSuiteCache = array();
      $localConfigCache = array();
      foreach ($actualInputs as $input) {
         $prevCount = count($tests);
         $tests += $this->collectTests($input, $testSuiteCache, $localConfigCache)[1];
         if ($prevCount == count($tests)) {
            TestLogger::warning(sprintf('input %s contained no tests', $input));
         }
      }
      // If there were any errors during test discovery, exit now.
      $numErrors = TestLogger::getNumErrors();
      if ($numErrors > 0) {
         TestLogger::fatal("%d errors, exiting.\n", $numErrors);
      }
      $this->tests = $tests;
   }

   private function doSearchConfigFunc(array $pathInSuite, TestSuite $testSuite, array $cache)
   {
      // Get the parent config.
      if (empty($pathInSuite)) {
         $parent = $testSuite->getConfig();
      } else {
         $parent = getLocalConfig(array_slice($pathInSuite, 0, -1), $testSuite, $cache);
      }
      // Check if there is a local configuration file.
      $sourcePath = $testSuite->getSourcePath($pathInSuite);
      $cfgPath = $this->chooseConfigFileFromDir($sourcePath, $this->litConfig->getLocalConfigNames());
      // If not, just reuse the parent config.
      if (is_null($cfgPath)) {
         return $parent;
      }
      // Otherwise, copy the current config and load the local configuration
      // file into it.
      $config = $parent->getCopyConfig();
      if ($this->litConfig->isDebug()) {
         TestLogger::note('loading local config %s', $cfgPath);
      }
      $config->loadFromPath($cfgPath, $this->litConfig);
      return $config;
   }

   private function getLocalConfig(TestSuite $testSuite, array $pathInSuite, array $cache) : string
   {
      $key = hash('sha256', serialize($testSuite) . serialize($pathInSuite));
      if (!array_key_exists($key, $cache)) {
         $cache[$key] = doSearchConfigFunc($pathInSuite, $testSuite, $cache);
      }
      return $cache[$key];
   }

   /**
    * Find the test suite containing @arg item.
    * @retval (None, ...) - Indicates no test suite contains @arg item.
    * @retval (suite, relative_path) - The suite that @arg item is in, and its
    * relative path inside that suite.
    */
   private function collectTestSuite(string $path, array $cache) : array
   {
      // Canonicalize the path.
      if (!is_absolute_path($path)) {
         $path = realpath(getcwd() . DIRECTORY_SEPARATOR . $path);
      } else {
         $path = realpath($path);
      }
      // Skip files and virtual components.
      $components = array();
      while (!is_dir($path)) {
         $parent = dirname($path);
         $base = basename($path);
         if ($parent == $path) {
            return [null, []];
         }
         $components[] = $base;
         $path = $parent;
      }
      $components = array_reverse($components);
      list($testSuite, $relative) = $this->doCollectionTestSuiteWithCache($path, $cache);
      return [$testSuite, $relative + $components];
   }

   private function doCollectionTestSuite(string $path, array $cache) : array
   {
      // Check for a site config or a lit config.
      $cfgPath = $this->findTestSuiteDir($path);
      // If we didn't find a config file, keep looking.
      if (is_null($cfgPath)) {
         $parent = dirname($path);
         $base = basename($path);
         if ($parent == $path) {
            return [null, []];
         }
         list($testSuite, $relative) = $this->doCollectionTestSuiteWithCache($path, $cache);
         return [$testSuite, $relative + [$base]];
      }
      // This is a private builtin parameter which can be used to perform
      // translation of configuration paths.  Specifically, this parameter
      // can be set to a dictionary that the discovery process will consult
      // when it finds a configuration it is about to load.  If the given
      // path is in the map, the value of that key is a path to the
      // configuration to load instead.
      $configMap = $this->litConfig->getParam('config_map');
      if (!is_null($configMap)) {
         $cfgPath = realpath($cfgPath);
         if (array_key_exists($cfgPath, $configMap)) {
            $cfgPath = $configMap[$cfgPath];
         }
      }
      // We found a test suite, create a new config for it and load it.
      if ($this->litConfig->isDebug()) {
         TestLogger::note(sprintf('loading suite config %s', $cfgPath));
      }
      $config = TestingConfig::fromDefaults($this->litConfig);
      $config->loadFromPath($cfgPath, $this->litConfig);
      $sourceRoot = realpath(!empty($config->getTestSourceRoot()) ? $config->getTestSourceRoot() : $path);
      $execRoot = realpath(!empty($config->getTestExecRoot()) ? $config->getTestExecRoot() : $path);
      return [new TestSuite($config->getName(), $sourceRoot, $execRoot, $config), []];
   }

   private function doCollectionTestSuiteWithCache(string $path, array $cache) : array
   {
      // Check for an already instantiated test suite.
      $realPath = realpath($path);
      if (!array_key_exists($realPath, $cache)) {
         $cache[$realPath] = $this->doCollectionTestSuite($path, $cache);
      }
      return $cache[$realPath];
   }

   private function collectTests(string $path, array $testSuiteCache, array $localConfigCache) : array
   {
      // Find the test suite for this input and its relative path.
      list($testSuite, $pathInSuite) = $this->collectTestSuite($path, $testSuiteCache);
      if (is_null($testSuite)) {
         TestLogger::warning("unable to find test suite for $path");
         return [null, []];
      }
      if ($this->litConfig->isDebug()) {
         TestLogger::note(sprintf('resolved input %s to %s::%s', $path, $testSuite->getName(), var_export($pathInSuite, true)));
      }
      return [$testSuite, $this->collectTestsInSuite($testSuite, $pathInSuite, $testSuiteCache, $localConfigCache)];
   }

   private function collectTestsInSuite(TestSuite $testSuite, array $pathInSuite, array $testSuiteCache, array $localConfigCache) : iterable
   {
      // Check that the source path exists (errors here are reported by the
      // caller).
      $sourcePath = $testSuite->getSourcePath($pathInSuite);
      if (!file_exists($sourcePath)) {
         return;
      }
      // Check if the user named a test directly.
      if (!is_dir($sourcePath)) {
         $localConfig = $this->getLocalConfig($testSuite, array_slice($pathInSuite, 0, -1), $localConfigCache);
         yield new TestCase($testSuite, $pathInSuite, $localConfig);
         return;
      }
      // Otherwise we have a directory to search for tests, start by getting the
      // local configuration.
      $localConfig = $this->getLocalConfig($testSuite, $pathInSuite, $localConfigCache);
      // Search for tests.
      $testFormat = $localConfig->getTestFormat();
      if (!is_null($testFormat)) {
         foreach ($testFormat->collectTestsInDirectory($testSuite, $pathInSuite, $localConfig) as $test) {
            yield $test;
         }
      }
      // Search subdirectories.
      $diter = new \DirectoryIterator($sourcePath);
      $excludes = $localConfig->getExcludes();
      foreach ($diter as $entry) {
         $filename = $entry->getFilename();
         if ($entry->isDot() || in_array($filename, ['Output', '.svn', '.git']) || in_array($filename, $excludes)) {
            continue;
         }
         if (!$entry->isDir()) {
            continue;
         }
         // Ignore non-directories.
         $fileSourcePath = $entry->getPathname();
         // Check for nested test suites, first in the execpath in case there is a
         // site configuration and then in the source path.
         $subPath = $pathInSuite + [$filename];
         $fileExecPath = $testSuite->getExecPath($subPath);
         if ($this->findTestSuiteDir($fileExecPath, $this->litConfig)) {
            list($subTestSuite, $subpathInSuite) = $this->collectTestSuite($fileExecPath,testSuiteCache);
         } elseif ($this->findTestSuiteDir($fileSourcePath, $this->litConfig)) {
            list($subTestSuite, $subpathInSuite) = $this->collectTestSuite($fileSourcePath,testSuiteCache);
         } else {
            $subTestSuite = null;
            $subpathInSuite = array();
         }
         // If the this directory recursively maps back to the current test suite,
         // disregard it (this can happen if the exec root is located inside the
         // current test suite, for example).
         if ($subTestSuite == $testSuite) {
            continue;
         }
         // Otherwise, load from the nested test suite, if present.
         if (!is_null($subTestSuite)) {
            $subDirTests = $this->collectTestsInSuite($subTestSuite, $subpathInSuite, $testSuiteCache, $localConfigCache);
         } else {
            $subDirTests = $this->collectTestsInSuite($testSuite, $subPath, $testSuiteCache, $localConfigCache);
         }
         $num = 0;
         foreach ($subDirTests as $subTest) {
            ++$num;
            yield $subTest;
         }
         if (!is_null($subTestSuite) && !$num) {
            TestLogger::warning(sprintf('test suite %s contained no tests', $subTestSuite->getName()));
         }
      }
   }

   private function chooseConfigFileFromDir(string $dir, array $configNames)
   {
      foreach ($configNames as $name) {
         $path = $dir . DIRECTORY_SEPARATOR . $name;
         if (file_exists($path)) {
            return $path;
         }
      }
      return null;
   }

   private function findTestSuiteDir(string $path) : string
   {
      $cfgPath = $this->chooseConfigFileFromDir($path, $this->litConfig->getSiteConfigNames());
      if (is_null($cfgPath)) {
         $cfgPath = $this->chooseConfigFileFromDir($path, $this->litConfig->getConfigNames());
      }
      return $cfgPath;
   }

   /**
    * @return LitConfig
    */
   public function getLitConfig(): LitConfig
   {
      return $this->litConfig;
   }

   /**
    * @return array
    */
   public function getInputDirs(): array
   {
      return $this->inputs;
   }

   /**
    * @return array
    */
   public function getTests(): array
   {
      return $this->tests;
   }
}