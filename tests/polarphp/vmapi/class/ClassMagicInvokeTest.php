<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("\NonMagicMethodClass") && class_exists("\MagicMethodClass")) {
    //$nonMagicMethodObject = new NonMagicMethodClass();
    // $nonMagicMethodObject(1, 2, 3); // fata error
    $magicMethodClass = new MagicMethodClass();
    $sum = $magicMethodClass();
    echo "the sum of \$magicMethodClass() is " . $sum ."\n";
    $sum = $magicMethodClass(1, 2, 3, 4);
    echo "the sum of \$magicMethodClass() is " . $sum ."\n";
}
// CHECK: MagicMethodClass::__invoke is called
// CHECK: the sum of $magicMethodClass() is 0
// CHECK: MagicMethodClass::__invoke is called
// CHECK: the sum of $magicMethodClass() is 10