<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("ObjectVariantClass")) {
    $object = new ObjectVariantClass();
    $object->forwardInvoke();
}

// CHECK: begin invoke ObjectVariant::classInvoke : the text is xxx
// CHECK: ObjectVariantClass::__invoke invoked
// CHECK: after invoke ObjectVariant::classInvoke : this text is polarphp