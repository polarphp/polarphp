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
    * @return Token[]
    */
   public static function getTokens(): array
   {
      return self::$tokens;
   }

   /**
    * @param Token[] $tokens
    */
   public static function setTokens(array $tokens): void
   {
      self::$tokens = $tokens;
   }

   /**
    * @param $name
    * @return mixed
    */
   public static function getTokenByName($name)
   {
      if (empty(self::$tokenMap)) {
         foreach (self::$tokens as $token) {
            self::$tokenMap[$token->getName().'Token'] = $token;
         }
      }
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
   public static function setCommonNodes(array $commonNodes): void
   {
      self::$commonNodes = $commonNodes;
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
   public static function setDeclNodes(array $declNodes): void
   {
      self::$declNodes = $declNodes;
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
   public static function setExprNodes(array $exprNodes): void
   {
      self::$exprNodes = $exprNodes;
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
   public static function setStmtNodes(array $stmtNodes): void
   {
      self::$stmtNodes = $stmtNodes;
   }
}
