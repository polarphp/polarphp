<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("ObjectVariantClass")) {
    $object = new ObjectVariantClass();
    $object->testInstanceOf();
}

// CHECK: A is instance of A
// CHECK: B is instance of B
// CHECK: C is instance of C
// CHECK: B is instance of A
// CHECK: C is instance of B
// CHECK: C is instance of A
// CHECK: A is not instance of B
// CHECK: C is not instance of B
// CHECK: C is not instance of A