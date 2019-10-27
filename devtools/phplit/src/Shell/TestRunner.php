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

use Lit\Exception\InternalShellException;
use Lit\Exception\KeyboardInterruptException;
use Lit\Exception\ExecuteCommandTimeoutException;
use Lit\Exception\ProcessSignaledException;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestCase;
use Lit\Kernel\TestResult;
use Lit\Kernel\TestResultCode;
use Lit\Exception\ValueException;
use Lit\Shell\BuiltinCommand\BuiltinCommandInterface;
use Lit\Shell\BuiltinCommand\CdCommand;
use Lit\Shell\BuiltinCommand\DiffCommand;
use Lit\Shell\BuiltinCommand\EchoCommand;
use Lit\Shell\BuiltinCommand\ExportCommand;
use Lit\Shell\BuiltinCommand\MkdirCommand;
use Lit\Shell\BuiltinCommand\RmCommand;
use Lit\Utils\ShellEnvironment;
use Lit\Utils\ShParser;
use Lit\Utils\TestLogger;
use Lit\Utils\TimeoutHelper;
use Symfony\Component\Filesystem\Filesystem;
use function Lit\Utils\array_to_str;
use function Lit\Utils\execute_command;
use function Lit\Utils\expand_glob;
use function Lit\Utils\expand_glob_expressions;
use function Lit\Utils\has_substr;
use function Lit\Utils\is_absolute_path;
use function Lit\Utils\make_temp_file;
use function Lit\Utils\open_temp_file;
use function Lit\Utils\path_split;
use function Lit\Utils\which;
use function Lit\Utils\update_shell_env;

class TestRunner
{
   const RUN_KEYWORD = 'RUN:';
   const XFAIL_KEYWORD = 'XFAIL:';
   const REQUIRES_KEYWORD = 'REQUIRES:';
   const REQUIRES_ANY_KEYWORD = 'REQUIRES-ANY:';
   const UNSUPPORTED_KEYWORD = 'UNSUPPORTED:';
   const END_KEYWORD = 'END.';

   const DEV_NULL = '/dev/null';


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

   /**
    * @var BuiltinCommandInterface[] $builtinCommands
    */
   private $builtinCommands = [];

   /**
    * @var array $builtinExecutables
    */
   private $builtinExecutables = [];

   public function __construct(LitConfig $litConfig, array $extraSubstitutions = array(),
                               bool $useExternalShell = false)
   {
      $this->litConfig = $litConfig;
      $this->extraSubstitutions = $extraSubstitutions;
      $this->useExternalShell = $useExternalShell;
      $this->setupBuiltinCommands();
      $this->setupBuiltinExecutables();
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
      for ($index = 0; $index < $attempts; ++$index) {
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
   private function executeScriptByProcess(TestCase $test, array &$commands, string $tempBase, string $cwd)
   {
      $isWindows = $this->litConfig->isWindows();
      $basePath = $this->litConfig->getBashPath();
      $isWin32CMDEXE = $isWindows && !$basePath;
      $script = $tempBase . '.script';
      if ($isWin32CMDEXE) {
         $script .= '.bat';
      }
      // Write script file
      $mode = 'w';
      if ($isWindows && !$isWin32CMDEXE) {
         $mode .= 'b';
      }
      $file = fopen($script, $mode);
      if ($isWin32CMDEXE) {
         foreach ($commands as $index => $line) {
            $commands[$index] = preg_replace(sprintf('/%s$/', IntegratedTestKeywordParser::PDBG_REGEX), "echo '\\1' > nul && ", $line);
         }
         if ($this->litConfig->isEchoAllCommands()) {
            fwrite($file, "@echo on\n");
         } else {
            fwrite($file, "@echo off\n");
         }
         fwrite($file, join("\n@if %ERRORLEVEL% NEQ 0 EXIT\n", $commands));
      } else {
         foreach ($commands as $index => $line) {
            $commands[$index] = preg_replace(sprintf('/%s/', IntegratedTestKeywordParser::PDBG_REGEX), ": '$1'; ", $line);
         }
         if ($test->getConfig()->isPipeFail()) {
            fwrite($file, "set -o pipefail;");
         }
         if ($this->litConfig->isEchoAllCommands()) {
            fwrite($file, "set -x;");
         }
         fwrite($file, '{ ' . join("; } &&\n{ ", $commands) . '; }');
      }
      fwrite($file, "\n");
      fclose($file);
      if ($isWin32CMDEXE) {
         $command = ['cmd', '/', $script];
      } else {
         if ($basePath) {
            $command = [$basePath, $script];
         } else {
            $command = ['/bin/sh', $script];
         }
         if ($this->litConfig->isUseValgrind()) {
            # FIXME: Running valgrind on sh is overkill. We probably could just
            # run on clang with no real loss.
            $command = array_merge($this->litConfig->getValgrindArgs(), $command);
         }
      }
      try {
         list($out, $err, $exitCode) = execute_command($command, $cwd, $test->getConfig()->getEnvironment(), null,
            $this->litConfig->getMaxIndividualTestTime());
         return [$this->filterInvalidUtf8($out), $this->filterInvalidUtf8($err), $exitCode, null];
      } catch (ExecuteCommandTimeoutException $e) {
         return [$e->getOutMsg(), $e->getErrorMsg(), $e->getExitCode(), $e->getMessage()];
      }
   }

   protected function filterInvalidUtf8(string $data)
   {
      $data = preg_replace_callback('/([\x00-\x08\x10\x0B\x0C\x0E-\x19\x7F]'.
         '|[\x00-\x7F][\x80-\xBF]+'.
         '|([\xC0\xC1]|[\xF0-\xFF])[\x80-\xBF]*'.
         '|[\xC2-\xDF]((?![\x80-\xBF])|[\x80-\xBF]{2,})'.
         '|[\xE0-\xEF](([\x80-\xBF](?![\x80-\xBF]))|(?![\x80-\xBF]{2})|[\x80-\xBF]{3,}))/S',
         function ($code) {
            return '\x'.dechex(ord($code[1]));
         }, $data);
      $data = preg_replace_callback('/(\xE0[\x80-\x9F][\x80-\xBF]'.
         '|\xED[\xA0-\xBF][\x80-\xBF])/S',
         function($code) {
            return '\x'.dechex(ord($code[1]));
         }, $data);
      return $data;
   }

   /**
    * @param TestCase $test
    * @param array $commands
    * @param string $tempBase
    * @param string $cwd
    */
   private function executeScriptInternal(TestCase $test, array &$commands, string $tempBase, string $cwd)
   {
      $cmds = [];
      foreach ($commands as $i => $line) {
         $line = preg_replace(sprintf('/%s/', IntegratedTestKeywordParser::PDBG_REGEX), ": '$1'; ", $line);
         $commands[$i] = $line;
         try {
            $parser = new ShParser($line, $this->litConfig->isWindows(), $test->getConfig()->isPipeFail());
            $cmds[] = $parser->parse();
         } catch (\Exception $e) {
            TestLogger::errorWithoutCount($e->getMessage());
            return new TestResult(TestResultCode::FAIL(), "shell parser error on: $line");
         }
      }
      $cmd = $cmds[0];
      foreach (array_slice($cmds, 1) as $c) {
         $cmd = new SeqCmmand($cmd, '&&', $c);
      }
      $results = [];
      $timeoutInfo = null;
      try {
         $shenv = new ShellEnvironment($cwd, $test->getConfig()->getEnvironment());
         list($exitCode, $timeoutInfo) = $this->executeShellCommand($cmd, $shenv, $results, $this->litConfig->getMaxIndividualTestTime());
      } catch (InternalShellException $e) {
         $exitCode = 127;
         $results[] = new ShellCommandResult($e->getCommand(), '', $e->getMessage(), $exitCode,false);
      }
      $out = $err = '';
      $litConfig = $this->litConfig;
      foreach ($results as $i => $result) {
         // Write the command line run.
         $args = $result->getCommand()->getArgs();
         $out .= sprintf("$ %s\n", join(' ', array_map(function ($item) {
            return sprintf('"%s"', $item);
         } ,$args)));
         $stdout = trim($result->getStdout());
         $stderr = trim($result->getStderr());
         $resultExitCode = $result->getExitCode();
         // If nothing interesting happened, move on.
         if ($litConfig->getMaxIndividualTestTime() == 0 && $resultExitCode == 0 &&
            empty($stdout) && empty($stderr)) {
            continue;
         }
         // Otherwise, something failed or was printed, show it.
         // Add the command output, if redirected.
         foreach ($result->getOutputFiles() as $fentry) {
            list($name, $path, $data) = $fentry;
            if (trim($data)) {
               $out .= "# redirected output from $name:\n";
               // TODO encoding of data ?
               if (iconv_strlen($data, 'UTF-8') > 1024) {
                  $out .= iconv_substr($data, 0, 1024, 'UTF-8') . "\n...\n";
                  $out .= "note: data was truncated\n";
               } else {
                  $out .= $data;
               }
               $out .= "\n";
            }
         }
         if ($stdout) {
            $out .= sprintf("# command output:\n%s\n", $result->getStdout());
         }
         if ($stderr) {
            $out .= sprintf("# command stderr:\n%s\n", $result->getStderr());
         }
         if (!$stdout && !$stderr) {
            $out .= "note: command had no output on stdout or stderr\n";
         }
         // Show the error conditions:
         if ($resultExitCode != 0) {
            // On Windows, a negative exit code indicates a signal, and those are
            // easier to recognize or look up if we print them in hex.
            if ($litConfig->isWindows() && $resultExitCode < 0) {
               $codeStr = dechex(intval($resultExitCode & 0xFFFFFFFF));
            } else {
               $codeStr = strval($resultExitCode);
            }
            $out .= "error: command failed with exit status: $codeStr\n";
         }
         if ($litConfig->getMaxIndividualTestTime() > 0 && $result->isTimeoutReached()) {
            $out .= sprintf("error: command reached timeout: %s\n", $result->isTimeoutReached() ? 'true' : 'false');
         }
      }
      return [$out, $err, $exitCode, $timeoutInfo];
   }

   /**
    * Wrapper around _executeShCmd that handles timeout
    * @param ShCommandInterface $cmd
    * @param ShellEnvironment $shenv
    * @param array $results
    * @param int $timeout
    */
   private function executeShellCommand(ShCommandInterface $cmd, ShellEnvironment $shenv, array &$results, int $timeout = 0)
   {
      // Use the helper even when no timeout is required to make
      // other code simpler (i.e. avoid bunch of ``!= None`` checks)
      $timeoutHelper = new TimeoutHelper($timeout);
      if ($timeout > 0) {
         $timeoutHelper->startTimer();
      }
      $finalExitCode = $this->doExecuteShellCommand($cmd, $shenv, $results, $timeoutHelper);
      $timeoutHelper->cancel();
      $timeoutInfo = null;
      if ($timeoutHelper->timeoutReached()) {
         $timeoutInfo = "Reached timeout of $timeout seconds";
      }
      return [$finalExitCode, $timeoutInfo];
   }

   private function doExecuteShellCommand(ShCommandInterface $cmd, ShellEnvironment $shenv, array &$results,
                                          TimeoutHelper $timeoutHelper)
   {
      $kIsWindows = $this->litConfig->isWindows();
      $kAvoidDevNull = $kIsWindows;
      if ($timeoutHelper->timeoutReached()) {
         // Prevent further recursion if the timeout has been hit
         // as we should try avoid launching more processes.
         return null;
      }
      if ($cmd instanceof SeqCmmand) {
         $op = $cmd->getOp();
         if ($op == ';') {
            $this->doExecuteShellCommand($cmd->getLhs(), $shenv, $results, $timeoutHelper);
            return $this->doExecuteShellCommand($cmd->getRhs(), $shenv, $results, $timeoutHelper);
         }
         if ($op == '&') {
            throw new InternalShellException($cmd, "unsupported shell operator: '&'");
         }
         if ($op == '||') {
            $result = $this->doExecuteShellCommand($cmd->getLhs(), $shenv, $results, $timeoutHelper);
            if (0 != $result) {
               $result = $this->doExecuteShellCommand($cmd->getRhs(), $shenv, $results, $timeoutHelper);
            }
            return $result;
         }
         if ($op == '&&') {
            $result = $this->doExecuteShellCommand($cmd->getLhs(), $shenv, $results, $timeoutHelper);
            if ($result === null) {
               return $result;
            }
            if (0 == $result) {
               $result = $this->doExecuteShellCommand($cmd->getRhs(), $shenv, $results, $timeoutHelper);
            }
            return $result;
         }
         throw new ValueException("Unknown shell command: $op");
      }
      assert($cmd instanceof PipelineCommand);
      $firstCommand = $cmd->getCommand(0);
      $firstCommandName = $firstCommand->getArgs()[0];

      // Handle shell builtins first.
      if ($firstCommandName == 'cd') {
         if ($cmd->getPipeSize() != 1) {
            throw new InternalShellException($firstCommand,"'cd' cannot be part of a pipeline");
         }
         $this->builtinCommands['cd']->execute($firstCommand, $shenv);
         return 0;
      }

      // Handle "echo" as a builtin if it is not part of a pipeline. This greatly
      // speeds up tests that construct input files by repeatedly echo-appending to
      // a file.
      // FIXME: Standardize on the builtin echo implementation. We can use a
      // temporary file to sidestep blocking pipe write issues.
      if ($firstCommandName == 'echo' && $cmd->getPipeSize() == 1) {
         $output = $this->builtinCommands['echo']->execute($firstCommand, $shenv);
         $results[] = new ShellCommandResult($firstCommand, $output, '', 0, false);
         return 0;
      }

      if ($firstCommandName == 'export') {
         if ($cmd->getPipeSize() != 1) {
            throw new InternalShellException($firstCommand, "'export' cannot be part of a pipeline");
         }
         $this->builtinCommands['export']->execute($firstCommand, $shenv);
         return 0;
      }
      if ($firstCommandName == 'mkdir') {
         if ($cmd->getPipeSize() != 1) {
            throw new InternalShellException($firstCommand,"Unsupported: 'mkdir' cannot be part of a pipeline");
         }
         $result = $this->builtinCommands['mkdir']->execute($firstCommand, $shenv);
         $results[] = $result;
         return $result->getExitCode();
      }
      if ($firstCommandName == 'diff') {
         if ($cmd->getPipeSize() != 1) {
            throw new InternalShellException($firstCommand, "Unsupported: 'diff' cannot be part of a pipeline");
         }
         $result = $this->builtinCommands['diff']->execute($firstCommand, $shenv);
         $results[] = $result;
         return $result->getExitCode();
      }
      if ($firstCommandName == 'rm') {
         if ($cmd->getPipeSize() != 1) {
            throw new InternalShellException($firstCommand, "Unsupported: 'rm' cannot be part of a pipeline");
         }
         $result = $this->builtinCommands['rm']->execute($firstCommand, $shenv);
         $results[] = $result;
         return $result->getExitCode();
      }
      if ($firstCommandName == ':') {
         if ($cmd->getPipeSize() != 1) {
            throw new InternalShellException($firstCommand,
               "Unsupported: ':' cannot be part of a pipeline");
         }
         $results[] = new ShellCommandResult($firstCommand, '', '', 0, false);
         return 0;
      }
      /**
       * @var Process[] $processes
       */
      $processes = [];
      $defaultStdin = Process::REDIRECT_PIPE;
      $stderrTempFiles = [];
      $openedFiles = [];
      $namedTempFiles = [];
      // To avoid deadlock, we use a single stderr stream for piped
      // output. This is null until we have seen some output using
      // stderr.
      foreach ($cmd->getCommands() as $i => $pcmd) {
         // Reference the global environment by default.
         $cmdShenv = $shenv;
         $pcmdArgs = $pcmd->getArgs();
         if ($pcmdArgs[0] == 'env') {
            // Create a copy of the global environment and modify it for this one
            // command. There might be multiple envs in a pipeline:
            //   env FOO=1 llc < %s | env BAR=2 llvm-mc | FileCheck %s
            $cmdShenv = new ShellEnvironment($shenv->getCwd(), $shenv->getEnv());
            update_shell_env($cmdShenv, $pcmd);
         }
         list($stdin, $stdout, $stderr) = $this->processRedirects($pcmd, $defaultStdin, $cmdShenv, $openedFiles);
         // If stderr wants to come from stdout, but stdout isn't a pipe, then put
         // stderr on a pipe and treat it as stdout.
         if ($stderr == Process::REDIRECT_STDOUT && $stdout != Process::REDIRECT_PIPE) {
            $stderr = Process::REDIRECT_PIPE;
            $stderrIsStdout = true;
         } else {
            $stderrIsStdout = false;
            // Don't allow stderr on a PIPE except for the last
            // process, this could deadlock.
            //
            // FIXME: This is slow, but so is deadlock.
            if ($stderr == Process::REDIRECT_PIPE && $pcmd != $cmd->getLastCommand()) {
               $stderr = open_temp_file(PHPLIT_TEMP_PREFIX, 'w+b');
               $stderrTempFiles[] = [$i, $stderr];
            }
         }
         // Resolve the executable path ourselves.
         $args = $pcmd->getArgs();
         $executable = null;
         $isBuiltinExecutable = array_key_exists($args[0], $this->builtinExecutables);
         if (!$isBuiltinExecutable) {
            // For paths relative to cwd, use the cwd of the shell environment.
            if ($args[0][0] == '.') {
               $exeInCwd = $cmdShenv->getCwd().DIRECTORY_SEPARATOR.$args[0];
               if (is_file($exeInCwd)) {
                  $executable = $exeInCwd;
               }
            } elseif ($args[0] == 'php') {
               $executable = PHP_BINARY;
            }

            if (!$executable) {
               $executable = which($args[0], $cmdShenv->getEnvVar('PATH', ''));
            }
            if (!$executable) {
               throw new InternalShellException($pcmd, sprintf("'%s': command not found", $args[0]));
            }
         } else {
            $executable = $this->builtinExecutables[$args[0]];
            if (!$executable) {
               throw new InternalShellException($pcmd, sprintf("'%s': builtin executable not found", $args[0]));
            }
         }
         $args[0] = $executable;
         // Replace uses of /dev/null with temporary files.
         if ($kAvoidDevNull) {
            foreach ($args as $argIndex => $arg) {
               if (is_string($arg) && has_substr($arg, self::DEV_NULL)) {
                  $tfilename = make_temp_file(PHPLIT_TEMP_PREFIX);
                  $namedTempFiles[] = $tfilename;
                  $args[$argIndex] = str_replace(self::DEV_NULL, $tfilename);
               }
            }
            $pcmd->setArgs($args);
         }

         // Expand all glob expressions
         $args = expand_glob_expressions($args, $cmdShenv->getCwd());
         // On Windows, do our own command line quoting for better compatibility
         // with some core utility distributions.
         if ($kIsWindows) {
            $args = $this->quoteWindowsCommand($args);
         }
         try {
            if (is_resource($stdin)) {
               $processInput = stream_get_contents($stdin);
               fseek($stdin, 1, SEEK_END);
            } else {
               $processInput = $stdin;
            }
            $process = new Process($args, $cmdShenv->getCwd(), $cmdShenv->getEnv(), $processInput, $stdout, $stderr);
            $process->start();
            $timeoutHelper->addProcess($process);
            $processes[] = $process;
            $process->wait();
         } catch (ProcessSignaledException $e) {
            if ($e->getSignal() != SIGKILL) {
               throw new InternalShellException($pcmd, sprintf('process (%s) exit due catch signal %d', $executable, $e->getSignal()));
            }
         } catch (\Exception $e) {
            throw new InternalShellException($pcmd, sprintf('Could not create process (%s) due to %s', $executable, $e->getMessage()));
         }
         // Update the current stdin source.
         if ($stdout == Process::REDIRECT_PIPE) {
            $defaultStdin = $process->getStdout();
         } elseif ($stderrIsStdout) {
            $defaultStdin = $process->getStderr();
         } else {
            $defaultStdin = Process::REDIRECT_PIPE;
         }
      }
      // Explicitly close any redirected files. We need to do this now because we
      // need to release any handles we may have on the temporary files (important
      // on Win32, for example). Since we have already spawned the subprocess, our
      // handles have already been transferred so we do not need them anymore.
      foreach ($openedFiles as $fentry) {
         list($name, $mode, $file, $path) = $fentry;
         fclose($file);
      }
      // FIXME: There is probably still deadlock potential here. Yawn.
      $processesData = [];
      foreach ($processes as $i => $process) {
         $processesData[$i] = [$this->filterInvalidUtf8($process->getOutput()), $this->filterInvalidUtf8($process->getErrorOutput())];
      }
      // Read stderr out of the temp files.
      foreach ($stderrTempFiles as $entry) {
         list($i, $file) = $entry;
         fseek($file, 0, SEEK_SET);
         $processesData[$i] = [$processesData[$i][0], stream_get_contents($file)];
         fclose($file);
      }
      $exitCode = null;
      foreach ($processesData as $i => $entry) {
         list($out, $err) = $entry;
         $result = $processes[$i]->getExitCode();
         // Ensure the resulting output is always of string type.
         $outputFiles = [];
         if ($result != 0) {
            foreach ($openedFiles as $entry) {
               list($name, $mode, $file, $path) = $entry;
               if ($path != null && in_array($mode, ['w', 'a'])) {
                  $file = fopen($path, 'rb');
                  $data = stream_get_contents($file);
                  if ($data) {
                     $outputFiles[] = [$name, $path, $data];
                  }
               }
            }
         }

         $results[] = new ShellCommandResult(
            $cmd->getCommands()[$i], $out, $err, $result, $timeoutHelper->timeoutReached(),
            $outputFiles
         );
         if ($cmd->isPipeError()) {
            // Take the last failing exit code from the pipeline.
            if (!$exitCode || $result != 0) {
               $exitCode = $result;
            }
         } else {
            $exitCode = $result;
         }
      }
      $fs = new Filesystem();
      // Remove any named temporary files we created.
      $fs->remove($namedTempFiles);
      if ($cmd->isNegate()) {
         $exitCode = !$exitCode;
      }
      return $exitCode;
   }

   /**
    * Return the standard fds for cmd after applying redirects
    *
    * Returns the three standard file descriptors for the new child process.  Each
    * fd may be an open, writable file object or a sentinel value from the
    * subprocess module.
    *
    * @param ShCommand $cmd
    * @param $stdSource
    * @param ShellEnvironment $cmdShEnv
    * @param array $openFiles
    */
   public function processRedirects(ShCommand $cmd, $stdSource, ShellEnvironment $cmdShEnv, array &$openFiles)
   {
      $kIsWindows = $this->litConfig->isWindows();
      $kAvoidDevNull = $kIsWindows;
      // Apply the redirections, we use (N,) as a sentinel to indicate stdin,
      // stdout, stderr for N equal to 0, 1, or 2 respectively. Redirects to or
      // from a file are represented with a list [file, mode, file-object]
      // where file-object is initially None.
      $redirects = array(
         [0], [1], [2]
      );
      foreach ($cmd->getRedirects() as $entry) {
         list($op, $filename) = $entry;
         if ($op === ['>', 2]) {
            $redirects[2] = [$filename, 'wb', null];
         } elseif ($op === ['>>', 2]) {
            $redirects[2] = [$filename, 'a', null];
         } elseif ($op === ['>&', 2] && has_substr('012', $filename)) {
            $redirects[2] = &$redirects[intval($filename)];
         } elseif ($op === ['>&'] || $op === ['&>']) {
            $redirects[1] = [$filename, 'wb', null];
            $redirects[2] = &$redirects[1];
         } elseif ($op === ['>']) {
            $redirects[1] = [$filename, 'wb', null];
         } elseif ($op === ['>>']) {
            $redirects[1] = [$filename, 'ab', null];
         } elseif ($op === ['<']) {
            $redirects[0] = [$filename, 'r', null];
         } else {
            throw new InternalShellException($cmd, sprintf("Unsupported redirect: %s", array_to_str([$op, $filename])));
         }
      }
      // Open file descriptors in a second pass.
      $stdFds = [null, null, null];
      foreach ($redirects as $i => &$redirect) {
         // Handle the sentinel values for defaults up front.
         if (count($redirect) == 1) {
            if ($redirect === [0]) {
               $fd = $stdSource;
            } elseif ($redirect == [1]) {
               if ($i == 0) {
                  throw new InternalShellException($cmd, 'Unsupported redirect for stdin');
               } elseif ($i == 1) {
                  $fd = Process::REDIRECT_PIPE;
               } else {
                  $fd = Process::REDIRECT_STDOUT;
               }
            } elseif ($redirect == [2]) {
               if ($i != 2) {
                  throw new InternalShellException($cmd, 'Unsupported redirect on stdout');
               }
               $fd = Process::REDIRECT_PIPE;
            } else {
               throw new InternalShellException($cmd, 'Bad redirect');
            }
            $stdFds[$i] = $fd;
            continue;
         }
         list($filename, $mode, $fd) = $redirect;
         // Check if we already have an open fd. This can happen if stdout and
         // stderr go to the same place.
         if ($fd != null) {
            $stdFds[$i] = $fd;
            continue;
         }
         $redirectFilename = null;
         $name = expand_glob($filename, $cmdShEnv->getCwd());
         if (count($name) != 1) {
            throw new InternalShellException($cmd, "Unsupported: glob in redirect expanded to multiple files");
         }
         $name = $name[0];
         if ($kAvoidDevNull && $name == self::DEV_NULL) {
            $fd = open_temp_file(PHPLIT_TEMP_PREFIX, $mode);
         } elseif ($kIsWindows && $name == '/dev/tty') {
            // Simulate /dev/tty on Windows.
            // "CON" is a special filename for the console.
            $fd = fopen("CON", $mode);
         } else {
            // Make sure relative paths are relative to the cwd.
            $redirectFilename = $name;
            if (!is_absolute_path($redirectFilename)) {
               $redirectFilename = $cmdShEnv->getCwd().DIRECTORY_SEPARATOR.$name;
            }
            $fd = fopen($redirectFilename, $mode);
         }
         // Workaround a Win32 and/or subprocess bug when appending.
         // FIXME: Actually, this is probably an instance of PR6753.
         if ($mode == 'a') {
            fseek($fd, 0, SEEK_SET);
         }
         // Mutate the underlying redirect list so that we can redirect stdout
         // and stderr to the same place without opening the file twice.
         $redirect[2] = $fd;
         $openFiles[] = [$filename, $mode, $fd, $redirectFilename];
         $stdFds[$i] = $fd;
      }
      return $stdFds;
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
         if ($commandType == 'END.' && [true] == $parser->getValue()) {
            break;
         }
      }
      $script = $keywordParsers[self::RUN_KEYWORD]->getValue();
      $test->setXfails($keywordParsers[self::XFAIL_KEYWORD]->getValue());
      $requires = $keywordParsers[self::REQUIRES_KEYWORD]->getValue();
      $requires = array_merge($requires, $keywordParsers[self::REQUIRES_ANY_KEYWORD]->getValue());
      $keywordParsers[self::REQUIRES_KEYWORD]->setValue($requires);
      $keywordParsers[self::REQUIRES_ANY_KEYWORD]->setValue($requires);
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
      if ($stop === null) {
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
      $substitutions = array_merge($substitutions, $test->getConfig()->getSubstitutions());
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
         ['%{phplit}', PHP_BINARY . ' ' . LIT_ROOT_DIR. DIRECTORY_SEPARATOR . 'lit run-test'],
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

   /**
    *  Reimplement Python's private subprocess.list2cmdline for MSys compatibility
    *
    * Based on CPython implementation here:
    * https://hg.python.org/cpython/file/849826a900d2/Lib/subprocess.py#l422
    *
    * Some core util distributions (MSys) don't tokenize command line arguments
    * the same way that MSVC CRT does. Lit rolls its own quoting logic similar to
    * the stock CPython logic to paper over these quoting and tokenization rule
    * differences.
    *
    * We use the same algorithm from MSDN as CPython
    * (http://msdn.microsoft.com/en-us/library/17w5ykft.aspx), but we treat more
    * characters as needing quoting, such as double quotes themselves.
    * @param array $seq
    */
   private function quoteWindowsCommand(array $seq)
   {
      $result = [];
      $needQuote = false;
      foreach ($seq as $arg) {
         $bsBuf = [];
         // Add a space to separate this argument from the others
         if ($result) {
            $result[] = ' ';
         }
         // This logic differs from upstream list2cmdline.
         $needquote = has_substr($arg, ' ') || has_substr($arg, "\t") ||
            has_substr($arg, "\"") || empty($arg);
         if ($needQuote) {
            $result[] = '"';
         }
         for ($i = 0; $i < strlen($arg); ++$i) {
            $char = $arg[$i];
            if ($char == "\\") {
               // Don't know if we need to double yet.
               $bsBuf[] = $char;
            } elseif ($char == '"') {
               // Double backslashes.
               $result[] = str_repeat("\\", count($bsBuf) * 2);
               $bsBuf = [];
               $result[] = '\\"';
            } else {
               // Normal char
               if ($bsBuf) {
                  $result = array_merge($result, $bsBuf);
                  $bsBuf = [];
               }
               $result[] = $char;
            }
         }
         // Add remaining backslashes, if any.
         if ($bsBuf) {
            $result = array_merge($result, $bsBuf);
         }
         if ($needQuote) {
            $result = array_merge($result, $bsBuf);
            $result[] = '"';
         }
      }
      return join('', $result);
   }

   private function setupBuiltinCommands()
   {
      $this->builtinCommands = array(
         'export' => new ExportCommand(),
         'mkdir' => new MkdirCommand(),
         'diff' => new DiffCommand(),
         'rm' => new RmCommand(),
         'echo' => new EchoCommand($this, $this->litConfig),
         'cd' => new CdCommand(),
      );
   }

   private function setupBuiltinExecutables()
   {
      $this->builtinExecutables = array(
         'cat' => POLARPHP_BIN_DIR.DIRECTORY_SEPARATOR.'cat'
      );
   }
}