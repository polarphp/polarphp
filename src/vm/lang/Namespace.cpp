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

#include "polarphp/vm/lang/Namespace.h"
#include "polarphp/vm/lang/internal/NamespacePrivate.h"
#include "polarphp/vm/lang/Constant.h"
#include "polarphp/vm/lang/Function.h"

#include <list>
#include <string>

namespace polar {
namespace vmapi {

namespace internal
{

void NamespacePrivate::initialize(const std::string &ns, int moduleNumber)
{
   initializeConstants(ns, moduleNumber);
   initializeClasses(ns, moduleNumber);
   // recursive initialize
   for (std::shared_ptr<Namespace> &subns : m_namespaces) {
      subns->m_implPtr->initialize(ns + "\\" + subns->m_implPtr->m_name, moduleNumber);
   }
}

void NamespacePrivate::iterateFunctions(const std::function<void(const std::string &ns, Function &func)> &callback)
{
   for (std::shared_ptr<Function> &func : m_functions) {
      callback(m_name, *func);
   }
   for (std::shared_ptr<Namespace> &subns : m_namespaces) {
      subns->m_implPtr->iterateFunctions([this, callback](const std::string &ns, Function &func){
         callback(m_name + "\\" + ns, func);
      });
   }
}

void NamespacePrivate::initializeConstants(const std::string &ns, int moduleNumber)
{
   // register self
   for (std::shared_ptr<Constant> &constant : m_constants) {
      constant->initialize(ns, moduleNumber);
   }
}

void NamespacePrivate::initializeClasses(const std::string &ns, int moduleNumber)
{
   // register self
   for (std::shared_ptr<AbstractClass> &cls : m_classes) {
      cls->initialize(ns, moduleNumber);
   }
}

size_t NamespacePrivate::calculateFunctionCount() const
{
   size_t ret = m_functions.size();
   for (const std::shared_ptr<Namespace> &ns : m_namespaces) {
      ret += ns->m_implPtr->calculateFunctionCount();
   }
   return ret;
}

size_t NamespacePrivate::calculateClassCount() const
{
   size_t ret = m_classes.size();
   for (const std::shared_ptr<Namespace> &ns : m_namespaces) {
      ret += ns->m_implPtr->calculateClassCount();
   }
   return ret;
}

size_t NamespacePrivate::calculateConstantCount() const
{
   size_t ret = m_constants.size();
   for (const std::shared_ptr<Namespace> &ns : m_namespaces) {
      ret += ns->m_implPtr->calculateConstantCount();
   }
   return ret;
}

} // internal

Namespace::Namespace(const std::string &name)
   : m_implPtr(new NamespacePrivate(name))
{}

Namespace::Namespace(const char *name)
   : m_implPtr(new NamespacePrivate(name))
{}

Namespace::Namespace(NamespacePrivate *implPtr)
   : m_implPtr(implPtr)
{}

Namespace::Namespace(const Namespace &other)
   : m_implPtr(other.m_implPtr)
{}

Namespace::Namespace(Namespace &&other) noexcept
   : m_implPtr(std::move(other.m_implPtr))
{}

Namespace &Namespace::operator=(const Namespace &other)
{
   if (this != &other) {
      m_implPtr = other.m_implPtr;
   }
   return *this;
}

Namespace &Namespace::operator=(Namespace &&other) noexcept
{
   assert(this != &other);
   m_implPtr = std::move(other.m_implPtr);
   return *this;
}

void Namespace::initialize(int moduleNumber)
{
   VMAPI_D(Namespace);
   implPtr->initialize(implPtr->m_name, moduleNumber);
}

Namespace &Namespace::registerFunction(const char *name, ZendCallable function, const Arguments &arguments)
{
   VMAPI_D(Namespace);
   implPtr->m_functions.push_back(std::make_shared<Function>(name, function, arguments));
   return *this;
}

Namespace *Namespace::findNamespace(const std::string &ns) const
{
   VMAPI_D(const Namespace);
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

AbstractClass *Namespace::findClass(const std::string &clsName) const
{
   VMAPI_D(const Namespace);
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

size_t Namespace::getFunctionCount() const
{
   VMAPI_D(const Namespace);
   return implPtr->calculateFunctionCount();
}

size_t Namespace::getClassQuanlity() const
{
   VMAPI_D(const Namespace);
   return implPtr->calculateClassCount();
}

const std::string &Namespace::getName() const
{
   VMAPI_D(const Namespace);
   return implPtr->m_name;
}

size_t Namespace::getConstantQuanlity() const
{
   VMAPI_D(const Namespace);
   return implPtr->calculateConstantCount();
}

Namespace &Namespace::registerNamespace(const Namespace &ns)
{
   VMAPI_D(Namespace);
   implPtr->m_namespaces.push_back(std::make_shared<Namespace>(ns));
   return *this;
}

Namespace &Namespace::registerNamespace(Namespace &&ns)
{
   VMAPI_D(Namespace);
   implPtr->m_namespaces.push_back(std::make_shared<Namespace>(std::move(ns)));
   return *this;
}

Namespace &Namespace::registerConstant(const Constant &constant)
{
   VMAPI_D(Namespace);
   implPtr->m_constants.push_back(std::make_shared<Constant>(constant));
   return *this;
}

Namespace &Namespace::registerConstant(Constant &&constant)
{
   VMAPI_D(Namespace);
   implPtr->m_constants.push_back(std::make_shared<Constant>(std::move(constant)));
   return *this;
}

Namespace::~Namespace()
{}

} // vmapi
} // polar
