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

#ifndef POLARPHP_VMAPI_LANG_CLASS_H
#define POLARPHP_VMAPI_LANG_CLASS_H

#include "polarphp/vm/AbstractClass.h"
#include "polarphp/vm/InvokeBridge.h"
#include "polarphp/vm/ds/StringVariant.h"
#include "polarphp/vm/ds/BooleanVariant.h"
#include "polarphp/vm/ds/DoubleVariant.h"
#include "polarphp/vm/ds/NumericVariant.h"
#include "polarphp/vm/ds/ArrayVariant.h"
#include "polarphp/vm/protocol/Serializable.h"
#include "polarphp/vm/protocol/Traversable.h"
#include "polarphp/vm/utils/CallableTraits.h"

#include "polarphp/basic/adt/StringRef.h"

namespace polar {
namespace vmapi {

class Constant;
class Interface;

// forward declare for internal::ClassMethodRegister
template <typename T>
class Class;

using polar::basic::StringRef;

namespace internal
{

template <typename TargetClassType,
          typename CallalbleType,
          CallalbleType callable,
          bool IsCallable,
          bool IsMemberCallable>
struct ClassMethodRegisterImpl
{};

template <typename TargetClassType,
          typename CallalbleType,
          CallalbleType callable>
struct ClassMethodRegisterImpl<TargetClassType, CallalbleType, callable, false, false>
{
   /// error situation
   /// is member pointer but class type of the pointer is not same with
   /// register class type
   ///public:
   ///   static void registerMethod(Class<TargetClassType> &meta, StringRef name, Modifier flags, const Arguments &args)
   ///   {
   ///      ZAPI_ASSERT_X(false, "Class::registerMethod",
   ///                    "try to register class member pointer, and the class type "
   ///                    "of the member pointer is not the registered class type");
   ///   }
};

template <typename TargetClassType,
          typename CallalbleType,
          CallalbleType callable>
struct ClassMethodRegisterImpl<TargetClassType, CallalbleType, callable, true, false>
{
   using ForwardCallableType = CallalbleType;
   /// for static method register
public:
   inline static void registerMethod(Class<TargetClassType> &meta, StringRef name, Modifier flags, const Arguments &args)
   {
      meta.registerMethod(name, &InvokeBridge<ForwardCallableType, callable>::invoke, flags | Modifier::Static, args);
   }
};

template <typename TargetClassType,
          typename CallalbleType,
          CallalbleType callable>
struct ClassMethodRegisterImpl<TargetClassType, CallalbleType, callable, false, true>
{
   /// for instance method register
   using ForwardCallableType = CallalbleType;
public:
   inline static void registerMethod(Class<TargetClassType> &meta, StringRef name, Modifier flags, const Arguments &args)
   {
      meta.registerMethod(name, &InvokeBridge<ForwardCallableType, callable>::invoke, flags, args);
   }
};

template <typename TargetClassType,
          typename CallalbleType,
          CallalbleType callable>
struct ClassMethodRegisterImpl<TargetClassType, CallalbleType, callable, true, true>
{
   /// error situation
};

template <typename TargetClassType,
          typename CallalbleType,
          CallalbleType callable>
struct ClassMethodRegister
      : public ClassMethodRegisterImpl<
      TargetClassType,
      CallalbleType,
      callable,
      is_function_pointer<typename std::decay<CallalbleType>::type>::value,
      CallableInfoTrait<typename std::decay<CallalbleType>::type>::isMemberCallable &&
      std::is_same<typename std::decay<typename member_pointer_traits<CallalbleType>::ClassType>::type,
      typename std::decay<TargetClassType>::type>::value>
{};

} // internal

///
/// Refactor Class::registerMethod
///
template <typename T>
class VMAPI_DECL_EXPORT Class final : public AbstractClass
{
public:
   using HandlerClassType = T;
   public:
   Class(StringRef name, ClassType classType = ClassType::Regular);
   Class(const Class<T> &other);
   Class(Class<T> &&other) noexcept;
   virtual ~Class();
   Class<T> &operator=(const Class<T> &other);
   Class<T> &operator=(Class<T> &&other);
public:
   template <typename CallableType, CallableType callable>
   Class<T> &registerMethod(StringRef name, Modifier flags, const Arguments &args = {});
   template <typename CallableType, CallableType callable>
   Class<T> &registerMethod(StringRef name, const Arguments &args = {});

   Class<T> &registerMethod(StringRef name, Modifier flags, const Arguments &args = {});
   Class<T> &registerMethod(StringRef name, const Arguments &args = {});

   Class<T> &registerProperty(StringRef name, std::nullptr_t value, Modifier flags = Modifier::Public);
   Class<T> &registerProperty(StringRef name, int16_t value, Modifier flags = Modifier::Public);
   Class<T> &registerProperty(StringRef name, int32_t value, Modifier flags = Modifier::Public);
   Class<T> &registerProperty(StringRef name, int64_t value, Modifier flags = Modifier::Public);
   Class<T> &registerProperty(StringRef name, char value, Modifier flags = Modifier::Public);
   Class<T> &registerProperty(StringRef name, const char *value, Modifier flags = Modifier::Public);
   Class<T> &registerProperty(StringRef name, const std::string &value, Modifier flags = Modifier::Public);
   Class<T> &registerProperty(StringRef name, bool value, Modifier flags = Modifier::Public);
   Class<T> &registerProperty(StringRef name, double value, Modifier flags = Modifier::Public);
   Class<T> &registerProperty(StringRef name, Variant (T::*getter)());
   Class<T> &registerProperty(StringRef name, Variant (T::*getter)() const);
   Class<T> &registerProperty(StringRef name, Variant (T::*getter)(), void (T::*setter)(const Variant &value));
   Class<T> &registerProperty(StringRef name, Variant (T::*getter)(), void (T::*setter)(const Variant &value) const);
   Class<T> &registerProperty(StringRef name, Variant (T::*getter)() const, void (T::*setter)(const Variant &value));
   Class<T> &registerProperty(StringRef name, Variant (T::*getter)() const, void (T::*setter)(const Variant &value) const);

   Class<T> &registerConstant(StringRef name, std::nullptr_t value);
   Class<T> &registerConstant(StringRef name, int16_t value);
   Class<T> &registerConstant(StringRef name, int32_t value);
   Class<T> &registerConstant(StringRef name, int64_t value);
   Class<T> &registerConstant(StringRef name, char value);
   Class<T> &registerConstant(StringRef name, const char *value);
   Class<T> &registerConstant(StringRef name, const std::string &value);
   Class<T> &registerConstant(StringRef name, bool value);
   Class<T> &registerConstant(StringRef name, double value);
   Class<T> &registerConstant(const Constant &constant);

   Class<T> &registerInterface(const Interface &interface);
   Class<T> &registerInterface(Interface &&interface);

   template <typename ClassType>
   Class<T> &registerBaseClass(const Class<ClassType> &baseClass);
   template <typename ClassType>
   Class<T> &registerBaseClass(Class<ClassType> &&baseClass);

private:
   virtual StdClass *construct() const override;
   virtual StdClass *clone(StdClass *orig) const override;
   virtual bool clonable() const override;
   virtual bool serializable() const override;
   virtual bool traversable() const override;
   virtual void callClone(StdClass *nativeObject) const override;
   virtual int callCompare(StdClass *left, StdClass *right) const override;
   virtual void callDestruct(StdClass *nativeObject) const override;
   virtual Variant callMagicCall(StdClass *nativeObject, StringRef name, Parameters &params) const override;
   virtual Variant callMagicStaticCall(StringRef name, Parameters &params) const override;
   virtual Variant callMagicInvoke(StdClass *nativeObject, Parameters &params) const override;
   virtual ArrayVariant callDebugInfo(StdClass *nativeObject) const override;

   virtual Variant callGet(StdClass *nativeObject, const std::string &name) const override;
   virtual void callSet(StdClass *nativeObject, const std::string &name, const Variant &value) const override;
   virtual bool callIsset(StdClass *nativeObject, const std::string &name) const override;
   virtual void callUnset(StdClass *nativeObject, const std::string &name) const override;

   virtual Variant castToString(StdClass *nativeObject) const override;
   virtual Variant castToInteger(StdClass *nativeObject) const override;
   virtual Variant castToDouble(StdClass *nativeObject) const override;
   virtual Variant castToBool(StdClass *nativeObject) const override;

   template <typename X = T>
   typename std::enable_if<std::is_default_constructible<X>::value, StdClass *>::type
   static doConstructObject();

   template <typename X = T>
   typename std::enable_if<!std::is_default_constructible<X>::value, StdClass *>::type
   static doConstructObject();

   template <typename X = T>
   typename std::enable_if<std::is_copy_constructible<X>::value, StdClass *>::type
   static doCloneObject(X *orig);

   template <typename X = T>
   typename std::enable_if<!std::is_copy_constructible<X>::value, StdClass *>::type
   static doCloneObject(X *orig);

   template <typename X>
   class HasCallStatic
   {
      typedef char one;
      typedef long two;
      template <typename C>
      static one test(decltype(&C::__callStatic));
      template <typename C>
      static two test(...);
   public:
      static const bool value = sizeof(test<X>(0)) == sizeof(char);
   };

   template <typename X>
   typename std::enable_if<HasCallStatic<X>::value, Variant>::type
   static doCallStatic(StringRef name, Parameters &params);

   template <typename X>
   typename std::enable_if<!HasCallStatic<X>::value, Variant>::type
   static doCallStatic(StringRef name, Parameters &params);
   using AbstractClass::registerMethod;

   template <typename TargetClassType, typename CallalbleType,
             CallalbleType callable, bool IsCallable, bool IsMemberCallable>
   friend struct internal::ClassMethodRegisterImpl;
};

template <typename T>
Class<T>::Class(StringRef name, ClassType classType)
   : AbstractClass(name, classType)
{}

template <typename T>
Class<T>::Class(const Class<T> &other)
   : AbstractClass(other)
{}

template <typename T>
Class<T>::Class(Class<T> &&other) noexcept
   : AbstractClass(std::move(other))
{}

template <typename T>
Class<T> &Class<T>::registerInterface(const Interface &interface)
{
   AbstractClass::registerInterface(interface);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerInterface(Interface &&interface)
{
   AbstractClass::registerInterface(std::move(interface));
   return *this;
}

template <typename T>
template <typename ClassType>
Class<T> &Class<T>::registerBaseClass(const Class<ClassType> &baseClass)
{
   AbstractClass::registerBaseClass(baseClass);
   return *this;
}

template <typename T>
template <typename ClassType>
Class<T> &Class<T>::registerBaseClass(Class<ClassType> &&baseClass)
{
   AbstractClass::registerBaseClass(std::move(baseClass));
   return *this;
}

template <typename T>
template <typename CallableType, CallableType callable>
Class<T> &Class<T>::registerMethod(StringRef name, Modifier flags, const Arguments &args)
{
   internal::ClassMethodRegister<T, typename std::decay<CallableType>::type, callable>::registerMethod(*this, name, flags, args);
   return *this;
}

template <typename T>
template <typename CallableType, CallableType callable>
Class<T> &Class<T>::registerMethod(StringRef name, const Arguments &args)
{
   // we must ensure the T is same with ClassType in CallableType
   internal::ClassMethodRegister<T, typename std::decay<CallableType>::type, callable>::registerMethod(*this, name, Modifier::Public, args);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerMethod(StringRef name, Modifier flags, const Arguments &args)
{
   AbstractClass::registerMethod(name, flags | Modifier::Abstract, args);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerMethod(StringRef name, const Arguments &args)
{
   AbstractClass::registerMethod(name, Modifier::Public | Modifier::Abstract, args);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, std::nullptr_t value, Modifier flags)
{
   AbstractClass::registerProperty(name, value, flags);
   return *this;
}
template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, int16_t value, Modifier flags)
{
   AbstractClass::registerProperty(name, value, flags);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, int32_t value, Modifier flags)
{
   AbstractClass::registerProperty(name, value, flags);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, int64_t value, Modifier flags)
{
   AbstractClass::registerProperty(name, value, flags);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, char value, Modifier flags)
{
   AbstractClass::registerProperty(name, value, flags);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, const char *value, Modifier flags)
{
   AbstractClass::registerProperty(name, value, flags);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, const std::string &value, Modifier flags)
{
   AbstractClass::registerProperty(name, value, flags);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, bool value, Modifier flags)
{
   AbstractClass::registerProperty(name, value, flags);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, double value, Modifier flags)
{
   AbstractClass::registerProperty(name, value, flags);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, Variant (T::*getter)())
{
   AbstractClass::registerProperty(name, static_cast<GetterMethodCallable0>(getter));
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, Variant (T::*getter)() const)
{
   AbstractClass::registerProperty(name, static_cast<GetterMethodCallable1>(getter));
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, Variant (T::*getter)(),
                                     void (T::*setter)(const Variant &value))
{
   AbstractClass::registerProperty(name, static_cast<GetterMethodCallable0>(getter),
                                   static_cast<SetterMethodCallable0>(setter));
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, Variant (T::*getter)(),
                                     void (T::*setter)(const Variant &value) const)
{
   AbstractClass::registerProperty(name, static_cast<GetterMethodCallable0>(getter),
                                   static_cast<SetterMethodCallable1>(setter));
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, Variant (T::*getter)() const,
                                     void (T::*setter)(const Variant &value))
{
   AbstractClass::registerProperty(name, static_cast<GetterMethodCallable1>(getter),
                                   static_cast<SetterMethodCallable0>(setter));
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerProperty(StringRef name, Variant (T::*getter)() const,
                                     void (T::*setter)(const Variant &value) const)
{
   AbstractClass::registerProperty(name, static_cast<GetterMethodCallable1>(getter),
                                   static_cast<SetterMethodCallable1>(setter));
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerConstant(StringRef name, std::nullptr_t value)
{
   AbstractClass::registerProperty(name, value, Modifier::Const);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerConstant(StringRef name, int16_t value)
{
   AbstractClass::registerProperty(name, value, Modifier::Const);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerConstant(StringRef name, int32_t value)
{
   AbstractClass::registerProperty(name, value, Modifier::Const);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerConstant(StringRef name, int64_t value)
{
   AbstractClass::registerProperty(name, value, Modifier::Const);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerConstant(StringRef name, char value)
{
   AbstractClass::registerProperty(name, value, Modifier::Const);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerConstant(StringRef name, const char *value)
{
   AbstractClass::registerProperty(name, value, Modifier::Const);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerConstant(StringRef name, const std::string &value)
{
   AbstractClass::registerProperty(name, value, Modifier::Const);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerConstant(StringRef name, bool value)
{
   AbstractClass::registerProperty(name, value, Modifier::Const);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerConstant(StringRef name, double value)
{
   AbstractClass::registerProperty(name, value, Modifier::Const);
   return *this;
}

template <typename T>
Class<T> &Class<T>::registerConstant(const Constant &constant)
{
   AbstractClass::registerConstant(constant);
   return *this;
}

template <typename T>
StdClass *Class<T>::construct() const
{
   return doConstructObject<T>();
}

template <typename T>
void Class<T>::callDestruct(StdClass *nativeObject) const
{
   T *object = static_cast<T *>(nativeObject);
   return object->__destruct();
}

template <typename T>
Variant Class<T>::callMagicCall(StdClass *nativeObject, StringRef name, Parameters &params) const
{
   T *object = static_cast<T *>(nativeObject);
   return object->__call(name, params);
}

template <typename T>
Variant Class<T>::callMagicStaticCall(StringRef name, Parameters &params) const
{
   return doCallStatic<T>(name, params);
}

template <typename T>
Variant Class<T>::callMagicInvoke(StdClass *nativeObject, Parameters &params) const
{
   T *object = static_cast<T *>(nativeObject);
   return object->__invoke(params);
}

template <typename T>
ArrayVariant Class<T>::callDebugInfo(StdClass *nativeObject) const
{
   T *object = static_cast<T *>(nativeObject);
   return object->__debugInfo();
}

template <typename T>
Variant Class<T>::callGet(StdClass *nativeObject, const std::string &name) const
{
   T *object = static_cast<T *>(nativeObject);
   return object->__get(name);
}

template <typename T>
void Class<T>::callSet(StdClass *nativeObject, const std::string &name, const Variant &value) const
{
   T *object = static_cast<T *>(nativeObject);
   object->__set(name, value);
}

template <typename T>
bool Class<T>::callIsset(StdClass *nativeObject, const std::string &name) const
{
   T *object = static_cast<T *>(nativeObject);
   return object->__isset(name);
}

template <typename T>
void Class<T>::callUnset(StdClass *nativeObject, const std::string &name) const
{
   T *object = static_cast<T *>(nativeObject);
   object->__unset(name);
}

template <typename T>
Variant Class<T>::castToString(StdClass *nativeObject) const
{
   T *object = static_cast<T *>(nativeObject);
   return StringVariant(object->__toString());
}

template <typename T>
Variant Class<T>::castToInteger(StdClass *nativeObject) const
{
   T *object = static_cast<T *>(nativeObject);
   return NumericVariant(object->__toInteger());
}

template <typename T>
Variant Class<T>::castToDouble(StdClass *nativeObject) const
{
   T *object = static_cast<T *>(nativeObject);
   return DoubleVariant(object->__toDouble());
}

template <typename T>
Variant Class<T>::castToBool(StdClass *nativeObject) const
{
   T *object = static_cast<T *>(nativeObject);
   return BooleanVariant(object->__toBool());
}

template <typename T>
template <typename X>
typename std::enable_if<std::is_default_constructible<X>::value, StdClass *>::type
Class<T>::doConstructObject()
{
   return new X();
}

template <typename T>
template <typename X>
typename std::enable_if<!std::is_default_constructible<X>::value, StdClass *>::type
Class<T>::doConstructObject()
{
   return nullptr;
}

template <typename T>
template <typename X>
typename std::enable_if<std::is_copy_constructible<X>::value, StdClass *>::type
Class<T>::doCloneObject(X *orig)
{
   return new X(*orig);
}

template <typename T>
template <typename X>
typename std::enable_if<!std::is_copy_constructible<X>::value, StdClass *>::type
Class<T>::doCloneObject(X *orig)
{
   return nullptr;
}

template <typename T>
template <typename X>
typename std::enable_if<Class<T>::template HasCallStatic<X>::value, Variant>::type
Class<T>::doCallStatic(StringRef name, Parameters &params)
{
   return X::__callStatic(name, params);
}

template <typename T>
template <typename X>
typename std::enable_if<!Class<T>::template HasCallStatic<X>::value, Variant>::type
Class<T>::doCallStatic(StringRef name, Parameters &params)
{
   notImplemented();
   // prevent some compiler warnning
   return nullptr;
}

template <typename T>
StdClass *Class<T>::clone(StdClass *orig) const
{
   return doCloneObject<T>(static_cast<T *>(orig));
}

template <typename T>
bool Class<T>::clonable() const
{
   return std::is_copy_constructible<T>::value;
}

template <typename T>
bool Class<T>::serializable() const
{
   return std::is_base_of<Serializable, T>::value;
}

template <typename T>
bool Class<T>::traversable() const
{
   return std::is_base_of<Traversable, T>::value;
}

template <typename T>
void Class<T>::callClone(StdClass *nativeObject) const
{
   T *object = static_cast<T *>(nativeObject);
   object->__clone();
}

template <typename T>
int Class<T>::callCompare(StdClass *left, StdClass *right) const
{
   T *leftNativeObject = static_cast<T *>(left);
   T *rightNativeObject = static_cast<T *>(right);
   return leftNativeObject->__compare(*rightNativeObject);
}

template <typename T>
Class<T>::~Class()
{}

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_CLASS_H
