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

namespace Gyb\Syntax\Token;

use Gyb\Syntax\Token;

/**
 * Represents a keyword token.
 *
 * @package Gyb\Syntax\Token
 */
class Keyword extends Token
{
   public function __construct(string $name, string $kind, string $text, int $serializationCode,
                               string $classification = 'Keyword')
   {
      parent::__construct($name, $kind, $serializationCode, $text, '', $classification, true);
   }

   public function getMacroName(): string
   {
      return 'KEYWORD';
   }

   public function getName()
   {
      return parent::getRawName().'Keyword';
   }
}