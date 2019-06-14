<?php
$projectDir = getcwd();
$syntaxSrcDir = $projectDir . "/src/syntax";
$grammerFile = $projectDir."/include/polarphp/parser/LangGrammer.y";

$lines = file($grammerFile);

function start_with($prefix, $str)
{
   $len = strlen($prefix);
   return substr($str, 0, $len) == $prefix;
}

$tokenInfoMap = array(
   "internal" => array(),
   "keyword" => array(),
   "declKeyword" => array(),
   "stmtKeyword" => array(),
   "exprKeyword" => array(),
   "punctuator" => array(),
   "misc" => array()
);

function parse_token_define_line($line)
{
   $line = substr($line, strlen("%token"));
   $line = preg_replace("/<.*?>/", "", $line);
   $tokenDescStartPos = strpos($line, '"');
   $tokenDescEndPos = strrpos($line, '"');
   $tokenDesc = substr($line, $tokenDescStartPos + 1, $tokenDescEndPos - $tokenDescStartPos - 1);
   $tokenDesc = trim(preg_replace("/\(.*?\)/", "", $tokenDesc));
   // trim \(*?\)
   $tokenKind = trim(substr($line, 0, $tokenDescStartPos));
   // maybe have int value
   $tokenKindParts = explode(' ', $tokenKind);
   if (count($tokenKindParts) == 2) {
      $tokenKind = trim($tokenKindParts[0]);
   }
   return array($tokenKind, $tokenDesc);
}
$infoKey = "internal";
foreach($lines as $line) {
   $line = trim($line);
   // get info map key
   if (false !== strpos($line, 'DECL_KEYWORD_MARK_START')) {
      $infoKey = "declKeyword";
   } elseif(false !== strpos($line, 'STMT_KEYWORD_MARK_STRAT')) {
      $infoKey = "stmtKeyword";
   } elseif(false !== strpos($line, 'EXPR_KEYWORD_MARK_START')) {
      $infoKey = "exprKeyword";
   } elseif (false !== strpos($line, 'KEYWORD_MARK_START')) {
      $infoKey = "keyword";
   } elseif(false !== strpos($line, 'PUNCTUATOR_MARK_START')) {
      $infoKey = "punctuator";
   } elseif(false !== strpos($line, 'MISC_MARK_START')) {
      $infoKey = "misc";
   }
   if (start_with("%token", $line)) {
      $tokenInfoMap[$infoKey][] = parse_token_define_line($line);
   }
}
// generate files


$tokenDescMapFile = $syntaxSrcDir . "/TokenDescMap.cpp";
$tokenDescMapTplFile = $tokenDescMapFile.".in";

if (!file_exists($tokenDescMapTplFile)) {
   exit("$tokenDescMapTplFile not exist");
}

$tokenTypeMap = array(
   "internal" => "",
   "keyword" => "",
   "declKeyword" => "",
   "stmtKeyword" => "",
   "exprKeyword" => "",
   "punctuator" => "",
   "misc" => ""
);

$tokenDescItems = array();
foreach($tokenInfoMap as $type => $tokens) {
   foreach($tokens as $token) {
      $tokenDescItems[] = "{TokenKindType::$token[0], {\"$token[0]\", \"$token[1]\"}},";
   }
}

$fileContent = file_get_contents($tokenDescMapTplFile);
$fileContent = str_replace("__TOKEN_RECORDS__", implode("\n      ", $tokenDescItems), $fileContent);
echo $fileContent;
