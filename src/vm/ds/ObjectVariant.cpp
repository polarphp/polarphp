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

#include "polarphp/vm/StdClass.h"
#include "polarphp/vm/internal/StdClassPrivate.h"
#include "polarphp/vm/internal/AbstractClassPrivate.h"
#include "polarphp/vm/ObjectBinder.h"
#include "polarphp/vm/ds/ObjectVariant.h"
#include "polarphp/vm/utils/Funcs.h"
#include "polarphp/vm/utils/Exception.h"
#include "polarphp/vm/utils/OrigException.h"
#include "polarphp/vm/utils/FatalError.h"
#include <ostream>

namespace polar {
namespace vmapi {

namespace
{
void do_execute(const zval *object, zval *method, int argc, zval *argv, zval *retval)
{
   zend_object *oldException = EG(exception);
   if (VMAPI_SUCCESS != call_user_function_ex(CG(function_table), const_cast<zval *>(object), method, retval,
                                              argc, argv, 1, nullptr)) {
      std::string msg("Invalid call to ");
      msg.append(Z_STRVAL_P(method), Z_STRLEN_P(method));
      throw Exception(std::move(msg));
      return; // just for prevent some compiler warnings
   } else {
      // detect whether has exception throw from PHP code, if we got,
      // we throw an native c++ exception, let's c++ code can
      // handle it
      if (oldException != EG(exception) && EG(exception)) {
         throw OrigException(EG(exception));
      }
   }
}

void do_invoke_class_closure_method(zval *self, int argc, zval *argv, zval *retval)
{
   zend_fcall_info fci;
   zend_fcall_info_cache fciCache;
   fci.size = sizeof(fci);
   fci.retval = retval;
   fci.param_count = argc;
   fci.params = argv;
   fci.no_separation = (zend_bool) 1;
   zend_class_entry *calledScope = Z_OBJCE_P(self);
   zend_function *func;
   zend_object *object;

   if (!EXPECTED(Z_OBJ_HANDLER_P(self, get_closure)) ||
       !EXPECTED(Z_OBJ_HANDLER_P(self, get_closure)(self, &calledScope, &func, &object) == VMAPI_SUCCESS)) {
      throw Exception("Function name must be a string" );
   }

   fciCache.called_scope = calledScope;
   fciCache.function_handler = func;
   fciCache.object = object;

   zend_object *oldException = EG(exception);
   if (VMAPI_SUCCESS != zend_call_function(&fci, &fciCache)) {
      std::string msg("Invalid call to ");
      msg.append("__invoke");
      throw Exception(std::move(msg));
      return; // just for prevent some compiler warnings
   } else {
      // detect whether has exception throw from PHP code, if we got,
      // we throw an native c++ exception, let's c++ code can
      // handle it
      if (oldException != EG(exception) && EG(exception)) {
         throw OrigException(EG(exception));
      }
   }
}
} // anonymous namespace

using GuardStrType = std::unique_ptr<zend_string, std::function<void(zend_string *)>>;
using internal::AbstractClassPrivate;

ObjectVariant::ObjectVariant()
{
   convert_to_object(getUnDerefZvalPtr());
}

ObjectVariant::ObjectVariant(const Variant &other)
{
   zval *from = const_cast<zval *>(other.getZvalPtr());
   zval *self = getUnDerefZvalPtr();
   if (other.getType() == Type::Object) {
      ZVAL_COPY(self, from);
   } else {
      zval temp;
      // will increase 1 to gc refcount
      ZVAL_DUP(&temp, from);
      // will decrease 1 to gc refcount
      convert_to_object(&temp);
      ZVAL_COPY_VALUE(self, &temp);
   }
}

ObjectVariant::ObjectVariant(const ObjectVariant &other)
   : Variant(other)
{}

ObjectVariant::ObjectVariant(const std::string &className, std::shared_ptr<StdClass> nativeObject)
{
   zend_object *zobject = nativeObject->m_implPtr->m_zendObject;
   if (!zobject) {
      // new construct
      zend_string *clsName = zend_string_init(className.c_str(), className.length(), 0);
      zend_class_entry *entry = zend_fetch_class(clsName, ZEND_FETCH_CLASS_SILENT);
      zend_string_free(clsName);
      if (!entry) {
         throw FatalError(std::string("Unknown class name ") + className);
      }
      ObjectBinder *binder = new ObjectBinder(entry, nativeObject,
                                              AbstractClassPrivate::getObjectHandlers(entry), 0);
      zobject = binder->getZendObject();
   }
   zval *self = getUnDerefZvalPtr();
   ZVAL_OBJ(self, zobject);
   Z_ADDREF_P(self);
}

ObjectVariant::ObjectVariant(zend_class_entry *entry, std::shared_ptr<StdClass> nativeObject)
{
   zend_object *zobject = nativeObject->m_implPtr->m_zendObject;
   if (!zobject) {
      // new construct
      VMAPI_ASSERT_X(entry, "ObjectVariant::ObjectVariant", "class entry pointer can't be nullptr");
      ObjectBinder *binder = new ObjectBinder(entry, nativeObject,
                                              AbstractClassPrivate::getObjectHandlers(entry), 0);
      zobject = binder->getZendObject();
   }
   zval *self = getUnDerefZvalPtr();
   ZVAL_OBJ(self, zobject);
   Z_ADDREF_P(self);
}

ObjectVariant::ObjectVariant(Variant &&other)
   : Variant(std::move(other))
{
   if (getUnDerefType() != Type::Object) {
      convert_to_object(getUnDerefZvalPtr());
   }
}

ObjectVariant::ObjectVariant(ObjectVariant &&other) noexcept
   : Variant(std::move(other))
{}

ObjectVariant::ObjectVariant(zval &other)
   : ObjectVariant(&other)
{}

ObjectVariant::ObjectVariant(zval &&other)
   : ObjectVariant(&other)
{}

ObjectVariant::ObjectVariant(zval *other)
{
   zval *self = getUnDerefZvalPtr();
   if (nullptr != other && Z_TYPE_P(other) != IS_NULL) {
      if ((Z_TYPE_P(other) == IS_OBJECT ||
           (Z_TYPE_P(other) == IS_REFERENCE && Z_TYPE_P(Z_REFVAL_P(other)) == IS_OBJECT))) {
         ZVAL_DEREF(other);
         ZVAL_COPY(self, other);
      }else {
         ZVAL_DUP(self, other);
         convert_to_object(self);
      }
   } else {
      Z_OBJ_P(self) = nullptr;
      Z_TYPE_INFO_P(self) = IS_OBJECT;
   }
}

ObjectVariant &ObjectVariant::operator =(const ObjectVariant &other)
{
   if (this != &other) {
      Variant::operator =(const_cast<zval *>(other.getZvalPtr()));
   }
   return *this;
}

ObjectVariant &ObjectVariant::operator =(const Variant &other)
{
   if (this != &other) {
      zval *self = getZvalPtr();
      zval *from = const_cast<zval *>(other.getZvalPtr());
      // need set gc info
      if (other.getType() == Type::Object) {
         // standard copy
         Variant::operator =(from);
      } else {
         zval temp;
         // will increase 1 to gc refcount
         ZVAL_DUP(&temp, from);
         // will decrease 1 to gc refcount
         convert_to_object(&temp);
         zval_dtor(self);
         ZVAL_COPY_VALUE(self, &temp);
      }
   }
   return *this;
}

ObjectVariant &ObjectVariant::operator =(ObjectVariant &&other) noexcept
{
   assert(this != &other);
   m_implPtr = std::move(other.m_implPtr);
   return *this;
}

ObjectVariant &ObjectVariant::operator =(Variant &&other)
{
   assert(this != &other);
   m_implPtr = std::move(other.m_implPtr);
   if (getUnDerefType() != Type::Object) {
      convert_to_object(getUnDerefZvalPtr());
   }
   return *this;
}

ObjectVariant &ObjectVariant::setProperty(const std::string &name, const Variant &value)
{
   zval *self = getUnDerefZvalPtr();
   zend_update_property(Z_OBJCE_P(self), self, name.c_str(), name.length(),
                        const_cast<zval *>(value.getUnDerefZvalPtr()));
   return *this;
}

Variant ObjectVariant::getProperty(const std::string &name)
{
   zval *self = getUnDerefZvalPtr();
   zval retval;
   return zend_read_property(Z_OBJCE_P(self), self, name.c_str(), name.length(), 1, &retval);
}

ObjectVariant &ObjectVariant::setStaticProperty(const std::string &name, const Variant &value)
{
   zend_update_static_property(Z_OBJCE_P(getUnDerefZvalPtr()), name.c_str(), name.length(),
                               const_cast<zval *>(value.getUnDerefZvalPtr()));
   return *this;
}

Variant ObjectVariant::getStaticProperty(const std::string &name)
{
   return zend_read_static_property(Z_OBJCE_P(getUnDerefZvalPtr()), name.c_str(), name.length(), 1);
}

bool ObjectVariant::hasProperty(const std::string &name)
{
   int value = 0;
   zval *self = getUnDerefZvalPtr();
#if ZEND_MODULE_API_NO >= 20160303 // imported after php-7.1.0
   zend_class_entry *scope = Z_OBJCE_P(self);
   zend_class_entry *old_scope = EG(fake_scope);
   EG(fake_scope) = scope;
#endif
   if (!Z_OBJ_HT_P(self)->has_property) {
      zend_error(E_CORE_ERROR, "Property %s of class %s cannot be read", name.c_str(), ZSTR_VAL(Z_OBJCE_P(self)->name));
   }
   zval nameZval;
   ZVAL_STRINGL(&nameZval, name.c_str(), name.length());
   // @TODO if I have time, I will find this cache, now pass nullptr
   value = Z_OBJ_HT_P(self)->has_property(self, &nameZval, 2, nullptr);
   zval_dtor(&nameZval);
#if ZEND_MODULE_API_NO >= 20160303 // imported after php-7.1.0
   EG(fake_scope) = old_scope;
#endif
   return 1 == value;
}

bool ObjectVariant::methodExist(const char *name) const
{
   if (Type::Object != getType()) {
      return false;
   }
   zval *self = const_cast<zval *>(getUnDerefZvalPtr());
   zend_class_entry *classEntry = Z_OBJCE_P(self);
   // TODO watch the resource release
   GuardStrType methodName(zend_string_init(name, std::strlen(name), 0),
                           std_zend_string_force_deleter);
   str_tolower(ZSTR_VAL(methodName.get()));
   if (zend_hash_exists(&classEntry->function_table, methodName.get())) {
      return true;
   }
   if (nullptr == Z_OBJ_HT_P(self)->get_method) {
      return false;
   }
   union _zend_function *func = Z_OBJ_HT_P(self)->get_method(&Z_OBJ_P(self), methodName.get(), nullptr);
   if (nullptr == func) {
      return false;
   }
   if (!(func->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
      return true;
   }
   bool result = func->common.scope == zend_ce_closure &&
         zend_string_equals_literal(methodName.get(), ZEND_INVOKE_FUNC_NAME);
   zend_string_release(func->common.function_name);
   zend_free_trampoline(func);
   return result;
}

Variant ObjectVariant::call(const char *name)
{
   return const_cast<const ObjectVariant&>(*this).call(name);
}

Variant ObjectVariant::call(const char *name) const
{
   Variant method(name);
   zval retval;
   do_execute(getZvalPtr(), method.getZvalPtr(), 0, nullptr, &retval);
   Variant ret(retval);
   if (!Z_ISUNDEF(retval) && Z_REFCOUNTED(retval)) {
      zval_ptr_dtor(&retval);
   }
   return std::move(ret);
}

bool ObjectVariant::instanceOf(const char *className, size_t size) const
{
   zend_class_entry *thisClsEntry = Z_OBJCE_P(getUnDerefZvalPtr());
   if (!thisClsEntry) {
      return false;
   }
   GuardStrType clsName(zend_string_init(className, size, 0),
                        std_zend_string_force_deleter);
   zend_class_entry *clsEntry = zend_lookup_class_ex(clsName.get(), nullptr, 0);
   if (!clsEntry) {
      return false;
   }
   return instanceof_function(thisClsEntry, clsEntry);
}

bool ObjectVariant::instanceOf(const char *className) const
{
   return instanceOf(className, std::strlen(className));
}

bool ObjectVariant::instanceOf(const std::string &className) const
{
   return instanceOf(className.c_str(), className.length());
}

bool ObjectVariant::instanceOf(const ObjectVariant &other) const
{
   zend_class_entry *thisClsEntry = Z_OBJCE_P(getUnDerefZvalPtr());
   if (!thisClsEntry) {
      return false;
   }
   zend_class_entry *clsEntry = Z_OBJCE_P(other.getUnDerefZvalPtr());
   if (!clsEntry) {
      return false;
   }
   return instanceof_function(thisClsEntry, clsEntry);
}

bool ObjectVariant::derivedFrom(const char *className, size_t size) const
{
   zend_class_entry *thisClsEntry = Z_OBJCE_P(getUnDerefZvalPtr());
   if (!thisClsEntry) {
      return false;
   }
   GuardStrType clsName(zend_string_init(className, size, 0),
                        std_zend_string_force_deleter);
   zend_class_entry *clsEntry = zend_lookup_class_ex(clsName.get(), nullptr, 0);
   if (!clsEntry) {
      return false;
   }
   if (thisClsEntry == clsEntry) {
      return false;
   }
   return instanceof_function(thisClsEntry, clsEntry);
}

bool ObjectVariant::derivedFrom(const char *className) const
{
   return derivedFrom(className, std::strlen(className));
}

bool ObjectVariant::derivedFrom(const std::string &className) const
{
   return derivedFrom(className.c_str(), className.length());
}

bool ObjectVariant::derivedFrom(const ObjectVariant &other) const
{
   zend_class_entry *thisClsEntry = Z_OBJCE_P(getUnDerefZvalPtr());
   if (!thisClsEntry) {
      return false;
   }
   zend_class_entry *clsEntry = Z_OBJCE_P(other.getUnDerefZvalPtr());
   if (!clsEntry) {
      return false;
   }
   if (thisClsEntry == clsEntry) {
      return false;
   }
   return instanceof_function(thisClsEntry, clsEntry);
}

ObjectVariant::ObjectVariant(StdClass *nativeObject)
{
   zend_object *zobject = nativeObject->m_implPtr->m_zendObject;
   assert(zobject);
   zval *self = getUnDerefZvalPtr();
   ZVAL_OBJ(self, zobject);
   Z_ADDREF_P(self);
   getUnDerefZvalPtr();
}

Variant ObjectVariant::exec(const char *name, int argc, Variant *argv)
{
   return const_cast<const ObjectVariant&>(*this).exec(name, argc, argv);
}

Variant ObjectVariant::exec(const char *name, int argc, Variant *argv) const
{
   Variant methodName(name);
   std::unique_ptr<zval[]> params(new zval[argc]);
   for (int i = 0; i < argc; i++) {
      params[i] = *argv[i].getUnDerefZvalPtr();
   }
   zval retval;
   do_execute(getZvalPtr(), methodName.getZvalPtr(), argc, params.get(), &retval);
   Variant ret(retval);
   if (!Z_ISUNDEF(retval) && Z_REFCOUNTED(retval)) {
      zval_ptr_dtor(&retval);
   }
   return std::move(ret);
}

void ObjectVariant::doClassInvoke(int argc, Variant *argv, zval *retval)
{
   std::unique_ptr<zval[]> params(new zval[argc]);
   for (int i = 0; i < argc; i++) {
      params[i] = *argv[i].getUnDerefZvalPtr();
   }
   do_invoke_class_closure_method(getUnDerefZvalPtr(), argc, params.get(), retval);
}

} // vmapi
} // polar
