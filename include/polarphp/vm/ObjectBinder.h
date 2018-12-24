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

#ifndef POLARPHP_VMAPI_OBJECT_BINDER_H
#define POLARPHP_VMAPI_OBJECT_BINDER_H

#include "polarphp/vm/ZendApi.h"

namespace polar {
namespace vmapi {

/// forward declare class
class StdClass;

class ObjectBinder
{
public:
   ObjectBinder(zend_class_entry *entry, std::shared_ptr<StdClass> nativeObject,
                const zend_object_handlers *objectHandlers, uint32_t refCount);
   ~ObjectBinder();
   void destroy();
   zend_object *getZendObject() const;
   StdClass *getNativeObject() const;
   static ObjectBinder *retrieveSelfPtr(const zend_object *object);
   static ObjectBinder *retrieveSelfPtr(zval *object);
   static constexpr size_t calculateZendObjectOffset()
   {
      return offsetof(Container, m_zendObject);
   }

private:
   struct Container
   {
      ObjectBinder *m_self;
      zend_object m_zendObject;
   } *m_container;
   std::shared_ptr<StdClass> m_nativeObject;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_OBJECT_BINDER_H
