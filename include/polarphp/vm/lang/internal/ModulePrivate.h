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

#ifndef POLARPHP_VMAPI_LANG_EXTENSION_PRIVATE_H
#define POLARPHP_VMAPI_LANG_EXTENSION_PRIVATE_H

#include "polarphp/vm/internal/DepsZendVmHeaders.h"
#include "polarphp/vm/lang/Argument.h"

#include <list>

namespace polar {
namespace vmapi {

/// forward declare class
class Function;
class Constant;
class Namespace;
class Ini;
class Module;
class AbstractClass;

namespace internal
{
class ModulePrivate
{
public:
   VMAPI_DECLARE_PUBLIC(Module);
   ModulePrivate(Module *module)
      :m_apiPtr(module)
   {}

   ModulePrivate(const char *name, const char *version, int apiversion, Module *module);
   ModulePrivate(const ModulePrivate &) = delete;
   ModulePrivate(const ModulePrivate &&) = delete;
   ~ModulePrivate();

   // methods

   ModulePrivate &registerFunction(const char *name, ZendCallable function, const Arguments &arguments = {});
   void iterateFunctions(const std::function<void(Function &func)> &callback);
   void iterateIniEntries(const std::function<void(Ini &ini)> &callback);
   void iterateConstants(const std::function<void(Constant &constant)> &callback);
   void iterateClasses(const std::function<void(AbstractClass &cls)> &callback);

   zend_module_entry *getModule();
   size_t getFunctionCount() const;
   size_t getIniCount() const;
   bool initialize(int moduleNumber);
   bool shutdown(int moduleNumber);
   static int processStartup(INIT_FUNC_ARGS);
   static int processShutdown(SHUTDOWN_FUNC_ARGS);
   static int processRequestStartup(INIT_FUNC_ARGS);
   static int processRequestShutdown(SHUTDOWN_FUNC_ARGS);
   static int processMismatch(INIT_FUNC_ARGS);
   static void processModuleInfo(ZEND_MODULE_INFO_FUNC_ARGS);
   // properties

   Module *m_apiPtr;
   Callback m_startupHandler;
   Callback m_requestStartupHandler;
   Callback m_requestShutdownHandler;
   Callback m_shutdownHandler;
   Callback m_minfoHandler;
   zend_module_entry m_entry;
   bool m_locked = false;
   std::list<std::shared_ptr<Ini>> m_iniEntries;
   std::unique_ptr<zend_ini_entry_def[]> m_zendIniDefs = nullptr;
   std::list<std::shared_ptr<Function>> m_functions;
   std::list<std::shared_ptr<Constant>> m_constants;
   std::list<std::shared_ptr<AbstractClass>> m_classes;
   std::list<std::shared_ptr<Namespace>> m_namespaces;
};

} // internal
} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_EXTENSION_PRIVATE_H
