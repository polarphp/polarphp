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

/**
 * Represents a classification a token can receive for syntax highlighting.
 *
 * @package Gyb\Syntax
 */
class SyntaxClassification
{
   /**
    * @var string $name
    */
   private $name;
   /**
    * @var string $polarName
    */
   private $polarName;
   /**
    * @var string $description
    */
   private $description;

   /**
    * @var array $pool
    */
   private static $pool = array();

   public function __construct(string $name, string $polarName, string $description)
   {
      $this->name = $name;
      $this->polarName = $polarName;
      $this->description = $description;
   }

   static function getByName($name)
   {
      if (empty(self::$pool)) {
         self::initPool();
      }
      if (is_null($name)) {
         return null;
      }
      assert(is_string($name), 'must pass string type for method SyntaxClassification::getByName');
      $name = trim($name);
      if (array_key_exists($name, self::$pool)) {
         return self::$pool[$name];
      }
      throw new \RuntimeException(sprintf("Unknown syntax classification '%s'", $name));
   }

   /**
    * @return mixed
    */
   public function getName()
   {
      return $this->name;
   }

   /**
    * @param mixed $name
    * @return SyntaxClassification
    */
   public function setName($name)
   {
      $this->name = $name;
      return $this;
   }

   /**
    * @return mixed
    */
   public function getPolarName()
   {
      return $this->polarName;
   }

   /**
    * @param mixed $polarName
    * @return SyntaxClassification
    */
   public function setPolarName($polarName)
   {
      $this->polarName = $polarName;
      return $this;
   }

   /**
    * @return mixed
    */
   public function getDescription()
   {
      return $this->description;
   }

   /**
    * @param mixed $description
    * @return SyntaxClassification
    */
   public function setDescription($description)
   {
      $this->description = $description;
      return $this;
   }

   protected static function initPool(): void
   {
      self::$pool = [
         'None' => new SyntaxClassification('None', '', 'The token should not receive syntax coloring.'),
         'Keyword' => new SyntaxClassification('Keyword', '', 'A polarphp keyword, including contextual keywords.'),
         'Identifier' => new SyntaxClassification('Identifier', '', 'A generic identifier.'),
         'IntegerLiteral' => new SyntaxClassification('IntegerLiteral', '', 'An integer literal.'),
         'FloatingLiteral' => new SyntaxClassification('FloatingLiteral', '', 'A floating point literal.'),
         'StringLiteral' => new SyntaxClassification('StringLiteral', '', 'A string literal including multiline string literals.'),
         'LineComment' => new SyntaxClassification('LineComment', '', 'A line comment starting with `//`.'),
         'DocLineComment' => new SyntaxClassification('DocLineComment','', 'A doc line comment starting with `///`.'),
         'BlockComment' => new SyntaxClassification('BlockComment', '', 'A block comment starting with `/**` and ending with `*/.'),
         'DocBlockComment' => new SyntaxClassification('DocBlockComment', '', 'A doc block comment starting with `/**` and ending with `*/.')
      ];
   }
}