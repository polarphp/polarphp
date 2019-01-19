<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("\VmApiClosure")) {
    echo "internal class VmApiClosure exists\n";
    $refl = new ReflectionClass("VmApiClosure");
    if ($refl->isFinal()) {
        echo "class VmApiClosure is final\n";
    }
}
if (class_exists("\ClosureTestClass")) {
    echo "internal class ClosureTestClass exists\n";
    $obj = new \ClosureTestClass();
    $obj->testClosureCallable();
    $callable = $obj->getNoArgAndReturnCallable();
    if (is_callable($callable)) {
        echo "\$callable is callable\n";
        $ret = $callable();
        echo "the return of callable is " . $ret."\n";
    }
    $hasParamCallable = $obj->getArgAndReturnCallable();
    if (is_callable($hasParamCallable)) {
        echo "\$hasParamCallable is callable\n";
        $ret = $hasParamCallable();
        echo "the return of hasParamCallable is " . $ret."\n";
        $ret = $hasParamCallable(123);
        echo "the return of hasParamCallable is " . $ret."\n";
        $ret = $hasParamCallable(3.14);
        echo "the return of hasParamCallable is " . $ret."\n";
        $ret = $hasParamCallable(true);
        echo "the return of hasParamCallable is " . $ret."\n";
    }
}

// CHECK: internal class VmApiClosure exists
// CHECK: class VmApiClosure is final
// CHECK: internal class ClosureTestClass exists
// CHECK: $callable is callable
// CHECK: print_something called
// CHECK: the return of callable is print_some
// CHECK: $hasParamCallable is callable
// CHECK: have_ret_and_have_arg called
// CHECK: the return of hasParamCallable is have_ret_and_have_arg
// CHECK: have_ret_and_have_arg called
// CHECK: the return of hasParamCallable is 123
// CHECK: have_ret_and_have_arg called
// CHECK: the return of hasParamCallable is 3.14
// CHECK: have_ret_and_have_arg called
// CHECK: the return of hasParamCallable is 1