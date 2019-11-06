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
 * A child of a node, that may be declared optional or a token with a
 * restricted subset of acceptable kinds or texts.
 * @package Gyb\Syntax
 */
class Child
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
    * @var string $syntaxKind
    */
   private $syntaxKind;
   /**
    * @var string $description
    */
   private $description;
   /**
    * @var string $polarSyntaxKind
    */
   private $polarSyntaxKind;
   /**
    * @var string $typeName
    */
   private $typeName;
   /**
    * @var string $collectionElementName
    */
   private $collectionElementName;
   /**
    * @var string $classification
    */
   private $classification;
   /**
    * @var string $forceClassification
    */
   private $forceClassification;
   /**
    * @var string $tokenKind
    */
   private $tokenKind;
   /**
    * @var string $token
    */
   private $token;
   /**
    * @var bool $isOptional
    */
   private $isOptional;
   /**
    * @var array $tokenChoices
    */
   private $tokenChoices = [];
   /**
    * @var array $textChoices
    */
   private $textChoices = [];
   /**
    * @var Child[] $nodeChoices
    */
   private $nodeChoices = [];

   /**
    * If a classification is passed, it specifies the color identifiers in
    * that subtree should inherit for syntax coloring. Must be a member of
    * SyntaxClassification in SyntaxClassifier.h.gyb
    * If force_classification is also set to true, all child nodes (not only
    * identifiers) inherit the syntax classification.
    *
    * @param string $name
    * @param string $kind
    * @param string $description
    * @param bool $isOptional
    * @param array $tokenChoices
    * @param array $textChoices
    * @param array $nodeChoices
    * @param string $collectionElementName
    * @param string $classification
    * @param bool $forceClassification
    */
   public function __construct(string $name, string $kind, string $description = '',
                               bool $isOptional = false, array $tokenChoices = [],
                               array $textChoices = [], array $nodeChoices = [],
                               string $collectionElementName = '', $classification = null,
                               bool $forceClassification = false)
   {
      $this->name = $name;
      $this->polarName = lcfirst($name);
      $this->syntaxKind = $kind;
      $this->description = $description;
      $this->typeName = Kinds::convertKindToType($kind);
      $this->polarSyntaxKind = lcfirst($this->syntaxKind);
      $this->collectionElementName = $collectionElementName;
      $this->classification = SyntaxClassification::getByName($classification);
      $this->forceClassification = $forceClassification;
      // If the child has "token" anywhere in the kind, it's considered
      // a token node. Grab the existing reference to that token from the
      // global list.
      $this->tokenKind = '';
      if (\Gyb\Utils\has_substr($this->syntaxKind, 'Token') || \Gyb\Utils\has_substr($this->syntaxKind, 'Keyword')) {
         $this->tokenKind = $this->syntaxKind;
      }
      $this->token = SyntaxRegistry::getTokenByName($this->tokenKind);
      $this->isOptional = $isOptional;
      // A restricted set of token kinds that will be accepted for this
      // child.
      $this->tokenChoices = [];
      if ($this->token) {
         $this->tokenChoices[] = $this->token;
      }
      foreach ($tokenChoices as $choice) {
         $token = SyntaxRegistry::getTokenByName($choice);
         assert($token != null, sprintf("token %s is not exist", $choice));
         $this->tokenChoices[] = $token;
      }
      // A list of valid text for tokens, if specified.
      // This will force validation logic to check the text passed into the
      // token against the choices.
      if (!empty($textChoices)) {
         $this->textChoices = $textChoices;
      }
      // A list of valid choices for a child
      if (!empty($nodeChoices)) {
         $this->nodeChoices = $nodeChoices;
      }
      // Check the choices are either empty or multiple
      assert(count($this->nodeChoices) != 1);
      // Check node choices are well-formed
      foreach ($this->nodeChoices as $choice) {
         assert(!$choice->isOptional(), sprintf('node choice %s cannot be optional',
            $choice->getName()));
         assert(!$choice->getNodeChoices(), sprintf('node choice %s cannot have further choices',
            $choice->getName()));
      }
   }

   /**
    * Returns true if this child has a token kind.
    */
   public function isToken()
   {
      return $this->tokenKind != null;
   }

   /**
    * Returns the first choice from the token_choices if there are any,
    * otherwise returns None.
    */
   public function getMainToken()
   {
      if (!empty($this->tokenChoices)) {
         return $this->tokenChoices[0];
      }
      return null;
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
    * @return Child
    */
   public function setName(string $name): Child
   {
      $this->name = $name;
      return $this;
   }

   /**
    * @return string
    */
   public function getPolarName(): string
   {
      return $this->polarName;
   }

   /**
    * @param string $polarName
    * @return Child
    */
   public function setPolarName(string $polarName): Child
   {
      $this->polarName = $polarName;
      return $this;
   }

   /**
    * @return string
    */
   public function getSyntaxKind(): string
   {
      return $this->syntaxKind;
   }

   /**
    * @param string $syntaxKind
    * @return Child
    */
   public function setSyntaxKind(string $syntaxKind): Child
   {
      $this->syntaxKind = $syntaxKind;
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
    * @return Child
    */
   public function setDescription(string $description): Child
   {
      $this->description = $description;
      return $this;
   }

   /**
    * @return string
    */
   public function getPolarSyntaxKind(): string
   {
      return $this->polarSyntaxKind;
   }

   /**
    * @param string $polarSyntaxKind
    * @return Child
    */
   public function setPolarSyntaxKind(string $polarSyntaxKind): Child
   {
      $this->polarSyntaxKind = $polarSyntaxKind;
      return $this;
   }

   /**
    * @return string
    */
   public function getTypeName(): string
   {
      return $this->typeName;
   }

   /**
    * @param string $typeName
    * @return Child
    */
   public function setTypeName(string $typeName): Child
   {
      $this->typeName = $typeName;
      return $this;
   }

   /**
    * @return string
    */
   public function getCollectionElementName(): string
   {
      return $this->collectionElementName;
   }

   /**
    * @param string $collectionElementName
    * @return Child
    */
   public function setCollectionElementName(string $collectionElementName): Child
   {
      $this->collectionElementName = $collectionElementName;
      return $this;
   }

   /**
    * @return string
    */
   public function getClassification(): string
   {
      return $this->classification;
   }

   /**
    * @param string $classification
    * @return Child
    */
   public function setClassification(string $classification): Child
   {
      $this->classification = $classification;
      return $this;
   }

   /**
    * @return string
    */
   public function getForceClassification(): string
   {
      return $this->forceClassification;
   }

   /**
    * @param string $forceClassification
    * @return Child
    */
   public function setForceClassification(string $forceClassification): Child
   {
      $this->forceClassification = $forceClassification;
      return $this;
   }

   /**
    * @return string
    */
   public function getTokenKind(): string
   {
      return $this->tokenKind;
   }

   /**
    * @param string $tokenKind
    * @return Child
    */
   public function setTokenKind(string $tokenKind): Child
   {
      $this->tokenKind = $tokenKind;
      return $this;
   }

   /**
    * @return string
    */
   public function getToken(): string
   {
      return $this->token;
   }

   /**
    * @param string $token
    * @return Child
    */
   public function setToken(string $token): Child
   {
      $this->token = $token;
      return $this;
   }

   /**
    * @return bool
    */
   public function isOptional(): bool
   {
      return $this->isOptional;
   }

   /**
    * @param bool $isOptional
    * @return Child
    */
   public function setIsOptional(bool $isOptional): Child
   {
      $this->isOptional = $isOptional;
      return $this;
   }

   /**
    * @return array
    */
   public function getTokenChoices(): array
   {
      return $this->tokenChoices;
   }

   /**
    * @param array $tokenChoices
    * @return Child
    */
   public function setTokenChoices(array $tokenChoices): Child
   {
      $this->tokenChoices = $tokenChoices;
      return $this;
   }

   /**
    * @return array
    */
   public function getTextChoices(): array
   {
      return $this->textChoices;
   }

   /**
    * @param array $textChoices
    * @return Child
    */
   public function setTextChoices(array $textChoices): Child
   {
      $this->textChoices = $textChoices;
      return $this;
   }

   /**
    * @return array
    */
   public function getNodeChoices(): array
   {
      return $this->nodeChoices;
   }

   /**
    * @param array $nodeChoices
    * @return Child
    */
   public function setNodeChoices(array $nodeChoices): Child
   {
      $this->nodeChoices = $nodeChoices;
      return $this;
   }
}