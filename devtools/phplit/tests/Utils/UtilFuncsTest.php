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
// Created by polarboy on 2019/10/16.

namespace Lit\Tests\Utils;

use function Lit\Utils\path_split;
use PHPUnit\Framework\TestCase;

class UtilFuncsTest extends TestCase
{
   public function testSplitPath()
   {
      $this->assertEquals(['/', ''], path_split('/'));
      $this->assertEquals([], path_split(''));
      $this->assertEquals(['/a', 'b'], path_split('/a/b'));
      $this->assertEquals(['/a/b', ''], path_split('/a/b/'));
   }
}