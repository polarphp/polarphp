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
};
} // internal

Function::Function(StringRef name, ZendCallable callable, const Arguments &arguments)
   : Callable(new FunctionPrivate(name, callable, arguments))
{}

Function::Function(StringRef name, const Arguments &arguments)
   : Callable(new FunctionPrivate(name, nullptr, arguments))
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

Variant Function::invoke(Parameters &parameters)
{
   // now we just do nothing
   return nullptr;
}

Function::~Function()
{}

} // vmapi
} // polar
