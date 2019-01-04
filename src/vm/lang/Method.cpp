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

#include "polarphp/vm/lang/Method.h"
#include "polarphp/vm/lang/Parameter.h"
#include "polarphp/vm/StdClass.h"
#include "polarphp/vm/internal/CallablePrivate.h"

namespace polar {
namespace vmapi {

namespace internal
{

class MethodPrivate : public CallablePrivate
{
public:
   MethodPrivate(const char *name, ZendCallable callback, Modifier flags, const Arguments &args)
      : CallablePrivate(name, callback, args)
   {
      m_flags = flags;
   }

   MethodPrivate(const char *name, Modifier flags, const Arguments &args)
      : CallablePrivate(name, args)
   {
      m_flags = flags;
   }

   void initialize(zend_function_entry *entry, const std::string &className);
};

void MethodPrivate::initialize(zend_function_entry *entry, const std::string &className)
{
   if ((m_flags & (Modifier::Public | Modifier::Private | Modifier::Protected)) == 0) {
      m_flags |= Modifier::Public;
   }
   CallablePrivate::initialize(entry, className.c_str(), static_cast<int>(m_flags));
}

} // internal

using internal::MethodPrivate;

Method::Method(const char *name, ZendCallable callback, Modifier flags, const Arguments &args)
   : Callable(new MethodPrivate(name, callback, flags, args))
{}

Method::Method(const char *name, Modifier flags, const Arguments &args)
   : Callable(new MethodPrivate(name, flags, args))
{}

Method::Method(const Method &other)
   : Callable(other)
{
}

Method &Method::operator=(const Method &other)
{
   if (this != &other) {
      Callable::operator=(other);
   }
   return *this;
}

Variant Method::invoke(Parameters &parameters)
{
   // now we just do nothing
   return nullptr;
}

Method::~Method()
{}

void Method::initialize(zend_function_entry *entry, const char *className)
{
   VMAPI_D(Method);
   implPtr->initialize(entry, className);
}

} // vmapi
} // polar
