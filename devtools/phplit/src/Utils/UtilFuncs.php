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

namespace Lit\Utils;
use Lit\Kernel\LitConfig;
use Lit\Kernel\TestCase;
use Lit\Kernel\TestingConfig;
use Lit\Exception\ExecuteCommandTimeoutException;
use Lit\Shell\GlobItemCommand;
use Lit\Shell\ShCommandInterface;
use Symfony\Component\Filesystem\Filesystem;
use Symfony\Component\Process\Exception\ProcessFailedException;
use Symfony\Component\Process\Exception\ProcessTimedOutException;
use Symfony\Component\Process\Exception\ProcessSignaledException;
use Symfony\Component\Process\Process;

function phpize_bool($value) : bool
{
   if (is_null($value)) {
      return false;
   }
   if (is_bool($value)) {
      return $value;
   }
   if (is_int($value)) {
      return $value != 0;
   }
   if (is_string($value)) {
      $value = strtolower(trim($value));
      if (in_array($value, array('1', 'true', 'on', 'yes'))) {
         return true;
      }
      if (in_array($value, array('', '0', 'false', 'off', 'no'))) {
         return false;
      }
   }
   throw new \Exception(sprintf('%s is not a valid boolean', strval($value)));
}

/**
 * @return bool
 */
function is_absolute_path(string $path) : bool
{
   $fs = new Filesystem();
   return $fs->isAbsolutePath($path);
}

/**
 * which(command, [paths]) - Look up the given command in the paths string
 * (or the PATH environment variable, if unspecified).
 *
 * @param string $command
 * @param string|null $paths
 * @return string
 */
function which(string $command, string $paths = null) : string
{
   if (is_null($paths)) {
      $paths = getenv('PATH');
      if (false === $paths) {
         $paths = '';
      }
   }
   if (is_absolute_path($command) && is_file($command)) {
      return realpath($command);
   }
   if (empty($paths)) {
      $paths = ':/bin/:/usr/bin';
   }
   // Get suffixes to search.
   // On Cygwin, 'PATHEXT' may exist but it should not be used.
   $pathExts = array('');
   if (PATH_SEPARATOR == ';') {
      $extStr = getenv('PATHEXT');
      $extStr = false === $extStr ? '' : $extStr;
      $pathExts = explode(PATH_SEPARATOR, $extStr);
   }
   $paths = explode(PATH_SEPARATOR, $paths);
   foreach ($paths as $path) {
      foreach ($pathExts as $ext) {
         $file = $path . DIRECTORY_SEPARATOR . $command . $ext;
         if (file_exists($file) && !is_dir($file)) {
            return realpath($file);
         }
      }
   }
   return '';
}

function check_tools_path(string $dir, array $tools) : bool
{
   foreach ($tools as $tool) {
      $file = $dir . PATH_SEPARATOR . $tool;
      if (!file_exists($file)) {
         return false;
      }
   }
   return true;
}

function which_tools(array $tools, string $paths) : string
{
   $paths = explode(PATH_SEPARATOR, $paths);
   foreach ($paths as $path) {
      if (check_tools_path($path, $tools)) {
         return $path;
      }
   }
   return null;
}

function get_envvar($name, $defaultValue) : string
{
   $value = getenv($name);
   return empty($value) ? $defaultValue : $value;
}

function is_os_win() : bool
{
   return strtoupper(substr(PHP_OS, 0, 3)) === 'WIN';
}

function copy_array(array $data) : array
{
   $temp = new \ArrayObject($data);
   return $temp->getArrayCopy();
}

function any_true(array $items, callable $callable)
{
   foreach ($items as $item) {
      if ($callable($item)) {
         return true;
      }
   }
   return false;
}

function str_start_with(string $str, string $prefix): bool
{
   $prefixLength = strlen($prefix);
   $strLength = strlen($str);
   if ($prefixLength == 0 || $strLength == 0) {
      return false;
   }
   if ($prefixLength > $strLength) {
      return false;
   }
   return substr($str, 0, $prefixLength) == $prefix;
}

function str_end_with(string $str, string $suffix): bool
{
   $suffixLength = strlen($suffix);
   $strLength = strlen($str);
   if ($suffixLength == 0 || $strLength == 0) {
      return false;
   }
   if ($suffixLength > $strLength) {
      return false;
   }
   return substr($str, -$suffixLength) == $suffix;
}

/**
 * @param string $dirname
 * @param array $suffixes
 * @param array $excludeFilenames
 * @return iterable
 */
function listdir_files(string $dirname, array $suffixes = [''], array $excludeFilenames = []) : iterable
{
   $iter = new \DirectoryIterator($dirname);
   foreach ($iter as $entry) {
      if ($entry->isDir() || $entry->isDot() || in_array($entry->getFilename(), $excludeFilenames) ||
         !any_true($suffixes, function ($suffix) use($entry) {
            return str_end_with($entry->getFilename(), $suffix);
      })) {
         continue;
      }
      yield $entry->getFilename();
   }
}

/**
 * Execute command ``command`` (list of arguments or string) with.
 * working directory ``cwd`` (str), use None to use the current
 * working directory
 * environment ``env`` (dict), use None for none
 * Input to the command ``input`` (str), use string to pass
 * no input.
 * Max execution time ``timeout`` (int) seconds. Use 0 for no timeout.
 * Returns a tuple (out, err, exitCode) where
 * ``out`` (str) is the standard output of running the command
 * ``err`` (str) is the standard error of running the command
 * ``exitCode`` (int) is the exitCode of running the command
 * If the timeout is hit an ``ExecuteCommandTimeoutException``
 * is raised.
 *
 * @param array $command
 * @param string|null $cwd
 * @param array $env
 * @param string|null $input
 * @param int $timeout
 */
function execute_command(array $command, string $cwd = null, array $env = [],
                         string $input = null, float $timeout = null): array
{
   $process = new Process($command, $cwd, $env, null, $timeout);
   $process->setInput($input);
   $process->start();
   $hitTimeout = false;
   $cpids = array();
   try {
      while ($process->isRunning()) {
         $cpids = retrieve_children_pids($process->getPid(), true);
         $process->checkTimeout();
         usleep(10000);
      }
      $process->wait();
   } catch (ProcessTimedOutException $e) {
      $hitTimeout = true;
      kill_processes($cpids);
   } catch (ProcessSignaledException $e) {
      kill_processes($cpids);
      throw $e;
   }
   $exitCode = $process->getExitCode();
   $out = $process->getOutput();
   $err = $process->getErrorOutput();
   if ($hitTimeout) {
      throw new ExecuteCommandTimeoutException(
         "Reached timeout of $timeout seconds",
         $out,
         $err,
         $exitCode
      );
   }
   return [$out, $err, $exitCode];
}

function use_platform_sdk_on_darwin(TestingConfig $config, LitConfig $litConfig)
{
   // On Darwin, support relocatable SDKs by providing Clang with a
   // default system root path.
   if (has_substr($config->hasExtraConfig('target_triple'), 'darwin')) {
      try {
         $cmd = ['xcrun', '--show-sdk-path', '--sdk', 'macosx'];
         $process = new Process($cmd);
         $sdkPath = trim($process->mustRun());
         TestLogger::note('using SDKROOT: %s', $sdkPath);
         $config->setEnvVar('SDKROOT', $sdkPath);
      } catch (ProcessFailedException $e) {
         TestLogger::errorWithoutCount('run xcrun error: %s', $e->getMessage());
      }
   }
}

function find_platform_sdk_version_on_macos(TestingConfig $config, LitConfig $litConfig)
{
   $sdkPath = '';
   if (has_substr($config->hasExtraConfig('target_triple'), 'darwin')) {
      try {
         $cmd = ['xcrun', '--show-sdk-path', '--sdk', 'macosx'];
         $process = new Process($cmd);
         $sdkPath = trim($process->mustRun());
      } catch (ProcessFailedException $e) {
         TestLogger::errorWithoutCount('run xcrun error: %s', $e->getMessage());
      }
   }
   return $sdkPath;
}

function has_substr(string $str, string $subStr)
{
   return strpos($str, $subStr) !== false;
}

function find_program_by_name(string $name, array $paths = []): string
{
   $name = trim($name);
   assert(!empty($name), "Must have a name!");
   // Use the given path verbatim if it contains any slashes; this matches
   // the behavior of sh(1) and friends.
   if (is_absolute_path($name)) {
      return $name;
   }
   if (empty($paths)) {
      $pathStr = get_envvar('PATH', '');
      $paths = explode(PATH_SEPARATOR, $pathStr);
   }
   foreach ($paths as $path) {
      $path = trim($path);
      if (empty($path)) {
         continue;
      }
      $filePath = $path . DIRECTORY_SEPARATOR . $name;
      if (file_exists($filePath) && is_executable($filePath)) {
         return $filePath;
      }
   }
   return '';
}

/**
 * TODO what about error ?
 *
 * @param int $pid
 * @return array
 */
function call_pgrep_command(int $pid): array
{
   $pgrep = find_program_by_name('pgrep');
   assert(!empty($pgrep), 'pgrep command not found.');
   $cmd = [$pgrep, '-P', strval($pid)];
   $process = new Process($cmd);
   $cpids = array();
   try {
      $process->mustRun();
      $output = $process->getOutput();
      $cpids = explode("\n", $output);
      $cpids = array_filter($cpids, function ($item) {
         return !empty(trim($item));
      });
   } catch (ProcessFailedException $e) {
      if (1 != $process->getExitCode()) {
         TestLogger::error("run pgrep error: %s\n%s", $e->getMessage(), $process->getErrorOutput());
      }
   }
   return $cpids;
}

function retrieve_children_pids(int $pid, bool $recursive = true): array
{
   $list = array();
   $children = call_pgrep_command($pid);
   $list = array_merge($list, $children);
   if (!$recursive) {
      return $list;
   }
   foreach ($list as $cpid) {
      $list = array_merge($list, retrieve_children_pids($cpid, true));
   }
   return $list;
}

function kill_process(int $pid)
{
   $tryCount = 3;
   while ($tryCount--) {
      $result = posix_kill($pid, SIGKILL);
      if ($result) {
         break;
      }
   }
}

function kill_process_and_children(int $pid)
{
   $children = retrieve_children_pids($pid, true);
   foreach ($children as $cpid) {
      kill_process($cpid);
   }
   kill_process($pid);
}

function kill_processes(array $pids)
{
   foreach ($pids as $pid) {
      kill_process($pid);
   }
}

/**
 * Detects the number of CPUs on a system.
 * Cribbed from pp.
 */
function detect_cpus()
{
   // Linux, Unix and MacOS:
   if (PHP_OS_FAMILY == OS_MACOS) {
      $shell = ['sysctl', '-n', 'hw.ncpu'];
   } elseif (PHP_OS_FAMILY == OS_LINUX) {
      $shell = ['cat', '/proc/cpuinfo', '|', 'grep', 'processor', '|', 'wc', '-l'];
   } else {
      // default
      return 1;
   }
   try {
      $process = new Process($shell);
      $process->mustRun();
      $output = $process->getOutput();
      return intval($output);
   } catch (ProcessFailedException $e) {
      TestLogger::warning($e);
      return 1;
   }
}

function array_to_str(array $arr)
{
   $msg = '[';
   $count = count($arr);
   for ($i = 0; $i < $count; ++$i) {
      $item = $arr[$i];
      if (is_array($item)) {
         $itemStr = array_to_str($item);
      } else {
         $itemStr = strval($item);
      }
      if ($i < $count - 1) {
         $itemStr .= ', ';
      }
      $msg .= $itemStr;
   }
   $msg .= ']';
   return $msg;
}

function array_extend_by_iterable(array &$target, iterable $source): void
{
   foreach ($source as $item) {
      array_push($target, $item);
   }
}

function sort_by_incremental_cache(array &$tests)
{
   usort($tests, function (TestCase $lhs, TestCase $rhs) {
      $lfname = $lhs->getFilePath();
      $rfname = $rhs->getFilePath();
      $lftime = fileatime($lfname);
      $rftime = fileatime($rfname);
      return $lftime <=> $rftime;
   });
}

function print_histogram(array $items, $title = 'Items')
{
   usort($items, function ($lhs, $rhs){
      return $lhs[1] <=> $rhs[1];
   });
   $maxValue = 0;
   foreach ($items as $item) {
      if ($item[1] > $maxValue) {
         $maxValue = $item[1];
      }
   }
   // Select first "nice" bar height that produces more than 10 bars.
   $power = intval(ceil(log($maxValue, 10)));
   foreach (generate_cycle([5, 2, 2.5, 1]) as $inc) {
      $barH = $inc * pow(10, $power);
      $num = intval(ceil($maxValue / $barH));
      if ($num > 10) {
         break;
      } elseif ($inc == 1) {
         $power -= 1;
      }
   }
   $histo = array();
   foreach (range(0, $num) as $index) {
      $histo[$index] = array();
   }
   foreach ($items as $name => $v) {
      $bin = min(intval($num * $v / $maxValue), $num - 1);
      $histo[$bin][] = $name;
   }
   $barW = 40;
   $hr = str_repeat('-', $barW + 34)."\n";
   printf("\nSlowest %s:", $title);
   print($hr);
   foreach (array_slice($items, -20) as $entry) {
      list($name, $value) = $entry;
      printf("%.2fs: %s", $value, $name);
   }
   printf("\n%s Times:", $title);
   print($hr);
   $pDigits = intval(ceil(log($maxValue, 10)));
   $pfDigits = max(0, 3 - $pDigits);
   if ($pfDigits) {
      $pDigits += $pfDigits + 1;
   }
   $cDigits = int(ceil(log(count($items), 10)));
   printf("[%s] :: [%s] :: [%s]\n",
      center_str('Range', ($pDigits + 1) * 2 + 3,' '),
      center_str('Percentage', $barW,' '),
      center_str('Count', $cDigits * 2 + 1,' '));
   print($hr);
   foreach ($histo as $i => $row) {
      $pct = (float)count($row) / count($items);
      $w = intval($barW * $pct);
      printf("[%*.*fs,%*.*fs) :: [%s%s] :: [%*d/%*d]",
         $pDigits, $pfDigits, $i * $barH, $pDigits, $pfDigits, ($i + 1) * $barH,
            str_repeat('*', $w), str_repeat(' ', $barW - $w), $cDigits, count($row), $cDigits, count($items));
   }
}

function center_str(string $text, int $width, string $fillStr)
{
   $textSize = strlen($text);
   if ($width < $textSize) {
      return $text;
   }
   $halfWidth = ($width - $textSize) / 2;
   $sideStr = str_repeat($fillStr, $halfWidth);
   return $sideStr . $text . $sideStr;
}

function generate_cycle(array $items)
{
   while (true) {
      foreach ($items as $item) {
         yield $item;
      }
   }
}

function path_split(string $path): array
{
   $path = trim($path);
   if (empty($path)) {
      return [];
   }
   $pos = strrpos($path, DIRECTORY_SEPARATOR);
   if (false === $pos) {
      return ['', $path];
   }
   if ($pos == 0) {
      return [DIRECTORY_SEPARATOR, substr($path, 1)];
   }
   return [substr($path, 0, $pos), substr($path, $pos + 1)];
}

function open_temp_file(string $prefix, string $mode)
{
   $fs = new Filesystem();
   $tempFilename = $fs->tempnam(sys_get_temp_dir(), $prefix);
   return fopen($tempFilename, $mode);
}

function make_temp_file(string $prefix): string
{
   $fs = new Filesystem();
   $tempFilename = $fs->tempnam(sys_get_temp_dir(), $prefix);
   $fs->touch($tempFilename);
   return $tempFilename;
}

function update_shell_env(ShellEnvironment $env, ShCommandInterface $cmd)
{
   $argIndex = 1;
   $unsetNextEnvVar = false;
   $cmdArgs = $cmd->getArgs();
   assert(count($cmdArgs) > 0, "command args count must greater than 0");
   $cmdArgs = array_slice($cmdArgs, 1);
   $envPool = $env->getEnv();
   foreach ($cmdArgs as $argIndex => $arg) {
      // Support for the -u flag (unsetting) for env command
      // e.g., env -u FOO -u BAR will remove both FOO and BAR
      // from the environment.
      if ($arg == '-u') {
         $unsetNextEnvVar = true;
         continue;
      }
      if ($unsetNextEnvVar) {
         $unsetNextEnvVar = false;
         if (array_key_exists($arg, $envPool)) {
            $env->unsetEnvVar($arg);
         }
         continue;
      }
      // Partition the string into KEY=VALUE.
      if (false === strpos($arg, '=')) {
         break;
      }
      $parts = explode('=', $arg, 2);
      assert(count($parts) == 2);
      $env->addEnvVar(trim($parts[0]), trim($parts[1]));
   }
   $cmd->setArgs(array_slice($cmdArgs, $argIndex));
}

function expand_glob($expr, string $cwd): array
{
   if ($expr instanceof GlobItemCommand) {
      $items = $expr->resolve($cwd);
      sort($items);
      return $items;
   }
   return [$expr];
}

function expand_glob_expressions(array $exprs, string $cwd): array
{
   $result = [$exprs[0]];
   foreach (array_slice($exprs, 1) as $expr) {
      $result = array_merge($result, expand_glob($expr, $cwd));
   }
   return $result;
}

function escape_xml_attr($text)
{
   return htmlspecialchars($text, ENT_XML1 | ENT_COMPAT, 'UTF-8');
}

function quote_xml_attr(string $data)
{
   $entities = [
      '\n'=> '&#10;',
      '\r'=> '&#13;',
      '\t'=> '&#9;'
   ];
   $data = escape_xml_attr(str_replace(array_keys($entities), array_values($entities), $data));
   if (has_substr($data, '"')) {
      if (has_substr($data, "'")) {
         $data = sprintf('"%s"', str_replace("'", '&quot;', $data));
      } else {
         $data = sprintf("'%s'", $data);
      }
   } else {
      $data = sprintf('"%s"', $data);
   }
   return $data;
}

function shcmd_split($source, bool $comments = false, bool $posix = true)
{
   $lexer = new SysShLexer($source, null, $posix);
   $lexer->setWhitespaceSplit(true);
   if (!$comments) {
      $lexer->setCommenters('');
   }
   return $lexer->toArray();
}

// dummy class
class UtilFuncs{}