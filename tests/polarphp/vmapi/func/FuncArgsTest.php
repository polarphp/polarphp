<?php
// RUN: %{polarphp} %s 1> %t.out 2>&1
// RUN: filechecker --input-file %t.out %s
