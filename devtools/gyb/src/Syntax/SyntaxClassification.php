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

   static function getByName(string $name)
   {
      if (empty(self::$pool)) {
         self::initPool();
      }
      if (array_key_exists($name)) {
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
         new SyntaxClassification('None', '', 'The token should not receive syntax coloring.'),
         new SyntaxClassification('Keyword', '', 'A polarphp keyword, including contextual keywords.'),
         new SyntaxClassification('Identifier', '', 'A generic identifier.'),
         new SyntaxClassification('IntegerLiteral', '', 'An integer literal.'),
         new SyntaxClassification('FloatingLiteral', '', 'A floating point literal.'),
         new SyntaxClassification('StringLiteral', '', 'A string literal including multiline string literals.'),
         new SyntaxClassification('LineComment', '', 'A line comment starting with `//`.'),
         new SyntaxClassification('DocLineComment','', 'A doc line comment starting with `///`.'),
         new SyntaxClassification('BlockComment', '', 'A block comment starting with `/**` and ending with `*/.'),
         new SyntaxClassification('DocBlockComment', '', 'A doc block comment starting with `/**` and ending with `*/.')
      ];
   }
}