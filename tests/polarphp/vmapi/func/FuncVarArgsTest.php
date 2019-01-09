<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
// here we test simple return string value

if (function_exists("\php\io\print_sum")) {
    \php\io\print_sum(1, 2);
    echo "\n";
    \php\io\print_sum(1, 2, 3, 4, 5, 6, 7, 8, 10);
    echo "\n";
    \php\io\print_sum(123, 321);
    echo "\n";
}
if (function_exists("\php\io\calculate_sum")) {
    echo \php\io\calculate_sum(1, 2);
    echo "\n";
    echo \php\io\calculate_sum(1, 2, 3, 4, 5, 6, 7, 8, 10);
    echo "\n";
    echo \php\io\calculate_sum(123, 321);
}

// CHECK: 3
// CHECK: 46
// CHECK: 444
// CHECK: 3
// CHECK: 46
// CHECK: 444