<?php
# Check the basic discovery process, including a sub-suite.
#
# RUN: %{phplit} %{inputs}/discovery \
# RUN:   -j 1 --debug --show-tests --show-suites \
# RUN:   -v > %t.out 2> %t.err

