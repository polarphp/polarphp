<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("\NonMagicMethodClass") && class_exists("\MagicMethodClass")) {
    $nonMagicMethodObject = new NonMagicMethodClass();
    // $nonMagicMethodObject->calculateSum(1, 2, 3); // fata error
    $magicMethodClass = new MagicMethodClass();
    $sum = $magicMethodClass->calculateSum(1, 2, 4);
    echo "the sum is " . $sum ."\n";
    $ret = $magicMethodClass->notProcessCase("zapi");
    if (is_null($ret)) {
        echo "magicMethodClass notProcessCase('zapi') return null\n";
    }
}
// CHECK: MagicMethodClass::__call is called
// CHECK: the sum is 7
// CHECK: MagicMethodClass::__call is called
// CHECK: magicMethodClass notProcessCase('zapi') return null