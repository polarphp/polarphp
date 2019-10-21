#!/usr/bin/env python
<?php
foreach (getenv() as $name => $value) {
    echo $name . ' = ' . $value . "\n";
}
