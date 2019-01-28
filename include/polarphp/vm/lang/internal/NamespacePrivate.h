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

#ifndef POLARPHP_VMAPI_LANG_NAMESPACE_PRIVATE_H
#define POLARPHP_VMAPI_LANG_NAMESPACE_PRIVATE_H

#include "polarphp/vm/internal/DepsZendVmHeaders.h"
#include "polarphp/vm/lang/Argument.h"

#include <list>
#include <string>

namespace polar {
namespace vmapi {

/// forward declare class
class AbstractClass;
class Function;
class Constant;
class Namespace;

namespace internal
{

class NamespacePrivate
{
public:
   NamespacePrivate(const char *name)
      : NamespacePrivate(std::string(name))
   {
   }

   NamespacePrivate(const std::string &name)
      : m_name(name)
   {
      VMAPI_ASSERT_X(!name.empty(), "NamespacePrivate", "namespace name can not be empty");
   }

   void iterateFunctions(const std::function<void(const std::string &ns, Function &func)> &callback);
   void initialize(const std::string &ns, int moduleName);
   void initializeConstants(const std::string &ns, int moduleName);
   void initializeClasses(const std::string &ns, int moduleName);
   size_t calculateFunctionCount() const;
   size_t calculateClassCount() const;
   size_t calculateConstantCount() const;
public:
   std::string m_name;
   std::list<std::shared_ptr<Function>> m_functions;
   std::list<std::shared_ptr<AbstractClass>> m_classes;
   std::list<std::shared_ptr<Constant>> m_constants;
   std::list<std::shared_ptr<Namespace>> m_namespaces;
};

} // internal

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_NAMESPACE_PRIVATE_H
