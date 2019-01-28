<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("Person")) {
    $p = new Person; 
    if (method_exists($p, "showName")) {
        echo "method Person::showName exists\n";
    }
    if (method_exists($p, "print_sum")) {
        echo "method Person::print_sum exists\n";
    }
    if (method_exists($p, "setAge")) {
        echo "method Person::setAge exists\n";
    }
    if (method_exists($p, "addTwoNum")) {
        echo "method Person::addTwoNum exists\n";
    }
    if (method_exists($p, "addSum")) {
        echo "method Person::addSum exists\n";
    }
}

// CHECK: method Person::showName exists
// CHECK: method Person::print_sum exists
// CHECK: method Person::setAge exists
// CHECK: method Person::addTwoNum exists
// CHECK: method Person::addSum exists
