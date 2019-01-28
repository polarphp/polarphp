<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("PropsTestClass")) {
    // instance property
    $object = new PropsTestClass();
    if (property_exists($object, "nullProp")) {
        if (is_null($object->nullProp)) {
            echo "the type of PropsTestClass::nullProp is null\n";
        }
        
    }
    if (property_exists($object, "trueProp")) {
        if (is_bool($object->trueProp)) {
            echo "the type of PropsTestClass::trueProp is : boolean\n";
        }
    }
    if (property_exists($object, "falseProp")) {
        if (is_bool($object->falseProp)) {
            echo "the type of PropsTestClass::falseProp is : boolean\n";
        }
    }
    if (property_exists($object, "numProp")) {
        if (is_int($object->numProp)) {
            echo "the type of PropsTestClass::numProp is : int\n";
        }
    }
    if (property_exists($object, "doubleProp")) {
        if (is_int($object->doubleProp)) {
            echo "the type of PropsTestClass::doubleProp is : double\n";
        }
    }
    if (property_exists($object, "strProp")) {
        if (is_string($object->strProp)) {
            echo "the type of PropsTestClass::strProp is : string\n";
        }
    }
    
    if (property_exists($object, "strProp1")) {
        if (is_string($object->strProp1)) {
            echo "the type of PropsTestClass::strProp1 is : string\n";
        }
    }
    
    if (property_exists($object, "nullProp")) {
        if (is_null($object->nullProp)) {
            echo "the type of PropsTestClass::nullProp is null\n";
        }
        
    }
    if (property_exists($object, "trueProp")) {
        if (is_bool($object->trueProp)) {
            echo "the type of PropsTestClass::trueProp is : boolean\n";
        }
    }
    if (property_exists($object, "falseProp")) {
        if (is_bool($object->falseProp)) {
            echo "the type of PropsTestClass::falseProp is : boolean\n";
        }
    }
    if (property_exists($object, "numProp")) {
        if (is_int($object->numProp)) {
            echo "the type of PropsTestClass::numProp is : int\n";
        }
    }
    if (property_exists($object, "doubleProp")) {
        if (is_int($object->doubleProp)) {
            echo "the type of PropsTestClass::doubleProp is : double\n";
        }
    }
    if (property_exists($object, "strProp")) {
        if (is_string($object->strProp)) {
            echo "the type of PropsTestClass::strProp is : string\n";
        }
    }
    
    if (property_exists($object, "strProp1")) {
        if (is_string($object->strProp1)) {
            echo "the type of PropsTestClass::strProp1 is : string\n";
        }
    }
    
    if (property_exists("PropsTestClass", "staticNullProp")) {
        if (is_null(PropsTestClass::$staticNullProp)) {
            echo "the type of PropsTestClass::staticNullProp is null\n";
        }
    }
    if (property_exists("PropsTestClass", "staticTrueProp")) {
        if (is_bool(PropsTestClass::$staticTrueProp)) {
            echo "the type of PropsTestClass::staticTrueProp is boolean\n";
        }
    }
    if (property_exists("PropsTestClass", "staticFalseProp")) {
        if (is_bool(PropsTestClass::$staticFalseProp)) {
            echo "the type of PropsTestClass::staticFalseProp is boolean\n";
        }
    }
    if (property_exists("PropsTestClass", "staticNumProp")) {
        if (is_int(PropsTestClass::$staticNumProp)) {
            echo "the type of PropsTestClass::staticNumProp is int\n";
        }
    }
    if (property_exists("PropsTestClass", "staticDoubleProp")) {
        if (is_double(PropsTestClass::$staticDoubleProp)) {
            echo "the type of PropsTestClass::staticDoubleProp is double\n";
        }
    }
    if (property_exists("PropsTestClass", "staticStrProp")) {
        if (is_string(PropsTestClass::$staticStrProp)) {
            echo "the type of PropsTestClass::staticStrProp is string\n";
        }
    }
    if (property_exists("PropsTestClass", "staticStrProp1")) {
        if (is_string(PropsTestClass::$staticStrProp1)) {
            echo "the type of PropsTestClass::staticStrProp is string\n";
        }
    }
    if (defined("PropsTestClass::MATH_PI")) {
        if (is_float(PropsTestClass::MATH_PI)) {
            echo "the type of PropsTestClass::MATH_PI is double\n";
        }
    }
}

// CHECK: the type of PropsTestClass::nullProp is null
// CHECK: the type of PropsTestClass::trueProp is : boolean
// CHECK: the type of PropsTestClass::falseProp is : boolean
// CHECK: the type of PropsTestClass::numProp is : int
// CHECK: the type of PropsTestClass::strProp is : string
// CHECK: the type of PropsTestClass::nullProp is null
// CHECK: the type of PropsTestClass::trueProp is : boolean
// CHECK: the type of PropsTestClass::falseProp is : boolean
// CHECK: the type of PropsTestClass::numProp is : int
// CHECK: the type of PropsTestClass::strProp is : string
// CHECK: the type of PropsTestClass::staticNullProp is null
// CHECK: the type of PropsTestClass::staticTrueProp is boolean
// CHECK: the type of PropsTestClass::staticFalseProp is boolean
// CHECK: the type of PropsTestClass::staticNumProp is int
// CHECK: the type of PropsTestClass::staticDoubleProp is double
// CHECK: the type of PropsTestClass::staticStrProp is string
// CHECK: the type of PropsTestClass::MATH_PI is double