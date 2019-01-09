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

#include "polarphp/vm/ds/CallableVariant.h"
#include "polarphp/vm/ds/ObjectVariant.h"
#include "polarphp/vm/utils/Exception.h"
#include "polarphp/vm/utils/OrigException.h"
#include "polarphp/vm/lang/Parameter.h"
#include <string>

namespace polar {
namespace vmapi {

namespace
{
Variant do_execute(zval *func, int argc, zval *argv)
{
   zval retval;
   zend_object *oldException = EG(exception);
   if (VMAPI_SUCCESS != call_user_function_ex(CG(function_table), nullptr, func, &retval,
                                              argc, argv, 1, nullptr)) {
      std::string msg("Invalid call to ");
      msg.append(Z_STRVAL_P(func), Z_STRLEN_P(func));
      throw Exception(std::move(msg));
      return nullptr; // just for prevent some compiler warnings
   } else {
      // detect whether has exception throw from PHP code, if we got,
      // we throw an native c++ exception, let's c++ code can
      // handle it
      if (oldException != EG(exception) && EG(exception)) {
         throw OrigException(EG(exception));
      }
      if (Z_ISUNDEF(retval)) {
         return nullptr;
      }
      // wrap the retval into Variant
      Variant result(&retval);
      // here we decrease the refcounter
      zval_ptr_dtor(&retval);
      return result;
   }
}

Variant do_nothing(Parameters &param)
{
   return nullptr;
}
} // anonymous namespace

CallableVariant::CallableVariant(HaveArgCallable callable)
   : Variant(ObjectVariant(Closure::getClassEntry(), std::make_shared<Closure>(std::function<HaveArgCallable>(callable))))
{}

CallableVariant::CallableVariant(NoArgCallable callable)
   : Variant(ObjectVariant(Closure::getClassEntry(), std::make_shared<Closure>([callable](Parameters &params) -> Variant {
                              return callable();
                           })))
{}

CallableVariant::CallableVariant(const Variant &other)
{
   zval *from = const_cast<zval *>(other.getZvalPtr());
   zval *self = getUnDerefZvalPtr();
   zend_class_entry *classEntry = nullptr;
   if (other.getType() == Type::Object &&
       (classEntry = Z_OBJCE_P(from)) && 0 == std::memcmp("VmApiClosure", classEntry->name->val, classEntry->name->len)) {
      ZVAL_COPY(self, from);
   } else {
      ObjectVariant defaultCallable(Closure::getClassEntry(), std::make_shared<Closure>(do_nothing));
      zval temp = defaultCallable.detach(true);
      ZVAL_COPY_VALUE(self, &temp);
   }
}

CallableVariant::CallableVariant(const CallableVariant &other)
   : Variant(other)
{}

CallableVariant::CallableVariant(Variant &&other)
   : Variant(std::move(other))
{
   zval *self = getUnDerefZvalPtr();
   zend_class_entry *classEntry = nullptr;
   if (getUnDerefType() != Type::Object ||
       ((classEntry = Z_OBJCE_P(self)) && 0 != std::memcmp("VmApiClosure", classEntry->name->val, classEntry->name->len))) {
      ObjectVariant defaultCallable(Closure::getClassEntry(), std::make_shared<Closure>(do_nothing));
      zval temp = defaultCallable.detach(true);
      ZVAL_COPY_VALUE(self, &temp);
   }
}

CallableVariant::CallableVariant(CallableVariant &&other) noexcept
   : Variant(std::move(other))
{}

CallableVariant::CallableVariant(zval &other)
   : CallableVariant(&other)
{}

CallableVariant::CallableVariant(zval &&other)
   : CallableVariant(&other)
{}

CallableVariant::CallableVariant(zval *other)
{
   zval *self = getUnDerefZvalPtr();
   if (nullptr != other && Z_TYPE_P(other) != IS_NULL) {
      if ((Z_TYPE_P(other) == IS_OBJECT ||
           (Z_TYPE_P(other) == IS_REFERENCE && Z_TYPE_P(Z_REFVAL_P(other)) == IS_OBJECT))) {
         ZVAL_DEREF(other);
         zend_class_entry *classEntry = Z_OBJCE_P(other);
         if (classEntry && 0 == std::memcmp("VmApiClosure", classEntry->name->val, classEntry->name->len)) {
            ZVAL_COPY(self, other);
            return;
         }
      }
   }
   // construct default
   ObjectVariant defaultCallable(Closure::getClassEntry(), std::make_shared<Closure>(do_nothing));
   zval temp = defaultCallable.detach(true);
   ZVAL_COPY_VALUE(self, &temp);
}

CallableVariant &CallableVariant::operator =(const CallableVariant &other)
{
   if (this != &other) {
      Variant::operator =(const_cast<zval *>(other.getZvalPtr()));
   }
   return *this;
}

CallableVariant &CallableVariant::operator =(const Variant &other)
{
   if (this != &other) {
      zval *self = getZvalPtr();
      zval *from = const_cast<zval *>(other.getZvalPtr());
      zend_class_entry *classEntry = nullptr;
      if (other.getType() == Type::Object &&
          (classEntry = Z_OBJCE_P(from)) && 0 == std::memcmp("VmApiClosure", classEntry->name->val, classEntry->name->len)) {
         // standard copy
         Variant::operator =(from);
      } else {
         ObjectVariant defaultCallable(Closure::getClassEntry(), std::make_shared<Closure>(do_nothing));
         zval temp = defaultCallable.detach(true);
         zval_dtor(self);
         ZVAL_COPY_VALUE(self, &temp);
      }
   }
   return *this;
}

CallableVariant &CallableVariant::operator =(CallableVariant &&other) noexcept
{
   assert(this != &other);
   m_implPtr = std::move(other.m_implPtr);
   return *this;
}

CallableVariant &CallableVariant::operator =(Variant &&other)
{
   assert(this != &other);
   m_implPtr = std::move(other.m_implPtr);
   zval *self = getUnDerefZvalPtr();
   zend_class_entry *classEntry = nullptr;
   if (getUnDerefType() != Type::Object ||
       ((classEntry = Z_OBJCE_P(self)) && 0 == std::memcmp("VmApiClosure", classEntry->name->val, classEntry->name->len))) {
      ObjectVariant defaultCallable(Closure::getClassEntry(), std::make_shared<Closure>(do_nothing));
      zval temp = defaultCallable.detach(true);
      ZVAL_COPY_VALUE(self, &temp);
   }
   return *this;
}

Variant CallableVariant::exec(int argc, Variant *argv) const
{
   std::unique_ptr<zval[]> params(new zval[argc]);
   for (int i = 0; i < argc; i++) {
      params[i] = *argv[i].getZvalPtr();
   }
   return do_execute(const_cast<zval *>(getUnDerefZvalPtr()), argc, params.get());
}

Variant CallableVariant::operator ()() const
{
   return do_execute(const_cast<zval *>(getUnDerefZvalPtr()), 0, nullptr);
}

CallableVariant::~CallableVariant()
{}

} // vmapi
} // polar
