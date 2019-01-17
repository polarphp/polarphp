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

#ifndef POLARPHP_VMAPI_INTERNAL_ABSTRACT_CLASS_PRIVATE_H
#define POLARPHP_VMAPI_INTERNAL_ABSTRACT_CLASS_PRIVATE_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/basic/adt/StringRef.h"

#include <string>
#include <list>
#include <map>

namespace polar {
namespace vmapi {

/// forward declare class
class Variant;
class Method;
class Interface;
class StdClass;
class AbstractMember;
class AbstractClass;
class Property;

using polar::basic::StringRef;

namespace internal
{

class AbstractClassPrivate;

struct CallContext
{
   zend_internal_function m_func;
   AbstractClassPrivate *m_selfPtr;
};

using ContextMapType = std::map<std::string, std::shared_ptr<CallContext>>;

class AbstractClassPrivate
{
public:
   AbstractClassPrivate(StringRef classname, ClassType type);
   zend_class_entry *initialize(AbstractClass *cls, const std::string &ns, int moduleNumber);
   std::unique_ptr<zend_function_entry[]> &getMethodEntries();
   zend_object_handlers *getObjectHandlers();
   ~AbstractClassPrivate();
   static zend_object_handlers *getObjectHandlers(zend_class_entry *entry);

   // php class system facility static handle methods
   static zend_object *createObject(zend_class_entry *entry);
   static int inlineInterfaceImplement(zend_class_entry *iface, zend_class_entry *classType);
   static zend_object *cloneObject(zval *object);

   // php object handlers
   static int countElements(zval *object, zend_long *count);
   static zval *readDimension(zval *object, zval *offset, int type, zval *returnValue);
   static void writeDimension(zval *object, zval *offset, zval *value);
   static int hasDimension(zval *object, zval *offset, int checkEmpty);
   static void unsetDimension(zval *object, zval *offset);
   static zend_object_iterator *getIterator(zend_class_entry *entry, zval *object, int byRef);
   static int serialize(zval *object, unsigned char **buffer, size_t *bufLength, zend_serialize_data *data);
   static int unserialize(zval *object, zend_class_entry *entry, const unsigned char *buffer,
                          size_t bufLength, zend_unserialize_data *data);
   static HashTable *debugInfo(zval *object, int *isTemp);
   // property
   static zval *readProperty(zval *object, zval *name, int type, void **cacheSlot, zval *returnValue);
   static void writeProperty(zval *object, zval *name, zval *value, void **cacheSlot);
   static int hasProperty(zval *object, zval *name, int hasSetExists, void **cacheSlot);
   static void unsetProperty(zval *object, zval *name, void **cacheSlot);
   // method call
   static zend_function *getMethod(zend_object **object, zend_string *method, const zval *key);
   static zend_function *getStaticMethod(zend_class_entry *entry, zend_string *methodName);
   static int getClosure(zval *object, zend_class_entry **entry, zend_function **retFunc, zend_object **objectPtr);
   static void magicCallForwarder(zend_execute_data *execute_data, zval *return_value);
   static void magicInvokeForwarder(zend_execute_data *execute_data, zval *return_value);
   // destruct object
   static void destructObject(zend_object *object);
   static void freeObject(zend_object *object);
   // cast
   static int cast(zval *object, zval *retValue, int type);
   static int compare(zval *left, zval *right);
   static zval *toZval(Variant &&value, int type, zval *rv);

public:
   bool m_intialized = false;
   ClassType m_type = ClassType::Regular;
   AbstractClass *m_apiPtr;
   zend_class_entry *m_classEntry = nullptr;
   std::shared_ptr<AbstractClass> m_parent;
   std::unique_ptr<zend_string, std::function<void(zend_string *)>> m_self = nullptr;
   std::unique_ptr<zend_function_entry[]> m_methodEntries;
   std::string m_name;
   zend_object_handlers m_handlers;
   std::list<std::shared_ptr<AbstractClass>> m_interfaces;
   std::list<std::shared_ptr<Method>> m_methods;
   std::list<std::shared_ptr<AbstractMember>> m_members;
   std::map<std::string, std::shared_ptr<Property>> m_properties;
};
} // internal
} // vmapi
} // polar

#endif // POLARPHP_VMAPI_INTERNAL_ABSTRACT_CLASS_PRIVATE_H
