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
// Created by polarboy on 2019/10/09.
namespace Lit\Kernel;
// A simple evaluator of boolean expressions.
//
// Grammar:
//   expr       :: or_expr
//   or_expr    :: and_expr ('||' and_expr)*
//   and_expr   :: not_expr ('&&' not_expr)*
//   not_expr   :: '!' not_expr
//                 '(' or_expr ')'
//                 identifier
//   identifier :: [-+=._a-zA-Z0-9]+

// Evaluates `string` as a boolean expression.
// Returns True or False. Throws a ValueError on syntax error.
//
// Variables in `variables` are true.
// Substrings of `triple` are true.
// 'true' is true.
// All other identifiers are false.
class BooleanExpr
{
   const TOKENIZATION_REGEX = '\A\s*([()]|[-+=._a-zA-Z0-9]+|&&|\|\||!)\s*(.*)\Z';
   /**
    * @var array $tokens
    */
   private $tokens;
   /**
    * @var array $variables
    */
   private $variables;
   /**
    * @var string $triple
    */
   private $triple;
   /**
    * @var mixed $value
    */
   private $value;
   /**
    * @var string $token
    */
   private $token;

   private static function tokenize(string $expr) : iterable
   {
      while (true) {

      }
   }
}