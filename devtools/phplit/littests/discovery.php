<?php
# Check the basic discovery process, including a sub-suite.
#
# RUN: %{phplit}  %{inputs}/discovery \
# RUN:   -j 1 --debug --show-tests --show-suites \
# RUN:   -v > %t.out 2> %t.err

# RUN: filechecker --check-prefix=CHECK-BASIC-ERR < %t.err %s
# CHECK-BASIC-ERR: loading suite config '{{.*(/|\\\\)discovery(/|\\\\)lit.cfg.php}}'
# CHECK-BASIC-ERR-DAG: loading suite config '{{.*(/|\\\\)discovery(/|\\\\)subsuite(/|\\\\)lit.cfg.php}}'
# CHECK-BASIC-ERR-DAG: loading local config '{{.*(/|\\\\)discovery(/|\\\\)subdir(/|\\\\)lit.local.cfg.php}}'
