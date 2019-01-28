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

#ifndef POLARPHP_VMAPI_UTILS_INTERNAL_FUNCS_H
#define POLARPHP_VMAPI_UTILS_INTERNAL_FUNCS_H

#include <string>
#include <list>

namespace polar {
namespace vmapi {
namespace internal {

bool parse_namespaces(const std::string &ns, std::list<std::string> &parts);

} // internal
} // vmapi
} // polar

#endif // POLARPHP_VMAPI_UTILS_INTERNAL_FUNCS_H
