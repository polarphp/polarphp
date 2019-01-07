// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/02/07.

#ifndef POLARPHP_STDLIBMOCK_CONSTANT_H
#define POLARPHP_STDLIBMOCK_CONSTANT_H

#include "polarphp/vm/lang/Module.h"
#include "polarphp/vm/lang/Constant.h"
#include "polarphp/vm/lang/Namespace.h"

namespace polar {
namespace vmapi {
class Module;
} // vmapi
} // polar

namespace php {

using polar::vmapi::Module;
using polar::vmapi::Constant;
using polar::vmapi::Namespace;

void register_constants_hook(Module &module)
{
//   module.registerConstant(Constant("MY_CONST", 12333));
//   module.registerConstant(Constant("PI", 3.14));
//   Constant nameConst("POLAR_NAME", "polarphp");
//   module.registerConstant(nameConst);
//   module.registerConstant(Constant("POLAR_VERSION", "v0.0.1"));
//   module.registerConstant(Constant("POLARPHP", "beijing polarphp"));
//   // register constant in namespace
//   Namespace *polarphp = module.findNamespace("polarphp");
//   Namespace *io = polarphp->findNamespace("io");
//   io->registerConstant(Constant("IO_TYPE", "ASYNC"));
//   io->registerConstant(Constant("NATIVE_STREAM", true));
//   polarphp->registerConstant(Constant("SYS_VERSION", "0.1.1-alpha"));
}

} // php

#endif // POLARPHP_STDLIBMOCK_CONSTANT_H
