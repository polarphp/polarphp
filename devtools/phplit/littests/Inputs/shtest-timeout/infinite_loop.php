<?php
# RUN: php %s
printf("Running infinite loop");
flush();
// Make sure the print gets flushed so it appears in lit output.
while(true){}
