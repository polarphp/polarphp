<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("\IterateTestClass")) {
    $data = new IterateTestClass();
    foreach ($data as $key => $value) {
        echo "key: $key value: $value\n";
    }
}

// CHECK: IterateTestClass::rewind called
// CHECK: IterateTestClass::valid called
// CHECK: IterateTestClass::current called
// CHECK: IterateTestClass::key called
// CHECK: key: key1 value: value1
// CHECK: IterateTestClass::next called
// CHECK: IterateTestClass::valid called
// CHECK: IterateTestClass::current called
// CHECK: IterateTestClass::key called
// CHECK: key: key2 value: value2
// CHECK: IterateTestClass::next called
// CHECK: IterateTestClass::valid called
// CHECK: IterateTestClass::current called
// CHECK: IterateTestClass::key called
// CHECK: key: key3 value: value3
// CHECK: IterateTestClass::next called
// CHECK: IterateTestClass::valid called
// CHECK: IterateTestClass::current called
// CHECK: IterateTestClass::key called
// CHECK: key: key4 value: value4
// CHECK: IterateTestClass::next called
// CHECK: IterateTestClass::valid called