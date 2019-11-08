<?php
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/10/28.

namespace Gyb\Utils;

use Gyb\Syntax\Child;
use Gyb\Syntax\Kinds;

function str_start_with(string $str, string $prefix): bool
{
   $prefixLength = strlen($prefix);
   $strLength = strlen($str);
   if ($prefixLength == 0 || $strLength == 0) {
      return false;
   }
   if ($prefixLength > $strLength) {
      return false;
   }
   return substr($str, 0, $prefixLength) == $prefix;
}

function str_end_with(string $str, string $suffix): bool
{
   $suffixLength = strlen($suffix);
   $strLength = strlen($str);
   if ($suffixLength == 0 || $strLength == 0) {
      return false;
   }
   if ($suffixLength > $strLength) {
      return false;
   }
   return substr($str, -$suffixLength) == $suffix;
}

function has_substr(string $str, string $subStr)
{
   return strpos($str, $subStr) !== false;
}

function process_collection_items(array $items)
{
   foreach ($items as &$item) {
      $item['baseKind'] = 'SyntaxCollection';
   }
   return $items;
}

function ensure_require_keys(array $targetArray, array $requireKeys): void
{
   $missedKeys = [];
   foreach ($requireKeys as $key) {
      if (!array_key_exists($key, $targetArray)) {
         $missedKeys[]  = $key;
      }
   }
   if (!empty($missedKeys)) {
      throw new \RuntimeException(sprintf("miss require keys %s", implode(', ', $missedKeys)));
   }
}

/**
 * Each line of the provided string with leading whitespace stripped.
 *
 * @param $description
 */
function dedented_lines($description)
{
   if (0 == strlen($description)) {
      return [];
   }
   $description = trim($description);
   return explode("\n", $description);
}

/**
 * Generates a C++ closure to check whether a given raw syntax node can
 * satisfy the requirements of child.
 *
 * @param Child $child
 * @return string
 */
function check_child_condition_raw(Child $child)
{
   $result = "[](const RefCountPtr<RawSyntax> &raw) {\n";
   $result .= sprintf(" // check %s\n", $child->getName());
   $tokenChoices = $child->getTokenChoices();
   $textChoices = $child->getTextChoices();
   $nodeChoices = $child->getNodeChoices();
   if (!empty($tokenChoices)) {
      $result .= "if (!raw->isToken()) return false;\n";
      $result .= "auto tokenKind = raw->getTokenKind();\n";
      $tokenChecks = [];
      foreach ($tokenChoices as $choice) {
         $tokenChecks[] = sprintf("tokenKind == TokenKindType::%s", $choice->getKind());
      }
      $result .= sprintf("return %s;\n", join(' || ', $tokenChecks));
   } elseif (!empty($textChoices)) {
      $result .= "if (!raw->isToken()) return false;\n";
      $result .= "auto text = raw->getTokenText();\n";
      $tokenChecks = [];
      foreach ($textChoices as $choice) {
         $tokenChecks[] = sprintf("text == %s", $choice);
      }
      $result .= sprintf("return %s;\n", join(' || ', $tokenChecks));
   } elseif (!empty($nodeChoices)) {
      $nodeChecks = [];
      foreach ($nodeChoices as $choice) {
         $nodeChecks[] = check_child_condition_raw($choice).'(raw)';
      }
      $result .= sprintf("return %s;\n", join(' || ', $nodeChecks));
   } else {
      $result .= sprintf("return %s::kindOf(raw->getKind());\n", $child->getTypeName());
   }
   $result .= '}';
   return $result;
}

/**
 * Generates a C++ call to make the raw syntax for a given Child object.
 *
 * @param Child $child
 */
function make_missing_child(Child $child)
{
   if ($child->isToken()) {
      $token = $child->getMainToken();
      $tokenKind = "T_UNKNOWN_MARK";
      $tokenText = "";
      if ($token) {
         $tokenKind = $token->getKind();
         $tokenText = $token->getText();
      }
      return sprintf('RawSyntax::missing(TokenKindType::%s, OwnedString::makeUnowned("%s"))',
         $tokenKind, $tokenText);
   } else {
      $missingKind = "";
      if ($child->getSyntaxKind() == Kinds::SYNTAX) {
         $missingKind = "Unknown";
      } else {
         $missingKind = $child->getSyntaxKind();
      }
      $nodeChoices = $child->getNodeChoices();
      if (!empty($nodeChoices)) {
         return make_missing_child($nodeChoices[0]);
      }
      return sprintf('RawSyntax::missing(SyntaxKind::%s)', $missingKind);
   }
}