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
use Lit\Kernel\LitConfig;
use Lit\Shell\ShCommandInterface;
use Lit\Shell\TestRunner;
use Lit\Utils\ShellEnvironment;
use Lit\Shell\Process;
use Symfony\Component\Console\Input\ArgvInput;
use Symfony\Component\Console\Input\InputArgument;
use Symfony\Component\Console\Input\InputDefinition;
use Symfony\Component\Console\Input\InputOption;

class EchoCommand implements BuiltinCommandInterface
{
   /**
    * @var TestRunner $testRunner
    */
   private $testRunner;
   /**
    * @var LitConfig $litConfig
    */
   private $litConfig;

   /**
    * @var InputDefinition $definitions
    */
   private $definitions;

   public function __construct(TestRunner $runner, LitConfig $litConfig)
   {
      $this->testRunner = $runner;
      $this->litConfig = $litConfig;
      $definitions = new InputDefinition();
      $definitions->addOption(new InputOption("escapes", 'e', InputOption::VALUE_NONE,
         "escapes"));
      $definitions->addOption(new InputOption("newline", 'n', InputOption::VALUE_NONE,
         "newline"));
      $definitions->addArgument(new InputArgument("items", InputArgument::IS_ARRAY, "directories to created"));
      $this->definitions = $definitions;
   }

   public function execute(ShCommandInterface $cmd, ShellEnvironment $shenv)
   {
      $openedFiles = [];
      list($stdin, $stdout, $stderr) = $this->testRunner->processRedirects($cmd, Process::REDIRECT_PIPE, $shenv, $openedFiles);
      if ($stdin != Process::REDIRECT_PIPE || $stderr != Process::REDIRECT_PIPE) {
         throw new InternalShellException($cmd, 'stdin and stderr redirects not supported for echo');
      }
      // Some tests have un-redirected echo commands to help debug test failures.
      // Buffer our output and return it to the caller.
      $isRedirected = true;
      if ($stdout == Process::REDIRECT_PIPE) {
         $isRedirected = false;
         $stdout = fopen('php://memory','rwb');
      }
      try {
         $input = new ArgvInput($cmd->getArgs(), $this->definitions);
      } catch (\Exception $e) {
         throw new InternalShellException($cmd, sprintf("Unsupported: 'rm': %s", $e->getMessage()));
      }
      // Implement echo flags. We only support -e and -n, and not yet in
      // combination. We have to ignore unknown flags, because `echo "-D FOO"`
      // prints the dash.
      $interpretEscapes = false;
      $writeNewline = true;
      if ($input->getOption('escapes')) {
         $interpretEscapes = true;
      }
      if ($input->getOption('newline')) {
         $writeNewline = false;
      }
      $items = $input->getArgument('items');
      $count = count($items);
      if ($count > 0) {
         for ($i = 0; $i < $count - 1; ++$i) {
            fwrite($stdout, $this->maybeUnescape($items[$i], $interpretEscapes));
            fwrite($stdout, ' ');
         }
         fwrite($stdout, $this->maybeUnescape($items[$count - 1], $interpretEscapes));
      }
      if ($writeNewline) {
         fwrite($stdout, PHP_EOL);
      }
      foreach ($openedFiles as $entry) {
         list($name, $mode, $f, $path) = $entry;
         fclose($f);
      }
      if (!$isRedirected) {
         rewind($stdout);
         return stream_get_contents($stdout);
      }
      return '';
   }

   private function maybeUnescape(string $str, bool $interpretEscapes)
   {
      if (!$interpretEscapes) {
         return $str;
      }
      return str_replace(
         array('\r', '\t', '\n', '\f'),
         array("\r", "\t", "\n", "\f"), $str);
   }
}