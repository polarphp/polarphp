<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("\MagicMethodClass")) {
    echo "class MagicMethodClass exists\n";
    $object = new \MagicMethodClass();
    var_dump($object);
    // have no __debuginfo()
    $anotherObj = new \FinalTestClass();
    var_dump($anotherObj);
}
// CHECK: class MagicMethodClass exists
// CHECK: object(MagicMethodClass)#1 (2) {
// CHECK:   ["name"]=>
// CHECK:   string(8) "polarphp"
// CHECK:   ["address"]=>
// CHECK:   string(7) "beijing"
// CHECK: }
// CHECK: object(FinalTestClass)#2 (0) {
// CHECK: }
