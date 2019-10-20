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
use function Lit\Utils\update_shell_env;

class ExportCommand implements BuiltinCommandInterface
{
   public function execute(ShCommandInterface $cmd, ShellEnvironment $shenv)
   {
      if ($cmd->getArgsCount() != 2) {
         throw new ValueException("'export' supports only one argument");
      }
      update_shell_env($shenv, $cmd);
      return 0;
   }
}