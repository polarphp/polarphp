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

#ifndef POLARPHP_VMAPI_ITERATOR_BRIDGE_H
#define POLARPHP_VMAPI_ITERATOR_BRIDGE_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/ds/Variant.h"

namespace polar {
namespace vmapi {

/// forward declare class
class AbstractIterator;

class IteratorBridge
{
public:
   IteratorBridge(zval *object, AbstractIterator *iterator);
   ~IteratorBridge();
   zend_object_iterator *getZendIterator();
   static zend_object_iterator_funcs *getIteratorFuncs();
private:
   bool valid();
   Variant &current();
   Variant key();
   void next();
   void rewind();
   void invalidate();
private:
   static IteratorBridge *getSelfPtr(zend_object_iterator *iterator);
   static void destructor(zend_object_iterator *iterator);
   static int valid(zend_object_iterator *iterator);
   static zval *current(zend_object_iterator *iterator);
   static void key(zend_object_iterator *iterator, zval *data);
   static void next(zend_object_iterator *iterator);
   static void rewind(zend_object_iterator *iterator);
   static void invalidate(zend_object_iterator *iterator);
private:
   zend_object_iterator m_iterator;
   AbstractIterator *m_userspaceIterator;
   Variant m_current;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_ITERATOR_BRIDGE_H
