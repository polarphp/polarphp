<?php
# Check that the per test timeout is enforced when running GTest tests.
#
# RUN: not %{phplit} -j 1 -v %{inputs}/googletest-timeout --timeout=1 > %t.cmd.out