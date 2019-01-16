<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s

if (class_exists("\MagicMethodClass")) {
    $object = new \MagicMethodClass();
    echo "cast to string : " . $object;
    echo "\n";
    echo "cast to integer : " . intval($object);
    echo "\n";
    echo "cast to integer : " . (int)$object;
    echo "\n";
    echo "cast to double : " . (double)$object;
    echo "\n";
    echo "cast to double : " . doubleval($object);
    echo "\n";
    echo "cast to boolean : " . boolval($object);
    echo "\n";
    echo "cast to boolean : " . (bool)$object;
    echo "\n";
}
