<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("Person")) {
    if (method_exists("Person", "protectedMethod")) {
        $pmRefl = new ReflectionMethod("Person", "protectedMethod");
        if (!$pmRefl->isStatic()) {
            echo "Person::protectedMethod is an instance method\n";
        }
        if ($pmRefl->isProtected()) {
            echo "Person::protectedMethod is an protected method\n";
        }
    }
    if (method_exists("Person", "privateMethod")) {
        $pmRefl = new ReflectionMethod("Person", "privateMethod");
        if (!$pmRefl->isStatic()) {
            echo "Person::privateMethod is an instance method\n";
        }
        if ($pmRefl->isPrivate()) {
            echo "Person::privateMethod is an private method\n";
        }
    }
    
    if (method_exists("Person", "staticProtectedMethod")) {
        $pmRefl = new ReflectionMethod("Person", "staticProtectedMethod");
        if ($pmRefl->isStatic()) {
            echo "Person::staticProtectedMethod is an static method\n";
        }
        if ($pmRefl->isProtected()) {
            echo "Person::staticProtectedMethod is an protected method\n";
        }
    }
    if (method_exists("Person", "staticPrivateMethod")) {
        $pmRefl = new ReflectionMethod("Person", "staticPrivateMethod");
        if ($pmRefl->isStatic()) {
            echo "Person::staticPrivateMethod is an static method\n";
        }
        if ($pmRefl->isPrivate()) {
            echo "Person::staticPrivateMethod is an private method\n";
        }
    }
}

// CHECK: Person::protectedMethod is an instance method
// CHECK: Person::protectedMethod is an protected method
// CHECK: Person::privateMethod is an instance method
// CHECK: Person::privateMethod is an private method
// CHECK: Person::staticProtectedMethod is an static method
// CHECK: Person::staticProtectedMethod is an protected method
// CHECK: Person::staticPrivateMethod is an static method
// CHECK: Person::staticPrivateMethod is an private method