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

#ifndef POLARPHP_VMAPI_DS_CALLABLE_VARIANT_H
#define POLARPHP_VMAPI_DS_CALLABLE_VARIANT_H

#include "polarphp/vm/ds/Variant.h"
#include "polarphp/vm/Closure.h"

namespace polar {
namespace vmapi {

/// forward declare class
class Parameters;

class VMAPI_DECL_EXPORT CallableVariant final : public Variant
{
public:
   using HaveArgCallable = Variant(Parameters &);
   using NoArgCallable = Variant();
   CallableVariant(HaveArgCallable callable);
   CallableVariant(NoArgCallable callable);

   CallableVariant(const Variant &other);
   CallableVariant(const CallableVariant &other);
   CallableVariant(Variant &&other);
   CallableVariant(CallableVariant &&other) noexcept;
   CallableVariant(zval &other);
   CallableVariant(zval &&other);
   CallableVariant(zval *other);

   CallableVariant &operator =(const CallableVariant &other);
   CallableVariant &operator =(const Variant &other);
   CallableVariant &operator =(CallableVariant &&other) noexcept;
   CallableVariant &operator =(Variant &&other);

   Variant operator ()() const;
   template <typename ...Args>
   Variant operator ()(Args&&... args);
   virtual ~CallableVariant();
protected:
   Variant exec(int argc, Variant *argv) const;
};

template <typename ...Args>
Variant CallableVariant::operator ()(Args&&... args)
{
   Variant vargs[] = { static_cast<Variant>(args)... };
   return exec(sizeof...(Args), vargs);
}

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_DS_CALLABLE_VARIANT_H
