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

#include "polarphp/vm/Closure.h"
#include "polarphp/vm/AbstractClass.h"
#include "polarphp/vm/lang/Class.h"

namespace polar {
namespace vmapi {

zend_class_entry *Closure::m_entry = nullptr;

Closure::Closure(const ClosureCallableType &callable)
   : m_callable(callable)
{}

Variant Closure::__invoke(Parameters &params) const
{
   return m_callable(params);
}

void Closure::registerToZendNg(int moduleNumber)
{
   // here we register ourself to zend engine
   if (m_entry) {
      return;
   }
   // @mark we save meta class as local static is really ok ?
   static std::unique_ptr<AbstractClass> closureWrapper(new Class<Closure>("VmApiClosure", ClassType::Final));
   m_entry = closureWrapper->initialize(moduleNumber);
}

void Closure::unregisterFromZendNg()
{
   m_entry = nullptr;
}

zend_class_entry *Closure::getClassEntry()
{
   return m_entry;
}

Closure::~Closure()
{}

} // vmapi
} // polar
