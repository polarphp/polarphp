<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("ObjectVariantClass")) {
    $object = new ObjectVariantClass();
    $object->testNoArgCall();
}

// CHECK: ObjectVariantClass::printName been called
// CHECK: ObjectVariantClass::getName been called
// CHECK: the result of ObjectVariantClass::getName is hello, polarphp