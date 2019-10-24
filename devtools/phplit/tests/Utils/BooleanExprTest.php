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
namespace Lit\Tests\Utils;

use Lit\Utils\BooleanExpr;
use Lit\Exception\ValueException;
use PHPUnit\Framework\TestCase;

class BooleanExprTest extends TestCase
{
   public function testVariables()
   {
      $variables = array(
         'its-true', 'false-lol-true', 'under_score',
         'e=quals', 'd1g1ts'
      );
      $this->assertTrue(BooleanExpr::evaluate('true', $variables));
      $this->assertTrue(BooleanExpr::evaluate('its-true', $variables));
      $this->assertTrue(BooleanExpr::evaluate('false-lol-true', $variables));
      $this->assertTrue(BooleanExpr::evaluate('under_score', $variables));
      $this->assertTrue(BooleanExpr::evaluate('e=quals', $variables));
      $this->assertTrue(BooleanExpr::evaluate('d1g1ts', $variables));

      $this->assertFalse(BooleanExpr::evaluate('false', $variables));
      $this->assertFalse(BooleanExpr::evaluate('True', $variables));
      $this->assertFalse(BooleanExpr::evaluate('true-ish', $variables));
      $this->assertFalse(BooleanExpr::evaluate('not_true', $variables));
      $this->assertFalse(BooleanExpr::evaluate('tru', $variables));
   }

   public function testTriple()
   {
      $triple = 'arch-vendor-os';
      $this->assertTrue(BooleanExpr::evaluate('arch', [], $triple));
      $this->assertTrue(BooleanExpr::evaluate('ar', [], $triple));
      $this->assertTrue(BooleanExpr::evaluate('ch-vend', [], $triple));
      $this->assertTrue(BooleanExpr::evaluate('-vendor-', [], $triple));
      $this->assertTrue(BooleanExpr::evaluate('-os', [], $triple));
      $this->assertFalse(BooleanExpr::evaluate('arch-os', [], $triple));
   }

   public function testOperators()
   {
      $this->assertTrue(BooleanExpr::evaluate('true || true', []));
      $this->assertTrue(BooleanExpr::evaluate('true || false', []));
      $this->assertTrue(BooleanExpr::evaluate('false || true', []));
      $this->assertFalse(BooleanExpr::evaluate('false || false', []));

      $this->assertTrue(BooleanExpr::evaluate('true && true', []));
      $this->assertFalse(BooleanExpr::evaluate('true && false', []));
      $this->assertFalse(BooleanExpr::evaluate('false && true', []));
      $this->assertFalse(BooleanExpr::evaluate('false && false', []));

      $this->assertFalse(BooleanExpr::evaluate('!true', []));
      $this->assertTrue(BooleanExpr::evaluate('!false', []));

      $this->assertTrue(BooleanExpr::evaluate('   ((!((false) ))   ) ', []));
      $this->assertTrue(BooleanExpr::evaluate('true && (true && (true))', []));
      $this->assertTrue(BooleanExpr::evaluate('!false && !false && !! !false', []));
      $this->assertTrue(BooleanExpr::evaluate('false && false || true', []));
      $this->assertTrue(BooleanExpr::evaluate('(false && false) || true', []));
      $this->assertFalse(BooleanExpr::evaluate('false && (false || true)', []));
   }

   public function testErrors()
   {
      $this->checkException('ba#d', "couldn't parse text: '#d'\nin expression: 'ba#d'");
      $this->checkException('true and true', "expected: <end of expression>\nhave: 'and'\nin expression: 'true and true'");
      $this->checkException('|| true', "expected: '!' or '(' or identifier\nhave: '||'\nin expression: '|| true'");
      $this->checkException('true &&', "expected: '!' or '(' or identifier\nhave: <end of expression>\nin expression: 'true &&'");
      $this->checkException('', "expected: '!' or '(' or identifier\nhave: <end of expression>\nin expression: ''");
      $this->checkException('*', "couldn't parse text: '*'\nin expression: '*'");
      $this->checkException('no wait stop', "expected: <end of expression>\nhave: 'wait'\nin expression: 'no wait stop'");
      $this->checkException('no-$-please', "couldn't parse text: '$-please'\nin expression: 'no-$-please'");
      $this->checkException('(((true && true) || true)', "expected: ')'\nhave: <end of expression>\nin expression: '(((true && true) || true)'");
      $this->checkException('true (true)', "expected: <end of expression>\nhave: '('\nin expression: 'true (true)'");
      $this->checkException('( )', "expected: '!' or '(' or identifier\nhave: ')'\nin expression: '( )'");
   }

   private function checkException(string $expr, $errorMsg)
   {
      try {
         BooleanExpr::evaluate($expr, []);
         $this->fail(sprintf('expression %s didn\'t cause an exception', $expr));
      } catch (ValueException $e) {
         if (false === strpos($e->getMessage(), $errorMsg)) {
            $this->fail(sprintf(join("\n", array(
               "expression %s caused the wrong ValueException",
               "actual error was:\n%s",
               "expected error was:\n%s"
            )), $expr, $e->getMessage(), $errorMsg));
         } else {
            $this->assertTrue(true);
         }
      } catch (\Exception $e) {
         $this->fail(sprintf("expression %s caused the wrong exception; actual exception was: \n%s", $e->getMessage(), $errorMsg));
      }
   }
}