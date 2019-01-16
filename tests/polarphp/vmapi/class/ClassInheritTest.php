<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("A") && class_exists("B") && class_exists("C")) {
    echo "class A and class B and class C exist\n";
    if (new B instanceof \A) {
       echo "object of class B is instance of A\n"; 
    }
    $reflClsB = new ReflectionClass("B");
    $parentOfClsB = $reflClsB->getParentClass();
    if ($parentOfClsB->getName() == "A") {
        echo "The parent class of class B is class A\n";
    }
    $reflClsC = new ReflectionClass("C");
    $parentOfClsC = $reflClsC->getParentClass();
    if ($parentOfClsC->getName() == "B") {
        echo "The parent class of class C is class B\n";
    }
    if (new C instanceof \B) {
       echo "object of class C is instance of B\n"; 
    }
    if (new C instanceof \A) {
       echo "object of class C is instance of A\n"; 
    }
}

// CHECK: class A and class B and class C exist
// CHECK: object of class B is instance of A
// CHECK: The parent class of class B is class A
// CHECK: The parent class of class C is class B
// CHECK: object of class C is instance of B
// CHECK: object of class C is instance of A