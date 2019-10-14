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
// Created by polarboy on 2019/10/08.

namespace Lit\Commands;

use Lit\Kernel\LitConfig;
use Lit\Kernel\TestCase;
use Lit\Kernel\TestCollector;
use Lit\Kernel\TestDispatcher;
use Symfony\Component\Console\Command\Command;
use Symfony\Component\Console\Input\InputArgument;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Input\InputOption;
use Symfony\Component\Console\Logger\ConsoleLogger;
use Symfony\Component\Console\Output\OutputInterface;
use Symfony\Component\Filesystem\Exception\IOException;
use Symfony\Component\Filesystem\Filesystem;

use function Lit\Utils\detect_cpus;
use Lit\Utils\TestLogger;

class MainCommand extends Command
{
   protected static $defaultName = 'run-test';

   protected function configure()
   {
      $this->setDescription('run regression tests');
      $this->setHelp('welcome use phplit');
      $this->setupOptions();
      $this->setupArguments();
   }

   private function setupOptions()
   {
      $this->addOption('threads', 'j', InputOption::VALUE_OPTIONAL, "Number of testing threads");
      $this->addOption('config-prefix', null, InputOption::VALUE_OPTIONAL, "Prefix for 'lit' config files");
      $this->addOption('param', 'D', InputOption::VALUE_REQUIRED|InputOption::VALUE_IS_ARRAY, "Add 'NAME' = 'VAL' to the user defined parameters", []);
      // format group
      $this->addOption('succinct', 's', InputOption::VALUE_NONE, 'Reduce amount of output');
      $this->addOption('show-all', 'a', InputOption::VALUE_NONE, 'Display all commandlines and output');
      $this->addOption('output', 'o', InputOption::VALUE_OPTIONAL, 'Write test results to the provided path');
      $this->addOption('no-progress-bar', null, InputOption::VALUE_NONE, 'Do not use curses based progress bar');
      $this->addOption('show-unsupported', null, InputOption::VALUE_NONE, 'Show unsupported tests');
      $this->addOption('show-xfail', null, InputOption::VALUE_NONE, 'Show tests that were expected to fail');
      // execution group
      $this->addOption('path', null, InputOption::VALUE_REQUIRED | InputOption::VALUE_IS_ARRAY, 'Additional paths to add to testing environment', []);
      $this->addOption('vg', null, InputOption::VALUE_NONE, 'Run tests under valgrind');
      $this->addOption('vg-leak', null, InputOption::VALUE_NONE, 'Check for memory leaks under valgrind');
      $this->addOption('vg-arg', null, InputOption::VALUE_REQUIRED | InputOption::VALUE_IS_ARRAY, 'Specify an extra argument for valgrind', []);
      $this->addOption('time-tests', null, InputOption::VALUE_NONE, 'Track elapsed wall time for each test');
      $this->addOption('no-execute', null, InputOption::VALUE_NONE, 'Don\'t execute any tests (assume PASS)');
      $this->addOption('xunit-xml-output', null, InputOption::VALUE_OPTIONAL, 'Write XUnit-compatible XML test reports to the specified file');
      $this->addOption('timeout', null, InputOption::VALUE_OPTIONAL, 'Maximum time to spend running a single test (in seconds). 0 means no time limit.');
      $this->addOption('max-failures', null, InputOption::VALUE_OPTIONAL, 'Stop execution after the given number of failures.');
      // selection group
      $this->addOption('max-tests', null, InputOption::VALUE_OPTIONAL, 'Maximum number of tests to run');
      $this->addOption('max-time', null, InputOption::VALUE_OPTIONAL, 'Maximum time to spend testing (in seconds)');
      $this->addOption('shuffle', null, InputOption::VALUE_NONE, 'Run tests in random order');
      $this->addOption('incremental', 'i', InputOption::VALUE_OPTIONAL, 'Run modified and failing tests first (updates mtimes)', false);
      $this->addOption('filter', null, InputOption::VALUE_OPTIONAL, 'Only run tests with paths matching the given regular expression', getenv('LIT_FILTER'));
      $this->addOption('num-shards', null, InputOption::VALUE_OPTIONAL, 'Split testsuite into M pieces and only run one', intval(getenv('LIT_NUM_SHARDS')));
      $this->addOption('run-shard', null, InputOption::VALUE_OPTIONAL, 'Run shard #N of the testsuite', intval(getenv('LIT_RUN_SHARD')));
      // debug group
      $this->addOption('show-suites', null, InputOption::VALUE_NONE, 'Show discovered test suites');
      $this->addOption('show-tests', null, InputOption::VALUE_NONE, 'Show all discovered tests');
   }

   private function setupArguments()
   {
      $this->addArgument("test_paths", InputArgument::IS_ARRAY, 'Files or paths to include in the test suite', []);
   }

   protected function execute(InputInterface $input, OutputInterface $output)
   {
      $logger = new ConsoleLogger($output);
      TestLogger::init($logger);
      $fs = new Filesystem();
      $tempDir = $this->prepareTempDir($fs);
      try {
         $this->doExecute($input, $output);
      } finally {
         if (!is_null($tempDir)) {
            $tryCount = 3;
            $removeException = null;
            $isOk = false;
            while ($tryCount--) {
               try {
                  $fs->remove($tempDir);
                  $isOk = true;
                  break;
               } catch (IOException $e) {
                  $removeException = $e;
               }
            }
            if (!$isOk) {
               throw $removeException;
            }
         }
      }
   }

   private function doExecute(InputInterface $input, OutputInterface $output)
   {
      $inputDirs = $input->getArgument('test_paths');
      if (empty($inputDirs)) {
         TestLogger::fatal("No inputs specified");
      }
      $userParams = $this->parseUserParams($input);
      $maxIndividualTestTime = 0;
      if (null != $input->getOption('timeout')) {
         $maxIndividualTestTime = intval($input->getOption('timeout'));
      }
      $isWindows = false;
      if (strtoupper(substr(PHP_OS, 0, 3)) === 'WIN') {
         $isWindows = true;
      }
      $numThreads = $input->getOption('threads');
      if (null != $numThreads) {
         $numThreads = intval($numThreads);
         if ($numThreads < 0) {
            TestLogger::fatal("Option '--threads' or '-j' requires positive integer");
         }
      } else {
         $numThreads = detect_cpus();
      }
      $maxFailures = $input->getOption('max-failures');
      if ($maxFailures != null) {
         $maxFailures = intval($maxFailures);
         if ($maxFailures < 0) {
            TestLogger::fatal("Option '--max-failures' requires positive integer");
         }
      }
      $showOutput = false;
      $echoAllCommands = false;
      if ($input->hasParameterOption('-v', true) || $input->hasParameterOption('--verbose=1', true) || 1 === $input->getParameterOption('--verbose', false, true)) {
         $showOutput = true;
      }
      if ($input->hasParameterOption('-vv', true) || $input->hasParameterOption('--verbose=2', true) || 2 === $input->getParameterOption('--verbose', false, true)) {
         $showOutput = true;
         $echoAllCommands = true;
      }
      $isDebug = false;
      if ($input->hasParameterOption('-vvv', true) || $input->hasParameterOption('--verbose=3', true) || 3 === $input->getParameterOption('--verbose', false, true)) {
         $isDebug = true;
      }
      $quite = false;
      if ($input->hasParameterOption(['--quiet', '-q'], true)) {
         $quite = true;
      }
      // Decide what the requested maximum indvidual test time should be
      $path = $input->getOption('path');
      if (null == $path) {
         $path = array();
      }
      $vgArg = $input->getOption('vg-arg');
      if (null == $vgArg) {
         $vgArg = array();
      }
      $litConfig = new LitConfig(
         'phplit',
         $path,
         $quite,
         $input->getOption('vg'),
         $input->getOption('vg-leak'),
         $vgArg,
         $input->getOption('no-execute'),
         $isDebug,
         $isWindows,
         $userParams,
         $input->getOption('config-prefix'),
         $maxIndividualTestTime,
         $maxFailures,
         array(),
         $echoAllCommands
      );
      // Perform test discovery.
      $testCollector = new TestCollector($litConfig, $inputDirs);
      $testCollector->resolve();
      $testDispatcher = new TestDispatcher($litConfig, $testCollector->getTests());
      // After test discovery the configuration might have changed
      // the maxIndividualTestTime. If we explicitly set this on the
      // command line then override what was set in the test configuration
      if (null != $input->getOption('timeout')) {
         $cmdValue = intval($input->getOption('timeout'));
         if ($cmdValue != $litConfig->getMaxIndividualTestTime()) {
            TestLogger::note(sprintf(
               join(array(
                  'The test suite configuration requested an individual',
                  ' test timeout of %d seconds but a timeout of %d seconds was',
                  ' requested on the command line. Forcing timeout to be %d',
                  ' seconds'
               ), $litConfig->getMaxIndividualTestTime(), $cmdValue, $cmdValue)
            ));
            $litConfig->setMaxIndividualTestTime($cmdValue);
         }
      }
      $this->handleShowTestsOpt($input, $testDispatcher);
   }

   private function parseUserParams(InputInterface $input)
   {
      $params = $input->getOption('param');
      $userParams = array();
      foreach ($params as $entry) {
         $entry = trim($entry);
         if (false === strpos($entry, '=')) {
            $userParams[$entry] = '';
         } else {
            $parts = explode('=', $entry, 2);
            $userParams[trim($parts[0])] = trim($parts[1]);
         }
      }
      return $userParams;
   }

   private function handleShowTestsOpt(InputInterface $input, TestDispatcher $dispatcher)
   {
      if ($input->getOption('show-suites') || $input->getOption('show-tests')) {
         // Aggregate the tests by suite.
         $suitesAndTests = array();
         $testSuites = array();
         foreach ($dispatcher->getTests() as $test) {
            $testSuite = $test->getTestSuite();
            $key = hash('sha256',$testSuite->getSourceRoot() . $testSuite->getExecRoot());
            $key = $key .'_'.$testSuite->getName();
            if (!array_key_exists($key, $suitesAndTests)) {
               $suitesAndTests[$key] = array();
               $testSuites[$key] = $testSuite;
            }
            $suitesAndTests[$key][] = $test;
         }
         uksort($suitesAndTests, function (string $lhs, string $rhs) {
            $lname = explode('_', $lhs)[1];
            $rname = explode('_', $rhs)[1];
            return $lname <=> $rname;
         });
         if ($input->getOption('show-suites')) {
            // Show the suites, if requested.
            print("-- Test Suites --\n");
            foreach ($suitesAndTests as $key => $tests) {
               $testSuite = $testSuites[$key];
               printf("  %s - %d tests\n", $testSuite->getName(), count($tests));
               printf("    Source Root: %s\n", $testSuite->getSourceRoot());
               printf("    Exec Root  : %s\n", $testSuite->getExecRoot());
            }
         }
         //  Show the tests, if requested.
         if ($input->getOption('show-tests')) {
            print("-- Available Tests --\n");
            foreach ($suitesAndTests as $key => $tests) {
               foreach ($tests as $test) {
                  printf("  %s\n", $test->getFullName());
               }
            }
         }
         exit(0);
      }
   }

   private function prepareTempDir(Filesystem $fs)
   {
      $tempDir = null;
      if (in_array('LIT_PRESERVES_TMP', $_ENV)) {
         $sysTempDir = sys_get_temp_dir();
         $tempDir = @tempnam($sysTempDir, 'lit_tmp_');
         $fs->remove($tempDir);
         $fs->mkdir($tempDir);
         $this->setupEnvVars($tempDir);
      }
      return $tempDir;
   }

   private function setupEnvVars($tempDir)
   {
      putenv("TMPDIR=$tempDir");
      putenv("TMP=$tempDir");
      putenv("TEMP=$tempDir");
      putenv("TEMPDIR=$tempDir");
      $_ENV['TMPDIR'] = $tempDir;
      $_ENV['TMP'] = $tempDir;
      $_ENV['TEMP'] = $tempDir;
      $_ENV['TEMPDIR'] = $tempDir;
   }
}
