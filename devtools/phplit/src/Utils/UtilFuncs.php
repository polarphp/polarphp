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
use Lit\Kernel\TestingConfig;
use Symfony\Component\Filesystem\Filesystem;

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
         $file = $path . PATH_SEPARATOR . $command . $ext;
         if (file_exists($file) && !is_dir($file)) {
            return realpath($file);
         }
      }
   }
   return null;
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
          !in_array($entry->getExtension(), $suffixes)) {
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
 * @param string $command
 * @param string|null $cwd
 * @param array $env
 * @param string|null $input
 * @param int $timeout
 */
function execute_command(string $command, string $cwd = null, array $env = [],
                         string $input = null, int $timeout = 0)
{

}

function use_platform_sdk_on_darwin(TestingConfig $config, LitConfig $litConfig)
{

}

function find_platform_sdk_version_on_macos(TestingConfig $config, LitConfig $litConfig)
{

}

function has_substr(string $str, string $subStr)
{
   return strpos($str, $subStr) !== false;
}

function kill_process_and_children(int $pid)
{

}

// dummy class
class UtilFuncs{}