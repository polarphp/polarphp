<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("\NonMagicMethodClass") && class_exists("\MagicMethodClass")) {
    $nonMagicMethodObject = new NonMagicMethodClass();
    // $nonMagicMethodObject->teamName; Notice: Undefined property: NonMagicMethodClass::$teamName
    if (!property_exists($nonMagicMethodObject, "teamName")) {
        echo "\$nonMagicMethodObject->teamName is not exist\n";
    }
    $magicMethodObject = new MagicMethodClass();
    if (property_exists($magicMethodObject, "teamName")) {
        echo "\$magicMethodObject->teamName is exist\n";
    }
    unset($magicMethodObject->teamName);
    if (!property_exists($magicMethodObject, "teamName")) {
        echo "\$magicMethodObject->teamName is not exist\n";
    }
}