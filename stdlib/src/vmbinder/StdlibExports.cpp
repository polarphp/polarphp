// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/26.

#include "polarphp/vm/lang/Module.h"
#include "php/vmbinder/StdlibExports.h"
#include "php/vmbinder/kernel/KernelExporter.h"
#include "php/kernel/Utils.h"

namespace php {
namespace vmbinder {

using polar::vmapi::Module;

Module sg_kernelModule("StdlibKernel");

bool export_stdlib_to_zendvm()
{
   if (!export_stdlib_kernel_module(sg_kernelModule)) {
      return false;
   }
   return true;
}

} // vmbinder
} // php
