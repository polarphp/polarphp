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

class PipelineCommand implements ShCommandInterface
{
   /**
    * @var array $commands
    */
   private $commands;
   /**
    * @var bool $negate
    */
   private $negate;
   /**
    * @var bool $pipeError
    */
   private $pipeError;

   public function __construct(array $commands, bool $negate = false, bool $pipeError = false)
   {
      $this->commands = $commands;
      $this->negate = $negate;
      $this->pipeError = $pipeError;
   }

   public function __toString() : string
   {
       //todo $this->args && $this->redirects 并不存在
       return sprintf('Pipeline(%s, %s, %s)', var_export($this->args, true), var_export($this->redirects, true));
   }

   public function equalWith($other) : bool
   {
      if (!$other instanceof PipelineCommand) {
         return false;
      }
      return $this->commands === $other->commands &&
         $this->negate == $other->negate &&
         $this->pipeError == $other->pipeError;
   }

   public function toShell($file, $pipeFail = false) : void
   {
      if ($pipeFail != $this->pipeError) {
         throw new ValueException('Inconsistent "pipefail" attribute!');
      }
      if ($this->negate) {
         fwrite($file, '! ');
      }
      $commandCount = count($this->commands);
      foreach ($this->commands as $i => $command) {
         $command->toShell($file);
         if ($i < $commandCount - 1) {
            fwrite($file, "|\n  ");
         }
      }
   }
}