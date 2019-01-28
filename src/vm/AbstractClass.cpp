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

#include "polarphp/vm/AbstractClass.h"
#include "polarphp/vm/IteratorBridge.h"
#include "polarphp/vm/AbstractClass.h"
#include "polarphp/vm/internal/AbstractClassPrivate.h"
#include "polarphp/vm/ObjectBinder.h"
#include "polarphp/vm/AbstractMember.h"
#include "polarphp/vm/StringMember.h"
#include "polarphp/vm/BooleanMember.h"
#include "polarphp/vm/FloatMember.h"
#include "polarphp/vm/NumericMember.h"
#include "polarphp/vm/NullMember.h"
#include "polarphp/vm/Property.h"
#include "polarphp/vm/StdClass.h"
#include "polarphp/vm/ds/Variant.h"
#include "polarphp/vm/ds/StringVariant.h"
#include "polarphp/vm/ds/NumericVariant.h"
#include "polarphp/vm/ds/DoubleVariant.h"
#include "polarphp/vm/ds/BooleanVariant.h"
#include "polarphp/vm/ds/ArrayVariant.h"
#include "polarphp/vm/lang/Method.h"

#include "polarphp/vm/lang/Constant.h"
#include "polarphp/vm/lang/Method.h"
#include "polarphp/vm/lang/Interface.h"
#include "polarphp/vm/lang/Parameter.h"
#include "polarphp/vm/utils/NotImplemented.h"
#include "polarphp/vm/utils/OrigException.h"
#include "polarphp/vm/protocol/AbstractIterator.h"
#include "polarphp/vm/protocol/ArrayAccess.h"
#include "polarphp/vm/protocol/Countable.h"
#include "polarphp/vm/protocol/Serializable.h"
#include "polarphp/vm/protocol/Traversable.h"
#include "polarphp/vm/utils/Funcs.h"
#include "polarphp/vm/utils/UserspaceFuncs.h"

#include "polarphp/runtime/RtDefs.h"

#include <iostream>
#include <cstring>

namespace polar {
namespace vmapi {
namespace internal
{

thread_local ContextMapType sg_contextPtrs{};

namespace
{
AbstractClassPrivate *retrieve_acp_ptr_from_cls_entry(zend_class_entry *entry)
{
   // we hide the pointer in entry->info.user.doc_comment
   // the format is \0 + pointer_address
   // if entry->info.user.doc_comment length > 0 or nullptr no pointer hide in it
   while (entry->parent && (nullptr == entry->info.user.doc_comment ||
                            ZSTR_LEN(entry->info.user.doc_comment) > 0))
   {
      // we find the pointer in parent classs
      entry = entry->parent;
   }
   const char *comment = ZSTR_VAL(entry->info.user.doc_comment);
   // here we retrieve the second byte, it have the pointer infomation
   return *reinterpret_cast<AbstractClassPrivate **>(const_cast<char *>(comment + 1));
}

void acp_ptr_deleter(zend_string *ptr)
{
   zend_string_release(ptr);
}

#define MAX_ABSTRACT_INFO_CNT 3
#define MAX_ABSTRACT_INFO_FMT "%s%s%s%s"
#define DISPLAY_ABSTRACT_FN(idx) \
   ai.afn[idx] ? ZEND_FN_SCOPE_NAME(ai.afn[idx]) : "", \
   ai.afn[idx] ? "::" : "", \
   ai.afn[idx] ? ZSTR_VAL(ai.afn[idx]->common.function_name) : "", \
   ai.afn[idx] && ai.afn[idx + 1] ? ", " : (ai.afn[idx] && ai.cnt > MAX_ABSTRACT_INFO_CNT ? ", ..." : "")

typedef struct _zend_abstract_info {
   zend_function *afn[MAX_ABSTRACT_INFO_CNT + 1];
   int cnt;
   int ctor;
} zend_abstract_info;

void verify_abstract_class_function(zend_function *fn, zend_abstract_info *ai) /* {{{ */
{
   if (fn->common.fn_flags & ZEND_ACC_ABSTRACT) {
      if (ai->cnt < MAX_ABSTRACT_INFO_CNT) {
         ai->afn[ai->cnt] = fn;
      }
      if (fn->common.fn_flags & ZEND_ACC_CTOR) {
         if (!ai->ctor) {
            ai->cnt++;
            ai->ctor = 1;
         } else {
            ai->afn[ai->cnt] = nullptr;
         }
      } else {
         ai->cnt++;
      }
   }
}

void verify_abstract_class(zend_class_entry *ce)
{
   void *func;
   zend_abstract_info ai;
   memset(&ai, 0, sizeof(ai));
   ZEND_HASH_FOREACH_PTR(&ce->function_table, func) {
      verify_abstract_class_function(reinterpret_cast<zend_function *>(func), &ai);
   } ZEND_HASH_FOREACH_END();

   if (ai.cnt) {
      zend_error(E_ERROR, "Class %s contains %d abstract method%s and must therefore be declared abstract or implement the remaining methods (" MAX_ABSTRACT_INFO_FMT MAX_ABSTRACT_INFO_FMT MAX_ABSTRACT_INFO_FMT ")",
                 ZSTR_VAL(ce->name), ai.cnt,
                 ai.cnt > 1 ? "s" : "",
                 DISPLAY_ABSTRACT_FN(0),
                 DISPLAY_ABSTRACT_FN(1),
                 DISPLAY_ABSTRACT_FN(2)
                 );
   }
}

} // anonymous namespace

AbstractClassPrivate::AbstractClassPrivate(StringRef className, ClassType type)
   : m_type(type),
     m_self(nullptr, acp_ptr_deleter),
     m_name(className.getStr())
{}

zend_class_entry *AbstractClassPrivate::initialize(AbstractClass *cls, const std::string &ns, int moduleNumber)
{
   m_apiPtr = cls;
   zend_class_entry entry;
   std::memset(&entry, 0, sizeof(zend_class_entry));
   if (ns.size() > 0 && ns != "\\") {
      m_name = ns + "\\" + m_name;
   }
   // initialize the class entry
   INIT_CLASS_ENTRY_EX(entry, m_name.c_str(), m_name.size(), getMethodEntries().get());
   if (m_type != ClassType::Interface) {
      entry.create_object = &AbstractClassPrivate::createObject;
   } else {
      entry.interface_gets_implemented = &AbstractClassPrivate::inlineInterfaceImplement;
   }
   entry.get_static_method = &AbstractClassPrivate::getStaticMethod;
   // check if traversable
   if (m_apiPtr->traversable()) {
      entry.get_iterator = &AbstractClassPrivate::getIterator;
      /// TODO review here
      /// from 7.3 and up, we may have to allocate it ourself
      /// entry.iterator_funcs_ptr = calloc(1, sizeof(zend_class_iterator_funcs))
   }
   // check if serializable
   if (m_apiPtr->serializable()) {
      entry.serialize = &AbstractClassPrivate::serialize;
      entry.unserialize = &AbstractClassPrivate::unserialize;
   }

   if (m_parent) {
      if (m_parent->m_implPtr->m_classEntry) {
         m_classEntry = zend_register_internal_class_ex(&entry, m_parent->m_implPtr->m_classEntry);
      } else {
         std::cerr << "Derived class " << m_name << " is initialized before base class " << m_parent->m_implPtr->m_name
                   << ": base class is ignored" << std::endl;
         // ignore base class
         m_classEntry = zend_register_internal_class(&entry);
      }
   } else {
      m_classEntry = zend_register_internal_class(&entry);
   }
   m_classEntry->ce_flags = static_cast<uint32_t>(m_type) | ZEND_ACC_CONSTANTS_UPDATED;
   // register the interfaces of the class
   for (std::shared_ptr<AbstractClass> &interface : m_interfaces) {
      if (interface->m_implPtr->m_classEntry) {
         zend_do_implement_interface(m_classEntry, interface->m_implPtr->m_classEntry);
      } else {
         // interface that want to implement is not initialized
         std::cerr << "Derived class " << m_name << " is initialized before base class "
                   << interface->m_implPtr->m_name << ": interface is ignored"
                   << std::endl;
      }
   }
   for (std::shared_ptr<AbstractMember> &member : m_members) {
      member->initialize(m_classEntry);
   }
   // save AbstractClassPrivate instance pointer into the info.user.doc_comment of zend_class_entry
   // we need save the address of this pointer
   AbstractClassPrivate *selfPtr = this;
   m_self.reset(zend_string_alloc(sizeof(this), 1));
   // make the string look like empty
   ZSTR_VAL(m_self)[0] = '\0';
   ZSTR_LEN(m_self) = 0;
   std::memcpy(ZSTR_VAL(m_self.get()) + 1, &selfPtr, sizeof(selfPtr));
   // save into the doc_comment
   m_classEntry->info.user.doc_comment = m_self.get();
   return m_classEntry;
}

std::unique_ptr<zend_function_entry[]>& AbstractClassPrivate::getMethodEntries()
{
   if (m_methodEntries) {
      return m_methodEntries;
   }
   m_methodEntries.reset(new zend_function_entry[m_methods.size() + 1]);
   size_t i = 0;
   for (std::shared_ptr<Method> &method : m_methods) {
      zend_function_entry *entry = &m_methodEntries[i++];
      method->initialize(entry, m_name.c_str());
   }
   // the last item must be set to 0
   // let zend engine know where to stop
   zend_function_entry *last = &m_methodEntries[i];
   memset(last, 0, sizeof(*last));
   return m_methodEntries;
}

zend_object_handlers *AbstractClassPrivate::getObjectHandlers()
{
   if (m_intialized) {
      return &m_handlers;
   }
   memcpy(&m_handlers, &std_object_handlers, sizeof(zend_object_handlers));
   if (!m_apiPtr->clonable()) {
      m_handlers.clone_obj = nullptr;
   } else {
      m_handlers.clone_obj = &AbstractClassPrivate::cloneObject;
   }
   // function for array access interface
   m_handlers.count_elements = &AbstractClassPrivate::countElements;
   m_handlers.write_dimension = &AbstractClassPrivate::writeDimension;
   m_handlers.read_dimension = &AbstractClassPrivate::readDimension;
   m_handlers.has_dimension = &AbstractClassPrivate::hasDimension;
   m_handlers.unset_dimension = &AbstractClassPrivate::unsetDimension;
   m_handlers.get_debug_info = &AbstractClassPrivate::debugInfo;
   // functions for magic properties handlers __get, __set, __isset and __unset
   m_handlers.write_property = &AbstractClassPrivate::writeProperty;
   m_handlers.read_property = &AbstractClassPrivate::readProperty;
   m_handlers.has_property = &AbstractClassPrivate::hasProperty;
   m_handlers.unset_property = &AbstractClassPrivate::unsetProperty;

   // functions for method is called
   m_handlers.get_method = &AbstractClassPrivate::getMethod;
   m_handlers.get_closure = &AbstractClassPrivate::getClosure;

   // functions for object destruct
   m_handlers.dtor_obj = &AbstractClassPrivate::destructObject;
   m_handlers.free_obj = &AbstractClassPrivate::freeObject;

   // functions for type cast
   m_handlers.cast_object = &AbstractClassPrivate::cast;
   m_handlers.compare_objects = &AbstractClassPrivate::compare;
   // we set offset here zend engine will free ObjectBinder::m_container
   // resource automatic
   // this offset is very important if you set this not right, memory will leak
   m_handlers.offset = ObjectBinder::calculateZendObjectOffset();
   m_intialized = true;
   return &m_handlers;
}

AbstractClassPrivate::~AbstractClassPrivate()
{}

zend_object *AbstractClassPrivate::createObject(zend_class_entry *entry)
{
   if (!(entry->ce_flags & (ZEND_ACC_TRAIT|ZEND_ACC_EXPLICIT_ABSTRACT_CLASS|ZEND_ACC_INTERFACE|ZEND_ACC_IMPLEMENT_INTERFACES|ZEND_ACC_IMPLEMENT_TRAITS))) {
      verify_abstract_class(entry);
   }
   // here we lose everything from AbstractClass object
   // of course we are in static method of AbstractClass
   // but we need get pointer to it, we need some meta info in it and we must
   // instantiate native c++ class associated with the meta class
   AbstractClassPrivate *abstractClsPrivatePtr = retrieve_acp_ptr_from_cls_entry(entry);
   // note: here we use StdClass type to store Derived class
   std::shared_ptr<StdClass> nativeObject(abstractClsPrivatePtr->m_apiPtr->construct());
   if (!nativeObject) {
      // report error on failure, because this function is called directly from the
      // Zend engine, we can call zend_error() here (which does a longjmp() back to
      // the Zend engine)
      zend_error(E_ERROR, "Unable to instantiate %s", entry->name->val);
      //return nullptr;
   }
   // here we assocaited a native object with an ObjectBinder object
   // ObjectBinder can make an relationship on nativeObject and zend_object
   // don't warry about memory, we do release
   // maybe memory leak
   ObjectBinder *binder = new ObjectBinder(entry, nativeObject, abstractClsPrivatePtr->getObjectHandlers(), 1);
   return binder->getZendObject();
}

///
/// special interface check
///
int AbstractClassPrivate::inlineInterfaceImplement(zend_class_entry *, zend_class_entry *)
{
   return SUCCESS;
}

zend_object_handlers *AbstractClassPrivate::getObjectHandlers(zend_class_entry *entry)
{
   AbstractClassPrivate *abstractClsPrivatePtr = retrieve_acp_ptr_from_cls_entry(entry);
   return abstractClsPrivatePtr->getObjectHandlers();
}

zend_object *AbstractClassPrivate::cloneObject(zval *object)
{
   zend_class_entry *entry = Z_OBJCE_P(object);
   ObjectBinder *objectBinder = ObjectBinder::retrieveSelfPtr(object);
   AbstractClassPrivate *selfPtr = retrieve_acp_ptr_from_cls_entry(entry);
   AbstractClass *meta = selfPtr->m_apiPtr;
   StdClass *origObject = objectBinder->getNativeObject();
   std::unique_ptr<StdClass> newNativeObject(meta->clone(origObject));
   // report error on failure (this does not occur because the cloneObject()
   // method is only installed as handler when we have seen that there is indeed
   // a copy constructor). Because this function is directly called from the
   // Zend engine, we can call zend_error() (which does a longjmp()) to throw
   // an exception back to the Zend engine)
   if (!newNativeObject) {
      zend_error(E_ERROR, "Unable to clone %s", ZSTR_VAL(entry->name));
   }
   ObjectBinder *newObjectBinder = new ObjectBinder(entry, std::move(newNativeObject), selfPtr->getObjectHandlers(), 1);
   zend_objects_clone_members(newObjectBinder->getZendObject(), objectBinder->getZendObject());
   if (!entry->clone) {
      meta->callClone(newNativeObject.get());
   }
   return newObjectBinder->getZendObject();
}

int AbstractClassPrivate::countElements(zval *object, zend_long *count)
{
   Countable *countable = dynamic_cast<Countable *>(ObjectBinder::retrieveSelfPtr(object)->getNativeObject());
   if (countable) {
      try {
         *count = countable->count();
         return VMAPI_SUCCESS;
      } catch (Exception &exception) {
         process_exception(exception);
         return VMAPI_FAILURE; // unreachable, prevent some compiler warning
      }
   } else {
      if (!std_object_handlers.count_elements) {
         return VMAPI_FAILURE;
      }
      return std_object_handlers.count_elements(object, count);
   }
}

///
/// review
///
zval *AbstractClassPrivate::readDimension(zval *object, zval *offset, int type, zval *returnValue)
{
   // what to do with the type?
   //
   // the type parameter tells us whether the dimension was read in READ
   // mode, WRITE mode, READWRITE mode or UNSET mode.
   //
   // In 99 out of 100 situations, it is called in regular READ mode (value 0),
   // only when it is called from a PHP script that has statements like
   // $x =& $object["x"], $object["x"]["y"] = "something" or unset($object["x"]["y"]),
   // the type parameter is set to a different value.
   //
   // But we must ask ourselves the question what we should be doing with such
   // cases. Internally, the object most likely has a full native implementation,
   // and the property that is returned is just a string or integer or some
   // other value, that is temporary WRAPPED into a zval to make it accessible
   // from PHP. If someone wants to get a reference to such an internal variable,
   // that is in most cases simply impossible.
   ArrayAccess *arrayAccess = dynamic_cast<ArrayAccess *>(ObjectBinder::retrieveSelfPtr(object)->getNativeObject());
   if (arrayAccess) {
      try {
         return toZval(arrayAccess->offsetGet(offset), type, returnValue);
      } catch (Exception &exception) {
         process_exception(exception);
         return nullptr; // unreachable, prevent some compiler warning
      }
   } else {
      if (!std_object_handlers.read_dimension) {
         return nullptr;
      } else {
         return std_object_handlers.read_dimension(object, offset, type, returnValue);
      }
   }
}

void AbstractClassPrivate::writeDimension(zval *object, zval *offset, zval *value)
{
   ArrayAccess *arrayAccess = dynamic_cast<ArrayAccess *>(ObjectBinder::retrieveSelfPtr(object)->getNativeObject());
   if (arrayAccess) {
      try {
         arrayAccess->offsetSet(offset, value);
      } catch (Exception &exception) {
         process_exception(exception);
      }
   } else {
      if (!std_object_handlers.write_dimension) {
         return;
      } else {
         std_object_handlers.write_dimension(object, offset, value);
      }
   }
}

int AbstractClassPrivate::hasDimension(zval *object, zval *offset, int checkEmpty)
{
   ArrayAccess *arrayAccess = dynamic_cast<ArrayAccess *>(ObjectBinder::retrieveSelfPtr(object)->getNativeObject());
   if (arrayAccess) {
      try {
         if (!arrayAccess->offsetExists(offset)) {
            return false;
         }
         if (!checkEmpty) {
            return true;
         }
         return empty(arrayAccess->offsetGet(offset));
      } catch (Exception &exception) {
         process_exception(exception);
         return false; // unreachable, prevent some compiler warning
      }
   } else {
      if (!std_object_handlers.has_dimension) {
         return false;
      }
      return std_object_handlers.has_dimension(object, offset, checkEmpty);
   }
}

void AbstractClassPrivate::unsetDimension(zval *object, zval *offset)
{
   ArrayAccess *arrayAccess = dynamic_cast<ArrayAccess *>(ObjectBinder::retrieveSelfPtr(object)->getNativeObject());
   if (arrayAccess) {
      try {
         arrayAccess->offsetUnset(offset);
      } catch (Exception &exception) {
         process_exception(exception);
      }
   } else {
      if (!std_object_handlers.unset_dimension) {
         return;
      }
      return std_object_handlers.unset_dimension(object, offset);
   }
}

zend_object_iterator *AbstractClassPrivate::getIterator(zend_class_entry *entry, zval *object, int byRef)
{
   // by-ref is not possible (copied from SPL), this function is called directly
   // from the Zend engine, so we can use zend_error() to longjmp() back to the
   // Zend engine)
   // TODO but why
   if (byRef) {
      zend_error(E_ERROR, "Foreach by ref is not possible");
   }
   Traversable *traversable = dynamic_cast<Traversable *>(ObjectBinder::retrieveSelfPtr(object)->getNativeObject());
   VMAPI_ASSERT_X(traversable, "AbstractClassPrivate::getIterator", "traversable can't be nullptr");
   try {
      AbstractIterator *iterator = traversable->getIterator();
      VMAPI_ASSERT_X(iterator,  "AbstractClassPrivate::getIterator", "iterator can't be nullptr");
      // @mark native memory alloc
      // we are going to allocate an extended iterator (because php nowadays destructs
      // the iteraters itself, we can no longer let c++ allocate the buffer + object
      // directly, so we first allocate the buffer, which is going to be cleaned up by php)
      void *buffer = emalloc(sizeof(IteratorBridge));
      IteratorBridge *iteratorBridge = new (buffer)IteratorBridge(object, iterator);
      return iteratorBridge->getZendIterator();
   } catch (Exception &exception) {
      process_exception(exception);
      return nullptr; // unreachable, prevent some compiler warning
   }
}

int AbstractClassPrivate::serialize(zval *object, unsigned char **buffer, size_t *bufLength, zend_serialize_data *data)
{
   Serializable *serializable = dynamic_cast<Serializable *>(ObjectBinder::retrieveSelfPtr(object)->getNativeObject());
   // user may throw an exception in the serialize() function
   try {
      std::string value = serializable->serialize();
      *buffer = reinterpret_cast<unsigned char *>(estrndup(value.c_str(), value.length()));
      *bufLength = value.length();
   } catch (Exception &exception) {
      process_exception(exception);
      return VMAPI_FAILURE; // unreachable, prevent some compiler warning
   }
   return VMAPI_SUCCESS;
}

int AbstractClassPrivate::unserialize(zval *object, zend_class_entry *entry, const unsigned char *buffer,
                                      size_t bufLength, zend_unserialize_data *data)
{
   object_init_ex(object, entry);
   Serializable *serializable = dynamic_cast<Serializable *>(ObjectBinder::retrieveSelfPtr(object)->getNativeObject());
   // user may throw an exception in the unserialize() function
   try {
      serializable->unserialize(reinterpret_cast<const char *>(buffer), bufLength);
   } catch (Exception &exception) {
      // user threw an exception in its method
      // implementation, send it to user space
      // process_exception(exception);
      php_error_docref(nullptr, E_NOTICE, "Error while unserializing");
      return VMAPI_FAILURE;
   }
   return VMAPI_SUCCESS;
}

HashTable *AbstractClassPrivate::debugInfo(zval *object, int *isTemp)
{
   try {
      ObjectBinder *objectBinder = ObjectBinder::retrieveSelfPtr(object);
      AbstractClassPrivate *selfPtr = retrieve_acp_ptr_from_cls_entry(Z_OBJCE_P(object));
      AbstractClass *meta = selfPtr->m_apiPtr;
      StdClass *nativeObject = objectBinder->getNativeObject();
      zval infoZval = meta->callDebugInfo(nativeObject).detach(true);
      *isTemp = 1;
      return Z_ARR(infoZval);
   } catch (const NotImplemented &exception) {
      if (!std_object_handlers.get_debug_info) {
         return nullptr;
      }
      return std_object_handlers.get_debug_info(object, isTemp);
   } catch (Exception &exception) {
      process_exception(exception);
      // this statement will never execute
      return nullptr;
   }
}

zval *AbstractClassPrivate::readProperty(zval *object, zval *name, int type, void **cacheSlot, zval *rv)
{
   // what to do with the type?
   //
   // the type parameter tells us whether the property was read in READ
   // mode, WRITE mode, READWRITE mode or UNSET mode.
   //
   // In 99 out of 100 situations, it is called in regular READ mode (value 0),
   // only when it is called from a PHP script that has statements like
   // $x =& $object->x, $object->x->y = "something" or unset($object->x->y)
   // the type parameter is set to a different value.
   //
   // But we must ask ourselves the question what we should be doing with such
   // cases. Internally, the object most likely has a full native implementation,
   // and the property that is returned is just a string or integer or some
   // other value, that is temporary WRAPPED into a zval to make it accessible
   // from PHP. If someone wants to get a reference to such an internal variable,
   // that is in most cases simply impossible.
   // retrieve the object and class
   zval *retval;
   try {
      ObjectBinder *objectBinder = ObjectBinder::retrieveSelfPtr(object);
      AbstractClassPrivate *selfPtr = retrieve_acp_ptr_from_cls_entry(Z_OBJCE_P(object));
      AbstractClass *meta = selfPtr->m_apiPtr;
      StdClass *nativeObject = objectBinder->getNativeObject();
      std::string key(Z_STRVAL_P(name), Z_STRLEN_P(name));
      auto iter = selfPtr->m_properties.find(key);
      if (iter != selfPtr->m_properties.end()) {
         // self defined getter method
         retval = toZval(iter->second->get(nativeObject), type, rv);
      } else {
         retval = toZval(meta->callGet(nativeObject, key), type, rv);
      }
      return retval;
   } catch (const NotImplemented &) {
      if (!std_object_handlers.read_property) {
         retval = &EG(uninitialized_zval);
         return retval;
      }
      return std_object_handlers.read_property(object, name, type, cacheSlot, rv);
   } catch (Exception &exception) {
      process_exception(exception);
      // this statement will never execute
   } catch(...) {
      retval = &EG(uninitialized_zval);
   }
   return retval;
}

void AbstractClassPrivate::writeProperty(zval *object, zval *name, zval *value, void **cacheSlot)
{
   try {
      ObjectBinder *objectBinder = ObjectBinder::retrieveSelfPtr(object);
      AbstractClassPrivate *selfPtr = retrieve_acp_ptr_from_cls_entry(Z_OBJCE_P(object));
      AbstractClass *meta = selfPtr->m_apiPtr;
      StdClass *nativeObject = objectBinder->getNativeObject();
      std::string key(Z_STRVAL_P(name), Z_STRLEN_P(name));
      auto iter = selfPtr->m_properties.find(key);
      if (iter != selfPtr->m_properties.end()) {
         if (iter->second->set(nativeObject, value)) {
            return;
         }
         zend_error(E_ERROR, "Unable to write to read-only property %s", key.c_str());
      } else {
         meta->callSet(nativeObject, key, value);
      }
   } catch (const NotImplemented &) {
      if (!std_object_handlers.write_property) {
         return;
      }
      std_object_handlers.write_property(object, name, value, cacheSlot);
   } catch (Exception &exception) {
      process_exception(exception);
   }
}

int AbstractClassPrivate::hasProperty(zval *object, zval *name, int hasSetExists, void **cacheSlot)
{
   try {
      ObjectBinder *objectBinder = ObjectBinder::retrieveSelfPtr(object);
      AbstractClassPrivate *selfPtr = retrieve_acp_ptr_from_cls_entry(Z_OBJCE_P(object));
      AbstractClass *meta = selfPtr->m_apiPtr;
      StdClass *nativeObject = objectBinder->getNativeObject();
      std::string key(Z_STRVAL_P(name), Z_STRLEN_P(name));
      // here we need check the hasSetExists
      if (selfPtr->m_properties.find(key) != selfPtr->m_properties.end()) {
         return true;
      }
      if (!meta->callIsset(nativeObject, key)) {
         return false;
      }
      if (ZEND_PROPERTY_EXISTS == hasSetExists) {
         return true;
      }
      Variant value = meta->callGet(nativeObject, key);
      if (ZEND_PROPERTY_ISSET == hasSetExists) {
         return value.getType() != Type::Null;
      } else {
         ZEND_ASSERT(ZEND_PROPERTY_NOT_EMPTY == hasSetExists);
         return value.toBoolean();
      }
   } catch (const NotImplemented &) {
      if (!std_object_handlers.has_property) {
         return false;
      }
      return std_object_handlers.has_property(object, name, hasSetExists, cacheSlot);
   } catch (Exception &exception) {
      process_exception(exception);
      return false;
   }
}

void AbstractClassPrivate::unsetProperty(zval *object, zval *name, void **cacheSlot)
{
   try {
      ObjectBinder *objectBinder = ObjectBinder::retrieveSelfPtr(object);
      AbstractClassPrivate *selfPtr = retrieve_acp_ptr_from_cls_entry(Z_OBJCE_P(object));
      AbstractClass *meta = selfPtr->m_apiPtr;
      StdClass *nativeObject = objectBinder->getNativeObject();
      std::string key(Z_STRVAL_P(name), Z_STRLEN_P(name));
      if (selfPtr->m_properties.find(key) == selfPtr->m_properties.end()) {
         meta->callUnset(nativeObject, key);
      } else {
         zend_error(E_ERROR, "Property %s can not be unset", key.c_str());
      }
   } catch (const NotImplemented &) {
      if (!std_object_handlers.unset_property) {
         return;
      }
      std_object_handlers.unset_property(object, name, cacheSlot);
   } catch (Exception &exception) {
      process_exception(exception);
   }
}

zend_function *AbstractClassPrivate::getMethod(zend_object **object, zend_string *methodName, const zval *key)
{
   // something strange about the Zend engine (once more). The structure with
   // object-handlers has a get_method and call_method member. When a function is
   // called, the get_method function is called first, to retrieve information
   // about the method (like the handler that should be called to execute it),
   // after that, this returned handler is also called. The call_method property
   // of the object_handlers structure however, never gets called. Typical.

   // first we'll check if the default handler does not have an implementation,
   // in that case the method is probably already implemented as a regular method
   zend_function *defaultFuncInfo = std_object_handlers.get_method(object, methodName, key);
   if (defaultFuncInfo) {
      return defaultFuncInfo;
   }
   // if exception throw before delete the memory will be relase after request cycle
   zend_class_entry *defClassEntry = (*object)->ce;
   assert(defClassEntry);
   std::string contextKey(defClassEntry->name->val, defClassEntry->name->len);
   contextKey.append("::");
   contextKey.append(methodName->val, methodName->len);
   CallContext *callContext  = nullptr;
   auto targetContext = sg_contextPtrs.find(contextKey);
   if (targetContext != sg_contextPtrs.end()) {
      callContext = targetContext->second.get();
   } else {
      std::shared_ptr<CallContext> targetContext(new CallContext);
      callContext = targetContext.get();
      zend_internal_function *func = &callContext->m_func;
      func->type = ZEND_INTERNAL_FUNCTION;
      func->module = nullptr;
      func->handler = AbstractClassPrivate::magicCallForwarder;
      func->arg_info = nullptr;
      func->num_args = 0;
      func->required_num_args = 0;
      func->scope = defClassEntry;
      func->fn_flags = ZEND_ACC_CALL_VIA_HANDLER;
      func->function_name = methodName;
      callContext->m_selfPtr = retrieve_acp_ptr_from_cls_entry(defClassEntry);
      sg_contextPtrs[contextKey] = std::move(targetContext);
   }
   return reinterpret_cast<zend_function *>(callContext);
}

zend_function *AbstractClassPrivate::getStaticMethod(zend_class_entry *entry, zend_string *methodName)
{
   zend_function *defaultFuncInfo = zend_std_get_static_method(entry, methodName, nullptr);
   if (defaultFuncInfo) {
      return defaultFuncInfo;
   }
   // if exception throw before delete the memory will be relase after request cycle
   std::string contextKey(entry->name->val,  entry->name->len);
   contextKey.append("::");
   contextKey.append(methodName->val, methodName->len);
   contextKey.append("static");
   CallContext *callContext  = nullptr;
   auto targetContext = sg_contextPtrs.find(contextKey);
   if (targetContext != sg_contextPtrs.end()) {
      callContext = targetContext->second.get();
   } else {
      std::shared_ptr<CallContext> targetContext(new CallContext);
      callContext = targetContext.get();
      zend_internal_function *func = &callContext->m_func;
      func->type = ZEND_INTERNAL_FUNCTION;
      func->module = nullptr;
      func->handler = &AbstractClassPrivate::magicCallForwarder;
      func->arg_info = nullptr;
      func->num_args = 0;
      func->required_num_args = 0;
      func->scope = nullptr;
      func->fn_flags = ZEND_ACC_CALL_VIA_HANDLER | ZEND_ACC_STATIC;
      func->function_name = methodName;
      callContext->m_selfPtr = retrieve_acp_ptr_from_cls_entry(entry);
      sg_contextPtrs[contextKey] = std::move(targetContext);
   }
   return reinterpret_cast<zend_function *>(callContext);
}

int AbstractClassPrivate::getClosure(zval *object, zend_class_entry **entry, zend_function **retFunc,
                                     zend_object **objectPtr)
{
   // it is really unbelievable how the Zend engine manages to implement every feature
   // in a complete different manner. You would expect the __invoke() and the
   // __call() functions not to be very different from each other. However, they
   // both have a completely different API. This getClosure method is supposed
   // to fill the function parameter with all information about the invoke()
   // method that is going to get called

   // just like we did for getMethod(), we're going to dynamically allocate memory
   // with all information about the function
   zend_class_entry *defClassEntry = Z_OBJCE_P(object);
   assert(defClassEntry);
   std::string contextKey(defClassEntry->name->val, defClassEntry->name->len);
   contextKey.append("::__invoke");
   CallContext *callContext  = nullptr;
   auto targetContext = sg_contextPtrs.find(contextKey);
   if (targetContext != sg_contextPtrs.end()) {
      callContext = targetContext->second.get();
   } else {
      std::shared_ptr<CallContext> targetContext(new CallContext);
      callContext = targetContext.get();
      zend_internal_function *func = &callContext->m_func;
      func->type = ZEND_INTERNAL_FUNCTION;
      func->module = nullptr;
      func->handler = &AbstractClassPrivate::magicInvokeForwarder;
      func->arg_info = nullptr;
      func->num_args = 0;
      func->required_num_args = 0;
      func->scope = *entry;
      func->fn_flags = ZEND_ACC_CALL_VIA_HANDLER;
      func->function_name = nullptr;
      callContext->m_selfPtr = retrieve_acp_ptr_from_cls_entry(Z_OBJCE_P(object));
      sg_contextPtrs[contextKey] = std::move(targetContext);
   }
   *retFunc = reinterpret_cast<zend_function *>(callContext);
   *objectPtr = Z_OBJ_P(object);
   return VMAPI_SUCCESS;
}

void AbstractClassPrivate::magicCallForwarder(INTERNAL_FUNCTION_PARAMETERS)
{
   CallContext *callContext = reinterpret_cast<CallContext *>(execute_data->func);
   ZEND_ASSERT(callContext);
   bool isStatic = false;
   AbstractClass *meta = callContext->m_selfPtr->m_apiPtr;
   zend_class_entry *defClassEntry = callContext->m_selfPtr->m_classEntry;
   zend_internal_function *func = &callContext->m_func;
   zend_string *funcName = func->function_name;
   std::string contextKey(defClassEntry->name->val, defClassEntry->name->len);
   contextKey.append("::");
   contextKey.append(funcName->val, funcName->len);
   if (!func->scope) {
      contextKey.append("static");
   }
   const char *name = ZSTR_VAL(funcName);
   try {
      Parameters params(getThis(), ZEND_NUM_ARGS());
      StdClass *nativeObject = params.getObject();
      if (nativeObject) {
         zval temp = meta->callMagicCall(nativeObject, name, params).detach(false);
         ZVAL_COPY(return_value, &temp);
      } else {
         isStatic = true;
         zval temp = meta->callMagicStaticCall(name, params).detach(false);
         ZVAL_COPY(return_value, &temp);
      }
   } catch (const NotImplemented &) {
      if (isStatic) {
         zend_error(E_ERROR, "Undefined static method %s::%s", meta->getClassName().c_str(), name);
      } else {
         zend_error(E_ERROR, "Undefined instance method %s of %s", name, meta->getClassName().c_str());
      }
   } catch (Exception &exception) {
      process_exception(exception);
   }
}

void AbstractClassPrivate::magicInvokeForwarder(INTERNAL_FUNCTION_PARAMETERS)
{
   CallContext *callContext = reinterpret_cast<CallContext *>(execute_data->func);
   ZEND_ASSERT(callContext);
   AbstractClass *meta = callContext->m_selfPtr->m_apiPtr;
   zend_class_entry *defClassEntry = callContext->m_selfPtr->m_classEntry;
   assert(defClassEntry);
   std::string contextKey(defClassEntry->name->val, defClassEntry->name->len);
   contextKey.append("::__invoke");
   try {
      Parameters params(getThis(), ZEND_NUM_ARGS());
      StdClass *nativeObject = params.getObject();
      zval temp = meta->callMagicInvoke(nativeObject, params).detach(false);
      ZVAL_COPY(return_value, &temp);
   } catch (const NotImplemented &) {
      zend_error(E_ERROR, "Function name must be a string");
   } catch (Exception &exception) {
      process_exception(exception);
   }
}

///
/// TODO Review
///
int AbstractClassPrivate::cast(zval *object, zval *retValue, int type)
{
   ObjectBinder *objectBinder = ObjectBinder::retrieveSelfPtr(object);
   AbstractClassPrivate *selfPtr = retrieve_acp_ptr_from_cls_entry(Z_OBJCE_P(object));
   AbstractClass *meta = selfPtr->m_apiPtr;
   StdClass *nativeObject = objectBinder->getNativeObject();
   try {
      zval temp;
      switch (static_cast<Type>(type)) {
      case Type::Numeric:
         temp = meta->castToInteger(nativeObject).detach(false);
         break;
      case Type::Double:
         temp = meta->castToDouble(nativeObject).detach(false);
         break;
      case Type::Boolean:
         temp = meta->castToBool(nativeObject).detach(false);
         break;
      case Type::String:
         temp = meta->castToString(nativeObject).detach(false);
         break;
      default:
         throw NotImplemented();
         break;
      }
      ZVAL_COPY(retValue, &temp);
      return VMAPI_SUCCESS;
   } catch (const NotImplemented &) {
      if (!std_object_handlers.cast_object) {
         return VMAPI_FAILURE;
      }
      return std_object_handlers.cast_object(object, retValue, type);
   } catch (Exception &exception) {
      process_exception(exception);
      return VMAPI_FAILURE;
   }
}

int AbstractClassPrivate::compare(zval *left, zval *right)
{
   try {
      zend_class_entry *entry = Z_OBJCE_P(left);
      if (entry != Z_OBJCE_P(right)) {
         throw NotImplemented();
      }
      AbstractClassPrivate *selfPtr = retrieve_acp_ptr_from_cls_entry(entry);
      AbstractClass *meta = selfPtr->m_apiPtr;
      StdClass *leftNativeObject = ObjectBinder::retrieveSelfPtr(left)->getNativeObject();
      StdClass *rightNativeObject = ObjectBinder::retrieveSelfPtr(right)->getNativeObject();
      return meta->callCompare(leftNativeObject, rightNativeObject);
   } catch (const NotImplemented &exception) {
      if (!std_object_handlers.compare_objects) {
         return 1;
      }
      return std_object_handlers.compare_objects(left, right);
   } catch (Exception &exception) {
      // a Exception was thrown by the extension __compare function,
      // pass this on to user space
      process_exception(exception);
      return 1;
   }
}

void AbstractClassPrivate::destructObject(zend_object *object)
{
   ObjectBinder *binder = ObjectBinder::retrieveSelfPtr(object);
   AbstractClassPrivate *selfPtr = retrieve_acp_ptr_from_cls_entry(object->ce);
   try {
      StdClass *nativeObject = binder->getNativeObject();
      if (nativeObject) {
         selfPtr->m_apiPtr->callDestruct(nativeObject);
      }
   } catch (const NotImplemented &) {
      zend_objects_destroy_object(object);
   } catch (Exception &exception) {
      // a regular Exception was thrown by the extension, pass it on
      // to PHP user space
      process_exception(exception);
   }
}

void AbstractClassPrivate::freeObject(zend_object *object)
{
   ObjectBinder *binder = ObjectBinder::retrieveSelfPtr(object);
   binder->destroy();
}

zval *AbstractClassPrivate::toZval(Variant &&value, int type, zval *rv)
{
   /// TODO review here
   zval result;
   if (type == 0 || value.getRefCount() <= 1) {
      result = value.detach(true);
   } else {
      // editable zval return a reference to it
      zval orig = value.detach(false);
      result = Variant(&orig, true).detach(true);
   }
   ZVAL_COPY_VALUE(rv, &result);
   return rv;
}
} // internal


AbstractClass::AbstractClass(StringRef className, ClassType type)
   : m_implPtr(std::make_shared<AbstractClassPrivate>(className, type))
{
}

AbstractClass::AbstractClass(const AbstractClass &other)
   : m_implPtr(other.m_implPtr)
{}

AbstractClass::AbstractClass(AbstractClass &&other) noexcept
   : m_implPtr(std::move(other.m_implPtr))
{}

AbstractClass &AbstractClass::operator=(const AbstractClass &other)
{
   if (this != &other) {
      m_implPtr = other.m_implPtr;
   }
   return *this;
}

AbstractClass::~AbstractClass()
{}

std::string AbstractClass::getClassName() const
{
   return m_implPtr->m_name;
}

size_t AbstractClass::getPropertyCount() const
{
   return m_implPtr->m_properties.size();
}

size_t AbstractClass::getInterfaceCount() const
{
   return m_implPtr->m_interfaces.size();
}

size_t AbstractClass::getMethodCount() const
{
   return m_implPtr->m_methods.size();
}

size_t AbstractClass::getConstantCount() const
{
   size_t ret = 0;
   for (std::shared_ptr<AbstractMember> member : m_implPtr->m_members) {
      if (member->isConstant()) {
         ++ret;
      }
   }
   return ret;
}

zend_class_entry *AbstractClass::buildClassEntry(StringRef ns, int moduleNumber) noexcept
{
   return initialize(ns, moduleNumber);
}

AbstractClass &AbstractClass::operator=(AbstractClass &&other) noexcept
{
   assert(this != &other);
   m_implPtr = std::move(other.m_implPtr);
   return *this;
}

void AbstractClass::registerInterface(const Interface &interface)
{
   VMAPI_D(AbstractClass);
   implPtr->m_interfaces.push_back(std::make_shared<Interface>(interface));
}

void AbstractClass::registerInterface(Interface &&interface)
{
   VMAPI_D(AbstractClass);
   implPtr->m_interfaces.push_back(std::make_shared<Interface>(std::move(interface)));
}

void AbstractClass::registerBaseClass(const AbstractClass &base)
{
   VMAPI_D(AbstractClass);
   implPtr->m_parent = std::make_shared<AbstractClass>(base);
}

void AbstractClass::registerBaseClass(AbstractClass &&base)
{
   VMAPI_D(AbstractClass);
   implPtr->m_parent = std::make_shared<AbstractClass>(std::move(base));
}

void AbstractClass::registerProperty(StringRef name, std::nullptr_t, Modifier flags)
{
   VMAPI_D(AbstractClass);
   implPtr->m_members.push_back(std::make_shared<NullMember>(name, flags & Modifier::PropertyModifiers));
}

void AbstractClass::registerProperty(StringRef name, int16_t value, Modifier flags)
{
   VMAPI_D(AbstractClass);
   implPtr->m_members.push_back(std::make_shared<NumericMember>(name, value,
                                                                flags & Modifier::PropertyModifiers));
}

void AbstractClass::registerProperty(StringRef name, int32_t value, Modifier flags)
{
   VMAPI_D(AbstractClass);
   implPtr->m_members.push_back(std::make_shared<NumericMember>(name, value,
                                                                flags & Modifier::PropertyModifiers));
}

void AbstractClass::registerProperty(StringRef name, int64_t value, Modifier flags)
{
   VMAPI_D(AbstractClass);
   implPtr->m_members.push_back(std::make_shared<NumericMember>(name, value,
                                                                flags & Modifier::PropertyModifiers));
}

void AbstractClass::registerProperty(StringRef name, const std::string &value, Modifier flags)
{
   VMAPI_D(AbstractClass);
   implPtr->m_members.push_back(std::make_shared<StringMember>(name, value,
                                                               flags & Modifier::PropertyModifiers));
}

void AbstractClass::registerProperty(StringRef name, const char *value, Modifier flags)
{
   VMAPI_D(AbstractClass);
   implPtr->m_members.push_back(std::make_shared<StringMember>(name, StringRef(value, std::strlen(value)),
                                                               flags & Modifier::PropertyModifiers));
}

void AbstractClass::registerProperty(StringRef name, bool value, Modifier flags)
{
   VMAPI_D(AbstractClass);
   implPtr->m_members.push_back(std::make_shared<BooleanMember>(name, value,
                                                                flags & Modifier::PropertyModifiers));
}

void AbstractClass::registerProperty(StringRef name, double value, Modifier flags)
{
   VMAPI_D(AbstractClass);
   implPtr->m_members.push_back(std::make_shared<FloatMember>(name, value,
                                                              flags & Modifier::PropertyModifiers));
}

void AbstractClass::registerProperty(StringRef name, const GetterMethodCallable0 &getter)
{
   VMAPI_D(AbstractClass);
   implPtr->m_properties[name] = std::make_shared<Property>(getter);
}

void AbstractClass::registerProperty(StringRef name, const GetterMethodCallable1 &getter)
{
   VMAPI_D(AbstractClass);
   implPtr->m_properties[name] = std::make_shared<Property>(getter);
}

void AbstractClass::registerProperty(StringRef name, const GetterMethodCallable0 &getter,
                                     const SetterMethodCallable0 &setter)
{
   VMAPI_D(AbstractClass);
   implPtr->m_properties[name] = std::make_shared<Property>(getter, setter);
}

void AbstractClass::registerProperty(StringRef name, const GetterMethodCallable0 &getter,
                                     const SetterMethodCallable1 &setter)
{
   VMAPI_D(AbstractClass);
   implPtr->m_properties[name] = std::make_shared<Property>(getter, setter);
}

void AbstractClass::registerProperty(StringRef name, const GetterMethodCallable1 &getter,
                                     const SetterMethodCallable0 &setter)
{
   VMAPI_D(AbstractClass);
   implPtr->m_properties[name] = std::make_shared<Property>(getter, setter);
}

void AbstractClass::registerProperty(StringRef name, const GetterMethodCallable1 &getter,
                                     const SetterMethodCallable1 &setter)
{
   VMAPI_D(AbstractClass);
   implPtr->m_properties[name] = std::make_shared<Property>(getter, setter);
}

void AbstractClass::registerConstant(const Constant &constant)
{
   const zend_constant &zendConst = constant.getZendConstant();
   const std::string &name = constant.getName();
   switch (Z_TYPE(zendConst.value)) {
   case IS_NULL:
      registerProperty(name.c_str(), nullptr, Modifier::Const);
      break;
   case IS_LONG:
      registerProperty(name.c_str(), static_cast<int64_t>(Z_LVAL(zendConst.value)), Modifier::Const);
      break;
   case IS_DOUBLE:
      registerProperty(name.c_str(), Z_DVAL(zendConst.value), Modifier::Const);
      break;
   case IS_TRUE:
      registerProperty(name.c_str(), true, Modifier::Const);
      break;
   case IS_FALSE:
      registerProperty(name.c_str(), false, Modifier::Const);
      break;
   case IS_STRING:
      registerProperty(name.c_str(), std::string(Z_STRVAL(zendConst.value), Z_STRLEN(zendConst.value)), Modifier::Const);
      break;
   default:
      // this should not happend
      // but we workaround this
      // shadow copy
      zval copy;
      ZVAL_DUP(&copy, &zendConst.value);
      convert_to_string(&copy);
      registerProperty(name.c_str(), std::string(Z_STRVAL(copy), Z_STRLEN(copy)), Modifier::Const);
      break;
   }
}

void AbstractClass::registerMethod(StringRef name, ZendCallable callable,
                                   Modifier flags, const Arguments &args)
{
   m_implPtr->m_methods.push_back(std::make_shared<Method>(name, callable, (flags & Modifier::MethodModifiers), args));
}

// abstract
void AbstractClass::registerMethod(StringRef name, Modifier flags, const Arguments &args)
{
   m_implPtr->m_methods.push_back(std::make_shared<Method>(name, (flags & Modifier::MethodModifiers) | Modifier::Abstract, args));
}

StdClass *AbstractClass::construct() const
{
   return nullptr;
}

StdClass *AbstractClass::clone(StdClass *orig) const
{
   return nullptr;
}

int AbstractClass::callCompare(StdClass *left, StdClass *right) const
{
   return 1;
}

void AbstractClass::callClone(StdClass *nativeObject) const
{}

void AbstractClass::callDestruct(StdClass *nativeObject) const
{}

Variant AbstractClass::callMagicCall(StdClass *nativeObject, StringRef name, Parameters &params) const
{
   return nullptr;
}

Variant AbstractClass::callMagicStaticCall(StringRef name, Parameters &params) const
{
   return nullptr;
}

Variant AbstractClass::callMagicInvoke(StdClass *nativeObject, Parameters &params) const
{
   return nullptr;
}

ArrayVariant AbstractClass::callDebugInfo(StdClass *nativeObject) const
{
   return nullptr;
}

Variant AbstractClass::callGet(StdClass *nativeObject, const std::string &name) const
{
   return nullptr;
}

void AbstractClass::callSet(StdClass *nativeObject, const std::string &name,
                            const Variant &value) const
{}

bool AbstractClass::callIsset(StdClass *nativeObject, const std::string &name) const
{
   return false;
}

void AbstractClass::callUnset(StdClass *nativeObject, const std::string &name) const
{}

Variant AbstractClass::castToString(StdClass *nativeObject) const
{
   return StringVariant();
}

Variant AbstractClass::castToInteger(StdClass *nativeObject) const
{
   return NumericVariant();
}

Variant AbstractClass::castToDouble(StdClass *nativeObject) const
{
   return DoubleVariant();
}

Variant AbstractClass::castToBool(StdClass *nativeObject) const
{
   return BooleanVariant();
}

bool AbstractClass::clonable() const
{
   return false;
}

bool AbstractClass::serializable() const
{
   return false;
}

bool AbstractClass::traversable() const
{
   return false;
}

zend_class_entry *AbstractClass::initialize(const std::string &prefix, int moduleNumber)
{
   return getImplPtr()->initialize(this, prefix, moduleNumber);
}

zend_class_entry *AbstractClass::initialize(int moduleNumber)
{
   return initialize("", moduleNumber);
}

void AbstractClass::notImplemented()
{
   throw NotImplemented();
}

} // vmapi
} // polar
