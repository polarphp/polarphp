<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if(defined("\php\SYS_VERSION")) {
   echo "\php\SYS_VERSION: " . \php\SYS_VERSION . "\n";
}
if(defined("\php\io\IO_TYPE")) {
   echo "\php\io\IO_TYPE: " . \php\io\IO_TYPE ."\n";
}

if(defined("\php\io\NATIVE_STREAM")) {
   echo "\php\io\NATIVE_STREAM: " . \php\io\NATIVE_STREAM . "\n";
}

// CHECK: \php\SYS_VERSION: 0.1.1-alpha
// CHECK: \php\io\IO_TYPE: ASYNC
// CHECK: \php\io\NATIVE_STREAM: 1
