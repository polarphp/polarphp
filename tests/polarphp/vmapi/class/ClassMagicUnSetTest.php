<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("\NonMagicMethodClass") && class_exists("\MagicMethodClass")) {
    $nonMagicMethodObject = new NonMagicMethodClass();
    // $nonMagicMethodObject->teamName; Notice: Undefined property: NonMagicMethodClass::$teamName
    if (!property_exists($nonMagicMethodObject, "teamName")) {
        echo "\$nonMagicMethodObject->teamName is not exist\n";
    }
    /// echo nothing
    unset($nonMagicMethodObject->notExist);
    $magicMethodObject = new MagicMethodClass();
    if (property_exists($magicMethodObject, "teamName")) {
        echo "\$magicMethodObject->teamName is exist\n";
    }
    unset($magicMethodObject->teamName);
    if (!property_exists($magicMethodObject, "teamName")) {
        echo "\$magicMethodObject->teamName is not exist\n";
    }
    /// dynamic property
    unset($magicMethodObject->notExist);
    ///  Fatal error: Property address can not be unset
    // unset($magicMethodObject->address);
}

// CHECK: $nonMagicMethodObject->teamName is not exist
// CHECK: MagicMethodClass::__isset is called
// CHECK: $magicMethodObject->teamName is exist
// CHECK: MagicMethodClass::__unset is called
// CHECK: MagicMethodClass::__isset is called
// CHECK: $magicMethodObject->teamName is not exist
// CHECK: MagicMethodClass::__unset is called
