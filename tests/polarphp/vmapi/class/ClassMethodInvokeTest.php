<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("\Person")) {
    Person::makeNewPerson();
    $object = new \Person();
    if (method_exists($object, "showName")) {
        $object->showName();
    }
    if (method_exists($object, "print_sum")) {
        $object->print_sum(1, 2, 3, 5);
    }
    if (method_exists($object, "getAge")) {
        echo "the original age is " . $object->getAge()."\n";
    }
    if (method_exists($object, "setAge")) {
        $object->setAge(27);
    }
    if (method_exists($object, "getAge")) {
        echo "the new age is " . $object->getAge()."\n";
    }
    if (method_exists($object, "getName")) {
        echo "the name is " . $object->getName()."\n";
    }
    if (method_exists($object, "addTwoNum")) {
        echo "the sum of 1 and 99 is " . $object->addTwoNum(1, 99)."\n";
    }
    if (method_exists($object, "addSum")) {
        echo "the sum of 1, 2, 3, 4, 5, 6, 7 is " . $object->addSum(1, 2, 3, 4, 5, 6, 7)."\n";
        echo "the sum of 1, 2, 3 is " . $object->addSum(1, 2, 3)."\n";
    }
    // static method invoke
    if (method_exists("Person", "staticShowName")) {
        Person::staticShowName();
    }

    if (method_exists("Person", "concatStr")) {
        echo "the concat of abc and def is " . Person::concatStr("abc", "def");
    }
}

// CHECK: my name is polarphp
// CHECK: the sum is 11
// CHECK: the original age is 0
// CHECK: the new age is 27
// CHECK: the name is zzu_softboy
// CHECK: the sum of 1 and 99 is 100
// CHECK: the sum of 1, 2, 3, 4, 5, 6, 7 is 28
// CHECK: the sum of 1, 2, 3 is 6
// CHECK: static my name is polarphp
// CHECK: the concat of abc and def is abcdef