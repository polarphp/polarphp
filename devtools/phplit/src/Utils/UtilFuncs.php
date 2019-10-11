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
use Symfony\Component\Filesystem\Filesystem;

use function foo\func;

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
 * TODO port to Windows
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

// dummy class
class UtilFuncs{}