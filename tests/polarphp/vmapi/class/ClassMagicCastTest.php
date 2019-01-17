<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("\MagicMethodClass")) {
    $object = new \MagicMethodClass();
    echo "cast to string : " . $object;
    echo "\n";
    echo "cast to integer : " . intval($object);
    echo "\n";
    echo "cast to integer : " . (int)$object;
    echo "\n";
    echo "cast to double : " . (double)$object;
    echo "\n";
    echo "cast to double : " . doubleval($object);
    echo "\n";
    echo "cast to boolean : " . boolval($object);
    echo "\n";
    echo "cast to boolean : " . (bool)$object;
    echo "\n";
}

// CHECK: MagicMethodClass::__toString is called
// CHECK: cast to string : hello, polarphp
// CHECK: MagicMethodClass::__toInteger is called
// CHECK: cast to integer : 2017
// CHECK: MagicMethodClass::__toInteger is called
// CHECK: cast to integer : 2017
// CHECK: MagicMethodClass::__toDouble is called
// CHECK: cast to double : 3.14
// CHECK: MagicMethodClass::__toDouble is called
// CHECK: cast to double : 3.14
// CHECK: MagicMethodClass::__toBool is called
// CHECK: cast to boolean : 1
// CHECK: MagicMethodClass::__toBool is called
// CHECK: cast to boolean : 1