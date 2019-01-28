<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if(defined("MY_CONST")) {
   echo "MY_CONST: ". MY_CONST . "\n";
}

if(defined("PI")) {
   echo "PI: " . PI . "\n";
}

if(defined("POLAR_NAME")) {
   echo "POLAR_NAME: " . POLAR_NAME . "\n";
}

if(defined("POLAR_VERSION")) {
   echo "POLAR_NAME: " . POLAR_VERSION . "\n";
}

if(defined("POLAR_FOUNDATION")) {
   echo "POLAR_FOUNDATION: " . POLAR_FOUNDATION . "\n";
}

// CHECK: MY_CONST: 12333
// CHECK: PI: 3.14
// CHECK: POLAR_NAME: polarphp
// CHECK: POLAR_NAME: v0.0.1
// CHECK: POLAR_FOUNDATION: polar foundation

