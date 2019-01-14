<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("Person")) {
    if (defined("Person::POLARPHP_TEAM") && is_string(Person::POLARPHP_TEAM)) {
       echo "Person::POLARPHP_TEAM is string\n";
    }
    if (defined("Person::MY_CONST") && is_string(Person::MY_CONST)) {
       echo "Person::MY_CONST is string\n";
    }
    if (defined("Person::PI") && is_double(Person::PI)) {
       echo "Person::PI is double\n";
    }
    if (defined("Person::HEADER_SIZE") && is_int(Person::HEADER_SIZE)) {
       echo "Person::HEADER_SIZE is int\n";
    }
    if (defined("Person::ALLOW_ACL") && is_bool(Person::ALLOW_ACL)) {
       echo "Person::ALLOW_ACL is bool\n";
    }
 }

// CHECK: Person::POLARPHP_TEAM is string
// CHECK: Person::MY_CONST is string
// CHECK: Person::PI is double
// CHECK: Person::HEADER_SIZE is int
// CHECK: Person::ALLOW_ACL is bool