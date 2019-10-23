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
// Created by polarboy on 2019/10/12.
namespace Lit\Utils;
use Lit\Shell\Process;

/**
 * Object used to helper manage enforcing a timeout in
 * _executeShCmd(). It is passed through recursive calls
 * to collect processes that have been executed so that when
 * the timeout happens they can be killed.
 *
 * @package Lit\Utils
 */
class TimeoutHelper
{
   /**
    * @var int $timeout
    */
   private $timeout;
   /**
    * @var \Symfony\Component\Process\Process[] $processes
    */
   private $processes = array();
   /**
    * @var bool $timeoutReached
    */
   private $timeoutReached = false;
   /**
    * @var bool $doneKillPass
    */
   private $doneKillPass = false;

   public function __construct(int $timeout)
   {
      $this->timeout = $timeout;
      pcntl_async_signals(true);
   }

   public function cancel()
   {
      if (!$this->active()) {
         return;
      }
      pcntl_alarm(0);
   }

   public function active()
   {
      return $this->timeout > 0;
   }

   public function addProcess(Process $process)
   {
      if (!$this->active()) {
         return;
      }
      $this->processes[] = $process;
      // Avoid re-entering the lock by finding out if kill needs to be run
      // again here but call it if necessary once we have left the lock.
      // We could use a reentrant lock here instead but this code seems
      // clearer to me.
      if ($this->doneKillPass) {
         assert($this->timeoutReached());
         $this->kill();
      }
   }

   public function startTimer()
   {
      if (!$this->active()) {
         return;
      }
      \pcntl_signal(SIGALRM, array($this, 'handleTimeoutReached'));
      // Do some late initialisation that's only needed
      // if there is a timeout set
      \pcntl_alarm($this->timeout);
   }

   public function timeoutReached()
   {
      return $this->timeoutReached;
   }

   public function handleTimeoutReached()
   {
      $this->timeoutReached = true;
      $this->kill();
   }

   private function kill()
   {
      foreach ($this->processes as $process) {
         kill_process_and_children($process->getPid());
      }
      $this->processes = array();
      $this->doneKillPass = true;
   }
}