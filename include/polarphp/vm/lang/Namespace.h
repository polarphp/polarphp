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

#ifndef POLARPHP_VMAPI_LANG_NAMESPACE_H
#define POLARPHP_VMAPI_LANG_NAMESPACE_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/InvokeBridge.h"
#include "polarphp/vm/lang/Class.h"
#include "polarphp/vm/lang/internal/NamespacePrivate.h"
#include "polarphp/utils/TypeTraits.h"

namespace polar {
namespace vmapi {

namespace internal
{
class ModulePrivate;
} // internal

class Constant;
using internal::ModulePrivate;
using polar::utils::is_function_ptr;

class VMAPI_DECL_EXPORT Namespace final
{
public:
   Namespace(const std::string &name);
   Namespace(const char *name);
   Namespace(const Namespace &other);
   Namespace(Namespace &&other) noexcept;
   Namespace &operator=(const Namespace &other);
   Namespace &operator=(Namespace &&other) noexcept;
   virtual ~Namespace();
public:
   template <typename CallableType,
             typename std::decay<CallableType>::type callable,
             typename DecayCallableType = typename std::decay<CallableType>::type,
             typename std::enable_if<is_function_ptr<DecayCallableType>::value &&
                                     callable_prototype_checker<DecayCallableType>::value, DecayCallableType>::type * = nullptr>
   Namespace &registerFunction(const char *name, const Arguments &args = {});

   Namespace &registerNamespace(const Namespace &ns);
   Namespace &registerNamespace(Namespace &&ns);
   Namespace &registerConstant(const Constant &constant);
   Namespace &registerConstant(Constant &&constant);

   template <typename T>
   Namespace &registerClass(const Class<T> &nativeClass);
   template <typename T>
   Namespace &registerClass(Class<T> &&nativeClass);

   Namespace *findNamespace(const std::string &ns) const;
   AbstractClass *findClass(const std::string &clsName) const;

   size_t getFunctionCount() const;
   size_t getConstantQuanlity() const;
   size_t getClassQuanlity() const;

   const std::string &getName() const;
protected:
   Namespace(NamespacePrivate *implPtr);
   Namespace &registerFunction(const char *name, ZendCallable function, const Arguments &arguments = {});
   void initialize(int moduleNumber);

private:
   VMAPI_DECLARE_PRIVATE(Namespace);
   std::shared_ptr<NamespacePrivate> m_implPtr;
   friend class ModulePrivate;
};

template <typename T>
Namespace &Namespace::registerClass(const Class<T> &nativeClass)
{
   VMAPI_D(Namespace);
   implPtr->m_classes.push_back(std::shared_ptr<AbstractClass>(new Class<T>(nativeClass)));
   return *this;
}

template <typename T>
Namespace &Namespace::registerClass(Class<T> &&nativeClass)
{
   VMAPI_D(Namespace);
   implPtr->m_classes.push_back(std::shared_ptr<AbstractClass>(new Class<T>(std::move(nativeClass))));
   return *this;
}

template <typename CallableType,
          typename std::decay<CallableType>::type callable,
          typename DecayCallableType = typename std::decay<CallableType>::type,
          typename std::enable_if<is_function_ptr<DecayCallableType>::value &&
                                  callable_prototype_checker<DecayCallableType>::value, DecayCallableType>::type * = nullptr>
Namespace &Namespace::registerFunction(const char *name, const Arguments &args)
{
   return registerFunction(name, &InvokeBridge<CallableType, callable>::invoke, args);
}

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_NAMESPACE_H
