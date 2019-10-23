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

namespace Lit\Shell\BuiltinCommand;

use Lit\Exception\ValueException;
use Lit\Shell\ShCommandInterface;
use Lit\Utils\ShellEnvironment;
use function Lit\Utils\is_absolute_path;

class CdCommand implements BuiltinCommandInterface
{
   public function execute(ShCommandInterface $cmd, ShellEnvironment $shenv)
   {
      if ($cmd->getArgsCount() != 2) {
         throw new ValueException("'cd' supports only one argument");
      }
      $args = $cmd->getArgs();
      $newDir = $args[1];
      // Update the cwd in the parent environment.
      if (is_absolute_path($newDir)) {
         $shenv->setCwd($newDir);
      } else {
         $shenv->setCwd($shenv->getCwd().DIRECTORY_SEPARATOR.$newDir);
      }
      // The cd builtin always succeeds. If the directory does not exist, the
      // following Popen calls will fail instead.
      return 0;
   }
}