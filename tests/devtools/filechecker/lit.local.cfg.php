<?php
# Unset environment variables that the FileCheck tests
# expect not to be set.
$fileCheckExpectedUnsetVars = [
  'FILECHECK_DUMP_INPUT_ON_FAILURE',
  'FILECHECK_OPTS',
];

$enviroment = $config->getEnvironment();
foreach ($fileCheckExpectedUnsetVars as $envVar) {
   if (in_array($envVar, $enviroment)) {
      TestLogger::note('Removing %s from environment for FileCheck tests', $envVar);
      $config->unsetEnvVar($envVar);
   }
}