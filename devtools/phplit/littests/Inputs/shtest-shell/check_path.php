#!/usr/bin/env php
<?php

function check_path(array $argv)
{
   if (count($argv) < 3) {
      echo "Wrong number of args";
      return 1;
   }
   $type = $argv[1];
   $paths = array_slice($argv, 2);
   $exitCode = 0;
   if ($type == 'dir') {
      foreach ($paths as $idx => $dir) {
         printf("%s\n", is_dir($dir) ? 'true' : 'false');
      }
   } elseif ($type == 'file') {
      foreach ($paths as $idx => $file) {
         printf("%s\n", is_file($file) ? 'true' : 'false');
      }
   } else {
      printf("Unrecognised type %s", $type);
      $exitCode = 0;
   }
   return $exitCode;
}
exit(check_path($_SERVER['argv']));