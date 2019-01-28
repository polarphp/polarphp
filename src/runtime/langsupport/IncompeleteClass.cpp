// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/01/20.

#include "polarphp/runtime/langsupport/IncompeleteClass.h"

namespace polar {
namespace runtime {

#define INCOMPLETE_CLASS_MSG \
   "The script tried to execute a method or "  \
   "access a property of an incomplete object. " \
   "Please ensure that the class definition \"%s\" of the object " \
   "you are trying to operate on was loaded _before_ " \
   "unserialize() gets called or provide an autoloader " \
   "to load the class definition"

static zend_object_handlers sg_incompleteObjectHandlers;

namespace {

void incomplete_class_message(zval *object, int error_type)
{
   zend_string *class_name;
   class_name = lookup_class_name(object);
   if (class_name) {
      php_error_docref(nullptr, error_type, INCOMPLETE_CLASS_MSG, ZSTR_VAL(class_name));
      zend_string_release_ex(class_name, 0);
   } else {
      php_error_docref(nullptr, error_type, INCOMPLETE_CLASS_MSG, "unknown");
   }
}

zval *incomplete_class_get_property(zval *object, zval *member, int type,
                                    void **cache_slot, zval *rv)
{
   incomplete_class_message(object, E_NOTICE);
   if (type == BP_VAR_W || type == BP_VAR_RW) {
      ZVAL_ERROR(rv);
      return rv;
   } else {
      return &EG(uninitialized_zval);
   }
}

void incomplete_class_write_property(zval *object, zval *member, zval *value,
                                     void **cache_slot)
{
   incomplete_class_message(object, E_NOTICE);
}

zval *incomplete_class_get_property_ptr_ptr(zval *object, zval *member, int type,
                                            void **cache_slot)
{
   incomplete_class_message(object, E_NOTICE);
   return &EG(error_zval);
}

void incomplete_class_unset_property(zval *object, zval *member, void **cache_slot)
{
   incomplete_class_message(object, E_NOTICE);
}

int incomplete_class_has_property(zval *object, zval *member, int check_empty,
                                  void **cache_slot)
{
   incomplete_class_message(object, E_NOTICE);
   return 0;
}

_zend_function *incomplete_class_get_method(zend_object **object, zend_string *method, const zval *key)
{
   zval zobject;
   ZVAL_OBJ(&zobject, *object);
   incomplete_class_message(&zobject, E_ERROR);
   return nullptr;
}

} // anonymous namespace

zend_object *create_incomplete_object(zend_class_entry *class_type)
{
   zend_object *object;
   object = zend_objects_new( class_type);
   object->handlers = &sg_incompleteObjectHandlers;
   object_properties_init(object, class_type);
   return object;
}

zend_class_entry *create_incomplete_class(void)
{
   zend_class_entry incomplete_class;
   INIT_CLASS_ENTRY(incomplete_class, INCOMPLETE_CLASS, nullptr);
   incomplete_class.create_object = create_incomplete_object;
   memcpy(&sg_incompleteObjectHandlers, &std_object_handlers, sizeof(zend_object_handlers));
   sg_incompleteObjectHandlers.read_property = incomplete_class_get_property;
   sg_incompleteObjectHandlers.has_property = incomplete_class_has_property;
   sg_incompleteObjectHandlers.unset_property = incomplete_class_unset_property;
   sg_incompleteObjectHandlers.write_property = incomplete_class_write_property;
   sg_incompleteObjectHandlers.get_property_ptr_ptr = incomplete_class_get_property_ptr_ptr;
   sg_incompleteObjectHandlers.get_method = incomplete_class_get_method;
   return zend_register_internal_class(&incomplete_class);
}

zend_string *lookup_class_name(zval *object)
{
   zval *val;
   HashTable *object_properties;
   object_properties = Z_OBJPROP_P(object);
   if ((val = zend_hash_str_find(object_properties, MAGIC_MEMBER, sizeof(MAGIC_MEMBER)-1)) != nullptr && Z_TYPE_P(val) == IS_STRING) {
      return zend_string_copy(Z_STR_P(val));
   }
   return nullptr;
}

void store_class_name(zval *object, const char *name, size_t len)
{
   zval val;
   ZVAL_STRINGL(&val, name, len);
   zend_hash_str_update(Z_OBJPROP_P(object), MAGIC_MEMBER, sizeof(MAGIC_MEMBER)-1, &val);
}

} // runtime
} // polar
