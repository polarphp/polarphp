//// This source file is part of the polarphp.org open source project
////
//// Copyright (c) 2017 - 2018 polarphp software foundation
//// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
//// Licensed under Apache License v2.0 with Runtime Library Exception
////
//// See https://polarphp.org/LICENSE.txt for license information
//// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
////
//// Created by polarboy on 2019/02/07.

#include "polarphp/vm/lang/Interface.h"
#include "polarphp/vm/lang/Module.h"
#include "Interfaces.h"

namespace php {

using polar::vmapi::Interface;
using polar::vmapi::Modifier;

void register_interfaces_hook(Module &module)
{
   Interface interfaceA("InterfaceA");
   Interface interfaceB("InterfaceB");
   Interface interfaceC("InterfaceC");
   Interface infoInterface("InfoProvider");

   interfaceA.registerMethod("methodOfA");
   interfaceA.registerMethod("protectedMethodOfA", Modifier::Protected);
   interfaceA.registerMethod("privateMethodOfA", Modifier::Private);
   interfaceB.registerMethod("methodOfB");
   interfaceB.registerMethod("protectedMethodOfB", Modifier::Protected);
   interfaceB.registerMethod("privateMethodOfB", Modifier::Private);
   interfaceC.registerMethod("methodOfC");
   interfaceC.registerMethod("protectedMethodOfC", Modifier::Protected);
   interfaceC.registerMethod("privateMethodOfC", Modifier::Private);
   interfaceC.registerBaseInterface(interfaceB);
   interfaceB.registerBaseInterface(interfaceA);

   module.registerInterface(infoInterface);
   module.registerInterface(interfaceA);
   module.registerInterface(interfaceB);
   module.registerInterface(interfaceC);
}

} // php
