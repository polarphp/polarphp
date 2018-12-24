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

namespace polar {
namespace vmapi {

namespace internal
{

class CallablePrivate
{
public:
   CallablePrivate(const char *name, ZendCallable callable, const Arguments &arguments);
   CallablePrivate(const char *name, const Arguments &arguments);
   void setupCallableArgInfo(zend_internal_arg_info *info, const Argument &arg) const;
   void initialize(zend_function_entry *entry, const char *className = nullptr, int flags = 0) const;
   void initialize(zend_internal_function_info *info, const char *className = nullptr) const;
   void initialize(const std::string &prefix, zend_function_entry *entry);
   CallablePrivate &operator=(const CallablePrivate &other);
   CallablePrivate &operator=(CallablePrivate &&other) noexcept;
   ZendCallable m_callable;
   std::string m_name;
   Type m_returnType = Type::Undefined;
   uint32_t m_required = 0;
   std::string m_retClsName;
   int m_argc = 0;
   std::unique_ptr<zend_internal_arg_info[]> m_argv;
};

} // internal

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_INTERNAL_CALLABLE_PRIVATE_H
