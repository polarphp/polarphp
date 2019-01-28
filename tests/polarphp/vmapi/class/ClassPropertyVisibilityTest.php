<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("\VisibilityClass")) {
    echo "class VisibilityClass exists\n";
    $refl = new ReflectionClass("VisibilityClass");
    $publicProp = $refl->getProperty("publicProp");
    $protectedProp = $refl->getProperty("protectedProp");
    $privateProp = $refl->getProperty("privateProp");
    if ($publicProp->isPublic()) {
        echo  "VisibilityClass::publicProp is public\n";
    }
    if ($protectedProp->isProtected()) {
        echo  "VisibilityClass::protectedProp is protected\n";
    }
    if ($privateProp->isPrivate()) {
        echo  "VisibilityClass::privateProp is private\n";
    }
}

// CHECK: class VisibilityClass exists
// CHECK: VisibilityClass::publicProp is public
// CHECK: VisibilityClass::protectedProp is protected
// CHECK: VisibilityClass::privateProp is private