#!/usr/bin/env php

<?php
$stderr = fopen("php://stderr", 'w');
fwrite($stderr, "a line on stderr\n");
fflush($stderr);

