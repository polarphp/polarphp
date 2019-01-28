<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("A") && class_exists("B") && class_exists("C")) {
    echo "class A and class B and class C exist\n";
    $obj = new C();
    $obj->testCallParentPassRefArg();
}

// CHECK: class A and class B and class C exist
// CHECK: C::testCallParentPassRefArg been called
// CHECK: before call changeNameByRef : xxxx
// CHECK: A::changeNameByRef been called
// CHECK: get ref arg
// CHECK: after call changeNameByRef : hello, polarphp
// CHECK: before call calculateSumByRef : 0
// CHECK: C::calculateSumByRef been called
// CHECK: got 4 args
// CHECK: retval is reference arg
// CHECK: after call calculateSumByRef : 47