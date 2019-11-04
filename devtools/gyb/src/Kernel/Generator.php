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

use Gyb\Syntax\Node;
use Gyb\Syntax\SyntaxRegistry;
use Gyb\Utils\Logger;
use Symfony\Component\Filesystem\Filesystem;
use function Gyb\Utils\ensure_require_keys;

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
      $executableFile = $this->preprocessTemplate();
      $this->prepareEnvironment();
      $result = $this->executeTpl($executableFile);
      $this->writeOutput($result);
   }

   private function preprocessTemplate(): string
   {
      $tpl = stream_get_contents($this->input);
      $executeFile = GYB_TEMP_DIR .DIRECTORY_SEPARATOR. 'tpl_compiled_'. md5($tpl).'.php';
      if ($tpl === false) {
         throw new \RuntimeException(sprintf('read input stream error: %s', error_get_last()['message']));
      }
      $searches = [
         '$TOKENS',
         '$TOKEN_MAP',
         '$COMMON_NODES',
         '$DECL_NODES',
         '$EXPR_NODES',
         '$STMT_NODES',
         '$SYNTAX_NODES',
         '$TRIVIAS',
         '$SYNTAX_NODE_SERIALIZATION_CODES'
      ];
      $replacements = [
         '\Gyb\Syntax\SyntaxRegistry::getTokens()',
         '\Gyb\Syntax\SyntaxRegistry::getTokenMap()',
         '\Gyb\Syntax\SyntaxRegistry::getCommonNodes()',
         '\Gyb\Syntax\SyntaxRegistry::getDeclNodes()',
         '\Gyb\Syntax\SyntaxRegistry::getExprNodes()',
         '\Gyb\Syntax\SyntaxRegistry::getStmtNodes()',
         '\Gyb\Syntax\SyntaxRegistry::getSyntaxNodes()',
         '\Gyb\Syntax\SyntaxRegistry::getTrivias()',
         '\Gyb\Syntax\SyntaxRegistry::getSyntaxNodeSerializationCodes()'
      ];
      $tpl = str_replace($searches, $replacements, $tpl);
      $fs = new Filesystem;
      $fs->dumpFile($executeFile, $tpl);
      return $executeFile;
   }

   private function prepareEnvironment()
   {
      $this->loadSyntaxNodes();
   }

   private function loadSyntaxNodes()
   {
      $baseDir = GYB_SYNTAX_DEFINITION_DIR;
      SyntaxRegistry::registerSyntaxNodeSerializationCodes(include $baseDir.DIRECTORY_SEPARATOR.'SyntaxNodeSerializationCodes.php');
      SyntaxRegistry::registerCommonNodes($this->createSyntaxNodes(include $baseDir.DIRECTORY_SEPARATOR.'CommonNodes.php'));
      SyntaxRegistry::registerDeclNodes($this->createSyntaxNodes(include $baseDir.DIRECTORY_SEPARATOR.'DeclNodes.php'));
      SyntaxRegistry::registerExprNodes($this->createSyntaxNodes(include $baseDir.DIRECTORY_SEPARATOR.'ExprNodes.php'));
      SyntaxRegistry::registerStmtNodes($this->createSyntaxNodes(include $baseDir.DIRECTORY_SEPARATOR.'StmtNodes.php'));
      SyntaxRegistry::registerTokens(include $baseDir.DIRECTORY_SEPARATOR.'Tokens.php');
   }

   private function executeTpl(string $executableFile): string
   {
      ob_start();
      include $executableFile;
      $result = ob_get_clean();
      return $result;
   }

   private function writeOutput(string $data): void
   {
      $result = fwrite($this->output, $data);
      if ($result === false) {
         throw new \RuntimeException(sprintf("write output error: %s", error_get_last()['message']));
      }
   }

   private function createSyntaxNodes(array $definitions)
   {
      $nodes = [];
      foreach ($definitions as $definition) {
         $nodes[] = $this->createSyntaxNodeFromDefinition($definition);
      }
      return $nodes;
   }

   private function createSyntaxNodeFromDefinition(array $definition)
   {
      try {
         ensure_require_keys($definition, ['kind', 'baseKind']);
      } catch (\RuntimeException $e) {
         Logger::error(var_export($definition, true));
         throw $e;
      }
      $createArgs = [
         $definition['kind'],
         $definition['baseKind'],
      ];
      if (isset($definition['description']) && is_string($definition['description'])) {
         $createArgs[] = $definition['description'];
      } else {
         $createArgs[] = '';
      }
      // traits
      $createArgs[] = [];
      // children
      if (isset($definition['children']) && is_array($definition['children'])) {
         $createArgs[] = $this->createChildNodes($definition);
      } else {
         $createArgs[] = [];
      }
      // elementKind
      if (isset($definition['elementKind'])) {
         $createArgs[] = $definition['elementKind'];
      } else {
         $createArgs[] = '';
      }
      // elementName
      if (isset($definition['elementName'])) {
         $createArgs[] = $definition['elementName'];
      } else {
         $createArgs[] = '';
      }
      // elementChoices
      if (isset($definition['elementChoices'])) {
         $createArgs[] = $definition['elementChoices'];
      } else {
         $createArgs[] = [];
      }
      // omitWhenEmpty
      if (isset($definition['omitWhenEmpty'])) {
         $createArgs[] = $definition['omitWhenEmpty'];
      } else {
         $createArgs[] = false;
      }
      $node = new Node(...$createArgs);
      return $node;
   }

   private function createChildNodes(array $definitions): array
   {
      return [];
   }

   private function createChildNodeFromDefinition(array $definition): array
   {

   }
}