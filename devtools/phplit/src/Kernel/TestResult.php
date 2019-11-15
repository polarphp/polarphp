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

use Lit\Exception\ValueException;
use Lit\Shell\AbstractMetricValue;

class TestResult
{
   /**
    * @var TestResultCode $code
    */
   private $code;
   /**
    * @var string $output
    */
   private $output;
   /**
    * @var float $elapsed
    */
   private $elapsed = 0.0;
   /**
    * @var array $metrics
    */
   private $metrics = array();
   /**
    * @var array $microResults
    */
   private $microResults = array();

   public function __construct(TestResultCode $code, string $output = '', $elapsed = 0.0)
   {
      $this->code = $code;
      $this->output = $output;
      $this->elapsed = $elapsed;
   }

   /**
    * Attach a test metric to the test result, with the given name and list of
    * values. It is an error to attempt to attach the metrics with the same
    * name multiple times.
    * Each value must be an instance of a MetricValue subclass.
    *
    * @param $name
    * @param $value
    */
   public function addMetric(string $name, AbstractMetricValue $value) : TestResult
   {
      if (array_key_exists($name, $this->metrics)) {
         throw new ValueException('result already includes metrics for %s', $name);
      }
      $this->metrics[$name] = $value;
      return $this;
   }

   /**
    * Attach a micro-test result to the test result, with the given name and
    * result.  It is an error to attempt to attach a micro-test with the
    * same name multiple times.
    * Each micro-test result must be an instance of the Result class.
    *
    * @param string $name
    * @param TestResult $result
    */
   public function addMicroResult(string $name, TestResult $result) : TestResult
   {
      if (array_key_exists($name, $this->microResults)) {
         throw new ValueException('Result already includes microResult for %s', $name);
      }
      $this->microResults[$name] = $result;
      return $this;
   }

   /**
    * @return TestResultCode
    */
   public function getCode(): TestResultCode
   {
      return $this->code;
   }

   /**
    * @param TestResultCode $code
    * @return TestResult
    */
   public function setCode(TestResultCode $code): TestResult
   {
      $this->code = $code;
      return $this;
   }

   /**
    * @return string
    */
   public function getOutput(): string
   {
      return $this->output;
   }

   /**
    * @param string $output
    * @return TestResult
    */
   public function setOutput(string $output): TestResult
   {
      $this->output = $output;
      return $this;
   }

   /**
    * @return float
    */
   public function getElapsed(): float
   {
      return $this->elapsed;
   }

   /**
    * @param float $elapsed
    * @return TestResult
    */
   public function setElapsed(float $elapsed): TestResult
   {
      $this->elapsed = $elapsed;
      return $this;
   }

   /**
    * @return array
    */
   public function getMetrics(): array
   {
      return $this->metrics;
   }

   /**
    * @param array $metrics
    * @return TestResult
    */
   public function setMetrics(array $metrics): TestResult
   {
      $this->metrics = $metrics;
      return $this;
   }

   /**
    * @return array
    */
   public function getMicroResults(): array
   {
      return $this->microResults;
   }

   /**
    * @param array $microResults
    * @return TestResult
    */
   public function setMicroResults(array $microResults): TestResult
   {
      $this->microResults = $microResults;
      return $this;
   }
}