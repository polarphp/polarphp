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

#ifndef POLARPHP_VMAPI_INTERNAL_CALLABLE_PRIVATE_H
#define POLARPHP_VMAPI_INTERNAL_CALLABLE_PRIVATE_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/lang/Argument.h"
#include "polarphp/basic/adt/StringRef.h"

namespace polar {
namespace vmapi {

namespace internal
{

using polar::basic::StringRef;

class CallablePrivate
{
public:
   CallablePrivate(StringRef name, ZendCallable callable, const Arguments &arguments);
   CallablePrivate(StringRef name, const Arguments &arguments);
   void setupCallableArgInfo(zend_internal_arg_info *info, const Argument &arg) const;
   void initialize(zend_function_entry *entry, bool isMethod = false, int flags = 0) const;
   void initialize(zend_internal_function_info *info, bool isMethod = false) const;
   void initialize(const std::string &prefix, zend_function_entry *entry, int flags = 0);
   CallablePrivate &operator=(const CallablePrivate &other);
   CallablePrivate &operator=(CallablePrivate &&other) noexcept;
   bool m_returnTypeNullable = true;
   Type m_returnType = Type::Undefined;
   uint32_t m_required = 0;
   int m_argc = 0;
   Modifier m_flags = Modifier::None;
   ZendCallable m_callable;
   std::string m_name;
   std::string m_retClsName;
   std::unique_ptr<zend_internal_arg_info[]> m_argv;
};

} // internal

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_INTERNAL_CALLABLE_PRIVATE_H
