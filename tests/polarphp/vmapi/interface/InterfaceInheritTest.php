<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
if (interface_exists("InterfaceA") && interface_exists("InterfaceB") && interface_exists("InterfaceC")) {
    if (!is_subclass_of("InterfaceC", "InterfaceB")) {
       echo "fail";
       return;
    }
    if (!is_subclass_of("InterfaceC", "InterfaceA")) {
        echo "fail";
        return;
    }
    if (!is_subclass_of("InterfaceB", "InterfaceA")) {
        echo "fail";
        return;
    }
    // method of interface
    if (!method_exists("InterfaceA", "methodOfA")) {
        echo "fail";
        return;
    }
    if (!method_exists("InterfaceA", "protectedMethodOfA")) {
        echo "fail";
        return;
    }
    if (!method_exists("InterfaceA", "privateMethodOfA")) {
        echo "fail";
        return;
    }
    
    if (!method_exists("InterfaceB", "methodOfB")) {
        echo "fail";
        return;
    }
    if (!method_exists("InterfaceB", "protectedMethodOfB")) {
        echo "fail";
        return;
    }
    if (!method_exists("InterfaceB", "privateMethodOfB")) {
        echo "fail";
        return;
    }
    
    if (!method_exists("InterfaceC", "methodOfC")) {
        echo "fail";
        return;
    }
    if (!method_exists("InterfaceC", "protectedMethodOfC")) {
        echo "fail";
        return;
    }
    if (!method_exists("InterfaceC", "privateMethodOfC")) {
        echo "fail";
        return;
    }
    
     if (!method_exists("InterfaceC", "methodOfA")) {
        echo "fail";
        return;
     }
     if (!method_exists("InterfaceC", "protectedMethodOfA")) {
        echo "fail";
        return;
     }
     if (!method_exists("InterfaceC", "privateMethodOfA")) {
        echo "fail";
        return;
     }
     if (!method_exists("InterfaceC", "methodOfB")) {
        echo "fail";
        return;
     }
     if (!method_exists("InterfaceC", "protectedMethodOfB")) {
        echo "fail";
        return;
     }
     if (!method_exists("InterfaceC", "privateMethodOfB")) {
        echo "fail";
        return;
     }
 } else {
    echo "fail";
    return;
 }
 
 echo "success";
 // CHECK: success