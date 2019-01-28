<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("\IterateTestClass")) {
    $data = new IterateTestClass();
    echo "the count of \$data is ".count($data)."\n";
}

// CHECK: IterateTestClass::count called
// CHECK: the count of $data is 4