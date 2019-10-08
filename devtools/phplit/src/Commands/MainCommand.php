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
// Created by polarboy on 2019/10/08.

namespace Lit\Commands;

use Symfony\Component\Console\Command\Command;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Output\OutputInterface;
use Symfony\Component\Filesystem\Exception\IOException;
use Symfony\Component\Filesystem\Filesystem;
use Lit\Utils\CmdOpts;

class MainCommand extends Command
{
   protected static $defaultName = 'run-test';
   /**
    * @var CmdOpts $cmdOpts
    */
   private $cmdOpts;

   protected function configure()
   {
      $this->setDescription('run regression tests');
      $this->setHelp('welcome use phplit');
      $this->setupOptions();
      $this->setupArguments();
   }

   private function setupOptions()
   {

   }

   private function setupArguments()
   {

   }

   protected function execute(InputInterface $input, OutputInterface $output)
   {
      $fs = new Filesystem();
      $tempDir = $this->prepareTempDir($fs);
      try {
         $this->doExecute();
      } finally {
         if (!is_null($tempDir)) {
            $tryCount = 3;
            $removeException = null;
            $isOk = false;
            while ($tryCount--) {
               try {
                  $fs->remove($tempDir);
                  $isOk = true;
                  break;
               } catch (IOException $e) {
                  $removeException = $e;
               }
            }
            if (!$isOk) {
               throw $removeException;
            }
         }
      }
   }

   private function doExecute()
   {

   }

   private function prepareTempDir(Filesystem $fs)
   {
      $tempDir = null;
      if (in_array('LIT_PRESERVES_TMP', $_ENV)) {
         $sysTempDir = sys_get_temp_dir();
         $tempDir = @tempnam($sysTempDir, 'lit_tmp_');
         $fs->remove($tempDir);
         $fs->mkdir($tempDir);
      }
      return $tempDir;
   }
}
