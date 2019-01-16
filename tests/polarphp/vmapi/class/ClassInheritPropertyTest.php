<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("A") && class_exists("B") && class_exists("C")) {
    echo "class A and class B and class C exist\n";
    $obj = new C();
    echo "the public property name is : " . $obj->name ."\n";
    if (property_exists($obj, "protectedName")) {
        echo "property protectedName exists\n";
    }
    if (!property_exists($obj, "privateName")) {
        echo "property privateName not exists\n";
    }
    if (property_exists($obj, "propsFromB")) {
        echo "property propsFromB exists\n";
    }
    
    // $obj->protectedName; fata error
}

// CHECK: class A and class B and class C exist
// CHECK: the public property name is : polarphp
// CHECK: property protectedName exists
// CHECK: property privateName not exists
// CHECK: property propsFromB exists