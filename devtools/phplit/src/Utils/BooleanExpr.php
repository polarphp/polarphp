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
namespace Lit\Utils;

use Lit\Exception\ValueException;

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
   const TOKENIZATION_REGEX = '/\A\s*([()]|[-+=._a-zA-Z0-9]+|&&|\|\||!)\s*(.*)\Z/';
   const END_TOKEN_MARK = '__END_MARK__';
   /**
    * @var iterable $tokens
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

   public function __construct(string $expr, array $variables, string $triple = '')
   {
      $this->tokens = self::tokenize($expr);
      $this->variables = $variables;
      $this->variables[] = 'true';
      $this->triple = $triple;
   }

   public static function evaluate(string $expr, array $variables, string $triple = '')
   {
      try {
         $parser = new self($expr, $variables, $triple);
         return $parser->parseAllExpr();
      } catch (ValueException $e) {
         throw new ValueException($e->getMessage() . sprintf("\nin expression: %s", var_export($expr, true)));
      }
   }

   private function quote(string $token) : string
   {
      if ($token == self::END_TOKEN_MARK) {
         return '<end of expression>';
      } else {
         return var_export($token, true);
      }
   }

   private function accept(string $token) : bool
   {
      if ($this->token == $token) {
         $this->token = $this->tokens->current();
         $this->tokens->next();
         return true;
      }
      return false;
   }

   private function expect(string $token) : void
   {
      if ($this->token == $token) {
         if ($this->token != self::END_TOKEN_MARK) {
            $this->token = $this->tokens->current();
            $this->tokens->next();
         }
      } else {
         throw new ValueException(sprintf("expected: %s\nhave: %s", self::quote($token), self::quote($this->token)));
      }
   }

   private function isIdentifier(string $token) : bool
   {
      if ($token == self::END_TOKEN_MARK || $token == '&&' || $token == '||' ||
          $token == '!' || $token == '(' || $token == ')') {
         return false;
      }
      return true;
   }

   private function parseNotExpr() : void
   {
      if ($this->accept('!')) {
         $this->parseNotExpr();
         $this->value = !$this->value;
      } elseif($this->accept('(')) {
         $this->parseOrExpr();
         $this->expect(')');
      } elseif (!$this->isIdentifier($this->token)) {
         throw new ValueException(sprintf("expected: '!' or '(' or identifier\nhave: %s", self::quote($this->token)));
      } else {
         $this->value = in_array($this->token, $this->variables) || $this->isSubStr($this->triple, $this->token);
         $this->token = $this->tokens->current();
         $this->tokens->next();
      }
   }

   private function isSubStr(string $haystack, string $needle) : bool
   {
      return strpos($haystack, $needle) === false ? false : true;
   }

   private function parseAndExpr() : void
   {
      $this->parseNotExpr();
      while ($this->accept('&&')) {
         $left = $this->value;
         $this->parseNotExpr();
         $right = $this->value;
         // this is technically the wrong associativity, but it
         // doesn't matter for this limited expression grammar
         $this->value = $left && $right;
      }
   }

   private function parseOrExpr() : void
   {
      $this->parseAndExpr();
      while ($this->accept('||')) {
         $left = $this->value;
         $this->parseAndExpr();
         $right = $this->value;
         // this is technically the wrong associativity, but it
         // doesn't matter for this limited expression grammar
         $this->value = $left || $right;
      }
   }

   private function parseAllExpr()
   {
      $this->token = $this->tokens->current();
      $this->tokens->next();
      $this->parseOrExpr();
      $this->expect(self::END_TOKEN_MARK);
      return $this->value;
   }

   private static function tokenize(string $expr) : iterable
   {
      while (true) {
         preg_match(self::TOKENIZATION_REGEX, $expr, $matches);
         if (empty($matches)) {
            $expr = trim($expr);
            if (strlen($expr) == 0) {
               yield self::END_TOKEN_MARK;
               return;
            } else {
               throw new ValueException(sprintf("couldn't parse text: %s", var_export($expr, true)));
            }
         }
         $token = $matches[1];
         $expr = $matches[2];
         yield $token;
      }
   }
}