<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("C") && interface_exists("InterfaceA") && interface_exists("InterfaceB") && interface_exists("InterfaceC")) {
    $obj = new C();
    var_dump(class_implements("C"));
    if ($obj instanceof InterfaceA) {
        echo "\$obj instanceof InterfaceA\n";
    }
    if (method_exists($obj, "methodOfA")) {
       echo "method C::methodOfA exists\n";
    }
    if (method_exists($obj, "protectedMethodOfA")) {
       echo "method C::protectedMethodOfA exists\n";
    }
    if (method_exists($obj, "privateMethodOfA")) {
       echo "method C::privateMethodOfA exists\n";
    }
}