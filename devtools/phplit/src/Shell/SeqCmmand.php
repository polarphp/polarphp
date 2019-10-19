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

class SeqCmmand extends AbstractCommand
{
   /**
    * @var string $op
    */
   private $op;
   /**
    * @var ShCommandInterface $lhs
    */
   private $lhs;
   /**
    * @var ShCommandInterface $rhs
    */
   private $rhs;

   public function __construct(ShCommandInterface $lhs, string $op, ShCommandInterface $rhs)
   {
      assert(in_array($op, [';', '&', '||', '&&']));
      $this->lhs = $lhs;
      $this->op = $op;
      $this->rhs = $rhs;
   }

   public function __toString() : string
   {
       //todo $this->args && $this->redirects 并不存在
      return sprintf('Seq(%s, %s, %s)', var_export($this->args, true), var_export($this->redirects, true));
   }

   public function equalWith($other) : bool
   {
      if (!$other instanceof SeqCmmand) {
         return false;
      }
      return $this->op === $other->op &&
         $this->lhs->equalWith($other->lhs) && $this->rhs->equalWith($other->rhs);
   }

   public function toShell($file, $pipeFail = false) : void
   {
      $this->lhs->toShell($file, $pipeFail);
      fwrite($file, sprintf(" %s\n", $this->op));
      $this->rhs->toShell($file, $pipeFail);
   }

   /**
    * @return string
    */
   public function getOp(): string
   {
      return $this->op;
   }

   /**
    * @return ShCommandInterface
    */
   public function getLhs(): ShCommandInterface
   {
      return $this->lhs;
   }

   /**
    * @return ShCommandInterface
    */
   public function getRhs(): ShCommandInterface
   {
      return $this->rhs;
   }
}