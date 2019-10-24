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
// Created by polarboy on 2019/10/24.
namespace Lit\Tests\Utils;

use Lit\Utils\ShLexer;
use PHPUnit\Framework\TestCase;

class ShLexerTest extends TestCase
{
   private function lex()
   {
      $data = [];
      $args = func_get_args();
      $lexer = new ShLexer(...$args);
      foreach ($lexer->lex() as $item) {
         $data[] = $item;
      }
      return $data;
   }

   public function testBasic()
   {
      $this->assertEquals($this->lex('a|b>c&d<e;f'), ['a', ['|'], 'b', ['>'], 'c', ['&'],
         'd', ['<'], 'e', [';'], 'f']);
   }

   public function testRedirectionTokens()
   {
      $this->assertEquals($this->lex('a2>c'),
         ['a2', ['>'], 'c']);
      $this->assertEquals($this->lex('a 2>c'),
         ['a', ['>', 2], 'c']);
   }

   public function testQuoting()
   {
      $this->assertEquals($this->lex(" 'a' "), ['a']);
      $this->assertEquals($this->lex('"hello\\"world"'), ['hello"world']);
      $this->assertEquals($this->lex('"hello\\\'world"'), ["hello\\'world"]);
      $this->assertEquals($this->lex('"hello\\\\world"'), ["hello\\world"]);
      $this->assertEquals($this->lex(' he"llo wo"rld '), ["hello world"]);
      $this->assertEquals($this->lex(' "" "" '), ["", ""]);
      $this->assertEquals($this->lex(' a\\ b ', true), ['a\\', 'b']);
   }
}