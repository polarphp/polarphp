<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("\FinalTestClass")) {
    echo "FinalTestClass exists\n";
    $refl = new ReflectionClass("FinalTestClass");
    if ($refl->isFinal()) {
        echo "class FinalTestClass is final\n";
    }
}
if (class_exists("\VisibilityClass")) {
    echo "VisibilityClass exists\n";
    $refl = new ReflectionClass("VisibilityClass");
    $mrefl = $refl->getMethod("finalMethod");
    if ($mrefl->isFinal()) {
        echo "method VisibilityClass::finalMethod is final\n";
    }
}
// class xx extends FinalTestClass
// {}
// // PHP Fatal error:  Class xx may not inherit from final class (FinalTestClass)

// CHECK: FinalTestClass exists
// CHECK: class FinalTestClass is final
// CHECK: VisibilityClass exists
// CHECK: method VisibilityClass::finalMethod is final
