<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("\NonMagicMethodClass") && class_exists("\MagicMethodClass")) {
    // $NonMagicMethodClass::staticCalculateSum(12, 24, 48); // fata error
    $sum = MagicMethodClass::staticCalculateSum(12, 24, 48);
    echo "the sum is " . $sum ."\n";
    $ret = MagicMethodClass::notProcessCase("polarphp");
    if (!is_null($ret)) {
       echo "MagicMethodClass::notProcessCase('polarphp') return $ret\n";
    }
}

// CHECK: MagicMethodClass::__callStatic is called
// CHECK: the sum is 84
// CHECK: MagicMethodClass::__callStatic is called
// CHECK: MagicMethodClass::notProcessCase('polarphp') return hello, polarphp