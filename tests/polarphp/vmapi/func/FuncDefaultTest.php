<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
// here we just invoke a function without params and return value

if (function_exists("show_something")) {
    show_something();
 }
 echo "\n";
 if (function_exists("\php\show_something")) {
    \php\show_something();
 }
 echo "\n";
 if (function_exists("\php\io\show_something")) {
    \php\io\show_something();
 }

// CHECK: hello world, polarphp
// CHECK: hello world, polarphp
// CHECK: hello world, polarphp
