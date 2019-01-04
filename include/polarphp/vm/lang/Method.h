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

#ifndef POLARPHP_VMAPI_LANG_METHOD_H
#define POLARPHP_VMAPI_LANG_METHOD_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/Callable.h"

namespace polar {
namespace vmapi {

namespace internal
{
class MethodPrivate;
class AbstractClassPrivate;
} // internal

using internal::MethodPrivate;
using internal::AbstractClassPrivate;

class VMAPI_DECL_EXPORT Method : public Callable
{
public:
   Method(const char *name, ZendCallable callback, Modifier flags, const Arguments &args);
   Method(const char *name, Modifier flags, const Arguments &args);
   Method(const Method &other);
   Method &operator=(const Method &other);
   virtual ~Method();
   virtual Variant invoke(Parameters &parameters) override;
protected:
   void initialize(zend_function_entry *entry, const char *className);

private:
   VMAPI_DECLARE_PRIVATE(Method);
   friend class AbstractClassPrivate;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_METHOD_H
