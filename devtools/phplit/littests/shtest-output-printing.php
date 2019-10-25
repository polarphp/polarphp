<?php
# Check the various features of the ShTest format.
#
# RUN: not %{phplit} -j 1 -v %{inputs}/shtest-output-printing > %t.out

#
# END.

# CHECK: -- Testing:

# CHECK: FAIL: shtest-output-printing :: basic.txt
# CHECK-NEXT: *** TEST 'shtest-output-printing :: basic.txt' FAILED ***
# CHECK-NEXT: Script:
# CHECK-NEXT: --
# CHECK:      --
# CHECK-NEXT: Exit Code: 1
#
# CHECK:      Command Output
# CHECK-NEXT: --
# CHECK-NEXT: $ ":" "RUN: at line 1"
# CHECK-NEXT: $ "true"
# CHECK-NEXT: $ ":" "RUN: at line 2"
# CHECK-NEXT: $ "echo" "hi"
# CHECK-NEXT: # command output:
# CHECK-NEXT: hi
#
# CHECK:      $ ":" "RUN: at line 3"
# CHECK-NEXT: $ "not" "not" "wc" "missing-file"
# CHECK-NEXT: # redirected output from '{{.*(/|\\\\)}}basic.txt.tmp.out':
# CHECK-NEXT: open: No such file or directory
# CHECK:      note: command had no output on stdout or stderr
# CHECK-NEXT: error: command failed with exit status: 1
