<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("PropsTestClass")) {
    $object = new PropsTestClass();
    if (property_exists($object, "name")) {
        echo "PropsTestClass::name is exist\n";
        echo "PropsTestClass::name value : {$object->name}\n";
    }
    if (property_exists($object, "name")) {
        $object->name = "unicornteam";
        echo "PropsTestClass::name value : {$object->name}\n";
    }
    if (!property_exists($object, "notExistsProp")) {
        echo "PropsTestClass::notExistsProp is not exist\n";
    }
    $object->notExistsProp = 123;
    if (property_exists($object, "notExistsProp")) {
        echo "PropsTestClass::notExistsProp is exist\n";
        echo "PropsTestClass::notExistsProp value : {$object->notExistsProp}\n";
    }
}

// CHECK: PropsTestClass::name is exist
// CHECK: PropsTestClass::name value : 
// CHECK: PropsTestClass::name value : polarphp:unicornteam
// CHECK: PropsTestClass::notExistsProp is not exist
// CHECK: PropsTestClass::notExistsProp is exist
// CHECK: PropsTestClass::notExistsProp value : 123
