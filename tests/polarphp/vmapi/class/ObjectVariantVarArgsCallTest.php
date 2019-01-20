<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("ObjectVariantClass")) {
    $object = new ObjectVariantClass();
    $object->testVarArgsCall();
}

// CHECK: ObjectVariantClass::printSum been called
// CHECK: got 3 args
// CHECK: the result is 36
// CHECK: ObjectVariantClass::calculateSum been called
// CHECK: got 3 args
// CHECK: the result is 7
// CHECK: the result of ObjectVariantClass::calculateSum is 7
// CHECK: before call by ref arg polarphp
// CHECK: ObjectVariantClass::changeNameByRef been called
// CHECK: get ref arg
// CHECK: after call by ref arg hello, polarphp