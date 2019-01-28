<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("\NonMagicMethodClass") && class_exists("\MagicMethodClass")) {
    $magicMethodObject = new MagicMethodClass();
    if (!property_exists($magicMethodObject, "address")) {
        echo "\$magicMethodObject->address is not exist\n";
    }
    $magicMethodObject->address = "beijing";
    if (property_exists($magicMethodObject, "address")) {
        echo "\$magicMethodObject->address is exist\n";
        echo "\$magicMethodObject->address value is {$magicMethodObject->address}\n";
    }
}
// CHECK: MagicMethodClass::__isset is called
// CHECK: $magicMethodObject->address is not exist
// CHECK: MagicMethodClass::__set is called
// CHECK: MagicMethodClass::__isset is called
// CHECK: $magicMethodObject->address is exist
// CHECK: MagicMethodClass::__get is called
// CHECK: $magicMethodObject->address value is beijing
