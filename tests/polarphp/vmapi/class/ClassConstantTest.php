<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("Person")) {
   if (defined("Person::POLARPHP_TEAM")) {
      echo "Person::POLARPHP_TEAM is ".Person::POLARPHP_TEAM."\n";
   }
   if (defined("Person::MY_CONST")) {
      echo "Person::MY_CONST is ".Person::MY_CONST."\n";
   }
   if (defined("Person::PI")) {
      echo "Person::PI is ".Person::PI."\n";
   }
   if (defined("Person::HEADER_SIZE")) {
      echo "Person::HEADER_SIZE is ".Person::HEADER_SIZE."\n";
   }
   if (defined("Person::ALLOW_ACL")) {
      echo "Person::ALLOW_ACL is ".(Person::ALLOW_ACL ? "true": "false")."\n";
   }
}

// CHECK: Person::POLARPHP_TEAM is beijing polarphp team
// CHECK: Person::MY_CONST is MY_CONST_VALUE
// CHECK: Person::PI is 3.1415926
// CHECK: Person::HEADER_SIZE is 123
// CHECK: Person::ALLOW_ACL is true
