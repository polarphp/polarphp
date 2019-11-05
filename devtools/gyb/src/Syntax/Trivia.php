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

class Trivia
{
   /**
    * @var string $name
    */
   private $name;
   /**
    * @var string $comment
    */
   private $comment;
   /**
    * @var string $serializationCode
    */
   private $serializationCode;
   /**
    * @var array $characters
    */
   private $characters;
   /**
    * @var string $lowerName
    */
   private $lowerName;
   /**
    * @var bool $isNewLine
    */
   private $isNewLine;
   /**
    * @var bool $isComment
    */
   private $isComment;
   /**
    * @var array $polarCharacters
    */
   private $polarCharacters;

   public function __construct(string $name, string $comment, string $serializationCode,
                               array $characters = [], array $polarCharacters = [],
                               bool $isNewLine = false, bool $isComment = false)
   {
      $this->name = $name;
      $this->comment = $comment;
      $this->serializationCode = $serializationCode;
      $this->characters = $characters;
      $this->lowerName = lcfirst($name);
      $this->isNewLine = $isNewLine;
      $this->isComment = $isComment;
      // polarphp sometimes doesn't support escaped characters like \f or \v;
      // we should allow specifying alternatives explicitly.
      if (!empty($polarCharacters)) {
         $this->polarCharacters = $polarCharacters;
      } else {
         $this->polarCharacters = $characters;
      }
      assert(count($this->polarCharacters) == count($this->characters));
   }

   /**
    * @param Trivia[] $trivias
    */
   public static function verifyNoDuplicateSerializationCodes(array $trivias)
   {
      $usedCodes = [];
      foreach ($trivias as $trivia) {
         $code = $trivia->getSerializationCode();
         if (in_array($code, $usedCodes)) {
            throw new \RuntimeException(sprintf('Serialization code %d used twice for trivia',
               $code));
         }
         $usedCodes[] = $code;
      }
   }

   public function getCharatersLen()
   {
      return count($this->characters);
   }

   public function isCollection()
   {
      return count($this->characters) > 0;
   }

   /**
    * @return string
    */
   public function getName(): string
   {
      return $this->name;
   }

   /**
    * @param string $name
    * @return Trivia
    */
   public function setName(string $name): Trivia
   {
      $this->name = $name;
      return $this;
   }

   /**
    * @return string
    */
   public function getComment(): string
   {
      return $this->comment;
   }

   /**
    * @param string $comment
    * @return Trivia
    */
   public function setComment(string $comment): Trivia
   {
      $this->comment = $comment;
      return $this;
   }

   /**
    * @return string
    */
   public function getSerializationCode(): string
   {
      return $this->serializationCode;
   }

   /**
    * @param string $serializationCode
    * @return Trivia
    */
   public function setSerializationCode(string $serializationCode): Trivia
   {
      $this->serializationCode = $serializationCode;
      return $this;
   }

   /**
    * @return string
    */
   public function getCharacters(): array
   {
      return $this->characters;
   }

   /**
    * @param string $characters
    * @return Trivia
    */
   public function setCharacters(string $characters): Trivia
   {
      $this->characters = $characters;
      return $this;
   }

   /**
    * @return string
    */
   public function getLowerName(): string
   {
      return $this->lowerName;
   }

   /**
    * @param string $lowerName
    * @return Trivia
    */
   public function setLowerName(string $lowerName): Trivia
   {
      $this->lowerName = $lowerName;
      return $this;
   }

   /**
    * @return bool
    */
   public function isNewLine(): bool
   {
      return $this->isNewLine;
   }

   /**
    * @param bool $isNewLine
    * @return Trivia
    */
   public function setIsNewLine(bool $isNewLine): Trivia
   {
      $this->isNewLine = $isNewLine;
      return $this;
   }

   /**
    * @return bool
    */
   public function isComment(): bool
   {
      return $this->isComment;
   }

   /**
    * @param bool $isComment
    * @return Trivia
    */
   public function setIsComment(bool $isComment): Trivia
   {
      $this->isComment = $isComment;
      return $this;
   }

   /**
    * @return string
    */
   public function getPolarCharacters(): string
   {
      return $this->polarCharacters;
   }

   /**
    * @param string $polarCharacters
    * @return Trivia
    */
   public function setPolarCharacters(string $polarCharacters): Trivia
   {
      $this->polarCharacters = $polarCharacters;
      return $this;
   }
}