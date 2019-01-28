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

#include "polarphp/vm/IteratorBridge.h"
#include "polarphp/vm/protocol/AbstractIterator.h"

namespace polar {
namespace vmapi {

IteratorBridge::IteratorBridge(zval *object, AbstractIterator *iterator)
   : m_userspaceIterator(iterator)
{
   zend_iterator_init(&m_iterator);
   ZVAL_COPY(&m_iterator.data, object);
   m_iterator.funcs = getIteratorFuncs();
}

IteratorBridge::~IteratorBridge()
{
   invalidate();
   zval_ptr_dtor(&m_iterator.data);
}

zend_object_iterator *IteratorBridge::getZendIterator()
{
   return &m_iterator;
}

zend_object_iterator_funcs *IteratorBridge::getIteratorFuncs()
{
   static zend_object_iterator_funcs funcs;
   static bool initialized = false;
   if (initialized) {
      return &funcs;
   }
   funcs.dtor = &IteratorBridge::destructor;
   funcs.valid = &IteratorBridge::valid;
   funcs.get_current_data = &IteratorBridge::current;
   funcs.get_current_key = &IteratorBridge::key;
   funcs.move_forward = &IteratorBridge::next;
   funcs.rewind = &IteratorBridge::rewind;
   funcs.invalidate_current = &IteratorBridge::invalidate;
   initialized = true;
   return &funcs;
}

bool IteratorBridge::valid()
{
   return m_userspaceIterator->valid();
}

Variant &IteratorBridge::current()
{
   return m_current = m_userspaceIterator->current();
}

Variant IteratorBridge::key()
{
   return m_userspaceIterator->key();
}

void IteratorBridge::next()
{
   return m_userspaceIterator->next();
}

void IteratorBridge::rewind()
{
   return m_userspaceIterator->rewind();
}

void IteratorBridge::invalidate()
{
   m_current.invalidate();
}

IteratorBridge *IteratorBridge::getSelfPtr(zend_object_iterator *iterator)
{
   return reinterpret_cast<IteratorBridge *>(iterator);
}

void IteratorBridge::destructor(zend_object_iterator *iterator)
{
   getSelfPtr(iterator)->~IteratorBridge();
}

int IteratorBridge::valid(zend_object_iterator *iterator)
{
   return getSelfPtr(iterator)->valid() ? VMAPI_SUCCESS : VMAPI_FAILURE;
}

zval *IteratorBridge::current(zend_object_iterator *iterator)
{
   return getSelfPtr(iterator)->current().getZvalPtr();
}

void IteratorBridge::key(zend_object_iterator *iterator, zval *data)
{
   Variant retValue(getSelfPtr(iterator)->key());
   zval val = retValue.detach(true);
   ZVAL_ZVAL(data, &val, 1, 1);
}

void IteratorBridge::next(zend_object_iterator *iterator)
{
   getSelfPtr(iterator)->next();
}

void IteratorBridge::rewind(zend_object_iterator *iterator)
{
   getSelfPtr(iterator)->rewind();
}

void IteratorBridge::invalidate(zend_object_iterator *iterator)
{
   getSelfPtr(iterator)->invalidate();
}

} // vmapi
} // polar
