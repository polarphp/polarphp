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

#include "polarphp/vm/lang/Function.h"
#include "polarphp/vm/ds/Variant.h"
#include "polarphp/vm/internal/CallablePrivate.h"

namespace polar {
namespace vmapi {

namespace internal {
class FunctionPrivate : public CallablePrivate
{
public:
   using CallablePrivate::CallablePrivate;
   void initialize(const std::string &prefix, zend_function_entry *entry);
   void initialize(zend_function_entry *entry);
};

void FunctionPrivate::initialize(const std::string &prefix, zend_function_entry *entry)
{
   /// TODO mask unsupport flags
   CallablePrivate::initialize(prefix, entry, static_cast<int>(m_flags));
}

void FunctionPrivate::initialize(zend_function_entry *entry)
{
   CallablePrivate::initialize("", entry, static_cast<int>(m_flags));
}

} // internal

Function::Function(StringRef name, ZendCallable callable, const Arguments &arguments)
   : Callable(new FunctionPrivate(name, callable, arguments))
{}

Function::Function(const Function &other)
   : Callable(other)
{
}

Function &Function::operator=(const Function &other)
{
   if (this != &other) {
      Callable::operator=(other);
   }
   return *this;
}

void Function::initialize(const std::string &prefix, zend_function_entry *entry)
{
   VMAPI_D(Function);
   implPtr->initialize(prefix, entry);
}

void Function::initialize(zend_function_entry *entry)
{
   VMAPI_D(Function);
   implPtr->initialize(entry);
}

Function::~Function()
{}

} // vmapi
} // polar
