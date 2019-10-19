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
// Created by polarboy on 2019/10/18.

namespace Lit\Shell;

abstract class AbstractCommand implements ShCommandInterface
{
   public function getArgs(): array
   {
      return [];
   }

   public function getArgsCount(): int
   {
      return count($this->getArgs());
   }

   public function setArgs(array $args = array())
   {}
}