<?php
$projectDir = getcwd();
$includeDir = $projectDir . "/include/polarphp";
$yyParserDefsFile = $includeDir . "/parser/internal/YYParserDefs.h";
$tokenEnumDefsFile = $includeDir . "/syntax/internal/TokenEnumDefs.h";

if (!file_exists($yyParserDefsFile)) {
   exit("$yyParserDefsFile is not exist");
}

$lines = file($yyParserDefsFile);
$beginEnumDef = false;
$inEnumRegion = false;
$defItems = array();
foreach($lines as $line) {
   $line = trim($line);
   if ($line == "enum yytokentype") {
      $beginEnumDef = true;
   }
   if ($beginEnumDef && $line == "{") {
      $inEnumRegion = true;
      continue;
   }
   if ($beginEnumDef && $line == "};") {
      break;
   }
   if ($inEnumRegion) {
      $defItems[] = $line;
   }
}

$tokenEnumDefsTplFile = $tokenEnumDefsFile.".in";

if (!file_exists($tokenEnumDefsTplFile)) {
   exit("$tokenEnumDefsTplFile is not exist");
}

$fileContent = file_get_contents($tokenEnumDefsTplFile);
$fileContent = str_replace("__TOKEN_DEFS__", implode("\n   ", $defItems), $fileContent);

$needWriteFile = false;
if (!file_exists($tokenEnumDefsFile)) {
   $needWriteFile = true;
} else {
   $oldMd5 = md5_file($tokenEnumDefsFile);
   $newMd5 = md5($fileContent);
   if ($oldMd5 != $newMd5) {
      $needWriteFile = true;
   }
}

if ($needWriteFile) {
   file_put_contents($tokenEnumDefsFile, $fileContent);
}
