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

#ifndef POLARPHP_STDLIBMOCK_MODULE_CYCLE_HOOKS_H
#define POLARPHP_STDLIBMOCK_MODULE_CYCLE_HOOKS_H

namespace polar {
namespace vmapi {
class Module;
} // vmapi
} // polar

namespace php {

using polar::vmapi::Module;

void register_module_cycle_hooks(Module &module);

} // php

#endif // POLARPHP_STDLIBMOCK_MODULE_CYCLE_HOOKS_H
