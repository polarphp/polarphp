<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
// here we test argument passed
if (function_exists("say_hello")) {
    say_hello();
    say_hello("polarphp team");
    say_hello("zzu_softboy");
    say_hello(3.14); // here will convert into string
}
// CHECK: hello, polarphp
// CHECK: hello, polarphp team
// CHECK: hello, zzu_softboy
// CHECK: hello, 3.14
