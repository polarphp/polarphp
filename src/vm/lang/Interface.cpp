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

#include "polarphp/vm/lang/Interface.h"

namespace polar {
namespace vmapi {

Interface::Interface(const char *name)
   : AbstractClass(name, ClassType::Interface)
{}

Interface::~Interface()
{}

Interface &Interface::registerMethod(const char *name, const Arguments arguments)
{
   AbstractClass::registerMethod(name, Modifier::Abstract | Modifier::Public, arguments);
   return *this;
}

Interface &Interface::registerMethod(const char *name, Modifier flags, const Arguments arguments)
{
   AbstractClass::registerMethod(name, (flags | Modifier::Public | Modifier::Abstract), arguments);
   return *this;
}

Interface &Interface::registerBaseInterface(const Interface &interface)
{
   AbstractClass::registerInterface(interface);
   return *this;
}

} // vmapi
} // polar
