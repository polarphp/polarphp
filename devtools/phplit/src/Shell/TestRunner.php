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
// Created by polarboy on 2019/10/12.

namespace Lit\Shell;

use Lit\Kernel\LitConfig;
use Lit\Kernel\TestCase;
use Lit\Kernel\TestResult;
use Lit\Kernel\TestResultCode;
use Lit\Kernel\ValueException;
use Lit\Utils\ShParser;
use Symfony\Component\Filesystem\Filesystem;
use function Lit\Utils\path_split;

class TestRunner
{
   const RUN_KEYWORD = 'RUN:';
   const XFAIL_KEYWORD = 'XFAIL:';
   const REQUIRES_KEYWORD = 'REQUIRES:';
   const REQUIRES_ANY_KEYWORD = 'REQUIRES-ANY:';
   const UNSUPPORTED_KEYWORD = 'UNSUPPORTED:';
   const END_KEYWORD = 'END.';

   /**
    * @var LitConfig $litConfig
    */
   private $litConfig;

   /**
    * @var array $extraSubstitutions
    */
   private $extraSubstitutions = array();

   /**
    * @var bool $useExternalShell
    */
   private $useExternalShell;

   public function __construct(LitConfig $litConfig, array $extraSubstitutions = array(),
                               bool $useExternalShell = false)
   {
      $this->litConfig = $litConfig;
      $this->extraSubstitutions = $extraSubstitutions;
      $this->useExternalShell = $useExternalShell;
   }

   public function executeTest(TestCase $test)
   {
      if ($test->getConfig()->getUnsupported()) {
         return new TestResult(TestResultCode::UNSUPPORTED(), 'Test is unsupported');
      }
      $script = $this->parseIntegratedTestScript($test);
      if ($script instanceof TestResult) {
         return $script;
      }
      if ($this->litConfig->isNoExecute()) {
         return new TestResult(TestResultCode::PASS());
      }
      list($tempDir, $tempBase) = $this->getTempPaths($test);
      $substitutions = $this->extraSubstitutions;
      $substitutions = array_merge($substitutions, $this->getDefaultSubstitutions($test, $tempDir, $tempBase, $this->useExternalShell));
      $script = $this->applySubstitutions($script, $substitutions);
      // Re-run failed tests up to test_retry_attempts times.
      $attempts = 1;
      if ($test->getConfig()->hasExtraConfig('test_retry_attempts')) {
         $attempts += (int)$test->getConfig()->getExtraConfig('test_retry_attempts');
      }
      foreach (range(0, $attempts) as $index) {
         $result = $this->doExecuteTest($test, $script, $tempBase);
         if ($result->getCode() != TestResultCode::FAIL()) {
            break;
         }
      }
      // If we had to run the test more than once, count it as a flaky pass. These
      // will be printed separately in the test summary.
      if ($index > 0 && $result->getCode() == TestResultCode::PASS()) {
         $result->setCode(TestResultCode::FLAKYPASS());
      }
      return $result;
   }

   private function doExecuteTest(TestCase $test, $script, string $tempBase): TestResult
   {
      // Create the output directory if it does not already exist.
      $fs = new Filesystem();
      $fs->mkdir($tempBase);
      $execDir = dirname($test->getExecPath());
      if ($this->useExternalShell) {
         $result = $this->executeScriptByProcess($test, $script, $tempBase, $execDir);
      } else {
         $result = $this->executeScriptInternal($test, $script, $tempBase, $execDir);
      }
      if ($result instanceof TestResult) {
         return $result;
      }
      list($out, $error, $exitCode, $timeoutInfo) = $result;
      if ($exitCode == 0) {
         $status = TestResultCode::PASS();
      } else {
         if ($timeoutInfo == null) {
            $status = TestResultCode::FAIL();
         } else {
            $status = TestResultCode::TIMEOUT();
         }
      }
      // Form the output log.
      $output = sprintf("Script:\n--\n%s\n--\nExit Code: %d\n", join("\n", $script), $exitCode);
      if ($timeoutInfo != null) {
         $output .= "Timeout: $timeoutInfo\n";
      }
      $output .= "\n";
      // Append the outputs, if present.
      if ($out) {
         $output .= "Command Output (stdout):\n--\n$out\n--\n";
      }
      if ($error) {
         $output .= "Command Output (stderr):\n--\n$error\n--\n";
      }
      return new TestResult($status, $output);
   }

   /**
    * @param TestCase $test
    * @param array $commands
    * @param string $tempBase
    * @param string $cwd
    */
   private function executeScriptByProcess(TestCase $test, array $commands, string $tempBase, string $cwd)
   {

   }

   /**
    * @param TestCase $test
    * @param array $commands
    * @param string $tempBase
    * @param string $cwd
    */
   private function executeScriptInternal(TestCase $test, array $commands, string $tempBase, string $cwd)
   {
      $cmds = [];
      foreach ($commands as $i => $line) {
         $line = preg_replace(sprintf('/%s/', IntegratedTestKeywordParser::PDBG_REGEX), ": '$1'; ", $line);
         $commands[$i] = $line;
         try {
            $parser = new ShParser($line, $this->litConfig->isWindows(), $test->getConfig()->isPipeFail());
            $cmds[] = $parser->parse();
         } catch (\Exception $e) {
            return new TestResult(TestResultCode::FAIL(), "shell parser error on: $line");
         }
      }
   }

   /**
    * parseIntegratedTestScript - Scan an LLVM/Clang style integrated test
    * script and extract the lines to 'RUN' as well as 'XFAIL' and 'REQUIRES'
    * and 'UNSUPPORTED' information.
    *
    * If additional parsers are specified then the test is also scanned for the
    * keywords they specify and all matches are passed to the custom parser.
    *
    * If 'require_script' is False an empty script
    * may be returned. This can be used for test formats where the actual script
    * is optional or ignored.
    * @param TestCase $test
    * @param IntegratedTestKeywordParser[] $additionalParsers
    * @param bool $requireScript
    */
   private function parseIntegratedTestScript(TestCase $test, array $additionalParsers = [],
                                              bool $requireScript= true)
   {
      // Install the built-in keyword parsers.
      $script = [];
      $buildinParsers = [
         new IntegratedTestKeywordParser(self::RUN_KEYWORD, ParserKind::COMMAND, null, $script),
         new IntegratedTestKeywordParser(self::XFAIL_KEYWORD, ParserKind::BOOLEAN_EXPR, null, $test->getXfails()),
         new IntegratedTestKeywordParser(self::REQUIRES_KEYWORD, ParserKind::BOOLEAN_EXPR, null, $test->getRequires()),
         new IntegratedTestKeywordParser(self::REQUIRES_ANY_KEYWORD, ParserKind::CUSTOM, IntegratedTestKeywordParser::REQUIRE_ANY_PARSER, $test->getRequires()),
         new IntegratedTestKeywordParser(self::UNSUPPORTED_KEYWORD, ParserKind::BOOLEAN_EXPR, null, $test->getUnsupportedFeatures()),
         new IntegratedTestKeywordParser(self::END_KEYWORD, ParserKind::TAG),
      ];
      $keywordParsers = array();
      foreach ($buildinParsers as $parser) {
         $keywordParsers[$parser->getKeyword()] = $parser;
      }

      // Install user-defined additional parsers.
      foreach ($additionalParsers as $parser) {
         if (!$parser instanceof IntegratedTestKeywordParser) {
            throw new ValueException('additional parser must be an instance of IntegratedTestKeywordParser');
         }
         if (array_key_exists($parser->getKeyword(), $keywordParsers)) {
            throw new ValueException(sprintf("Parser for keyword '%s' already exists", $parser->getKeyword()));
         }
         $keywordParsers[$parser->getKeyword()] = $parser;
      }
      // Collect the test lines from the script.
      $sourcePath = $test->getSourcePath();
      $scriptCommands = $this->parseIntegratedTestScriptCommands($sourcePath, array_keys($keywordParsers));
      foreach ($scriptCommands as $centry) {
         list($lineNumber, $commandType, $line) = $centry;
         $parser = $keywordParsers[$commandType];
         $parser->parseLine($lineNumber, $line);
         if ($commandType == 'END.' && $parser->getValue()) {
            break;
         }
      }
      $script = $keywordParsers[self::RUN_KEYWORD]->getValue();
      $test->setXfails($keywordParsers[self::XFAIL_KEYWORD]->getValue());
      $requires = $keywordParsers[self::REQUIRES_KEYWORD]->getValue();
      $requires = array_merge($requires, $keywordParsers[self::REQUIRES_ANY_KEYWORD]->getValue());
      $test->setRequires($requires);
      $test->setUnsupported($keywordParsers[self::UNSUPPORTED_KEYWORD]->getValue());
      // Verify the script contains a run line.
      if ($requireScript && empty($script)) {
         return new TestResult(TestResultCode::UNRESOLVED(), 'Test has no run line!');
      }

      // Check for unterminated run lines.
      if (!empty($script) && $script[count($script) - 1][-1] == '\\') {
         return new TestResult(TestResultCode::UNRESOLVED(), 'Test has unterminated run lines (with \'\\\')');
      }

      // Check boolean expressions for unterminated lines.
      foreach ($keywordParsers as $key => $parser) {
         if ($parser->getKind() != ParserKind::BOOLEAN_EXPR) {
            continue;
         }
         $value = $parser->getValue();
         if (!empty($value) && $value[count($value) - 1][-1] == '\\') {
            throw new ValueException("Test has unterminated $key lines (with '\\')");
         }
      }

      // Enforce REQUIRES:
      $missingRequiredFeatures = $test->getMissingRequiredFeatures();
      if (!empty($missingRequiredFeatures)) {
         $msg = join(', ', $missingRequiredFeatures);
         return new TestResult(TestResultCode::UNSUPPORTED(), "Test requires the following unavailable features: $msg");
      }

      // Enforce UNSUPPORTED:
      $unsupportedFeatures = $test->getUnsupportedFeatures();
      if (!empty($unsupportedFeatures)) {
         $msg = join(', ', $unsupportedFeatures);
         return new TestResult(TestResultCode::UNSUPPORTED(), "Test does not support the following features and/or targets: $msg");
      }

      // Enforce limit_to_features.
      if (!$test->isWithinFeatureLimits()) {
         $msg = join(', ', $test->getConfig()->getLimitToFeatures());
         return new TestResult(TestResultCode::UNSUPPORTED(), "Test does not require any of the features specified in limit_to_features: $msg");
      }
      return $script;
   }

   /**
    * parseIntegratedTestScriptCommands(source_path) -> commands
    *
    * Parse the commands in an integrated test script file into a list of
    * (line_number, command_type, line).
    *
    * @param string $sourcePath
    * @param array $keywords
    */
   private function parseIntegratedTestScriptCommands(string $sourcePath, array $keywords): iterable
   {
      $quotedKeywords = array_map(function ($keyword) {
         return preg_quote($keyword);
      }, $keywords);
      $keywordsRegex = sprintf("/(%s)(.*)\n/", join('|', $quotedKeywords));
      $data = file_get_contents($sourcePath);
      if (false !== $data) {
         // Ensure the data ends with a newline.
         if ($data[-1] != "\n") {
            $data .= "\n";
         }
         // Iterate over the matches.
         $lineNumber = 1;
         $lastMatchPosition = 0;
         if (preg_match_all($keywordsRegex, $data, $matches, PREG_OFFSET_CAPTURE)) {
            $allCaptureArray = $matches[0];
            $keywordsCaptureArray = $matches[1];
            $lineCaptureArray = $matches[2];
            foreach ($allCaptureArray as $index => $item) {
               $matchPosition = $item[1];
               $lineNumber += $this->countNewline($data, $lastMatchPosition, $matchPosition);
               $lastMatchPosition = $matchPosition;
               $keyword = $keywordsCaptureArray[$index][0];
               $line = $lineCaptureArray[$index][0];
               yield [$lineNumber, $keyword, $line];
            }
         }
      }
   }

   private function countNewline(string $str, int $start = 0, int $stop = null): int
   {
      if ($stop == null) {
         $stop = strlen($str);
      }
      $count = 0;
      for ($i = $start; $i < $stop; ++$i) {
         if ($str[$i] == "\n") {
            ++$count;
         }
      }
      return $count;
   }

   /**
    * Get the temporary location, this is always relative to the test suite
    * root, not test source root.
    *
    * @param TestCase $test
    * @return array
    */
   private function getTempPaths(TestCase $test): array
   {
      $execPath = $test->getExecPath();
      list($execDir, $execBase) = path_split($execPath);
      $tempDir = $execDir . DIRECTORY_SEPARATOR . 'Output';
      $tempBase = $tempDir . DIRECTORY_SEPARATOR . $execBase;
      return [$tempDir, $tempBase];
   }

   private function colonNormalizePath(string $path): string
   {
      if ($this->litConfig->isWindows()) {
         $path = str_replace('\\', '/', $path);
         $path = preg_replace('/^(.):/', '$1', $path);
      } else {
         assert($path[0] == '/');
         $path = substr($path, 1);
      }
      return $path;
   }

   private function getDefaultSubstitutions(TestCase $test, string $tempDir, string $tempBase,
                                            bool $normalizeSlashes = false): array
   {
      $sourcePath = $test->getSourcePath();
      $sourceDir = dirname($sourcePath);
      // Normalize slashes, if requested.
      if ($normalizeSlashes) {
         $sourcePath = str_replace('\\', '/', $sourcePath);
         $sourceDir = str_replace('\\', '/', $sourceDir);
         $tempDir = str_replace('\\', '/', $tempDir);
         $tempBase = str_replace('\\', '/', $tempBase);
      }
      // We use #_MARKER_# to hide %% while we do the other substitutions.
      $substitutions = [];
      $substitutions[] = ['%%', '#_MARKER_#'];
      array_merge($substitutions, $test->getConfig()->getSubstitutions());
      $tempName = $tempBase . '.temp';
      $baseName = basename($tempBase);
      $substitutions = array_merge($substitutions, array(
         ['%s', $sourcePath],
         ['%S', $sourceDir],
         ['%p', $sourceDir],
         ['%{pathsep}', DIRECTORY_SEPARATOR],
         ['%t', $tempName],
         ['%basename_t', $baseName],
         ['%T', $tempDir],
         ['%{php}', PHP_BINARY],
         ['#_MARKER_#', '%']
      ));
      // "%/[STpst]" should be normalized.
      $substitutions = array_merge($substitutions, array(
         ['%/s', $sourcePath],
         ['%/S', $sourceDir],
         ['%/p', $sourceDir],
         ['%/t', $tempName],
         ['%/T', $tempDir],
      ));
      // "%:[STpst]" are normalized paths without colons and without a leading
      // slash.
      $substitutions = array_merge($substitutions, array(
         ['%:s', $this->colonNormalizePath($sourcePath)],
         ['%:S', $this->colonNormalizePath($sourceDir)],
         ['%:p', $this->colonNormalizePath($sourceDir)],
         ['%:t', $this->colonNormalizePath($tempName)],
         ['%:T', $this->colonNormalizePath($tempDir)],
      ));
      return $substitutions;
   }

   /**
    * Apply substitutions to the script.  Allow full regular expression syntax.
    * Replace each matching occurrence of regular expression pattern a with
    * substitution b in line ln.
    *
    * @param $script
    * @param array $substitutions
    * @return array
    */
   private function applySubstitutions($script, array $substitutions): array
   {
      $isWindows = $this->litConfig->isWindows();
      return array_map(function (string $line) use($isWindows, $substitutions) {
         // Apply substitutions
         foreach ($substitutions as $sentry) {
            list($mark, $value) = $sentry;
            if ($isWindows) {
               $value = str_replace('\\', '\\\\', $value);
            }
            $line = str_replace($mark, $value, $line);
         }
         // Strip the trailing newline and any extra whitespace.
         return trim($line);
      }, $script);
   }
}