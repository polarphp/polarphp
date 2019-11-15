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
// Created by polarboy on 2019/10/10.

namespace Lit\Kernel;

/**
 * Test result codes.
 *
 * @package Lit\Kernel
 */
class TestResultCode
{
   /**
    * @var array $instances
    */
   private static $instances = array();
   /**
    * @var string $name
    */
   private $name;
   /**
    * @var bool $isFailure
    */
   private $isFailure;

   public function __construct(string $name, bool $isFailure)
   {
      $this->name = $name;
      $this->isFailure = $isFailure;
   }

   public static function getInstance(string $name, bool $isFailure)
   {
      if (!array_key_exists($name, self::$instances)) {
         self::$instances[$name] = new self($name, $isFailure);
      }
      return self::$instances[$name];
   }

   public function __toString() : string
   {
      return sprintf('%s%s', self::class, var_export([$this->name, $this->isFailure], true));
   }

   public static function PASS() : TestResultCode
   {
      return self::getInstance('PASS', false);
   }

   public static function FLAKYPASS() : TestResultCode
   {
      return self::getInstance('FLAKYPASS', false);
   }

   public static function XFAIL() : TestResultCode
   {
      return self::getInstance('XFAIL', false);
   }

   public static function FAIL() : TestResultCode
   {
      return self::getInstance('FAIL', true);
   }

   public static function XPASS() : TestResultCode
   {
      return self::getInstance('XPASS', true);
   }

   public static function UNRESOLVED() : TestResultCode
   {
      return self::getInstance('UNRESOLVED', true);
   }

   public static function UNSUPPORTED() : TestResultCode
   {
      return self::getInstance('UNSUPPORTED', false);
   }

   public static function TIMEOUT() : TestResultCode
   {
      return self::getInstance('TIMEOUT', true);
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
    * @return TestResultCode
    */
   public function setName(string $name): TestResultCode
   {
      $this->name = $name;
      return $this;
   }

   /**
    * @return bool
    */
   public function isFailure(): bool
   {
      return $this->isFailure;
   }

   /**
    * @param bool $isFailure
    * @return TestResultCode
    */
   public function setIsFailure(bool $isFailure): TestResultCode
   {
      $this->isFailure = $isFailure;
      return $this;
   }
}