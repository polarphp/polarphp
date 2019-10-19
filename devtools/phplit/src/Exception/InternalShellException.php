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
namespace Lit\Exception;

use Lit\Shell\ShCommand;
use Lit\Shell\ShCommandInterface;
use Throwable;

class InternalShellException extends \RuntimeException
{
   /**
    * @var ShCommandInterface $command
    */
   private $command;

   public function __construct(ShCommandInterface $command, string $message, $code = 0, Throwable $previous = null)
   {
      parent::__construct($message, $code, $previous);
      $this->command = $command;
   }

   /**
    * @return ShCommandInterface
    */
   public function getCommand(): ShCommandInterface
   {
      return $this->command;
   }
}