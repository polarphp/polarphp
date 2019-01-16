<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("\MagicMethodClass")) {
    $object = new \MagicMethodClass();
    $object1 = clone $object;
    var_dump($object);
    var_dump($object1);
}

// CHECK: MagicMethodClass::__clone is called
// CHECK: object(MagicMethodClass)#1 (2) {
// CHECK:   ["name"]=>
// CHECK:   string(8) "polarphp"
// CHECK:  ["address"]=>
// CHECK:   string(7) "beijing"
// CHECK: }
// CHECK: object(MagicMethodClass)#2 (2) {
// CHECK:   ["name"]=>
// CHECK:   string(8) "polarphp"
// CHECK:   ["address"]=>
// CHECK:   string(7) "beijing"
// CHECK: }
