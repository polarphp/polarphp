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
use Lit\Kernel\TestResultCode;
use Lit\Utils\TestingProgressDisplay;
use Lit\Utils\ConsoleLogger;
use Symfony\Component\Console\Command\Command;
use Symfony\Component\Console\Input\InputArgument;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Input\InputOption;
use Symfony\Component\Console\Output\OutputInterface;
use Symfony\Component\Console\Helper\ProgressBar;
use Symfony\Component\Filesystem\Exception\IOException;
use Symfony\Component\Filesystem\Filesystem;

use function Lit\Utils\detect_cpus;
use function Lit\Utils\print_histogram;
use function Lit\Utils\sort_by_incremental_cache;
use function Lit\Utils\quote_xml_attr;
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
      $this->addOption('workers', 'j', InputOption::VALUE_OPTIONAL, "Number of testing threads");
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
      $this->addOption('debug', null, InputOption::VALUE_NONE, 'Debug and Experimental Options');
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
      $numWorkers = $input->getOption('workers');
      if (null != $numWorkers) {
         $numWorkers = intval($numWorkers);
         if ($numWorkers < 0) {
            TestLogger::fatal("Option '--workers' or '-j' requires positive integer");
         }
      } else {
         $numWorkers = detect_cpus();
      }
      $maxFailures = $input->getOption('max-failures');
      if ($maxFailures != null) {
         $maxFailures = intval($maxFailures);
         if ($maxFailures <= 0) {
            TestLogger::fatal("Option '--max-failures' requires positive integer");
         }
         $input->setOption('max-failures', $maxFailures);
      }
      $echoAllCommands = false;
      if ($input->hasParameterOption('-vv', true) || $input->hasParameterOption('--verbose=2', true) || 2 === $input->getParameterOption('--verbose', false, true)) {
         $echoAllCommands = true;
      }
      if ($input->hasParameterOption('-vvv', true) || $input->hasParameterOption('--verbose=3', true) || 3 === $input->getParameterOption('--verbose', false, true)) {
         $echoAllCommands = true;
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
         $input->getOption('debug'),
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
            TestLogger::note(
               join('', array(
                  'The test suite configuration requested an individual',
                  ' test timeout of %d seconds but a timeout of %d seconds was',
                  ' requested on the command line. Forcing timeout to be %d',
                  ' seconds'
               )), $litConfig->getMaxIndividualTestTime(), $cmdValue, $cmdValue
            );
            $litConfig->setMaxIndividualTestTime($cmdValue);
         }
      }
      $this->handleShowTestsOpt($input, $testDispatcher, $output);
      // Select and order the tests.
      $numTotalTests = count($testDispatcher->getTests());
      $this->filterTests($input, $testDispatcher);
      $this->handleTestsOrder($input, $testDispatcher);
      $this->handleTestShard($input, $testDispatcher);
      // Finally limit the number of tests, if desired.
      if ($input->getOption('max-tests')) {
         $maxTests = intval($input->getOption('max-tests'));
         $testDispatcher->setTests(array_slice($testDispatcher->getTests(), 0, $maxTests));
      }
      $tests = $testDispatcher->getTests();
      $numWorkers = min(count($tests), $numWorkers);
      $actualTestNum = count($tests);
      $extra = '';
      if ($actualTestNum != $numTotalTests) {
         $extra = " of $numTotalTests";
      }
      $workersText = "";
      if ($numWorkers == 1) {
         $workersText = 'single process';
      } else {
         $workersText = "$numWorkers workers";
      }
      if ($litConfig->isDebug()) {
         $output->writeln('');
      }
      $header = sprintf("-- Testing: %d%s tests, %s --", $actualTestNum, $extra, $workersText);
      $progressBar = null;
      if (!$quite) {
         $output->writeln($header);
         if ($input->getOption('succinct') && !$input->getOption('no-progress-bar')) {
            $progressBar = new ProgressBar($output, $actualTestNum);
            $progressBar->setOverwrite(false);
         }
      }
      $startTime = microtime(true);
      $opts = $input->getOptions();
      $progressDisplay = new TestingProgressDisplay($opts, $actualTestNum, $progressBar, $output);
      $testDispatcher->setDisplay($progressDisplay);
      $maxTime = null;
      if ($input->getOption('max-time')) {
         $maxTime = (int)$input->getOption('max-time');
      }
      try {
         $testDispatcher->executeTests($numWorkers, $maxTime);
      } catch (\Exception $e) {
         if ($litConfig->isDebug()) {
            TestLogger::errorWithoutCount("catch exception:\nlocation: %s:%d", $e->getFile(), $e->getLine());
            TestLogger::errorWithoutCount($e->getTraceAsString());
         }
         TestLogger::fatal("execute tests error: %s", $e->getMessage());
      }
      $progressDisplay->finish();
      $testingTime = microtime(true) - $startTime;
      if (!$quite) {
         $output->writeln(sprintf("\nTesting Time: %.2fs", $testingTime));
      }
      // Write out the test data, if requested.
      if ($input->getOption('output')) {
         $testDispatcher->writeTestResults($testingTime, $input->getOption('output'));
      }
      $this->handleTestResults($input, $testDispatcher, $hasFailures, $output);
      $this->handleXuintOutput($input, $testDispatcher);
      //  If we encountered any additional errors, exit abnormally.
      $numErrors = TestLogger::getNumErrors();
      if ($numErrors > 0) {
         TestLogger::errorWithoutCount("\n%d error(s), exiting.\n", $numErrors);
         exit(2);
      }
      // Warn about warnings.
      $numWarnings = TestLogger::getNumWarnings();
      if ($numWarnings > 0) {
         TestLogger::errorWithoutCount("\n%d warning(s), exiting.\n", $numWarnings);
      }
      if ($hasFailures) {
         exit(1);
      }
      exit(0);
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
            $value = trim($parts[1]);
            if (ctype_digit($value)) {
               if (strpos($value,'.') === false) {
                  $value = intval($value);
               } else {
                  $value = doubleval($value);
               }
            }
            $userParams[trim($parts[0])] = $value;
         }
      }
      return $userParams;
   }

   private function handleShowTestsOpt(InputInterface $input, TestDispatcher $dispatcher, OutputInterface $output): void
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
            $output->writeln('-- Test Suites --');
            foreach ($suitesAndTests as $key => $tests) {
               $testSuite = $testSuites[$key];
               $output->writeln(sprintf("  %s - %d tests", $testSuite->getName(), count($tests)));
               $output->writeln(sprintf("    Source Root: %s", $testSuite->getSourceRoot()));
               $output->writeln(sprintf("    Exec Root  : %s", $testSuite->getExecRoot()));
            }
         }
         //  Show the tests, if requested.
         if ($input->getOption('show-tests')) {
            $output->writeln("-- Available Tests --");
            foreach ($suitesAndTests as $key => $tests) {
               foreach ($tests as $test) {
                  $output->writeln(sprintf("  %s", $test->getFullName()));
               }
            }
         }
         exit(0);
      }
   }

   private function filterTests(InputInterface $input, TestDispatcher $testDispatcher): void
   {
      // First, select based on the filter expression if given.
      $filter = $input->getOption('filter');
      if ($filter) {
         $tests = $testDispatcher->getTests();
         $filteredTests = array();
         foreach ($tests as $test) {
            $mresult = preg_match("/$filter/", $test->getFullName());
            if ($mresult === false) {
               TestLogger::fatal("invalid regular expression for --filter: %s", $filter);
            } elseif ($mresult > 0) {
               $filteredTests[] = $test;
            }
         }
         $testDispatcher->setTests($filteredTests);
      }
   }

   private function handleTestsOrder(InputInterface $input, TestDispatcher $testDispatcher): void
   {
      $tests = $testDispatcher->getTests();
      // Then select the order.
      if ($input->getOption('shuffle')) {
         shuffle($tests);
      } elseif ($input->getOption('incremental')) {
         sort_by_incremental_cache($tests);
      } else {
         usort($tests, function (TestCase $lhs, TestCase $rhs) {
            $learlyTest = intval($lhs->isEarlyTest());
            $rearlyTest = intval($rhs->isEarlyTest());
            if ($learlyTest < $rearlyTest) {
               return -1;
            } elseif ($learlyTest > $rearlyTest) {
               return 1;
            } else {
               // by fullname
               return $lhs->getFullName() <=> $rhs->getFullName();
            }
         });
      }
      $testDispatcher->setTests($tests);
   }

   private function handleTestShard(InputInterface $input, TestDispatcher $testDispatcher): void
   {
      $runShard = $input->getOption('run-shard');
      $numShards = $input->getOption('num-shards');
      if (null != $runShard || null != $numShards) {
         if (null == $runShard || null == $numShards) {
            TestLogger::fatal("--num-shards and --run-shard must be used together");
         }
         $runShard = intval($runShard);
         $numShards = intval($numShards);
         if ($numShards <= 0) {
            TestLogger::fatal("--num-shards must be positive");
         }
         if ($runShard < 1 || $runShard > $numShards) {
            TestLogger::fatal("--run-shard must be between 1 and --num-shards (inclusive)");
         }
         $tests = $testDispatcher->getTests();
         $selectedTests = array();
         $numTests = count($tests);
         // Note: user views tests and shard numbers counting from 1.
         $testIndexes = $this->range($runShard - 1, $numTests, $numShards);
         foreach ($testIndexes as $index) {
            $selectedTests[] = $tests[$index];
         }
         $testDispatcher->setTests($selectedTests);
         $tests = $selectedTests;
         // Generate a preview of the first few test indices in the shard
         // to accompany the arithmetic expression, for clarity.
         $previewLen = min(3, count($testIndexes));
         $idxPreview = array();
         foreach (array_slice($testIndexes, 0, $previewLen) as $index) {
            $idxPreview[] = 1 + $index;
         }
         $idxPreview = join(', ', $idxPreview);
         if (count($testIndexes) > $previewLen) {
            $idxPreview .= ', ...';
         }
         TestLogger::note('Selecting shard %d/%d = size %d/%d = tests #(%d*k)+%d = [%s]',
            $runShard, $numShards, count($tests), $numTests, $numShards, $runShard, $idxPreview);
      }
   }

   private function range(int $start, int $end, $step = 1): array
   {
      $itmes = [];
      for ($i = $start; $i < $end; $i += $step) {
         $itmes[] = $i;
      }
      return $itmes;
   }

   private function handleTestResults(InputInterface $input, TestDispatcher $testDispatcher, &$hasFailures, OutputInterface $output): void
   {
      $hasFailures = false;
      $byCode = array();
      $tests = $testDispatcher->getTests();
      foreach ($tests as $test) {
         $resultCode = $test->getResult()->getCode();
         $codeName = $resultCode->getName();
         if (!array_key_exists($codeName, $byCode)) {
            $byCode[$codeName] = array();
         }
         $byCode[$codeName][] = $test;
         if ($resultCode->isFailure()) {
            $hasFailures = true;
         }
      }
      // Print each test in any of the failing groups.
      $codeTitleMap = array(
         ['Unexpected Passing Tests', TestResultCode::XPASS()],
         ['Failing Tests', TestResultCode::FAIL()],
         ['Unresolved Tests', TestResultCode::UNRESOLVED()],
         ['Unsupported Tests', TestResultCode::UNSUPPORTED()],
         ['Expected Failing Tests', TestResultCode::XFAIL()],
         ['Timed Out Tests', TestResultCode::TIMEOUT()],
      );
      foreach ($codeTitleMap as $entry) {
         list($title, $code) = $entry;
         if (($code == TestResultCode::XFAIL() && !$input->getOption('show-xfail')) ||
            ($code == TestResultCode::UNSUPPORTED() && !$input->getOption('show-unsupported')) ||
            ($code == TestResultCode::UNRESOLVED() && $input->getOption('max-failures') !== null)) {
            continue;
         }
         $codeName = $code->getName();
         if (!array_key_exists($codeName, $byCode) || empty($byCode[$codeName])) {
            continue;
         }
         $elements = $byCode[$codeName];
         if (empty($elements)) {
            continue;
         }
         $output->writeln(str_repeat('*', 20));
         $output->writeln(sprintf('%s (%d):', $title, count($elements)));
         foreach ($elements as $test) {
            $output->writeln(sprintf("    %s", $test->getFullName()));
         }
      }

      if ($input->getOption('time-tests') && !empty($tests)) {
         // Order by time.
         $testTimes = array();
         foreach ($testDispatcher->getTests() as $test) {
            $testTimes[] = [$test->getFullName(), $test->getResult()->getElapsed()];
         }
         print_histogram($testTimes, 'Tests');
      }
      $codeNameMap = array(
         ['Expected Passes    ', TestResultCode::PASS()],
         ['Passes With Retry  ', TestResultCode::FLAKYPASS()],
         ['Expected Failures  ', TestResultCode::XFAIL()],
         ['Unsupported Tests  ', TestResultCode::UNSUPPORTED()],
         ['Unresolved Tests   ', TestResultCode::UNRESOLVED()],
         ['Unexpected Passes  ', TestResultCode::XPASS()],
         ['Unexpected Failures', TestResultCode::FAIL()],
         ['Individual Timeouts', TestResultCode::TIMEOUT()],
      );
      foreach ($codeNameMap as $entry) {
         list($name, $code) = $entry;
         if ($input->hasParameterOption(['--quiet', '-q'], true) && !$code->isFailure()) {
            continue;
         }
         $codeName = $code->getName();
         if (array_key_exists($codeName, $byCode)) {
            $num = count($byCode[$codeName]);
         } else {
            $num = 0;
         }
         if ($num > 0) {
            $output->writeln(sprintf("  %s: %d", $name, $num));
         }
      }
   }

   private function handleXuintOutput(InputInterface $input, TestDispatcher $testDispatcher)
   {
      if (!$input->getOption('xunit-xml-output')) {
         return;
      }
      // Collect the tests, indexed by test suite
      $bySuite = [];
      $tests = $testDispatcher->getTests();
      foreach ($tests as $resultTest) {
         /**
          * @var TestCase $resultTest
          */
         $suite = $resultTest->getTestSuite()->getConfig()->getName();
         if (!array_key_exists($suite, $bySuite)) {
            $bySuite[$suite] = [
               'passes'   => 0,
               'failures' => 0,
               'skipped'=> 0,
               'tests' => []
            ];
         }
         $bySuite[$suite]['tests'][] = $resultTest;
         if ($resultTest->getResult()->getCode()->isFailure()) {
            $bySuite[$suite]['failures'] += 1;
         } elseif ($resultTest->getResult()->getCode() == TestResultCode::UNSUPPORTED()) {
            $bySuite[$suite]['skipped'] += 1;
         } else {
            $bySuite[$suite]['passes'] += 1;
         }
      }
      $xunitOutputFile = fopen($input->getOption('xunit-xml-output'),'w');
      fwrite($xunitOutputFile, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
      fwrite($xunitOutputFile, "<testsuites>\n");
      foreach ($bySuite as $suiteName => $suite) {
         $safeSuiteName = quote_xml_attr(str_replace(".", "-", $suiteName));
         fwrite($xunitOutputFile,"<testsuite name=".$safeSuiteName);
         fwrite($xunitOutputFile," tests=\"" . strval($suite['passes'] + $suite['failures'] + $suite['skipped']) . "\"");
         fwrite($xunitOutputFile," failures=\"" . strval($suite['failures']) . "\"");
         fwrite($xunitOutputFile," skipped=\"" . strval($suite['skipped']) . "\">\n");
         foreach ($suite['tests'] as $resultTest) {
            $resultTest->writeJUnitXML($xunitOutputFile);
            fwrite($xunitOutputFile, "\n");
         }
         fwrite($xunitOutputFile, "</testsuite>\n");
      }
      fwrite($xunitOutputFile, "</testsuites>");
      fclose($xunitOutputFile);
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
