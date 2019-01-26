// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/01/26.

#include "polarphp/vm/lang/Module.h"
#include "php/kernel/Utils.h"
#include "php/vmbinder/kernel/KernelExporter.h"

namespace php {
namespace vmbinder {

namespace {
void export_stdlib_kernel_funcs(Module &module);
} // anonymous namespace

bool export_stdlib_kernel_module(Module &module)
{
   export_stdlib_kernel_funcs(module);
   return module.registerToVM();
}

namespace {
void export_stdlib_kernel_funcs(Module &module)
{
   //module.registerFunction<decltype(&php::kernel::retrieve_version_str), &php::kernel::retrieve_version_str>("show_something");
}
} // anonymous namespace

} // vmbinder
} // php
