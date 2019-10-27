<?php
# Check the internal shell handling component of the ShTest format.
#
# RUN: not %{phplit} -j 1 -v %{inputs}/shtest-shell > %t.out
# FIXME: Temporarily dump test output so we can debug failing tests on
# buildbots.
# RUN: cat %t.out
# RUN: filechecker --input-file %t.out %s
#
# END.

# CHECK: -- Testing:

# CHECK: FAIL: shtest-shell :: cat-error-0.txt
# CHECK: *** TEST 'shtest-shell :: cat-error-0.txt' FAILED ***
# CHECK: $ "cat" "-b" "temp1.txt"
# CHECK: # command stderr:
# CHECK: The following argument was not expected: -b
# CHECK: error: command failed with exit status: 109

# CHECK: FAIL: shtest-shell :: cat-error-1.txt
# CHECK: *** TEST 'shtest-shell :: cat-error-1.txt' FAILED ***
# CHECK: $ "cat" "temp1.txt"
# CHECK: # command stderr:
# CHECK: No such file or directory: temp1.txt
# CHECK: error: command failed with exit status: 1
# CHECK: ***

# CHECK: FAIL: shtest-shell :: colon-error.txt
# CHECK: *** TEST 'shtest-shell :: colon-error.txt' FAILED ***
# CHECK: $ ":"
# CHECK: # command stderr:
# CHECK: Unsupported: ':' cannot be part of a pipeline
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: diff-error-0.txt
# CHECK: *** TEST 'shtest-shell :: diff-error-0.txt' FAILED ***
# CHECK: $ "diff" "diff-error-0.txt" "diff-error-0.txt"
# CHECK: # command stderr:
# CHECK: Unsupported: 'diff' cannot be part of a pipeline
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: diff-error-1.txt
# CHECK: *** TEST 'shtest-shell :: diff-error-1.txt' FAILED ***
# CHECK: $ "diff" "-B" "temp1.txt" "temp2.txt"
# CHECK: # command stderr:
# CHECK: Unsupported: 'diff':  The "-B" option does not exist.
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: diff-error-2.txt
# CHECK: *** TEST 'shtest-shell :: diff-error-2.txt' FAILED ***
# CHECK: $ "diff" "temp.txt"
# CHECK: # command stderr:
# CHECK: Error:  missing or extra operand
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: diff-error-3.txt
# CHECK: *** TEST 'shtest-shell :: diff-error-3.txt' FAILED ***
# CHECK: $ "diff" "temp.txt" "temp1.txt"
# CHECK: # command stderr:
# CHECK: Error: 'diff' command failed
# CHECK: error: command failed with exit status: 1
# CHECK: ***

# CHECK: FAIL: shtest-shell :: diff-error-4.txt
# CHECK: *** TEST 'shtest-shell :: diff-error-4.txt' FAILED ***
# CHECK: Exit Code: 1
# CHECK: # command output:
# CHECK: diff-error-4.txt.temp
# CHECK: diff-error-4.txt.temp1
# CHECK: @@ -1,1 +1,1 @@
# CHECK: -hello-first
# CHECK: +hello-second

# CHECK: FAIL: shtest-shell :: diff-error-5.txt
# CHECK: *** TEST 'shtest-shell :: diff-error-5.txt' FAILED ***
# CHECK: $ "diff"
# CHECK: # command stderr:
# CHECK: Error:  missing or extra operand
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: diff-error-6.txt
# CHECK: *** TEST 'shtest-shell :: diff-error-6.txt' FAILED ***
# CHECK: $ "diff"
# CHECK: # command stderr:
# CHECK: Error:  missing or extra operand
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: diff-r-error-0.txt
# CHECK: *** TEST 'shtest-shell :: diff-r-error-0.txt' FAILED ***
# CHECK: $ "diff" "-r"
# CHECK: # command output:
# CHECK: Only in {{.*}}dir1: dir1unique
# CHECK: Only in {{.*}}dir2: dir2unique
# CHECK: error: command failed with exit status: 1

# CHECK: FAIL: shtest-shell :: diff-r-error-1.txt
# CHECK: *** TEST 'shtest-shell :: diff-r-error-1.txt' FAILED ***
# CHECK: $ "diff" "-r"
# CHECK: # command output:
# CHECK: *** {{.*}}dir1{{.*}}subdir{{.*}}f01
# CHECK: --- {{.*}}dir2{{.*}}subdir{{.*}}f01
# CHECK: 12345
# CHECK: 00000
# CHECK: error: command failed with exit status: 1

# CHECK: FAIL: shtest-shell :: diff-r-error-2.txt
# CHECK: *** TEST 'shtest-shell :: diff-r-error-2.txt' FAILED ***
# CHECK: $ "diff" "-r"
# CHECK: # command output:
# CHECK: Only in {{.*}}dir2: extrafile
# CHECK: error: command failed with exit status: 1

# CHECK: FAIL: shtest-shell :: diff-r-error-3.txt
# CHECK: *** TEST 'shtest-shell :: diff-r-error-3.txt' FAILED ***
# CHECK: $ "diff" "-r"
# CHECK: # command output:
# CHECK: Only in {{.*}}dir1: extra_subdir
# CHECK: error: command failed with exit status: 1

# CHECK: FAIL: shtest-shell :: diff-r-error-4.txt
# CHECK: *** TEST 'shtest-shell :: diff-r-error-4.txt' FAILED ***
# CHECK: $ "diff" "-r"
# CHECK: # command output:
# CHECK: File {{.*}}dir1{{.*}}extra_subdir is a directory while file {{.*}}dir2{{.*}}extra_subdir is a regular file
# CHECK: error: command failed with exit status: 1

# CHECK: FAIL: shtest-shell :: diff-r-error-5.txt
# CHECK: *** TEST 'shtest-shell :: diff-r-error-5.txt' FAILED ***
# CHECK: $ "diff" "-r"
# CHECK: # command output:
# CHECK: Only in {{.*}}dir1: extra_subdir
# CHECK: error: command failed with exit status: 1

# CHECK: FAIL: shtest-shell :: diff-r-error-6.txt
# CHECK: *** TEST 'shtest-shell :: diff-r-error-6.txt' FAILED ***
# CHECK: $ "diff" "-r"
# CHECK: # command output:
# CHECK: File {{.*}}dir1{{.*}}extra_file is a regular empty file while file {{.*}}dir2{{.*}}extra_file is a directory
# CHECK: error: command failed with exit status: 1

# CHECK: PASS: shtest-shell :: diff-r.txt

# CHECK: FAIL: shtest-shell :: error-0.txt
# CHECK: *** TEST 'shtest-shell :: error-0.txt' FAILED ***
# CHECK: $ "not-a-real-command"
# CHECK: # command stderr:
# CHECK: 'not-a-real-command': command not found
# CHECK: error: command failed with exit status: 127
# CHECK: ***


# FIXME: The output here sucks.
#
# CHECK: FAIL: shtest-shell :: error-1.txt
# CHECK: *** TEST 'shtest-shell :: error-1.txt' FAILED ***
# CHECK: shell parser error on: : 'RUN: at line 3';   echo "missing quote
# CHECK: ***

# CHECK: FAIL: shtest-shell :: error-2.txt
# CHECK: *** TEST 'shtest-shell :: error-2.txt' FAILED ***
# CHECK: Unsupported redirect:
# CHECK: ***

# CHECK: FAIL: shtest-shell :: mkdir-error-0.txt
# CHECK: *** TEST 'shtest-shell :: mkdir-error-0.txt' FAILED ***
# CHECK: $ "mkdir" "-p" "temp"
# CHECK: # command stderr:
# CHECK: Unsupported: 'mkdir' cannot be part of a pipeline
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: mkdir-error-1.txt
# CHECK: *** TEST 'shtest-shell :: mkdir-error-1.txt' FAILED ***
# CHECK: $ "mkdir" "-p" "-m" "777" "temp"
# CHECK: # command stderr:
# CHECK: Unsupported: 'mkdir':  The "-m" option does not exist.
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: mkdir-error-2.txt
# CHECK: *** TEST 'shtest-shell :: mkdir-error-2.txt' FAILED ***
# CHECK: $ "mkdir" "-p"
# CHECK: # command stderr:
# CHECK: Error: 'mkdir' is missing an operand
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: PASS: shtest-shell :: redirects.txt

# CHECK: FAIL: shtest-shell :: rm-error-0.txt
# CHECK: *** TEST 'shtest-shell :: rm-error-0.txt' FAILED ***
# CHECK: $ "rm" "-rf" "temp"
# CHECK: # command stderr:
# CHECK: Unsupported: 'rm' cannot be part of a pipeline
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: rm-error-1.txt
# CHECK: *** TEST 'shtest-shell :: rm-error-1.txt' FAILED ***
# CHECK: $ "rm" "-f" "-v" "temp"
# CHECK: # command stderr:
# CHECK: Unsupported: 'rm': The "-v" option does not exist.
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: rm-error-2.txt
# CHECK: *** TEST 'shtest-shell :: rm-error-2.txt' FAILED ***
# CHECK: $ "rm" "-r" "hello"
# CHECK: # command stderr:
# CHECK: Error: 'rm' command failed
# CHECK: error: command failed with exit status: 1
# CHECK: ***

# CHECK: FAIL: shtest-shell :: rm-error-3.txt
# CHECK: *** TEST 'shtest-shell :: rm-error-3.txt' FAILED ***
# CHECK: Exit Code: 1
# CHECK: ***

# CHECK: PASS: shtest-shell :: rm-unicode-0.txt
# CHECK: PASS: shtest-shell :: sequencing-0.txt
# CHECK: XFAIL: shtest-shell :: sequencing-1.txt
# CHECK: PASS: shtest-shell :: valid-shell.txt
# CHECK: Failing Tests (27)