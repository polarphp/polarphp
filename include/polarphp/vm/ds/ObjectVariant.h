// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/23.

#ifndef POLARPHP_VMAPI_DS_OBJECT_VARIANT_H
#define POLARPHP_VMAPI_DS_OBJECT_VARIANT_H

#include "polarphp/vm/ds/Variant.h"

#include <iostream>

namespace polar {
namespace vmapi {

class StdClass;

class VMAPI_DECL_EXPORT ObjectVariant final : public Variant
{
public:
   ObjectVariant();
   ObjectVariant(const std::string &className, std::shared_ptr<StdClass> nativeObject);
   ObjectVariant(zend_class_entry *entry, std::shared_ptr<StdClass> nativeObject);
   ObjectVariant(const Variant &other);
   ObjectVariant(const ObjectVariant &other);
   ObjectVariant(Variant &&other);
   ObjectVariant(ObjectVariant &&other) noexcept;
   ObjectVariant(zval &other);
   ObjectVariant(zval &&other);
   ObjectVariant(zval *other);

   ObjectVariant &operator =(const ObjectVariant &other);
   ObjectVariant &operator =(const Variant &other);
   ObjectVariant &operator =(ObjectVariant &&other) noexcept;
   ObjectVariant &operator =(Variant &&other);

   template <typename ...Args>
   Variant operator()(Args&&... args);

   ObjectVariant &setProperty(const std::string &name, const Variant &value);
   Variant getProperty(const std::string &name);
   ObjectVariant &setStaticProperty(const std::string &name, const Variant &value);
   Variant getStaticProperty(const std::string &name);
   bool hasProperty(const std::string &name);

   bool methodExist(const char *name) const;
   Variant call(const char *name);
   Variant call(const char *name) const;

   template <typename ...Args>
   Variant call(const char *name, Args&&... args);
   template <typename ...Args>
   Variant call(const char *name, Args&&... args) const;

   bool instanceOf(const char *className, size_t size) const;
   bool instanceOf(const char *className) const;
   bool instanceOf(const std::string &className) const;
   bool instanceOf(const ObjectVariant &other) const;

   bool derivedFrom(const char *className, size_t size) const;
   bool derivedFrom(const char *className) const;
   bool derivedFrom(const std::string &className) const;
   bool derivedFrom(const ObjectVariant &other) const;
private:
   ObjectVariant(StdClass *nativeObject);
   Variant exec(const char *name, int argc, Variant *argv);
   Variant exec(const char *name, int argc, Variant *argv) const;
   void doClassInvoke(int argc, Variant *argv, zval *retval);
   friend class StdClass;
};

template <typename ...Args>
Variant ObjectVariant::call(const char *name, Args&&... args) const
{
   Variant vargs[] = { Variant(std::forward<Args>(args))... };
   return exec(name, sizeof...(Args), vargs);
}

template <typename ...Args>
Variant ObjectVariant::call(const char *name, Args&&... args)
{
   return const_cast<const ObjectVariant &>(*this).call(name, std::forward<Args>(args)...);
}

template <typename ...Args>
Variant ObjectVariant::operator ()(Args&&... args)
{
   Variant vargs[] = { Variant(std::forward<Args>(args))... };
   zval result;
   std::memset(&result, 0, sizeof(result));
   doClassInvoke(sizeof...(Args), vargs, &result);
   Variant ret(&result);
   zval_ptr_dtor(&result);
   return std::move(ret);
}

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_DS_OBJECT_VARIANT_H
