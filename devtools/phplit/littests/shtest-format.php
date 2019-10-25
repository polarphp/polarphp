<?php
# Check the various features of the ShTest format.
#
# RUN: rm -f %t.xml
# RUN: not %{phplit} -j 1 -v %{inputs}/shtest-format --xunit-xml-output %t.xml > %t.out



# END.