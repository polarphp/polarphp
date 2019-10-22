#!/usr/bin/env php
<?php

use function Lit\Utils\is_os_win;

function has_substr(string $str, string $subStr)
{
   return strpos($str, $subStr) !== false;
}

$argv = $_SERVER['argv'];
$argc = $_SERVER['argc'];
$myArg = null;
if ($argc == 1) {
    echo "OK";
}
$argv = array_slice($argv, 1);
$myArgFound = false;
foreach ($argv as $arg) {
    $arg = trim($arg);
    if (has_substr($arg, '-a') || has_substr($arg, '--my_arg')) {
       $myArgFound = true;
       if ($arg == '-a' || $arg == '--my_arg') {
           continue;
       }
       if (substr($arg, 2) == '/dev/null' || substr($arg, 2) == '=/dev/null' ||
          substr($arg, 8) == '/dev/null' || substr($arg, 8) == '=/dev/null') {
          $myArg = '/dev/null';
          break;
       }
    } elseif ($myArgFound) {
        if ($arg == '/dev/null') {
           $myArg = '/dev/null';
           break;
        }
    }
}
$answer = 'OK';
if (strtoupper(substr(PHP_OS, 0, 3)) === 'WIN' && $myArg == '/dev/null') {
    $answer = 'ERROR';
}
echo $answer;