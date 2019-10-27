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
// Created by polarboy on 2019/10/16.

namespace Lit\Commands;

use Lit\ProcessPool\TaskExecutor;
use Symfony\Component\Console\Command\Command;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Logger\ConsoleLogger;
use Symfony\Component\Console\Output\OutputInterface;

class TestWorkerCommand extends Command
{
   protected static $defaultName = 'run-worker';

   protected function execute(InputInterface $input, OutputInterface $output)
   {
      $logger = new ConsoleLogger($output);
      $stdin = fopen( 'php://stdin', 'r' );
      $execText = stream_get_contents($stdin);
      fclose($stdin);
      $execText = trim($execText);
      if (empty($execText)) {
         $logger->error("task executable text empty");
         exit(TaskExecutor::E_TASK_TEXT_EMPTY);
      }
      $data = unserialize($execText);
      if (false === $data) {
         $logger->error("task executable text unserialize error");
         exit(TaskExecutor::E_TASK_UNSERIALIZED);
      }
      $initializer = $data['initializer'];
      $task = $data['task'];
      try {
         $initializer->init();
         $executor = new TaskExecutor($task);
         fwrite(STDOUT, $executor->exec());
         exit(0);
      } catch (\Exception $e) {
         $logger->error($e->getMessage());
         exit($e->getCode());
      }
   }
}