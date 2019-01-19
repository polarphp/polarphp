<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("A") && class_exists("B") && class_exists("C")) {
    echo "class A and class B and class C exist\n";
    $obj = new C();
    $obj->testGetObjectVaraintPtr();
}

// CHECK: class A and class B and class C exist
// CHECK: C::testGetObjectVaraintPtr been called
// CHECK: property C::address exists
// CHECK: property value : beijing
// CHECK: property C::privateName not exists
// CHECK: property C::protectedName exists
// CHECK: property value : protected polarphp
// CHECK: method C::showSomething exists
// CHECK: B::showSomething been called
// CHECK: method C::privateCMethod exists
// CHECK: C::privateCMethod been called
// CHECK: method C::privateCMethod exists
// CHECK: method C::protectedAMethod exists
// CHECK: A::protectedAMethod been called
// CHECK: method C::privateBMethod exists
// CHECK: method C::protectedBMethod exists
// CHECK: B::protectedBMethod been called
// CHECK: A::protectedAMethod been called