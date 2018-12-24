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

#ifndef POLARPHP_VMAPI_PROPERTY_H
#define POLARPHP_VMAPI_PROPERTY_H

#include "polarphp/vm/ZendApi.h"

namespace polar {
namespace vmapi {

/// forward declare class
class Variant;
class StdClass;

namespace internal
{
class AbstractClassPrivate;
class PropertyPrivate;
} // internal

using internal::AbstractClassPrivate;
using internal::PropertyPrivate;

class VMAPI_DECL_EXPORT Property final
{
public:
   Property(const GetterMethodCallable0 &getter);
   Property(const GetterMethodCallable1 &getter);
   Property(const GetterMethodCallable0 &getter, const SetterMethodCallable0 &setter);
   Property(const GetterMethodCallable0 &getter, const SetterMethodCallable1 &setter);
   Property(const GetterMethodCallable1 &getter, const SetterMethodCallable0 &setter);
   Property(const GetterMethodCallable1 &getter, const SetterMethodCallable1 &setter);
   Property(const Property &other);
   Property(Property &&other) noexcept;
   Property &operator=(const Property &other);
   Property &operator=(Property &&other) noexcept;
   virtual ~Property();
private:
   Variant get(StdClass *nativeObject);
   bool set(StdClass *nativeObject, const Variant &value);
private:
   VMAPI_DECLARE_PRIVATE(Property);
   std::shared_ptr<PropertyPrivate> m_implPtr;
   friend class AbstractClassPrivate;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_PROPERTY_H
