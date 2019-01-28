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

#ifndef POLARPHP_VMAPI_LANG_MODULE_H
#define POLARPHP_VMAPI_LANG_MODULE_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/lang/Argument.h"
#include "polarphp/vm/lang/Interface.h"
#include "polarphp/vm/lang/internal/ModulePrivate.h"
#include "polarphp/vm/InvokeBridge.h"
#include "polarphp/vm/AbstractClass.h"
#include "polarphp/utils/TypeTraits.h"

#include <list>

namespace polar {
namespace vmapi {

/// forward declare class
class Variant;
class Parameters;
class Constant;
class Namespace;
class Ini;
template <typename> class Class;
using polar::utils::is_function_ptr;

using internal::ModulePrivate;

class VMAPI_DECL_EXPORT Module
{
public:
   /**
   * Constructor that defines a number of functions right away
   *
   * The first two parameters should be filled by the extension programmer with the
   * name of the extension, and the version number of the extension (like "1.0").
   * The third parameter, apiversion, does not have to be supplied and is best kept
   * to the default value. This third parameter checks whether the PHP-CPP version
   * that is currently installed on the server is the same as the PHP-CPP version
   * that was used to compile the extension with.
   *
   * @param  name        Module name
   * @param  version     Module version string
   * @param  apiversion  ZAPI API version (this should always be VMAPI_API_VERSION, so you better not supply it)
   */
   Module(const char *name, const char *version = "1.0", int apiVersion = VMAPI_API_VERSION);
   Module(const Module &extension) = delete;
   Module(Module &&extension) = delete;
   virtual ~Module();
public:
   template <typename CallableType,
             typename std::decay<CallableType>::type callable,
             typename DecayCallableType = typename std::decay<CallableType>::type,
             typename std::enable_if<is_function_ptr<DecayCallableType>::value &&
                                     callable_prototype_checker<DecayCallableType>::value, DecayCallableType>::type * = nullptr>
   Module &registerFunction(const char *name, const Arguments &args = {});

   Module &registerIni(const Ini &entry);
   Module &registerIni(Ini &&entry);

   template <typename T>
   Module &registerClass(const Class<T> &nativeClass);
   template <typename T>
   Module &registerClass(Class<T> &&nativeClass);

   Module &registerInterface(const Interface &interface);
   Module &registerInterface(Interface &&interface);

   Module &registerNamespace(std::shared_ptr<Namespace> ns);
   Module &registerNamespace(const Namespace &ns);
   Module &registerNamespace(Namespace &&ns);

   Module &registerConstant(Constant &&constant);
   Module &registerConstant(const Constant &constant);

   Namespace *findNamespace(const std::string &ns) const;
   AbstractClass *findClass(const std::string &clsName) const;

   size_t getIniCount() const;
   size_t getFunctionCount() const;
   size_t getConstantCount() const;
   size_t getNamespaceCount() const;
   size_t getClassCount() const;

   bool registerToVM();

   operator void * ()
   {
      return getModule();
   }

   /**
   * Register a function to be called when the PHP engine is ready
   *
   * The callback will be called after all extensions are loaded, and all
   * functions and classes are available, but before the first pageview/request
   * is handled. You can register this callback if you want to be notified
   * when the engine is ready, for example to initialize certain things.
   *
   * @param Callback callback Function to be called
   * @return Module Same object to allow chaining
   */
   Module &setStartupHandler(const Callback &callback);

   /**
   * Register a function to be called when the PHP engine is going to stop
   *
   * The callback will be called right before the process is going to stop.
   * You can register a function if you want to clean up certain things.
   *
   * @param callback Function to be called
   * @return Module Same object to allow chaining
   */
   Module &setShutdownHandler(const Callback &callback);

   /**
   * Register a callback that is called at the beginning of each pageview/request
   *
   * You can register a callback if you want to initialize certain things
   * at the beginning of each request. Remember that the extension can handle
   * multiple requests after each other, and you may want to set back certain
   * global variables to their initial variables in front of each request
   *
   * @param Callback callback Function to be called
   * @return Module Same object to allow chaining
   */
   Module &setRequestStartupHandler(const Callback &callback);

   /**
   * Register a callback that is called to cleanup things after a pageview/request
   *
   * The callback will be called after _each_ request, so that you can clean up
   * certain things and make your extension ready to handle the next request.
   * This method is called onIdle because the extension is idle in between
   * requests.
   *
   * @param Callback callback Function to be called
   * @return Module Same object to allow chaining
   */
   Module &setRequestShutdownHandler(const Callback &callback);

   Module &setInfoHandler(const Callback &callback);

   /**
   * Retrieve the module pointer
   *
   * This is the memory address that should be exported by the get_module()
   * function.
   *
   * @return void*
   */
   void *getModule();
   const char *getName() const;
   const char *getVersion() const;
protected:
   Module &registerFunction(const char *name, ZendCallable function, const Arguments &args);
   bool isLocked() const;
private:
   bool initialize(int moduleNumber);
private:
   VMAPI_DECLARE_PRIVATE(Module);
   /**
   * The implementation object
   *
   * @var std::unique_ptr<ModulePrivate> m_implPtr
   */
   std::unique_ptr<internal::ModulePrivate> m_implPtr;
};

template <typename T>
Module &Module::registerClass(const Class<T> &nativeClass)
{
   VMAPI_D(Module);
   if (implPtr->m_locked) {
      return *this;
   }
   // just shadow copy
   implPtr->m_classes.push_back(std::shared_ptr<AbstractClass>(new Class<T>(nativeClass)));
   return *this;
}

template <typename T>
Module &Module::registerClass(Class<T> &&nativeClass)
{
   VMAPI_D(Module);
   if (implPtr->m_locked) {
      return *this;
   }
   implPtr->m_classes.push_back(std::shared_ptr<AbstractClass>(new Class<T>(std::move(nativeClass))));
   return *this;
}

template <typename CallableType,
          typename std::decay<CallableType>::type callable,
          typename DecayCallableType = typename std::decay<CallableType>::type,
          typename std::enable_if<is_function_ptr<DecayCallableType>::value &&
                                  callable_prototype_checker<DecayCallableType>::value, DecayCallableType>::type * = nullptr>
Module &Module::registerFunction(const char *name, const Arguments &args)
{
   return registerFunction(name, &InvokeBridge<CallableType, callable>::invoke, args);
}

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_MODULE_H
