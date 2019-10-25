<?php
# Basic sanity check that usage works.
#
# RUN: %{phplit} --help > %t.out
# RUN: filechecker < %t.out %s
#
# CHECK: Usage:
# CHECK-NEXT: run-test [options] [--] [<test_paths>...]
