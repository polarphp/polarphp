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

namespace Gyb\Syntax;

use function Gyb\Utils\str_end_with;

class Kinds
{
   const DECL = 'Decl';
   const EXPR = 'Expr';
   const STMT = 'Stmt';
   const SYNTAX = 'Syntax';
   const SYNTAX_COLLECTION = 'SyntaxCollection';
   const TOKEN_SYNTAX = 'TokenSyntax';
   const UNKNOWN = 'Unknown';
   /**
    * @var array $baseKinds
    */
   private static $baseKinds = array(
      self::DECL,
      self::EXPR,
      self::STMT,
      self::SYNTAX,
      self::SYNTAX_COLLECTION
   );

   /**
    * @return array
    */
   public static function getBaseSyntaxKinds(): array
   {
      return self::$baseKinds;
   }

   public static function isValidBaseKind(string $kind): bool
   {
      return in_array($kind, self::$baseKinds);
   }

   /**
    * Converts a SyntaxKind to a type name, checking to see if the kind is
    * Syntax or SyntaxCollection first.
    * A type name is the same as the SyntaxKind name with the suffix "Syntax"
    * added.
    */
   public static function convertKindToType(string $kind): string
   {
      if (in_array($kind, [self::SYNTAX, self::SYNTAX_COLLECTION])) {
         return $kind;
      }
      if (str_end_with($kind, 'Token') || str_end_with($kind, 'Keyword')) {
         return self::TOKEN_SYNTAX;
      }
      return $kind.self::SYNTAX;
   }
}