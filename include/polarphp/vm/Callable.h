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

#ifndef POLARPHP_VMAPI_CALLABLE_H
#define POLARPHP_VMAPI_CALLABLE_H

#include <string>

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/lang/Argument.h"
#include "polarphp/basic/adt/StringRef.h"

namespace polar {
namespace vmapi {

/// forward declare class
class Variant;
class Parameters;
namespace internal
{
class ModulePrivate;
class CallablePrivate;
} // internal

using internal::CallablePrivate;
using internal::ModulePrivate;
using polar::basic::StringRef;

class VMAPI_DECL_EXPORT Callable
{
public:
   Callable();
   Callable(StringRef name, ZendCallable callable, const Arguments &arguments = {});
   Callable(StringRef name, const Arguments &arguments = {});
   Callable(const Callable &other);
   Callable(Callable &&other) noexcept;
   virtual ~Callable() = 0;

public:
   Callable &operator =(const Callable &other);
   Callable &operator =(Callable &&other) noexcept;
   Callable &setReturnType(Type type, bool nullable = true) noexcept;
   Callable &setReturnType(StringRef clsName, bool nullable = true) noexcept;
   Callable &markDeprecated() noexcept;
   ///
   /// for unittest only
   ///
   zend_function_entry buildCallableEntry(bool isMethod = false) const noexcept;

public:
   virtual Variant invoke(Parameters &parameters);

protected:
   Callable(CallablePrivate *implPtr);
   void setupCallableArgInfo(zend_internal_arg_info *info, const Argument &arg) const;
   static void invoke(INTERNAL_FUNCTION_PARAMETERS);
   void initialize(zend_function_entry *entry, bool isMethod = false, int flags = 0) const;
   void initialize(zend_internal_function_info *info, bool isMethod = false) const;
   void initialize(const std::string &prefix, zend_function_entry *entry);

protected:
   VMAPI_DECLARE_PRIVATE(Callable);
   std::shared_ptr<CallablePrivate> m_implPtr;
   friend class ModulePrivate;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_CALLABLE_H
