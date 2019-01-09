<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (function_exists("show_something")) {
    echo "show_something exist\n";
 }
 if (function_exists("get_name")) {
    echo "get_name exist\n";
 }
 if (function_exists("print_name")) {
    echo "print_name exist\n";
 }
 if (function_exists("print_name_and_age")) {
    echo "print_name_and_age exist\n";
 }
 if (function_exists("add_two_number")) {
    echo "add_two_number exist\n";
 }
 // test function in namespace
 if (function_exists("\php\get_name")) {
    echo "\php\get_name exist\n";
 }
 if (function_exists("\php\io\print_name")) {
    echo "\php\io\print_name exist\n";
 }
 if (function_exists("\php\io\show_something")) {
    echo "\php\io\show_something exist\n";
 }

// CHECK: show_something exist
// CHECK: get_name exist
// CHECK: print_name exist
// CHECK: print_name_and_age exist
// CHECK: add_two_number exist
// CHECK: \php\get_name exist
// CHECK: \php\io\print_name exist
// CHECK: \php\io\show_something exist
