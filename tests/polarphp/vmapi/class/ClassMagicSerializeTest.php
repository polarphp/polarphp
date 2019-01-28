<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("\MagicMethodClass")) {
    $object = new \MagicMethodClass();
    $str = serialize($object);
    echo $str."\n";
    unserialize($str);
}

// CHECK: MagicMethodClass::serialize is called
// CHECK: C:16:"MagicMethodClass":14:{serialize data}
// CHECK: MagicMethodClass::unserialize is called
// CHECK: serialize data : serialize data}