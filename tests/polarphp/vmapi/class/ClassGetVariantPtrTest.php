<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("A") && class_exists("B") && class_exists("C")) {
    echo "class A and class B and class C exist\n";
    $obj = new C();
    $obj->testGetObjectVaraintPtr();
}