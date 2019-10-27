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
use Lit\Utils\TestLogger;
use Symfony\Component\Console\Input\ArgvInput;
use Symfony\Component\Console\Input\InputArgument;
use Symfony\Component\Console\Input\InputDefinition;
use Symfony\Component\Console\Input\InputOption;

use Lit\Shell\ShellCommandResult;
use Lit\Shell\ShCommandInterface;
use Lit\Utils\ShellEnvironment;
use Lit\Utils\Filesystem;
use Symfony\Component\Filesystem\Exception\IOException;
use function Lit\Utils\expand_glob_expressions;
use function Lit\Utils\is_absolute_path;

class RmCommand implements BuiltinCommandInterface
{
   /**
    * @var InputDefinition $definitions
    */
   private $definitions;
   public function __construct()
   {
      $definitions = new InputDefinition();
      $definitions->addOption(new InputOption("force", 'f', InputOption::VALUE_NONE,
         "force remove directories"));
      $definitions->addOption(new InputOption("recursive","r", InputOption::VALUE_NONE,
         "recursive remove"));
      $definitions->addArgument(new InputArgument("paths", InputArgument::IS_ARRAY, "paths to remove"));
      $this->definitions = $definitions;
   }
   public function execute(ShCommandInterface $cmd, ShellEnvironment $shenv)
   {
      try {
         $args = expand_glob_expressions($cmd->getArgs(), $shenv->getCwd());
         $input = new ArgvInput($args, $this->definitions);
      } catch (\Exception $e) {
         throw new InternalShellException($cmd, sprintf("Unsupported: 'rm': %s", $e->getMessage()));
      }
      $force = $input->getOption('force');
      $recurive = $input->getOption('recursive');
      $fs = new Filesystem();
      $paths = $input->getArgument('paths');
      if (empty($paths)) {
         throw new InternalShellException($cmd, "Error: 'rm' is missing an operand");
      }
      $errorMsg = '';
      $exitCode = 0;
      try {
         foreach ($paths as $path) {
            if (!is_absolute_path($path)) {
               $path = $shenv->getCwd() . DIRECTORY_SEPARATOR . $path;
            }
            if ($force && !file_exists($path)) {
               continue;
            }
            try {
               if (is_dir($path)) {
                  if (!$recurive) {
                     $errorMsg .= sprintf("Error: %s is a directory\n", $path);
                     $exitCode = 1;
                  } else {
                     $fs->remove($path, true);
                  }
               } else {
                  if ($force && !is_writable($path)) {
                     $fs->chmod($path, 0777);
                  }
                  if (!file_exists($path)) {
                     throw new IOException(sprintf('No such file or directory: %s', $path));
                  }
                  $fs->remove($path);
               }
            } catch (IOException $ioException) {
               if ($force) {
                  $fs->chmod($path, 0777);
                  $fs->remove($path);
               } elseif(!file_exists($path)) {
                  throw new InternalShellException($cmd, $ioException->getMessage());
               }
            }
         }
      } catch (\Exception $e) {
         $errorMsg = sprintf("Error: 'rm' command failed, %s", $e->getMessage());
         $exitCode = 1;
      }
      return new ShellCommandResult($cmd, '', $errorMsg, $exitCode, false);
   }
}