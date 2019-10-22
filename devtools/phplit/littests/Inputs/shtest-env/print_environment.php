#!/usr/bin/env python
<?php
$env = getenv();
ksort($env);
foreach ($env as $name => $value) {
    echo $name . ' = ' . $value . "\n";
}
