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
// Created by polarboy on 2019/10/28.

namespace Gyb\Kernel;

use Gyb\Syntax\SyntaxRegistry;

class Generator
{
   /**
    * @var resource $input
    */
   private $input;
   /**
    * @var resource $output
    */
   private $output;

   /**
    * Generator constructor.
    * @param resource $input
    * @param resource $output
    */
   public function __construct($input, $output)
   {
      $this->input = $input;
      $this->output = $output;
   }

   public function generate()
   {
      $tpl = $this->preprocessTemplate();
      if (empty($tpl)) {
         return;
      }
      $this->prepareEnvironment();
      $this->executeTpl($tpl);
      $this->writeOutput($tpl);
   }

   private function preprocessTemplate(): string
   {
      $tpl = stream_get_contents($this->input);
      if ($tpl === false) {
         throw new \RuntimeException(sprintf('read input stream error: %s', error_get_last()['message']));
      }
      return '';
   }

   private function prepareEnvironment()
   {
      $this->loadSyntaxNodes();
   }

   private function loadSyntaxNodes()
   {
      $baseDir = GYB_SYNTAX_DEFINITION_DIR;
      SyntaxRegistry::setCommonNodes(include $baseDir.DIRECTORY_SEPARATOR.'CommonNodes.php');
      SyntaxRegistry::setDeclNodes(include $baseDir.DIRECTORY_SEPARATOR.'DeclNodes.php');
      SyntaxRegistry::setExprNodes(include $baseDir.DIRECTORY_SEPARATOR.'ExprNodes.php');
      SyntaxRegistry::setStmtNodes(include $baseDir.DIRECTORY_SEPARATOR.'StmtNodes.php');
      SyntaxRegistry::setTokens(include $baseDir.DIRECTORY_SEPARATOR.'Tokens.php');

   }

   private function executeTpl(string &$tpl): void
   {

   }

   private function writeOutput(string &$tpl): void
   {
      $result = fwrite($this->output, $tpl);
      if ($result === false) {
         throw new \RuntimeException(sprintf("write output error: %s", error_get_last()['message']));
      }
   }
}