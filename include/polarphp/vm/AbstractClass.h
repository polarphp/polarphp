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

#ifndef POLARPHP_VMAPI_ABSTRACT_CLASS_H
#define POLARPHP_VMAPI_ABSTRACT_CLASS_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/lang/Argument.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/vm/TypeDefs.h"

namespace polar {
namespace vmapi {

/// forward declare class
class Variant;
class ArrayVariant;
class StdClass;
class Constant;
class Interface;
class Parameters;
class Closure;
namespace internal
{
class ModulePrivate;
class NamespacePrivate;
class AbstractClassPrivate;
} // internal
using internal::ModulePrivate;
using internal::NamespacePrivate;
using internal::AbstractClassPrivate;
using polar::basic::StringRef;

class VMAPI_DECL_EXPORT AbstractClass
{
public:
   AbstractClass(StringRef className, ClassType type);
   AbstractClass(const AbstractClass &other);
   AbstractClass(AbstractClass &&other) noexcept;
   AbstractClass &operator=(const AbstractClass &other);
   AbstractClass &operator=(AbstractClass &&other) noexcept;
   virtual ~AbstractClass();
   std::string getClassName() const;

   size_t getPropertyCount() const;
   size_t getInterfaceCount() const;
   size_t getMethodCount() const;
   size_t getConstantCount() const;

   ///
   /// for test only
   ///
   zend_class_entry *buildClassEntry(StringRef ns, int moduleNumber) noexcept;

protected:
   void registerMethod(StringRef name, vmapi::ZendCallable callable, Modifier flags = Modifier::None, const Arguments &args = {});
   // abstract
   void registerMethod(StringRef name, Modifier flags = Modifier::None, const Arguments &args = {});

   void registerProperty(StringRef name, std::nullptr_t value, Modifier flags = Modifier::Public);
   void registerProperty(StringRef name, int16_t value, Modifier flags = Modifier::Public);
   void registerProperty(StringRef name, int32_t value, Modifier flags = Modifier::Public);
   void registerProperty(StringRef name, int64_t value, Modifier flags = Modifier::Public);
   void registerProperty(StringRef name, const std::string &value, Modifier flags = Modifier::Public);
   void registerProperty(StringRef name, const char *value, Modifier flags = Modifier::Public);
   void registerProperty(StringRef name, bool value, Modifier flags = Modifier::Public);
   void registerProperty(StringRef name, double value, Modifier flags = Modifier::Public);
   // callback property
   void registerProperty(StringRef name, const vmapi::GetterMethodCallable0 &getter);
   void registerProperty(StringRef name, const vmapi::GetterMethodCallable1 &getter);
   void registerProperty(StringRef name, const vmapi::GetterMethodCallable0 &getter, const vmapi::SetterMethodCallable0 &setter);
   void registerProperty(StringRef name, const vmapi::GetterMethodCallable0 &getter, const vmapi::SetterMethodCallable1 &setter);
   void registerProperty(StringRef name, const vmapi::GetterMethodCallable1 &getter, const vmapi::SetterMethodCallable0 &setter);
   void registerProperty(StringRef name, const vmapi::GetterMethodCallable1 &getter, const vmapi::SetterMethodCallable1 &setter);

   void registerConstant(const Constant &constant);

   void registerInterface(const Interface &interface);
   void registerInterface(Interface &&interface);
   void registerBaseClass(const AbstractClass &base);
   void registerBaseClass(AbstractClass &&base);

protected:
   virtual StdClass *construct() const;
   virtual StdClass *clone(StdClass *orig) const;
   virtual bool clonable() const;
   virtual bool serializable() const;
   virtual bool traversable() const;
   virtual int callCompare(StdClass *left, StdClass *right) const;
   virtual void callClone(StdClass *nativeObject) const;
   virtual void callDestruct(StdClass *nativeObject) const;
   virtual Variant callMagicCall(StdClass *nativeObject, StringRef name, Parameters &params) const;
   virtual Variant callMagicStaticCall(StringRef name, Parameters &params) const;
   virtual Variant callMagicInvoke(StdClass *nativeObject, Parameters &params) const;
   virtual ArrayVariant callDebugInfo(StdClass *nativeObject) const;
   // property
   virtual Variant callGet(StdClass *nativeObject, const std::string &name) const;
   virtual void callSet(StdClass *nativeObject, const std::string &name, const Variant &value) const;
   virtual bool callIsset(StdClass *nativeObject, const std::string &name) const;
   virtual void callUnset(StdClass *nativeObject, const std::string &name) const;
   // cast
   virtual Variant castToString(StdClass *nativeObject) const;
   virtual Variant castToInteger(StdClass *nativeObject) const;
   virtual Variant castToDouble(StdClass *nativeObject) const;
   virtual Variant castToBool(StdClass *nativeObject) const;

protected:
   static void notImplemented();

private:
   zend_class_entry *initialize(const std::string &ns, int moduleNumber);
   zend_class_entry *initialize(int moduleNumber);

private:
   VMAPI_DECLARE_PRIVATE(AbstractClass);
   std::shared_ptr<AbstractClassPrivate> m_implPtr;
   friend class ModulePrivate;
   friend class NamespacePrivate;
   friend class Closure;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_ABSTRACT_CLASS_H
