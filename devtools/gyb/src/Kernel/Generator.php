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

use Gyb\Syntax\Child;
use Gyb\Syntax\Node;
use Gyb\Syntax\SyntaxRegistry;
use Gyb\Utils\Logger;
use http\Exception\RuntimeException;
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
         '$SYNTAX_NODE_MAP',
         '$TRIVIAS',
         '$SYNTAX_NODE_SERIALIZATION_CODES',
         '$SYNTAX_BASE_KINDS'
      ];
      $replacements = [
         '\Gyb\Syntax\SyntaxRegistry::getTokens()',
         '\Gyb\Syntax\SyntaxRegistry::getTokenMap()',
         '\Gyb\Syntax\SyntaxRegistry::getCommonNodes()',
         '\Gyb\Syntax\SyntaxRegistry::getDeclNodes()',
         '\Gyb\Syntax\SyntaxRegistry::getExprNodes()',
         '\Gyb\Syntax\SyntaxRegistry::getStmtNodes()',
         '\Gyb\Syntax\SyntaxRegistry::getSyntaxNodes()',
         '\Gyb\Syntax\SyntaxRegistry::getSyntaxNodeMap()',
         '\Gyb\Syntax\SyntaxRegistry::getTrivias()',
         '\Gyb\Syntax\SyntaxRegistry::getSyntaxNodeSerializationCodes()',
         '\Gyb\Syntax\Kinds::getBaseSyntaxKinds()'
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
      SyntaxRegistry::registerTrivias(include $baseDir.DIRECTORY_SEPARATOR."Trivias.php");
      SyntaxRegistry::registerSyntaxNodeSerializationCodes(include $baseDir.DIRECTORY_SEPARATOR.'SyntaxNodeSerializationCodes.php');
      SyntaxRegistry::registerTokens($this->createTokens(include $baseDir.DIRECTORY_SEPARATOR.'Tokens.php'));
      SyntaxRegistry::registerCommonNodes($this->createSyntaxNodes(include $baseDir.DIRECTORY_SEPARATOR.'CommonNodes.php'));
      SyntaxRegistry::registerDeclNodes($this->createSyntaxNodes(include $baseDir.DIRECTORY_SEPARATOR.'DeclNodes.php'));
      SyntaxRegistry::registerExprNodes($this->createSyntaxNodes(include $baseDir.DIRECTORY_SEPARATOR.'ExprNodes.php'));
      SyntaxRegistry::registerStmtNodes($this->createSyntaxNodes(include $baseDir.DIRECTORY_SEPARATOR.'StmtNodes.php'));
   }

   private function executeTpl(string $executableFile): string
   {
      $fs = new Filesystem();
      ob_start();
      try {
         include $executableFile;
      } catch (\Exception $e) {
         ob_get_clean();
         $fs->remove($executableFile);
         throw $e;
      }
      $result = ob_get_clean();
      $fs->remove($executableFile);
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
         $createArgs[] = $this->createChildNodes($definition['children']);
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
      $children = [];
      foreach ($definitions as $definition) {
         if (!is_array($definition)) {
            Logger::error(var_export($definitions, true));
         }
         $children[] = $this->createChildNodeFromDefinition($definition);
      }
      return $children;
   }

   private function createChildNodeFromDefinition(array $definition)
   {
      try {
         ensure_require_keys($definition, ['name', 'kind']);
      } catch (\RuntimeException $e) {
         Logger::error(var_export($definition, true));
         throw $e;
      }
      $createArgs = [
         $definition['name'],
         $definition['kind'],
      ];
      // description
      if (isset($definition['description']) && is_string($definition['description'])) {
         $createArgs[] = $definition['description'];
      } else {
         $createArgs[] = '';
      }
      // isOptional
      if (isset($definition['isOptional']) && is_bool($definition['isOptional'])) {
         $createArgs[] = $definition['isOptional'];
      } else {
         $createArgs[] = false;
      }
      // tokenChoices
      if (isset($definition['tokenChoices']) && is_array($definition['tokenChoices'])) {
         $createArgs[] = $definition['tokenChoices'];
      } else {
         $createArgs[] = [];
      }
      // textChoices
      if (isset($definition['textChoices']) && is_array($definition['textChoices'])) {
         $createArgs[] = $definition['textChoices'];
      } else {
         $createArgs[] = [];
      }
      // nodeChoices
      if (isset($definition['nodeChoices']) && is_array($definition['nodeChoices'])) {
         $createArgs[] = $this->createChildNodes($definition['nodeChoices']);
      } else {
         $createArgs[] = [];
      }
      // collectionElementName
      if (isset($definition['collectionElementName']) && is_string($definition['collectionElementName'])) {
         $createArgs[] = $definition['collectionElementName'];
      } else {
         $createArgs[] = '';
      }
      // classification
      if (isset($definition['classification']) && is_string($definition['classification'])) {
         $createArgs[] = $definition['classification'];
      } else {
         $createArgs[] = null;
      }
      // isOptional
      if (isset($definition['forceClassification']) && is_bool($definition['forceClassification'])) {
         $createArgs[] = $definition['forceClassification'];
      } else {
         $createArgs[] = false;
      }
      $child = new Child(...$createArgs);
      return $child;
   }

   /**
    * @param array $tokenDefinitions
    */
   private function createTokens(array $tokenDefinitions)
   {
      $children = [];
      foreach ($tokenDefinitions as $definition) {
         if (!is_array($definition)) {
            Logger::error(var_export($tokenDefinitions, true));
         }
         $children[] = $this->createToken($definition);
      }
      return $children;
   }

   private function createToken(array $definition)
   {
      try {
         ensure_require_keys($definition, ['class', 'name', 'kind', 'text', 'serializationCode']);
         $class = $definition['class'];
         if (!in_array($class, ['InternalKeyword', 'Keyword', 'DeclKeyword',
            'StmtKeyword', 'ExprKeyword', 'Punctuator', 'Misc'])) {
            throw new RuntimeException("class type: $class is not supported.");
         }
      } catch (\RuntimeException $e) {
         Logger::error(var_export($definition, true));
         throw $e;
      }
      $createArgs = [
         $definition['name'],
         $definition['kind'],
         $definition['text'],
         $definition['serializationCode'],
      ];
      $classification = 'None';
      if (\Gyb\Utils\has_substr($class, 'Keyword')) {
         $classification = 'Keyword';
      }
      if ($class == 'Misc') {
         $valueType = '';
         if (isset($definition['valueType']) && is_string($definition['valueType'])) {
            $valueType = $definition['valueType'];
         }
         $createArgs[] = $valueType;
      }
      if (!isset($definition['classification']) || !is_string($definition['classification'])) {
         $createArgs[] = $classification;
      } else {
         $createArgs[] = $definition['classification'];
      }
      $class = '\\Gyb\\Syntax\\Token\\'.$class;
      $token = new $class(...$createArgs);
      return $token;
   }
}