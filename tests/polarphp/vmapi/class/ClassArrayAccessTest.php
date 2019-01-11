<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("\IterateTestClass")) {
    echo "class IterateTestClass exists\n";
    $data = new IterateTestClass();
    if (isset($data["key1"])) {
        echo "key key1 is exists\n";
        echo "the value of item key1 is : ".$data["key1"]."\n";
    }
    if (isset($data["key2"])) {
        echo "key key2 is exists\n";
        echo "the value of item key2 is : ".$data["key2"]."\n";
    }
    if (isset($data["key3"])) {
        echo "key key3 is exists\n";
        echo "the value of item key3 is : ".$data["key3"]."\n";
    }
    // test unset
    echo "now \$data item count is " . count($data)."\n";
    unset($data["key1"]);
    if (!isset($data["key1"])) {
        echo "key key1 is not exists\n";
    }
    echo "now \$data item count is " . count($data)."\n";
}