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

namespace Lit\Shell;

abstract class AbstractMetricValue
{
   /**
    * @var mixed $value
    */
   protected $value;

   public function __construct($value)
   {
      $this->value = $value;
   }

   public static function toMetricValue($value)
   {
      if ($value instanceof AbstractMetricValue) {
         return $value;
      } elseif (is_int($value)) {
         return new IntMetricValue($value);
      } elseif (is_float($value)) {
         return new RealMetricValue($value);
      } else {
         # Try to create a JSONMetricValue and let the constructor throw
         # if value is not a valid type.
         return new JSONMetricValue($value);
      }
   }

   /**
    * Convert this metric to a string suitable for displaying as part of the
    * console output.
    * @return mixed
    */
   abstract public function format() : string;

   /**
    * Convert this metric to content suitable for serializing in the JSON test
    * output.
    *
    * @return string
    */
   abstract public function toData();
}