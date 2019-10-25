#!/usr/bin/env php
<?php
$stdout = fopen("php://stdout", 'b');
fwrite($stdout, "a line with bad encoding: \xc2.");
fflush($stdout);
