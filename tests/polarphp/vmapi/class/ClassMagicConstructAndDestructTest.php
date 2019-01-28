<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("\ConstructAndDestruct")) {
    {
        $object = new \ConstructAndDestruct();
        $object = null;
    }
}
// CHECK: constructor been invoked
// CHECK: destructor been invoked