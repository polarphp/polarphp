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
namespace Lit\Shell;

class ShellCommandResult
{
   /**
    * @var string $command
    */
   private $command;
   /**
    * @var string $stdout
    */
   private $stdout;
   /**
    * @var string $stderr
    */
   private $stderr;
   /**
    * @var int $exitCode
    */
   private $exitCode;
   /**
    * @var bool $timeoutReached
    */
   private $timeoutReached;
   /**
    * @var array $outputFiles
    */
   private $outputFiles;

   /**
    * ShellCommandResult constructor.
    * @param ShCommandInterface $command
    * @param string $stdout
    * @param string $stderr
    * @param int $exitCode
    * @param bool $timeoutReached
    * @param array $outputFiles
    */
   public function __construct(ShCommandInterface $command, string $stdout, string $stderr,
                               int $exitCode, bool $timeoutReached, array $outputFiles = array())
   {
      $this->command = $command;
      $this->stdout = $stdout;
      $this->stderr = $stderr;
      $this->exitCode = $exitCode;
      $this->timeoutReached = $timeoutReached;
      $this->outputFiles = $outputFiles;
   }

   /**
    * @return ShCommandInterface
    */
   public function getCommand(): ShCommandInterface
   {
      return $this->command;
   }

   /**
    * @return string
    */
   public function getStdout(): string
   {
      return $this->stdout;
   }

   /**
    * @return string
    */
   public function getStderr(): string
   {
      return $this->stderr;
   }

   /**
    * @return int
    */
   public function getExitCode(): int
   {
      return $this->exitCode;
   }

   /**
    * @return bool
    */
   public function isTimeoutReached(): bool
   {
      return $this->timeoutReached;
   }

   /**
    * @return array
    */
   public function getOutputFiles(): array
   {
      return $this->outputFiles;
   }
}