<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("Person")) {
    echo "class \Person exist\n";
}
if (class_exists("\php\EmptyClass")) {
    echo "class \php\EmptyClass exist\n";
}
if (class_exists("ConstructAndDestruct")) {
    echo "class \ConstructAndDestruct exist\n";
}
if (!class_exists("NotExistClass")) {
    echo "class \NotExistClass is not exist\n";
}
// CHECK: class \Person exist
// CHECK: class \php\EmptyClass exist
// CHECK: class \ConstructAndDestruct exist
// CHECK: class \NotExistClass is not exist
