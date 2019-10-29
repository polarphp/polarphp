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

class Traits
{
   /**
    * @var string $traitName
    */
   private $traitName;
   /**
    * @var Child[] $children
    */
   private $children;
   /**
    * @var string $description
    */
   private $description;

   /**
    * Traits constructor.
    * @param string $traitName
    * @param Child[] $children
    * @param string $description
    */
   public function __construct(string $traitName, string $description = '', array $children = [])
   {
      $this->traitName = $traitName;
      $this->children = $children;
      $this->description = $description;
   }

   /**
    * @return string
    */
   public function getTraitName(): string
   {
      return $this->traitName;
   }

   /**
    * @param string $traitName
    * @return Traits
    */
   public function setTraitName(string $traitName): Traits
   {
      $this->traitName = $traitName;
      return $this;
   }

   /**
    * @return Child[]
    */
   public function getChildren(): string
   {
      return $this->children;
   }

   /**
    * @param Child[] $children
    * @return Traits
    */
   public function setChildren(string $children): Traits
   {
      $this->children = $children;
      return $this;
   }

   /**
    * @return string
    */
   public function getDescription(): string
   {
      return $this->description;
   }

   /**
    * @param string $description
    * @return Traits
    */
   public function setDescription(string $description): Traits
   {
      $this->description = $description;
      return $this;
   }
}