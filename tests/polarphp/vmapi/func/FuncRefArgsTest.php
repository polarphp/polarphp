<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (function_exists("get_value_ref")) {
    $num = 123;
    get_value_ref($num);
    echo $num;
}
echo "\n";
if (function_exists("passby_value")) {
    $num = 123;
    passby_value($num);
    echo $num;
}

// CHECK: 321
// CHECK: 123