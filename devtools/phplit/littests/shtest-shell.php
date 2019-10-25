<?php
# Check the internal shell handling component of the ShTest format.
#
# RUN: not %{phplit} -j 1 -v %{inputs}/shtest-shell > %t.out
# FIXME: Temporarily dump test output so we can debug failing tests on
# buildbots.
# RUN: cat %t.out
#
# END.