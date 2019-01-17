<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("PropsTestClass")) {
    // instance property
    $object = new PropsTestClass();
    if (property_exists($object, "nullProp")) {
        echo "PropsTestClass::nullProp is exist\n";
        echo "PropsTestClass::nullProp value : ". (is_null($object->nullProp) ? "null\n" : "notnull\n");
    }
    if (property_exists($object, "trueProp")) {
        echo "PropsTestClass::trueProp is exist\n";
        echo "PropsTestClass::trueProp value : " . ($object->trueProp ? "true" : "false");
        echo "\n";
    }
    if (property_exists($object, "falseProp")) {
        echo "PropsTestClass::falseProp is exist\n";
        echo "PropsTestClass::falseProp value : " . ($object->falseProp ? "true" : "false");
        echo "\n";
    }
    if (property_exists($object, "numProp")) {
        echo "PropsTestClass::numProp is exist\n";
        echo "PropsTestClass::numProp value : {$object->numProp}\n";
    }
    if (property_exists($object, "doubleProp")) {
        echo "PropsTestClass::doubleProp is exist\n";
        echo "PropsTestClass::doubleProp value : {$object->doubleProp}\n";
    }
    if (property_exists($object, "strProp")) {
        echo "PropsTestClass::strProp is exist\n";
        echo "PropsTestClass::strProp value : {$object->strProp}\n";
    }
    
    if (property_exists($object, "strProp1")) {
        echo "PropsTestClass::strProp1 is exist\n";
        echo "PropsTestClass::strProp1 value : {$object->strProp1}\n";
    }
}