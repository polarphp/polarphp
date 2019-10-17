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
// Created by polarboy on 2019/10/12.
namespace Lit\Shell;

/**
 * An enumeration representing the style of an integrated test keyword or
 * command.
 * TAG: A keyword taking no value. Ex 'END.'
 * COMMAND: A keyword taking a list of shell commands. Ex 'RUN:'
 * LIST: A keyword taking a comma-separated list of values.
 * BOOLEAN_EXPR: A keyword taking a comma-separated list of
 * boolean expressions. Ex 'XFAIL:'
 * CUSTOM: A keyword with custom parsing semantics.
 */
class ParserKind
{
   const TAG = 0;
   const COMMAND = 1;
   const LIST = 2;
   const BOOLEAN_EXPR = 3;
   const CUSTOM = 4;

   public static function allowedKeywordSuffixes(int $value)
   {
      return array(
         self::TAG =>          ['.'],
         self::COMMAND =>      [':'],
         self::LIST =>         [':'],
         self::BOOLEAN_EXPR => [':'],
         self::CUSTOM      => [':', '.']
      )[$value];
   }

   public static function getStr($value)
   {
      return array(
         self::TAG          => 'TAG',
         self::COMMAND      => 'COMMAND',
         self::LIST         => 'LIST',
         self::BOOLEAN_EXPR => 'BOOLEAN_EXPR',
         self::CUSTOM       => 'CUSTOM'
      )[$value];
   }
}