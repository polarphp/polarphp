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
// Created by polarboy on 2019/10/19.

namespace Lit\Shell;

use Symfony\Component\Process\Exception\InvalidArgumentException;
use Symfony\Component\Process\ProcessUtils as BaseProcessUtils;

class ProcessUtils extends BaseProcessUtils
{
   public static function validateStdout($caller, $stdout)
   {
      if (null !== $stdout) {
         if (\is_resource($stdout)) {
            return $stdout;
         }
         if (\is_string($stdout)) {
            return $stdout;
         }
         if (\is_scalar($stdout)) {
            return $stdout;
         }

         throw new InvalidArgumentException(sprintf('%s only accepts strings, Traversable objects or stream resources.', $caller));
      }

      return $stdout;
   }

   public static function validateStderr($caller, $stderr)
   {
      if (null !== $stderr) {
         if (\is_resource($stderr)) {
            return $stderr;
         }
         if (\is_string($stderr)) {
            return $stderr;
         }
         if (\is_scalar($stderr)) {
            return $stderr;
         }
         throw new InvalidArgumentException(sprintf('%s only accepts strings, Traversable objects or stream resources.', $caller));
      }

      return $stderr;
   }
}