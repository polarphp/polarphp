<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
// here we test argument passed
if (function_exists("print_name")) {
    print_name("polarphp team");
    echo "\n";
    print_name(3.14);
    echo "\n";
    print_name(true);
 }
 echo "\n";
 if (function_exists("\php\io\print_name")) {
    \php\io\print_name("hello, zapi");
    echo "\n";
    \php\io\print_name(4.16);
    echo "\n";
    \php\io\print_name(false);
 }

// CHECK: polarphp team
// CHECK: 3.14
// CHECK: 1
// CHECK: hello, zapi
// CHECK: 4.16
