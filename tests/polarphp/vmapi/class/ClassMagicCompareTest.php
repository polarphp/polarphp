<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("\MagicMethodClass")) {
    $lhs = new MagicMethodClass();
    $rhs = new MagicMethodClass();
    if ($lhs == $rhs) {
        echo "\$lhs == \$rhs\n";
    }
    if ($lhs <= $rhs) {
        echo "\$lhs <= \$rhs\n";
    }
    if ($lhs >= $rhs) {
        echo "\$lhs >= \$rhs\n";
    }
    if ($lhs > $rhs) {
        echo "\$lhs > \$rhs\n";
    }
    if ($lhs < $rhs) {
        echo "\$lhs < \$rhs\n";
    }
    if ($lhs != $rhs) {
        echo "\$lhs != \$rhs\n";
    }
    // assign value
    $lhs->length = 1;
    $rhs->length = 2;
    echo "-------\n";
    if ($lhs == $rhs) {
        echo "\$lhs == \$rhs\n";
    }
    if ($lhs <= $rhs) {
        echo "\$lhs <= \$rhs\n";
    }
    if ($lhs >= $rhs) {
        echo "\$lhs >= \$rhs\n";
    }
    if ($lhs > $rhs) {
        echo "\$lhs > \$rhs\n";
    }
    if ($lhs < $rhs) {
        echo "\$lhs < \$rhs\n";
    }
    if ($lhs != $rhs) {
        echo "\$lhs != \$rhs\n";
    }
    
    $lhs->length = 2;
    $rhs->length = 1;
    echo "-------\n";
    if ($lhs == $rhs) {
        echo "\$lhs == \$rhs\n";
    }
    if ($lhs <= $rhs) {
        echo "\$lhs <= \$rhs\n";
    }
    if ($lhs >= $rhs) {
        echo "\$lhs >= \$rhs\n";
    }
    if ($lhs > $rhs) {
        echo "\$lhs > \$rhs\n";
    }
    if ($lhs < $rhs) {
        echo "\$lhs < \$rhs\n";
    }
    if ($lhs != $rhs) {
        echo "\$lhs != \$rhs\n";
    }
}

// CHECK: MagicMethodClass::__compare is called
// CHECK: $lhs == $rhs
// CHECK: MagicMethodClass::__compare is called
// CHECK: $lhs <= $rhs
// CHECK: MagicMethodClass::__compare is called
// CHECK: $lhs >= $rhs
// CHECK: MagicMethodClass::__compare is called
// CHECK: MagicMethodClass::__compare is called
// CHECK: MagicMethodClass::__compare is called
// CHECK: MagicMethodClass::__set is called
// CHECK: MagicMethodClass::__set is called
// CHECK: -------
// CHECK: MagicMethodClass::__compare is called
// CHECK: MagicMethodClass::__compare is called
// CHECK: $lhs <= $rhs
// CHECK: MagicMethodClass::__compare is called
// CHECK: MagicMethodClass::__compare is called
// CHECK: MagicMethodClass::__compare is called
// CHECK: $lhs < $rhs
// CHECK: MagicMethodClass::__compare is called
// CHECK: $lhs != $rhs
// CHECK: MagicMethodClass::__set is called
// CHECK: MagicMethodClass::__set is called
// CHECK: -------
// CHECK: MagicMethodClass::__compare is called
// CHECK: MagicMethodClass::__compare is called
// CHECK: MagicMethodClass::__compare is called
// CHECK: $lhs >= $rhs
// CHECK: MagicMethodClass::__compare is called
// CHECK: $lhs > $rhs
// CHECK: MagicMethodClass::__compare is called
// CHECK: MagicMethodClass::__compare is called
// CHECK: $lhs != $rhs