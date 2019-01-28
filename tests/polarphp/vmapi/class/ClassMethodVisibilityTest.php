<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (class_exists("\VisibilityClass")) {
    echo "class VisibilityClass exists\n";
    $refl = new ReflectionClass("VisibilityClass");
    $publicMethod = $refl->getMethod("publicMethod");
    $protectedMethod = $refl->getMethod("protectedMethod");
    $privateMethod = $refl->getMethod("privateMethod");
    if ($publicMethod->isPublic()) {
        echo  "VisibilityClass::publicMethod is public\n";
    }
    if ($protectedMethod->isProtected()) {
        echo  "VisibilityClass::protectedMethod is protected\n";
    }
    if ($privateMethod->isPrivate()) {
        echo  "VisibilityClass::privateMethod is private\n";
    }
}

// CHECK: class VisibilityClass exists
// CHECK: VisibilityClass::publicMethod is public
// CHECK: VisibilityClass::protectedMethod is protected
// CHECK: VisibilityClass::privateMethod is private