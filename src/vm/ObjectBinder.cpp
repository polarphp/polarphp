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

#include "polarphp/vm/ObjectBinder.h"
#include "polarphp/vm/StdClass.h"
#include "polarphp/vm/internal/StdClassPrivate.h"

#include <cstring>

namespace polar {
namespace vmapi {

using internal::StdClassPrivate;

ObjectBinder::ObjectBinder(zend_class_entry *entry, std::shared_ptr<StdClass> nativeObject,
                           const zend_object_handlers *objectHandlers, uint32_t refCount)
   : m_nativeObject(nativeObject)
{
   m_container = static_cast<Container *>(ecalloc(1, sizeof(Container) + zend_object_properties_size(entry)));
   m_container->m_self = this;
   m_container->m_zendObject.ce = entry;
   zend_object_std_init(&m_container->m_zendObject, entry);
   object_properties_init(&m_container->m_zendObject, entry);
   m_container->m_zendObject.handlers = objectHandlers;
   if (refCount != 1) {
      GC_SET_REFCOUNT(&m_container->m_zendObject, refCount);
   }
   m_nativeObject->m_implPtr->m_zendObject = &m_container->m_zendObject;
}

zend_object *ObjectBinder::getZendObject() const
{
   return &m_container->m_zendObject;
}

ObjectBinder::~ObjectBinder()
{
   zend_object_std_dtor(&m_container->m_zendObject);
}

void ObjectBinder::destroy()
{
   delete this;
}

StdClass *ObjectBinder::getNativeObject() const
{
   return m_nativeObject.get();
}

ObjectBinder *ObjectBinder::retrieveSelfPtr(const zend_object *object)
{
   Container *container = reinterpret_cast<Container *>(const_cast<char *>(reinterpret_cast<const char *>(object) - calculateZendObjectOffset()));
   return container->m_self;
}

ObjectBinder *ObjectBinder::retrieveSelfPtr(zval *object)
{
   return retrieveSelfPtr(Z_OBJ_P(object));
}

} // vmapi
} // polar
