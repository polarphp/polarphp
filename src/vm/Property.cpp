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

#include "polarphp/vm/Property.h"
#include "polarphp/vm/StdClass.h"

namespace polar {
namespace vmapi {

namespace internal
{
class PropertyPrivate
{
public:
   PropertyPrivate(const GetterMethodCallable0 &getter)
      : m_getterType(0)
   {
      m_getter.getter0 = getter;
   }

   PropertyPrivate(const GetterMethodCallable1 &getter)
      : m_getterType(0)
   {
      m_getter.getter1 = getter;
   }

   PropertyPrivate(const SetterMethodCallable0 &setter)
      : m_setterType(0)
   {
      m_setter.setter0 = setter;
   }

   PropertyPrivate(const SetterMethodCallable1 &setter)
      : m_setterType(1)
   {
      m_setter.setter1 = setter;
   }

   PropertyPrivate(const GetterMethodCallable0 &getter, const SetterMethodCallable0 &setter)
      : m_getterType(0),
        m_setterType(0)
   {
      m_getter.getter0 = getter;
      m_setter.setter0 = setter;
   }

   PropertyPrivate(const GetterMethodCallable0 &getter, const SetterMethodCallable1 &setter)
      : m_getterType(0),
        m_setterType(1)
   {
      m_getter.getter0 = getter;
      m_setter.setter1 = setter;
   }

   PropertyPrivate(const GetterMethodCallable1 &getter, const SetterMethodCallable0 &setter)
      : m_getterType(1),
        m_setterType(0)
   {
      m_getter.getter1 = getter;
      m_setter.setter0 = setter;
   }

   PropertyPrivate(const GetterMethodCallable1 &getter, const SetterMethodCallable1 &setter)
      : m_getterType(1),
        m_setterType(1)
   {
      m_getter.getter1 = getter;
      m_setter.setter1 = setter;
   }

   PropertyPrivate(const PropertyPrivate &other)
      : m_getter(other.m_getter),
        m_setter(other.m_setter),
        m_getterType(other.m_getterType),
        m_setterType(other.m_setterType)
   {}

   union {
      GetterMethodCallable0 getter0;
      GetterMethodCallable1 getter1;
   } m_getter;
   union {
      SetterMethodCallable0 setter0;
      SetterMethodCallable1 setter1;
   } m_setter;
   int m_getterType = -1;
   int m_setterType = -1;
};
} // internal

Property::Property(const GetterMethodCallable0 &getter)
   : m_implPtr(new PropertyPrivate(getter))
{}

Property::Property(const GetterMethodCallable1 &getter)
   : m_implPtr(new PropertyPrivate(getter))
{}

Property::Property(const GetterMethodCallable0 &getter, const SetterMethodCallable0 &setter)
   : m_implPtr(new PropertyPrivate(getter, setter))
{}

Property::Property(const GetterMethodCallable0 &getter, const SetterMethodCallable1 &setter)
   : m_implPtr(new PropertyPrivate(getter, setter))
{}

Property::Property(const GetterMethodCallable1 &getter, const SetterMethodCallable0 &setter)
   : m_implPtr(new PropertyPrivate(getter, setter))
{}

Property::Property(const GetterMethodCallable1 &getter, const SetterMethodCallable1 &setter)
   : m_implPtr(new PropertyPrivate(getter, setter))
{}

Property::Property(const Property &other)
   : m_implPtr(other.m_implPtr)
{}

Property::Property(Property &&other) noexcept
   : m_implPtr(std::move(other.m_implPtr))
{}

Property &Property::operator=(const Property &other)
{
   if (this != &other) {
      m_implPtr = other.m_implPtr;
   }
   return *this;
}

Property &Property::operator=(Property &&other) noexcept
{
   assert(this != &other);
   m_implPtr = std::move(other.m_implPtr);
   return *this;
}

Property::~Property()
{}

Variant Property::get(StdClass *nativeObject)
{
   VMAPI_D(Property);
   if (0 == implPtr->m_getterType) {
      return (nativeObject->*(implPtr->m_getter.getter0))();
   } else {
      return (nativeObject->*(implPtr->m_getter.getter1))();
   }
}

bool Property::set(StdClass *nativeObject, const Variant &value)
{
   VMAPI_D(Property);
   if (0 == implPtr->m_setterType) {
      (nativeObject->*(implPtr->m_setter.setter0))(value);
      return true;
   } else if (1 == implPtr->m_setterType) {
      (nativeObject->*(implPtr->m_setter.setter1))(value);
      return true;
   } else {
      return false;
   }
}

} // vmapi
} // polar

