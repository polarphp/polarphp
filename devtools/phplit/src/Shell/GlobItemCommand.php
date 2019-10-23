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
// Created by polarboy on 2019/10/10.

namespace Lit\Shell;

use function Lit\Utils\is_absolute_path;

class GlobItemCommand extends AbstractCommand
{
   /**
    * @var string $pattern
    */
   protected $pattern;

   public function __construct(string $pattern)
   {
      $this->pattern = $pattern;
   }

   public function __toString(): string
   {
      return $this->pattern;
   }

   public function equalWith($other) : bool
   {
      if (!$other instanceof GlobItemCommand) {
         return false;
      }
      return $this->pattern == $other->pattern;
   }

   public function resolve(string $cwd) : array
   {
       //TODO is_absolute_path函数不存在 是否实在类中实现一个函数
      if (is_absolute_path($this->pattern)) {
         $abspath = $this->pattern;
      } else {
         $abspath = $cwd . DIRECTORY_SEPARATOR . $this->pattern;
      }
      $results = glob($abspath);
      return empty($results) ? [$this->pattern] : $results;
   }

   public function toShell($file, $pipeFail = false) : void
   {}
}