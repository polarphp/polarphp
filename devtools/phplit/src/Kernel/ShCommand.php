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

namespace Lit\Kernel;

class ShCommand implements ShCommandInterface
{
   /**
    * @var array $args
    */
   protected $args;
   /**
    * @var array $redirects
    */
   protected $redirects;

   public function __toString() : string
   {
      return sprintf('Command(%s, %s)', var_export($this->args, true), var_export($this->redirects, true));
   }

   public function equalWith($other) : bool
   {
      if (!$other instanceof ShCommand) {
         return false;
      }
      return $this->args === $other->args &&
         $this->redirects === $other->redirects;
   }

   public function toShell($file, $pipeFail = false) : void
   {
   }
}