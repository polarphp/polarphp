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
 * Represents the specification for a Token in the TokenSyntax file.
 *
 * Class Token
 * @package Gyb\Syntax
 */
class Token
{
   /**
    * @var string $name
    */
   private $name;
   /**
    * @var string $kind
    */
   private $kind;
   /**
    * @var string $serializationCode
    */
   private $serializationCode;
   /**
    * @var string $text
    */
   private $text;
   /**
    * @var string $classification
    */
   private $classification;
   /**
    * @var bool $keyword
    */
   private $keyword;

   /**
    * @var string $valueType
    */
   private $valueType = '';

   public function __construct(string $name, string $kind, int $serializationCode,
                               string $text = '', string $valueType = '', string $classification = 'None',
                               bool $keyword = false)
   {
      $this->name = trim($name);
      $this->kind = trim($kind);
      $this->serializationCode = $serializationCode;
      $this->text = trim($text);
      $this->classification = SyntaxClassification::getByName(trim($classification));
      $this->keyword = $keyword;
      $this->valueType = $valueType;
   }

   public function getPolarKind(): string
   {
      $name = lcfirst($this->name);
      if ($this->keyword) {
         return $name . 'Keyword';
      }
      return $name;
   }

   /**
    * @return mixed
    */
   public function getRawName()
   {
      return $this->name;
   }

   public function getName()
   {
      return $this->name.'Token';
   }

   /**
    * @return mixed
    */
   public function getKind()
   {
      return $this->kind;
   }

   /**
    * @return mixed
    */
   public function getSerializationCode()
   {
      return $this->serializationCode;
   }

   /**
    * @return mixed
    */
   public function getText()
   {
      return $this->text;
   }

   /**
    * @return bool
    */
   public function isTextEmpty(): bool
   {
      return strlen($this->text) == 0;
   }

   /**
    * @return mixed
    */
   public function getClassification()
   {
      return $this->classification;
   }

   /**
    * @return bool
    */
   public function isKeyword(): bool
   {
      return $this->keyword;
   }

   /**
    * @return string
    */
   public function getMacroName(): string
   {
      return 'TOKEN';
   }

   /**
    * @return string
    */
   public function getValueType(): string
   {
      return $this->valueType;
   }
}