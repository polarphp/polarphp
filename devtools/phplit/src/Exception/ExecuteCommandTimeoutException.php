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
// Created by polarboy on 2019/10/11.

namespace Lit\Exception;

class ExecuteCommandTimeoutException extends \RuntimeException
{
   /**
    * @var string $outMsg
    */
   private $outMsg;
   /**
    * @var string $errorMsg
    */
   private $errorMsg;
   /**
    * @var int $exitCode
    */
   private $exitCode;

   public function __construct(string $msg, string $outMsg, string $errorMsg, int $exitCode)
   {
      parent::__construct($msg, 0, null);
      $this->outMsg = $outMsg;
      $this->errorMsg = $errorMsg;
      $this->exitCode = $exitCode;
   }

   /**
    * @return string
    */
   public function getOutMsg(): string
   {
      return $this->outMsg;
   }

   /**
    * @return string
    */
   public function getErrorMsg(): string
   {
      return $this->errorMsg;
   }

   /**
    * @return int
    */
   public function getExitCode(): int
   {
      return $this->exitCode;
   }
}