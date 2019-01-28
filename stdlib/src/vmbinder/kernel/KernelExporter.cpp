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
#include "polarphp/vm/lang/Namespace.h"

#include "php/kernel/Utils.h"
#include "php/vmbinder/kernel/KernelExporter.h"
#include "php/vmbinder/NamespaceDefs.h"

namespace php {
namespace vmbinder {

using polar::vmapi::Namespace;

namespace {
void export_stdlib_kernel_funcs(Module &module);
} // anonymous namespace

bool export_stdlib_kernel_module(Module &module)
{
   register_stdlib_namespaces(module);
   export_stdlib_kernel_funcs(module);
   return module.registerToVM();
}

namespace {
void export_stdlib_kernel_funcs(Module &module)
{
   Namespace *php = module.findNamespace("php");
   php->registerFunction<decltype(php::kernel::retrieve_version_str), php::kernel::retrieve_version_str>("retrieve_version_str");
   php->registerFunction<decltype(php::kernel::retrieve_major_version), php::kernel::retrieve_major_version>("retrieve_major_version");
   php->registerFunction<decltype(php::kernel::retrieve_minor_version), php::kernel::retrieve_minor_version>("retrieve_minor_version");
   php->registerFunction<decltype(php::kernel::retrieve_patch_version), php::kernel::retrieve_patch_version>("retrieve_patch_version");
   php->registerFunction<decltype(php::kernel::retrieve_version_id), php::kernel::retrieve_version_id>("retrieve_version_id");
}
} // anonymous namespace

} // vmbinder
} // php
