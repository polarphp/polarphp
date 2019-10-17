<?php

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

$argc = $_SERVER['argc'];
$argv = $_SERVER['argv'];
if ($argc != 2) {
   throw new RuntimeException("unexpected number of args");
}
if ($argv[1] == '--gtest_list_tests') {
   echo <<<END
FirstTest.
  subTestA
  subTestB
  subTestC
END;
   exit(0);
} elseif (!str_start_with($argv[1],'--gtest_filter=')) {
   throw new RuntimeException("unexpected argument: ${argv[1]}");
}

$testName = explode('=', $argv[1], 2)[1];
if ($testName == 'FirstTest.subTestA') {
   print("I am subTest A, I PASS\n");
   print("[  PASSED  ] 1 test.");
   exit(0);
} elseif ($testName == 'FirstTest.subTestB') {
   print("I am subTest B, I am slow\n");
   sleep(6);
   print("[  PASSED  ] 1 test.\n");
   exit(0);
} elseif ($testName == 'FirstTest.subTestC') {
   print("I am subTest C, I will hang\n");
   while (true)
   {

   }
} else {
   die("error: invalid test name: $testName");
}