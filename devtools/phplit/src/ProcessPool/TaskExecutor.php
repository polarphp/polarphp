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
// Created by polarboy on 2019/10/16.

namespace Lit\ProcessPool;

class TaskExecutor
{
   const E_TASK_TEXT_EMPTY = -99;
   const E_TASK_UNSERIALIZED = -100;
   /**
    * @var array $data
    */
   private static $data = array();
   /**
    * @var TaskInterface $task
    */
   private $task;

   public function __construct(TaskInterface $task)
   {
      $this->task = $task;
   }

   public function exec()
   {
      return $this->task->exec(self::$data);
   }

   public static function setDataItem(string $name, $value)
   {
      self::$data[$name] = $value;
   }

   public static function getDataItem(string $name, $defaultValue = null)
   {
      if (array_key_exists($name, self::$data)) {
         return self::$data[$name];
      }
      return $defaultValue;
   }

   public static function getDataPool()
   {
      return self::$data;
   }
}
