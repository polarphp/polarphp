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
 * A test suite groups together a set of logically related tests.
 */
class TestSuite
{
   /**
    * @var string $name
    */
   private $name;
   /**
    * @var string $sourceRoot
    */
   private $sourceRoot;
   /**
    * @var string $execRoot
    */
   private $execRoot;
   /**
    * @var TestingConfig $config
    */
   private $config;

   public function __construct(string $name, string $sourceRoot, string $execRoot, TestingConfig $config)
   {
      $this->name = $name;
      $this->sourceRoot = $sourceRoot;
      $this->execRoot = $execRoot;
      $this->config = $config;
   }

   public function getSourcePath(array $components) : string
   {
      $parts = array_merge([$this->sourceRoot], $components);
      return implode(DIRECTORY_SEPARATOR, $parts);
   }

   public function getExecPath(array $components) : string
   {
      $parts = array_merge([$this->execRoot], $components);
      return implode(DIRECTORY_SEPARATOR, $parts);
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
    * @return TestSuite
    */
   public function setName(string $name): TestSuite
   {
      $this->name = $name;
      return $this;
   }

   /**
    * @return string
    */
   public function getSourceRoot(): string
   {
      return $this->sourceRoot;
   }

   /**
    * @param string $sourceRoot
    * @return TestSuite
    */
   public function setSourceRoot(string $sourceRoot): TestSuite
   {
      $this->sourceRoot = $sourceRoot;
      return $this;
   }

   /**
    * @return string
    */
   public function getExecRoot(): string
   {
      return $this->execRoot;
   }

   /**
    * @param string $execRoot
    * @return TestSuite
    */
   public function setExecRoot(string $execRoot): TestSuite
   {
      $this->execRoot = $execRoot;
      return $this;
   }

   /**
    * @return TestingConfig
    */
   public function getConfig(): TestingConfig
   {
      return $this->config;
   }

   /**
    * @param TestingConfig $config
    * @return TestSuite
    */
   public function setConfig(TestingConfig $config): TestSuite
   {
      $this->config = $config;
      return $this;
   }
}