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

#ifndef POLARPHP_VMAPI_DS_INTERNAL_ARRAY_ITEM_PROXY_PRIVATE_H
#define POLARPHP_VMAPI_DS_INTERNAL_ARRAY_ITEM_PROXY_PRIVATE_H

#include "polarphp/vm/ZendApi.h"

namespace polar {
namespace vmapi {

class ArrayItemProxy;

namespace internal
{

using KeyType = ArrayItemProxy::KeyType;
class ArrayItemProxyPrivate
{
public:
   ArrayItemProxyPrivate(zval *array, const KeyType &requestKey,
                         ArrayItemProxy *apiPtr, ArrayItemProxy *parent)
      : m_requestKey(requestKey),
        m_array(array),
        m_parent(parent),
        m_apiPtr(apiPtr)
   {}

   ArrayItemProxyPrivate(zval *array, const std::string &key,
                         ArrayItemProxy *apiPtr, ArrayItemProxy *parent)
      : m_requestKey(-1, std::make_shared<std::string>(key)), // -1 is very big ulong
        m_array(array),
        m_parent(parent),
        m_apiPtr(apiPtr)
   {}

   ArrayItemProxyPrivate(zval *array, vmapi_ulong index,
                         ArrayItemProxy *apiPtr, ArrayItemProxy *parent)
      : m_requestKey(index, nullptr),
        m_array(array),
        m_parent(parent),
        m_apiPtr(apiPtr)
   {}

   ~ArrayItemProxyPrivate()
   {
      // here we check some things
      if (m_parent) {
         bool stop = false;
         m_apiPtr->checkExistRecursive(stop, m_array, m_apiPtr);
      } else if (m_needCheckRequestItem) {
         m_apiPtr->retrieveZvalPtr();
      }
   }
   VMAPI_DECLARE_PUBLIC(ArrayItemProxy);
   ArrayItemProxy::KeyType m_requestKey;
   zval *m_array;
   bool m_needCheckRequestItem = true;
   ArrayItemProxy *m_parent = nullptr;
   ArrayItemProxy *m_apiPtr;
};

} // internal
} // vmapi
} // polar

#endif // POLARPHP_VMAPI_DS_INTERNAL_ARRAY_ITEM_PROXY_PRIVATE_H
