<?php
# Check the basic discovery process, including a sub-suite.
#
# RUN: %{phplit}  %{inputs}/discovery \
# RUN:   -j 1 --debug --show-tests --show-suites \
# RUN:   -v > %t.out 2> %t.err

# RUN: filechecker --check-prefix=CHECK-BASIC-OUT < %t.out %s
# RUN: filechecker --check-prefix=CHECK-BASIC-ERR < %t.err %s
#
# CHECK-BASIC-ERR: loading suite config '{{.*(/|\\\\)discovery(/|\\\\)lit.cfg.php}}'
# CHECK-BASIC-ERR-DAG: loading suite config '{{.*(/|\\\\)discovery(/|\\\\)subsuite(/|\\\\)lit.cfg.php}}'
# CHECK-BASIC-ERR-DAG: loading local config '{{.*(/|\\\\)discovery(/|\\\\)subdir(/|\\\\)lit.local.cfg.php}}'
#
# CHECK-BASIC-OUT: -- Test Suites --
# CHECK-BASIC-OUT:   sub-suite - 2 tests
# CHECK-BASIC-OUT:     Source Root: {{.*[/\\]discovery[/\\]subsuite$}}
# CHECK-BASIC-OUT:     Exec Root  : {{.*[/\\]discovery[/\\]subsuite$}}
# CHECK-BASIC-OUT:   top-level-suite - 3 tests
# CHECK-BASIC-OUT:     Source Root: {{.*[/\\]discovery$}}
# CHECK-BASIC-OUT:     Exec Root  : {{.*[/\\]discovery$}}
#
# CHECK-BASIC-OUT: -- Available Tests --
# CHECK-BASIC-OUT: sub-suite :: test-one.txt
# CHECK-BASIC-OUT: sub-suite :: test-two.txt
# CHECK-BASIC-OUT: top-level-suite :: test-one.txt
# CHECK-BASIC-OUT: top-level-suite :: test-two.txt
# CHECK-BASIC-OUT: top-level-suite :: subdir/test-three.php

# Check discovery when exact test names are given.
#
# RUN: %{phplit}  \
# RUN:     %{inputs}/discovery/subdir/test-three.php \
# RUN:     %{inputs}/discovery/subsuite/test-one.txt \
# RUN:   -j 1 --show-tests --show-suites -v > %t.out
# RUN: filechecker --check-prefix=CHECK-EXACT-TEST < %t.out %s
#
# CHECK-EXACT-TEST: -- Available Tests --
# CHECK-EXACT-TEST: sub-suite :: test-one.txt
# CHECK-EXACT-TEST: top-level-suite :: subdir/test-three.php

# Check discovery when config files end in .py
# RUN: %{phplit} %{inputs}/php-config-discovery \
# RUN:   -j 1 --debug --show-tests --show-suites \
# RUN:   -v > %t.out 2> %t.err
# RUN: filechecker --check-prefix=CHECK-PHPCONFIG-OUT < %t.out %s
# RUN: filechecker --check-prefix=CHECK-PHPCONFIG-ERR < %t.err %s

#
# CHECK-PHPCONFIG-ERR: loading suite config '{{.*(/|\\\\)php-config-discovery(/|\\\\)lit.site.cfg.php}}'
# CHECK-PHPCONFIG-ERR: load_config from '{{.*(/|\\\\)discovery(/|\\\\)lit.cfg.php}}'
# CHECK-PHPCONFIG-ERR: loaded config '{{.*(/|\\\\)discovery(/|\\\\)lit.cfg.php}}'
# CHECK-PHPCONFIG-ERR: loaded config '{{.*(/|\\\\)php-config-discovery(/|\\\\)lit.site.cfg.php}}'
# CHECK-PHPCONFIG-ERR-DAG: loading suite config '{{.*(/|\\\\)discovery(/|\\\\)subsuite(/|\\\\)lit.cfg.php}}'
# CHECK-PHPCONFIG-ERR-DAG: loading local config '{{.*(/|\\\\)discovery(/|\\\\)subdir(/|\\\\)lit.local.cfg.php}}'
#

# CHECK-PHPCONFIG-OUT: -- Test Suites --
# CHECK-PHPCONFIG-OUT:   sub-suite - 2 tests
# CHECK-PHPCONFIG-OUT:     Source Root: {{.*[/\\]discovery[/\\]subsuite$}}
# CHECK-PHPCONFIG-OUT:     Exec Root  : {{.*[/\\]discovery[/\\]subsuite$}}
# CHECK-PHPCONFIG-OUT:   top-level-suite - 3 tests
# CHECK-PHPCONFIG-OUT:     Source Root: {{.*[/\\]discovery$}}
# CHECK-PHPCONFIG-OUT:     Exec Root  : {{.*[/\\]php-config-discovery$}}
#
# CHECK-PHPCONFIG-OUT: -- Available Tests --
# CHECK-PHPCONFIG-OUT: sub-suite :: test-one.txt
# CHECK-PHPCONFIG-OUT: sub-suite :: test-two.txt
# CHECK-PHPCONFIG-OUT: top-level-suite :: test-one.txt
# CHECK-PHPCONFIG-OUT: top-level-suite :: test-two.txt
# CHECK-PHPCONFIG-OUT: top-level-suite :: subdir/test-three.php
#
# Check discovery when using an exec path.
#
# RUN: %{phplit} %{inputs}/exec-discovery \
# RUN:   -j 1 --debug --show-tests --show-suites \
# RUN:   -v > %t.out 2> %t.err
# RUN: filechecker --check-prefix=CHECK-ASEXEC-OUT < %t.out %s
# RUN: filechecker --check-prefix=CHECK-ASEXEC-ERR < %t.err %s

#
# CHECK-ASEXEC-ERR: loading suite config '{{.*(/|\\\\)exec-discovery(/|\\\\)lit.site.cfg.php}}'
# CHECK-ASEXEC-ERR: load_config from '{{.*(/|\\\\)discovery(/|\\\\)lit.cfg.php}}'
# CHECK-ASEXEC-ERR: loaded config '{{.*(/|\\\\)discovery(/|\\\\)lit.cfg.php}}'
# CHECK-ASEXEC-ERR: loaded config '{{.*(/|\\\\)exec-discovery(/|\\\\)lit.site.cfg.php}}'
# CHECK-ASEXEC-ERR-DAG: loading suite config '{{.*(/|\\\\)discovery(/|\\\\)subsuite(/|\\\\)lit.cfg.php}}'
# CHECK-ASEXEC-ERR-DAG: loading local config '{{.*(/|\\\\)discovery(/|\\\\)subdir(/|\\\\)lit.local.cfg.php}}'
#
# CHECK-ASEXEC-OUT: -- Test Suites --
# CHECK-ASEXEC-OUT:   sub-suite - 2 tests
# CHECK-ASEXEC-OUT:     Source Root: {{.*[/\\]discovery[/\\]subsuite$}}
# CHECK-ASEXEC-OUT:     Exec Root  : {{.*[/\\]discovery[/\\]subsuite$}}
# CHECK-ASEXEC-OUT:   top-level-suite - 3 tests
# CHECK-ASEXEC-OUT:     Source Root: {{.*[/\\]discovery$}}
# CHECK-ASEXEC-OUT:     Exec Root  : {{.*[/\\]exec-discovery$}}
#
# CHECK-ASEXEC-OUT: -- Available Tests --
# CHECK-ASEXEC-OUT: sub-suite :: test-one.txt
# CHECK-ASEXEC-OUT: sub-suite :: test-two.txt
# CHECK-ASEXEC-OUT: top-level-suite :: test-one.txt
# CHECK-ASEXEC-OUT: top-level-suite :: test-two.txt
# CHECK-ASEXEC-OUT: top-level-suite :: subdir/test-three.php

# Check discovery when exact test names are given.
#
# FIXME: Note that using a path into a subsuite doesn't work correctly here.
#
# RUN: %{phplit} \
# RUN:     %{inputs}/exec-discovery/subdir/test-three.php \
# RUN:   -j 1 --show-tests --show-suites -v > %t.out
# RUN: filechecker --check-prefix=CHECK-ASEXEC-EXACT-TEST < %t.out %s
#
# CHECK-ASEXEC-EXACT-TEST: -- Available Tests --
# CHECK-ASEXEC-EXACT-TEST: top-level-suite :: subdir/test-three.php


# Check that we don't recurse infinitely when loading an site specific test
# suite located inside the test source root.
#
# RUN: %{phplit} \
# RUN:     %{inputs}/exec-discovery-in-tree/obj/ \
# RUN:   -j 1 --show-tests --show-suites -v > %t.out
# RUN: filechecker --check-prefix=CHECK-ASEXEC-INTREE < %t.out %s
#
# Try it again after cd'ing into the test suite using a short relative path.
#
# RUN: cd %{inputs}/exec-discovery-in-tree/obj/
# RUN: %{phplit} . \
# RUN:   -j 1 --show-tests --show-suites -v > %t.out
# RUN: filechecker --check-prefix=CHECK-ASEXEC-INTREE < %t.out %s
#
#      CHECK-ASEXEC-INTREE:   exec-discovery-in-tree-suite - 1 tests
# CHECK-ASEXEC-INTREE-NEXT:     Source Root: {{.*[/\\]exec-discovery-in-tree$}}
# CHECK-ASEXEC-INTREE-NEXT:     Exec Root  : {{.*[/\\]exec-discovery-in-tree[/\\]obj$}}
# CHECK-ASEXEC-INTREE-NEXT: -- Available Tests --
# CHECK-ASEXEC-INTREE-NEXT: exec-discovery-in-tree-suite :: test-one.txt


