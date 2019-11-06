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

namespace Gyb\Syntax;

class SyntaxRegistry
{
   /**
    * @var Token[] $tokens
    */
   private static $tokens = [];
   /**
    * @var array $tokenMap
    */
   private static $tokenMap = [];
   /**
    * @var array $commonNodes
    */
   private static $commonNodes = [];
   /**
    * @var array $declNodes
    */
   private static $declNodes = [];
   /**
    * @var array $exprNodes
    */
   private static $exprNodes = [];
   /**
    * @var array $stmtNodes
    */
   private static $stmtNodes = [];
   /**
    * @var array $syntaxNodes
    */
   private static $syntaxNodes = [];
   /**
    * @var Trivia[] $trivias
    */
   private static $trivias = [];

   /**
    * @var array $syntaxNodeMap
    */
   private static $syntaxNodeMap = [];

   /**
    * @var array $syntaxNodeSerializationCodes
    */
   private static $syntaxNodeSerializationCodes = [];

   /**
    * @param $name
    * @return mixed
    */
   public static function getTokenByName(string $name)
   {
      if (empty(self::$tokenMap)) {
         foreach (self::$tokens as $token) {
            self::$tokenMap[$token->getName()] = $token;
         }
      }
      if (!array_key_exists($name, self::$tokenMap)) {
         return null;
      }
      return self::$tokenMap[$name];
   }

   /**
    * @param string $name
    * @return bool
    */
   public static function hasToken(string $name): bool
   {
      if (empty(self::$tokenMap)) {
         foreach (self::$tokens as $token) {
            self::$tokenMap[$token->getName()] = $token;
         }
      }
      return array_key_exists($name, self::$tokenMap);
   }

   /**
    * @return Node[]
    */
   public static function getSyntaxNodes(): array
   {
      if (empty(self::$syntaxNodes)) {
         self::$syntaxNodes = array_merge(self::getCommonNodes(), self::getExprNodes(),
            self::getDeclNodes(), self::getStmtNodes());
      }
      return self::$syntaxNodes;
   }

   /**
    * @return Token[]
    */
   public static function getTokens(): array
   {
      return self::$tokens;
   }

   /**
    * @param Token[] $tokens
    */
   public static function registerTokens(array $tokens): void
   {
      if (empty(self::$tokens)) {
         self::$tokens = $tokens;
      }
   }

   /**
    * @return array
    */
   public static function getTokenMap(): array
   {
      return self::$tokenMap;
   }

   /**
    * @param array $tokenMap
    */
   public static function registerTokenMap(array $tokenMap): void
   {
      if (empty(self::$tokenMap)) {
         self::$tokenMap = $tokenMap;
      }
   }

   /**
    * @return array
    */
   public static function getSyntaxNodeMap(): array
   {
      if (empty(self::$syntaxNodeMap)) {
         foreach (self::getSyntaxNodes() as $node) {
            self::$syntaxNodeMap[$node->getSyntaxKind()] = $node;
         }
      }
      return self::$syntaxNodeMap;
   }

   /**
    * @return array
    */
   public static function getCommonNodes(): array
   {
      return self::$commonNodes;
   }

   /**
    * @param array $commonNodes
    */
   public static function registerCommonNodes(array $commonNodes): void
   {
      if (empty(self::$commonNodes)) {
         self::verifySyntaxNodeSerializationCodes($commonNodes);
         self::$commonNodes = $commonNodes;
      }
   }

   /**
    * @return array
    */
   public static function getDeclNodes(): array
   {
      return self::$declNodes;
   }

   /**
    * @param array $declNodes
    */
   public static function registerDeclNodes(array $declNodes): void
   {
      if (empty(self::$declNodes)) {
         self::verifySyntaxNodeSerializationCodes($declNodes);
         self::$declNodes = $declNodes;
      }
   }

   /**
    * @return array
    */
   public static function getExprNodes(): array
   {
      return self::$exprNodes;
   }

   /**
    * @param array $exprNodes
    */
   public static function registerExprNodes(array $exprNodes): void
   {
      if (empty(self::$exprNodes)) {
         self::verifySyntaxNodeSerializationCodes($exprNodes);
         self::$exprNodes = $exprNodes;
      }
   }

   /**
    * @return array
    */
   public static function getStmtNodes(): array
   {
      return self::$stmtNodes;
   }

   /**
    * @param array $stmtNodes
    */
   public static function registerStmtNodes(array $stmtNodes): void
   {
      if (empty(self::$stmtNodes)) {
         self::verifySyntaxNodeSerializationCodes($stmtNodes);
         self::$stmtNodes = $stmtNodes;
      }
   }

   /**
    * @return Trivia[]
    */
   public static function getTrivias(): array
   {
      return self::$trivias;
   }

   /**
    * @param Trivia[] $trivias
    */
   public static function registerTrivias(array $trivias): void
   {
      if (empty(self::$trivias)) {
         self::$trivias = $trivias;
      }
   }

   /**
    * @return array
    */
   public static function getSyntaxNodeSerializationCodes(): array
   {
      return self::$syntaxNodeSerializationCodes;
   }

   /**
    * @param array $syntaxNodeSerializationCodes
    */
   public static function registerSyntaxNodeSerializationCodes(array $syntaxNodeSerializationCodes): void
   {
      if (empty(self::$syntaxNodeSerializationCodes)) {
         self::$syntaxNodeSerializationCodes = $syntaxNodeSerializationCodes;
      }
   }

   /**
    * @param string $kind
    * @return int
    */
   public static function getSerializationCode(string $kind): int
   {
      return self::$syntaxNodeSerializationCodes[$kind];
   }

   /**
    * @param Node[] $nodes
    */
   private static function verifySyntaxNodeSerializationCodes(array $nodes)
   {
      // Verify that all nodes have serialization codes
      foreach ($nodes as $node) {
         if (!$node->isBase() && !array_key_exists($node->getSyntaxKind(), self::$syntaxNodeSerializationCodes)) {
            throw new \RuntimeException(sprintf('Node %s has no serialization code', $node->getSyntaxKind()));
         }
      }
      // Verify that no serialization code is used twice
      $usedCodes = [];
      foreach (self::$syntaxNodeSerializationCodes as $code) {
         if (in_array($code, $usedCodes)) {
            throw new \RuntimeException("Serialization code $code used twice");
         }
         $usedCodes[] = $code;
      }
   }
}
