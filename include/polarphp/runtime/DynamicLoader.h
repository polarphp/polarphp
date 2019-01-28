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

#ifndef POLARPHP_RUNTIME_DYNAMIC_LOADER_H
#define POLARPHP_RUNTIME_DYNAMIC_LOADER_H

#include "polarphp/runtime/RtDefs.h"
#include "polarphp/basic/adt/StringRef.h"

namespace polar {
namespace runtime {

using polar::basic::StringRef;

POLAR_DECL_EXPORT bool php_load_extension(StringRef filename, int type, bool startNow);
POLAR_DECL_EXPORT void php_dl(StringRef file, int type, zval *return_value, bool startNow);
POLAR_DECL_EXPORT void *php_load_shlib(StringRef path, std::string &errp);

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_DYNAMIC_LOADER_H
