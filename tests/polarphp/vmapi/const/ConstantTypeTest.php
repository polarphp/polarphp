<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if(defined("MY_CONST")) {
   if (is_int(MY_CONST)) {
      echo "MY_CONST is int\n";
   }
}

if(defined("PI")) {
   if (is_double(PI)) {
      echo "PI is double\n";
   }
}

if(defined("POLAR_NAME")) {
   if (is_string(POLAR_NAME)) {
      echo "POLAR_NAME is string\n";
   }
}

if(defined("POLAR_VERSION")) {
   if (is_string(POLAR_VERSION)) {
      echo "POLAR_VERSION is string\n";
   }
}

if(defined("POLAR_FOUNDATION")) {
   if (is_string(POLAR_FOUNDATION)) {
      echo "POLAR_FOUNDATION is string\n";
   }
}
if(defined("\php\SYS_VERSION")) {
   if (is_string(\php\SYS_VERSION)) {
      echo "\php\SYS_VERSION is string\n";
   }
}

if(defined("\php\io\IO_TYPE")) {
   if (is_string(\php\io\IO_TYPE)) {
      echo "\php\io\IO_TYPE is string\n";
   }
}

if(defined("\php\io\NATIVE_STREAM")) {
   if (is_bool(\php\io\NATIVE_STREAM)) {
      echo "\php\io\NATIVE_STREAM is bool\n";
   }
}

// CHECK: MY_CONST is int
// CHECK: PI is double
// CHECK: POLAR_NAME is string
// CHECK: POLAR_VERSION is string
// CHECK: POLAR_FOUNDATION is string
// CHECK: \php\SYS_VERSION is string
// CHECK: \php\io\IO_TYPE is string
// CHECK: \php\io\NATIVE_STREAM is bool

