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

use Lit\Shell\PipelineCommand;
use Lit\Shell\SeqCmmand;
use Lit\Shell\ShCommand;
use Lit\Utils\ShParser;
use PHPUnit\Framework\TestCase;

class ShParserTest extends TestCase
{
   private function parse($str)
   {
      $parser = new ShParser($str);
      return $parser->parse();
   }

   public function testBaisc()
   {
      $this->assertEquals($this->parse('echo hello'), new PipelineCommand(
         [new ShCommand(["echo", "hello"], [])], false
      ));
      $this->assertEquals($this->parse('echo ""'), new PipelineCommand(
         [new ShCommand(["echo", ""], [])], false
      ));
      $this->assertEquals($this->parse("echo -DFOO='a'"), new PipelineCommand(
         [new ShCommand(["echo", "-DFOO=a"], [])], false
      ));
      $this->assertEquals($this->parse('echo -DFOO="a"'), new PipelineCommand(
         [new ShCommand(["echo", "-DFOO=a"], [])], false
      ));
   }

   public function testRedirection()
   {
      $this->assertEquals($this->parse('echo hello > c'), new PipelineCommand(
         [new ShCommand(['echo', 'hello'], [[['>'], 'c']])], false
      ));
      $this->assertEquals($this->parse('echo hello > c >> d'), new PipelineCommand(
         [new ShCommand(['echo', 'hello'], [[['>'], 'c'],[['>>'], 'd']])], false
      ));
      $this->assertEquals($this->parse('a 2>&1'), new PipelineCommand(
         [new ShCommand(['a'], [[['>&', 2], '1']])], false
      ));
   }

   public function testPipeline()
   {
      $this->assertEquals($this->parse('a | b'), new PipelineCommand(
         [
            new ShCommand(['a'], []),
            new ShCommand(['b'], [])
         ], false
      ));

      $this->assertEquals($this->parse('a | b | c'), new PipelineCommand(
         [
            new ShCommand(['a'], []),
            new ShCommand(['b'], []),
            new ShCommand(['c'], [])
         ], false
      ));
   }

   public function testList()
   {
      $this->assertEquals($this->parse('a ; b'),
         new SeqCmmand(
            new PipelineCommand([new ShCommand(['a'], [])], false),
            ';',
            new PipelineCommand([new ShCommand(['b'], [])], false),
         ));

      $this->assertEquals($this->parse('a & b'),
         new SeqCmmand(
            new PipelineCommand([new ShCommand(['a'], [])], false),
            '&',
            new PipelineCommand([new ShCommand(['b'], [])], false),
         ));

      $this->assertEquals($this->parse('a && b'),
         new SeqCmmand(
            new PipelineCommand([new ShCommand(['a'], [])], false),
            '&&',
            new PipelineCommand([new ShCommand(['b'], [])], false),
         ));

      $this->assertEquals($this->parse('a || b'),
         new SeqCmmand(
            new PipelineCommand([new ShCommand(['a'], [])], false),
            '||',
            new PipelineCommand([new ShCommand(['b'], [])], false),
         ));

      $this->assertEquals($this->parse('a && b || c'),
         new SeqCmmand(
            new SeqCmmand(
               new PipelineCommand([new ShCommand(['a'], [])], false),
               '&&',
               new PipelineCommand([new ShCommand(['b'], [])], false),
            ),
            '||',
            new PipelineCommand([new ShCommand(['c'], [])], false),
         ));

      $this->assertEquals($this->parse('a; b'),
         new SeqCmmand(
            new PipelineCommand([new ShCommand(['a'], [])], false),
            ';',
            new PipelineCommand([new ShCommand(['b'], [])], false),
         ));
   }
}