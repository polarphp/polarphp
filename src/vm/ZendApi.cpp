// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/27.

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/lang/Module.h"

namespace polar {
namespace vmapi {

bool register_module_to_zendvm(const Module &module)
{
   return true;
}

} // vmapi
} // polar
