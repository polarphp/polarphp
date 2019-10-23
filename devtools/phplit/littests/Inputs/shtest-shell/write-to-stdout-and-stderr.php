#!/usr/bin/env php
<?php
$stdout = fopen("php://stdout", 'w');
$stderr = fopen("php://stderr", 'w');
fwrite($stdout, "a line on stdout\n");
fwrite($stderr, "a line on stderr\n");
fflush($stdout);
fflush($stderr);
