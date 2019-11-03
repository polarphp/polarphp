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
   foreach ($items as $item) {
      $item['baseKind'] = 'SyntaxCollection';
   }
   return $items;
}