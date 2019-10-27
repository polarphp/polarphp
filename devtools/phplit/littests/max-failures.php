<?php

# Check the behavior of --max-failures option.
#
# RUN: not %{phplit} -j 1 -v %{inputs}/max-failures > %t.out
# RUN: not %{phplit} --max-failures=1 -j 1 -v %{inputs}/max-failures >> %t.out
# RUN: not %{phplit} --max-failures=2 -j 1 -v %{inputs}/max-failures >> %t.out
# RUN: not %{phplit} --max-failures=0 -j 1 -v %{inputs}/max-failures 2>> %t.out
# RUN: filechecker < %t.out %s
#
# END.

# CHECK: Failing Tests (27)
# CHECK: Failing Tests (1)
# CHECK: Failing Tests (2)
# CHECK: emergency: Option '--max-failures' requires positive integer
