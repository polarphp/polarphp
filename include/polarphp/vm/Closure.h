// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/23.

#ifndef POLARPHP_VMAPI_CLOSURE_H
#define POLARPHP_VMAPI_CLOSURE_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/StdClass.h"

#include <functional>

namespace polar {
namespace vmapi {

/// forward declare class
class Variant;
class Parameters;
namespace internal {
class ModulePrivate;
} // internal
using internal::ModulePrivate;
using ClosureCallableType = std::function<Variant(Parameters &)>;

class Closure final : public StdClass
{
public:
   Closure(const ClosureCallableType &callable);
   Variant __invoke(Parameters &params) const;
   virtual ~Closure();
   static zend_class_entry *getClassEntry();
private:
   static void registerToZendNg(int moduleNumber);
   static void unregisterFromZendNg();
private:
   friend class ModulePrivate;
   static zend_class_entry *m_entry;
   const ClosureCallableType m_callable;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_CLOSURE_H
