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

use Lit\Exception\InternalShellException;
use Lit\Shell\ShellCommandResult;
use Lit\Utils\TestLogger;
use Symfony\Component\Console\Input\ArgvInput;
use Symfony\Component\Console\Input\InputArgument;
use Symfony\Component\Console\Input\InputDefinition;
use Symfony\Component\Console\Input\InputOption;

use Lit\Shell\ShCommandInterface;
use Lit\Utils\ShellEnvironment;
use function Lit\Utils\expand_glob_expressions;
use function Lit\Utils\is_absolute_path;

class MkdirCommand implements BuiltinCommandInterface
{
   /**
    * @var InputDefinition $definitions
    */
   private $definitions;
   public function __construct()
   {
      $definitions = new InputDefinition();
      $definitions->addOption(new InputOption("parent", 'p', InputOption::VALUE_NONE,
         "Create intermediate directories as required."));
      $definitions->addArgument(new InputArgument("dirs", InputArgument::IS_ARRAY, "directories to created"));
      $this->definitions = $definitions;
   }

   public function execute(ShCommandInterface $cmd, ShellEnvironment $shenv)
   {
      try {
         $args = expand_glob_expressions($cmd->getArgs(), $shenv->getCwd());
         $input = new ArgvInput($args, $this->definitions);
      } catch (\Exception $e) {
         throw new InternalShellException($cmd, sprintf("Unsupported: 'mkdir': %s", $e->getMessage()));
      }
      $parent = $input->getOption('parent');
      $dirs = $input->getArgument('dirs');
      if (empty($dirs)) {
         throw new InternalShellException($cmd, "Error: 'mkdir' is missing an operand");
      }
      $exitCode = 0;
      $errorMsg = '';
      $cwd = $shenv->getCwd();
      foreach ($dirs as $dir) {
         if (!is_absolute_path($dir)) {
            $dir = $cwd . DIRECTORY_SEPARATOR . $dir;
         }
         if (file_exists($dir)) {
            continue;
         }
         $result = @mkdir($dir, 0777, $parent);
         if (false === $result) {
            $exitCode = 1;
            $e = error_get_last();
            $errorMsg = sprintf("Error: 'mkdir' command failed, %s\n", $e['message']);
         }
      }
      return new ShellCommandResult($cmd, "", $errorMsg, $exitCode, false);
   }
}