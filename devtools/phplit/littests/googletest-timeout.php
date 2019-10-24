<?php
# Check that the per test timeout is enforced when running GTest tests.
#
# RUN: not %{phplit} -j 1 -v %{inputs}/googletest-timeout --timeout=1 > %t.cmd.out
# RUN: filechecker < %t.cmd.out %s

# CHECK: -- Testing:
# CHECK: PASS: googletest-timeout :: {{[Dd]ummy[Ss]ub[Dd]ir}}/OneTest.php/FirstTest.subTestA
# CHECK: TIMEOUT: googletest-timeout :: {{[Dd]ummy[Ss]ub[Dd]ir}}/OneTest.php/FirstTest.subTestB
# CHECK: TIMEOUT: googletest-timeout :: {{[Dd]ummy[Ss]ub[Dd]ir}}/OneTest.php/FirstTest.subTestC
# CHECK: Expected Passes    : 1
# CHECK: Individual Timeouts: 2

# Test per test timeout via a config file and on the command line.
# The value set on the command line should override the config file.
# RUN: not %{phplit} -j 1 -v %{inputs}/googletest-timeout \
# RUN: --param set_timeout=1 --timeout=2 > %t.cmdover.out 2> %t.cmdover.err
# RUN: filechecker < %t.cmdover.out %s
# RUN: filechecker --check-prefix=CHECK-CMDLINE-OVERRIDE-ERR < %t.cmdover.err %s

# CHECK-CMDLINE-OVERRIDE-ERR: Forcing timeout to be 2 seconds