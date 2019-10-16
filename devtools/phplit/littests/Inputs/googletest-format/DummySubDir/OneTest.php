<?php

use Lit\Kernel\ValueException;

$argc = $_SERVER['argc'];
$argv = $_SERVER['argv'];
if ($argc != 2) {
   throw new ValueException("unexpected number of args");
}

if ($argv[1] == '--gtest_list_tests') {
   echo <<<END
FirstTest.
  subTestA
  subTestB
ParameterizedTest/0.
  subTest
ParameterizedTest/1.
  subTest
END;
   exit(0);
}
$testName = explode('=', $argv[1], 2)[1];
if ($testName == 'FirstTest.subTestA') {
   print("I am subTest A, I PASS\n");
   print("[  PASSED  ] 1 test.\n");
   exit(0);
} elseif ($testName == 'FirstTest.subTestB') {
   print("I am subTest B, I FAIL\n");
   print("And I have two lines of output\n");
   exit(1);
} elseif (in_array($testName, ['ParameterizedTest/0.subTest', 'ParameterizedTest/1.subTest'])) {
   print("I am a parameterized test, I also PASS\n");
   print("[  PASSED  ] 1 test.\n");
   exit(0);
} else {
   die("error: invalid test name: $testName");
}
