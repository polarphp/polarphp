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

#include "polarphp/vm/internal/CallablePrivate.h"
#include "polarphp/vm/internal/AbstractClassPrivate.h"
#include "polarphp/vm/internal/DepsZendVmHeaders.h"
#include "polarphp/vm/lang/Module.h"
#include "polarphp/vm/lang/internal/ModulePrivate.h"
#include "polarphp/vm/lang/internal/NamespacePrivate.h"
#include "polarphp/vm/lang/Ini.h"
#include "polarphp/vm/lang/Function.h"
#include "polarphp/vm/lang/Constant.h"
#include "polarphp/vm/lang/Namespace.h"
#include "polarphp/vm/Closure.h"
#include "polarphp/runtime/RtDefs.h"

#include <ostream>
#include <map>

/**
 * We're almost there, we now need to declare an instance of the
 * structure defined above (if building for a single thread) or some
 * sort of impossible to understand magic pointer-to-a-pointer (for
 * multi-threading builds). We make this a static variable because
 * this already is bad enough.
 */

namespace polar {
namespace vmapi {

using internal::ModulePrivate;
using internal::AbstractClassPrivate;
using internal::NamespacePrivate;

namespace
{

std::map<std::string, Module *> name2extension;
std::map<int, Module *> mid2extension;

int match_module(zval *value)
{
   zend_module_entry *entry = static_cast<zend_module_entry *>(Z_PTR_P(value));
   auto iter = name2extension.find(entry->name);
   if (iter == name2extension.end()) {
      return ZEND_HASH_APPLY_KEEP;
   }
   mid2extension[entry->module_number] = iter->second;
   return ZEND_HASH_APPLY_KEEP;
}

Module *find_module(int mid)
{
   auto iter = mid2extension.find(mid);
   if (iter != mid2extension.end()) {
      return iter->second;
   }
   zend_hash_apply(&module_registry, match_module);
   iter = mid2extension.find(mid);
   if (iter == mid2extension.end()) {
      return nullptr;
   }
   return iter->second;
}
} // anonymous namespace


Module::Module(const char *name, const char *version, int apiVersion)
   : m_implPtr(new ModulePrivate(name, version, apiVersion, this))
{
   name2extension[name] = this;
}

Module::~Module()
{}

Module &Module::setStartupHandler(const Callback &callback)
{
   VMAPI_D(Module);
   implPtr->m_startupHandler = callback;
   return *this;
}

Module &Module::setShutdownHandler(const Callback &callback)
{
   VMAPI_D(Module);
   implPtr->m_shutdownHandler = callback;
   return *this;
}

Module &Module::setRequestStartupHandler(const Callback &callback)
{
   VMAPI_D(Module);
   implPtr->m_requestStartupHandler = callback;
   return *this;
}

Module &Module::setRequestShutdownHandler(const Callback &callback)
{
   VMAPI_D(Module);
   implPtr->m_requestShutdownHandler = callback;
   return *this;
}

Module &Module::setInfoHandler(const Callback &callback)
{
   VMAPI_D(Module);
   implPtr->m_minfoHandler = callback;
   return *this;
}

void *Module::getModule()
{
   return getImplPtr()->getModule();
}

bool Module::isLocked() const
{
   VMAPI_D(const Module);
   return implPtr->m_locked;
}

const char *Module::getName() const
{
   VMAPI_D(const Module);
   return implPtr->m_entry.name;
}

const char *Module::getVersion() const
{
   VMAPI_D(const Module);
   return implPtr->m_entry.version;
}

Module &Module::registerFunction(const char *name, ZendCallable function,
                                 const Arguments &arguments)
{
   getImplPtr()->registerFunction(name, function, arguments);
   return *this;
}

Module &Module::registerInterface(const Interface &interface)
{
   VMAPI_D(Module);
   if (implPtr->m_locked) {
      return *this;
   }
   implPtr->m_classes.push_back(std::make_shared<Interface>(interface));
   return *this;
}

Module &Module::registerInterface(Interface &&interface)
{

   VMAPI_D(Module);
   if (implPtr->m_locked) {
      return *this;
   }
   implPtr->m_classes.push_back(std::make_shared<Interface>(std::move(interface)));
   return *this;
}

Module &Module::registerNamespace(std::shared_ptr<Namespace> ns)
{
   VMAPI_D(Module);
   if (implPtr->m_locked) {
      return *this;
   }
   implPtr->m_namespaces.push_back(ns);
   return *this;
}

Module &Module::registerNamespace(const Namespace &ns)
{
   VMAPI_D(Module);
   if (implPtr->m_locked) {
      return *this;
   }
   implPtr->m_namespaces.push_back(std::make_shared<Namespace>(ns));
   return *this;
}

Module &Module::registerNamespace(Namespace &&ns)
{
   VMAPI_D(Module);
   if (implPtr->m_locked) {
      return *this;
   }
   implPtr->m_namespaces.push_back(std::make_shared<Namespace>(std::move(ns)));
   return *this;
}

Module &Module::registerConstant(const Constant &constant)
{
   VMAPI_D(Module);
   if (implPtr->m_locked) {
      return *this;
   }
   implPtr->m_constants.push_back(std::make_shared<Constant>(constant));
   return *this;
}

Module &Module::registerConstant(Constant &&constant)
{
   VMAPI_D(Module);
   if (implPtr->m_locked) {
      return *this;
   }
   implPtr->m_constants.push_back(std::make_shared<Constant>(std::move(constant)));
   return *this;
}

Namespace *Module::findNamespace(const std::string &ns) const
{
   VMAPI_D(const Module);
   auto begin = implPtr->m_namespaces.begin();
   auto end = implPtr->m_namespaces.end();
   while (begin != end) {
      auto &cur = *begin;
      if (cur->getName() == ns) {
         return cur.get();
      }
      ++begin;
   }
   return nullptr;
}

AbstractClass *Module::findClass(const std::string &clsName) const
{
   VMAPI_D(const Module);
   auto begin = implPtr->m_classes.begin();
   auto end = implPtr->m_classes.end();
   while (begin != end) {
      auto &cur = *begin;
      if (cur->getClassName() == clsName) {
         return cur.get();
      }
      ++begin;
   }
   return nullptr;
}

bool Module::initialize(int moduleNumber)
{
   return getImplPtr()->initialize(moduleNumber);
}

Module &Module::registerIni(const Ini &entry)
{
   VMAPI_D(Module);
   if (isLocked()) {
      return *this;
   }
   implPtr->m_iniEntries.emplace_back(new Ini(entry));
   return *this;
}

Module &Module::registerIni(Ini &&entry)
{
   VMAPI_D(Module);
   if (isLocked()) {
      return *this;
   }
   implPtr->m_iniEntries.emplace_back(new Ini(std::move(entry)));
   return *this;
}

size_t Module::getFunctionCount() const
{
   VMAPI_D(const Module);
   return implPtr->m_functions.size();
}

size_t Module::getIniCount() const
{
   VMAPI_D(const Module);
   return implPtr->m_iniEntries.size();
}

size_t Module::getConstantCount() const
{
   VMAPI_D(const Module);
   return implPtr->m_constants.size();
}

size_t Module::getNamespaceCount() const
{
   VMAPI_D(const Module);
   return implPtr->m_namespaces.size();
}

size_t Module::getClassCount() const
{
   VMAPI_D(const Module);
   return implPtr->m_classes.size();
}

bool Module::registerToVM()
{
   zend_module_entry *mentry = reinterpret_cast<zend_module_entry *>(getModule());
   if (mentry->zend_api != ZEND_MODULE_API_NO) {
      php_error_docref(NULL, E_CORE_WARNING,
                       "%s: Unable to initialize module\n"
                       "Module compiled with module API=%d\n"
                       "PHP    compiled with module API=%d\n"
                       "These options need to match\n",
                       mentry->name, mentry->zend_api, ZEND_MODULE_API_NO);
      return false;
   }
   if(strcmp(mentry->build_id, ZEND_MODULE_BUILD_ID)) {
      php_error_docref(NULL, E_CORE_WARNING,
                       "%s: Unable to initialize module\n"
                       "Module compiled with build ID=%s\n"
                       "PHP    compiled with build ID=%s\n"
                       "These options need to match\n",
                       mentry->name, mentry->build_id, ZEND_MODULE_BUILD_ID);
      return false;
   }
   mentry->type = MODULE_PERSISTENT;
   mentry->module_number = zend_next_free_module();
   mentry->handle = nullptr;
   if ((mentry = zend_register_module_ex(mentry)) == NULL) {
      return false;
   }
   return true;
}

namespace internal
{
ModulePrivate::ModulePrivate(const char *name, const char *version, int apiversion, Module *extension)
   : m_apiPtr(extension)
{
   // assign all members (apart from the globals)
   m_entry.size = sizeof(zend_module_entry);
   m_entry.zend_api = ZEND_MODULE_API_NO;
   m_entry.zend_debug = ZEND_DEBUG;
   m_entry.zts = USING_ZTS;
   m_entry.ini_entry = nullptr;
   m_entry.deps = nullptr;
   m_entry.name = name;
   m_entry.functions = nullptr;
   m_entry.module_startup_func = &ModulePrivate::processStartup;
   m_entry.module_shutdown_func = &ModulePrivate::processShutdown;
   m_entry.request_startup_func = &ModulePrivate::processRequestStartup;
   m_entry.request_shutdown_func = &ModulePrivate::processRequestShutdown;
   m_entry.info_func = &ModulePrivate::processModuleInfo;
   m_entry.version = version;
   m_entry.globals_size = 0;
   m_entry.globals_ctor = nullptr;
   m_entry.globals_dtor = nullptr;
   m_entry.post_deactivate_func = nullptr;
   m_entry.module_started = 0;
   m_entry.type = 0;
   m_entry.handle = nullptr;
   m_entry.module_number = 0;
   m_entry.build_id = const_cast<char *>(static_cast<const char *>(ZEND_MODULE_BUILD_ID));
   m_entry.globals_id_ptr = nullptr;
   if (apiversion == VMAPI_API_VERSION) {
      return;
   }
   // mismatch between api versions, the extension is invalid, we use a
   // different startup function to report to the user
   m_entry.module_startup_func = &ModulePrivate::processMismatch;
   // the other callback functions are no longer necessary
   m_entry.module_shutdown_func = nullptr;
   m_entry.request_startup_func = nullptr;
   m_entry.request_shutdown_func = nullptr;
   m_entry.info_func = nullptr;
}

ModulePrivate::~ModulePrivate()
{
   name2extension.erase(m_entry.name);
   delete[] m_entry.functions;
}

size_t ModulePrivate::getFunctionCount() const
{
   // now just return global namespaces functions
   size_t ret = m_functions.size();
   for (const std::shared_ptr<Namespace> &ns : m_namespaces) {
      ret += ns->getFunctionCount();
   }
   return ret;
}

size_t ModulePrivate::getIniCount() const
{
   return m_iniEntries.size();
}

zend_module_entry *ModulePrivate::getModule()
{
   if (m_entry.functions) {
      return &m_entry;
   }
   if (m_entry.module_startup_func == &ModulePrivate::processMismatch) {
      return &m_entry;
   }
   size_t count = getFunctionCount();
   if (0 == count) {
      return &m_entry;
   }
   int i = 0;
   zend_function_entry *entries = new zend_function_entry[count + 1];
   iterateFunctions([&i, entries](Function &callable){
      callable.initialize(&entries[i]);
      i++;
   });
   for (std::shared_ptr<Namespace> &ns : m_namespaces) {
      ns->m_implPtr->iterateFunctions([&i, entries](const std::string &ns, Function &callable){
         callable.initialize(ns, &entries[i]);
         i++;
      });
   }
   zend_function_entry *last = &entries[count];
   memset(last, 0, sizeof(zend_function_entry));
   m_entry.functions = entries;
   return &m_entry;
}

void ModulePrivate::iterateFunctions(const std::function<void(Function &func)> &callback)
{
   for (auto &function : m_functions) {
      callback(*function);
   }
}

void ModulePrivate::iterateIniEntries(const std::function<void (Ini &)> &callback)
{
   for (auto &entry : m_iniEntries) {
      callback(*entry);
   }
}

void ModulePrivate::iterateConstants(const std::function<void (Constant &)> &callback)
{
   for (auto &constant : m_constants) {
      callback(*constant);
   }
}

void ModulePrivate::iterateClasses(const std::function<void(AbstractClass &cls)> &callback)
{
   for (auto &cls : m_classes) {
      callback(*cls);
   }
}

int ModulePrivate::processMismatch(INIT_FUNC_ARGS)
{
   Module *extension = find_module(module_number);
   // @TODO is this really good? we need a method to check compatibility more graceful
   vmapi::warning() << " Version mismatch between polarphp vmapi and extension " << extension->getName()
                    << " " << extension->getVersion() << " (recompile needed?) " << std::endl;
   return BOOL2SUCCESS(true);
}

int ModulePrivate::processRequestStartup(INIT_FUNC_ARGS)
{
   Module *extension = find_module(module_number);
   if (extension->m_implPtr->m_requestStartupHandler) {
      extension->m_implPtr->m_requestStartupHandler();
   }
   return BOOL2SUCCESS(true);
}

int ModulePrivate::processRequestShutdown(SHUTDOWN_FUNC_ARGS)
{
   Module *extension = find_module(module_number);
   if (extension->m_implPtr->m_requestShutdownHandler) {
      extension->m_implPtr->m_requestShutdownHandler();
   }
   // release call context
   return BOOL2SUCCESS(true);
}

int ModulePrivate::processStartup(INIT_FUNC_ARGS)
{
   Module *extension = find_module(module_number);
   return BOOL2SUCCESS(extension->initialize(module_number));
}

int ModulePrivate::processShutdown(SHUTDOWN_FUNC_ARGS)
{
   Module *extension = find_module(module_number);
   mid2extension.erase(module_number);
   return BOOL2SUCCESS(extension->m_implPtr->shutdown(module_number));
}

void ModulePrivate::processModuleInfo(ZEND_MODULE_INFO_FUNC_ARGS)
{
   Module *extension = find_module(zend_module->module_number);
   if (extension->m_implPtr->m_minfoHandler) {
      extension->m_implPtr->m_minfoHandler();
   }
}

ModulePrivate &ModulePrivate::registerFunction(const char *name, ZendCallable function,
                                               const Arguments &arguments)
{
   if (m_locked) {
      return *this;
   }
   m_functions.push_back(std::make_shared<Function>(name, function, arguments));
   return *this;
}

bool ModulePrivate::initialize(int moduleNumber)
{
   m_zendIniDefs.reset(new zend_ini_entry_def[getIniCount() + 1]);
   int i = 0;
   // fill ini entry def
   iterateIniEntries([this, &i, moduleNumber](Ini &iniEntry){
      zend_ini_entry_def *zendIniDef = &m_zendIniDefs[i];
      iniEntry.setupIniDef(zendIniDef, moduleNumber);
      i++;
   });
   memset(&m_zendIniDefs[i], 0, sizeof(m_zendIniDefs[i]));
   zend_register_ini_entries(m_zendIniDefs.get(), moduleNumber);

   iterateConstants([moduleNumber](Constant &constant) {
      constant.initialize(moduleNumber);
   });
   // here we register all global classes and interfaces
   iterateClasses([moduleNumber](AbstractClass &cls) {
      cls.initialize(moduleNumber);
   });
   // work with register namespaces

   for (std::shared_ptr<Namespace> &ns : m_namespaces) {
      ns->initialize(moduleNumber);
   }
   // initialize closure class
   Closure::registerToZendNg(moduleNumber);

   // remember that we're initialized (when you use "apache reload" it is
   // possible that the processStartup() method is called more than once)
   m_locked = true;
   if (m_startupHandler) {
      m_startupHandler();
   }
   return true;
}

bool ModulePrivate::shutdown(int moduleNumber)
{
   zend_unregister_ini_entries(moduleNumber);
   m_zendIniDefs.reset();
   if (m_shutdownHandler) {
      m_shutdownHandler();
   }
   Closure::unregisterFromZendNg();
   m_locked = false;
   return true;
}
} // internal

} // vmapi
} // polar
