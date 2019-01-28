<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (!interface_exists("InterfaceA") || !interface_exists("InterfaceB") || !interface_exists("InterfaceC")) {
    echo "fail";
} else {
    echo "success";
}
// CHECK: success
