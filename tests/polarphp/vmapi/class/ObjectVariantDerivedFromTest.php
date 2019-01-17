<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("ObjectVariantClass")) {
    $object = new ObjectVariantClass();
    $object->testDerivedFrom();
}

// CHECK: A is not derived from A
// CHECK: B is not derived from B
// CHECK: C is not derived from C
// CHECK: B is derived from A
// CHECK: C is derived from B
// CHECK: C is derived from A
// CHECK: A is not derived from B
// CHECK: C is not derived from B
// CHECK: C is not derived from A