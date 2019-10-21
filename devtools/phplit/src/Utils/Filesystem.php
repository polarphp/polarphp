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
// Created by polarboy on 2019/10/21.

namespace Lit\Utils;

use Symfony\Component\Filesystem\Exception\IOException;
use Symfony\Component\Filesystem\Filesystem as BaseFilesystem;

class Filesystem extends BaseFilesystem
{
   /**
    * Removes files or directories.
    *
    * @param string|iterable $files A filename, an array of files, or a \Traversable instance to remove
    *
    * @throws IOException When removal fails
    */
   public function remove($files, bool $recursive = true)
   {
      if ($files instanceof \Traversable) {
         $files = iterator_to_array($files, false);
      } elseif (!\is_array($files)) {
         $files = [$files];
      }
      $files = array_reverse($files);
      foreach ($files as $file) {
         if (is_dir($file) && !$recursive) {
            continue;
         }
         parent::remove($file);
      }
   }
}