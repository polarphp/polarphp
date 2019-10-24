<?php
# Check the various features of the GoogleTest format.
#
# RUN: not %{phplit} -j 1 -v %{inputs}/googletest-upstream-format > %t.out
# RUN: filechecker < %t.out %s
#
# END.

# CHECK: -- Testing:
# CHECK: PASS: googletest-upstream-format :: {{[Dd]ummy[Ss]ub[Dd]ir}}/OneTest.php/FirstTest.subTestA
# CHECK: FAIL: googletest-upstream-format :: {{[Dd]ummy[Ss]ub[Dd]ir}}/OneTest.php/FirstTest.subTestB
# CHECK-NEXT: ******************** TEST 'googletest-upstream-format :: DummySubDir/OneTest.php/FirstTest.subTestB' FAILED ********************
# CHECK-NEXT: Running main() from gtest_main.cc
# CHECK-NEXT: I am subTest B, I FAIL
# CHECK-NEXT: And I have two lines of output
# CHECK: ********************
# CHECK: PASS: googletest-upstream-format :: {{[Dd]ummy[Ss]ub[Dd]ir}}/OneTest.php/ParameterizedTest/0.subTest
# CHECK: PASS: googletest-upstream-format :: {{[Dd]ummy[Ss]ub[Dd]ir}}/OneTest.php/ParameterizedTest/1.subTest
# CHECK: Failing Tests (1)
# CHECK: Expected Passes    : 3
# CHECK: Unexpected Failures: 1