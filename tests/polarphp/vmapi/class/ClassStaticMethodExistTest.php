<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("Person")) {
   if (method_exists("Person", "staticShowName")) {
       echo "static method Person::staticShowName exists\n";
   }
   if (method_exists("Person", "staticProtectedMethod")) {
       echo "static method Person::staticProtectedMethod exists\n";
   }
   if (method_exists("Person", "staticPrivateMethod")) {
       echo "static method Person::staticPrivateMethod exists\n";
   }
}

// CHECK: static method Person::staticShowName exists
// CHECK: static method Person::staticProtectedMethod exists
// CHECK: static method Person::staticPrivateMethod exists