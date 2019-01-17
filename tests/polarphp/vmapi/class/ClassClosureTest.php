<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("\ZapiClosure")) {
    echo "internal class ZapiClosure exists\n";
    $refl = new ReflectionClass("ZapiClosure");
    if ($refl->isFinal()) {
        echo "class ZapiClosure is final\n";
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
