<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
function print_info(A $obj)
{
    $obj->printInfo();
}

if (class_exists("A") && class_exists("B") && class_exists("C")) {
    echo "class A and class B and class C exist\n";
    print_info(new A);
    print_info(new B);
    print_info(new C);
}

// CHECK: class A and class B and class C exist
// CHECK: A::printInfo been called
// CHECK: B::printInfo been called
// CHECK: C::printInfo been called
// CHECK: B::printInfo been called
// CHECK: B::showSomething been called