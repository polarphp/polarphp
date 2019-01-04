// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/24.

#ifndef POLARPHP_VMAPI_LANG_FUNCTION_H
#define POLARPHP_VMAPI_LANG_FUNCTION_H

#include "polarphp/vm/Callable.h"

namespace polar {
namespace vmapi {

namespace internal {
class FunctionPrivate;
} // internal

using internal::FunctionPrivate;
using polar::basic::StringRef;

class VMAPI_DECL_EXPORT Function : public Callable
{
public:
   Function(StringRef name, ZendCallable callable, const Arguments &arguments = {});
   Function(const Function &other);
   Function &operator=(const Function &other);
   virtual ~Function();

protected:
   void initialize(const std::string &prefix, zend_function_entry *entry);
   void initialize(zend_function_entry *entry);
   friend class ModulePrivate;

private:
   VMAPI_DECLARE_PRIVATE(Function);
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_FUNCTION_H
