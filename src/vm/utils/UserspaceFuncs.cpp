// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/27.

#include "polarphp/vm/utils/UserspaceFuncs.h"
#include "polarphp/vm/ds/ArrayItemProxy.h"
#include "polarphp/vm/ds/internal/ArrayItemProxyPrivate.h"
#include "polarphp/vm/ds/Variant.h"

#include <string>

namespace polar {
namespace vmapi {

bool array_unset(ArrayItemProxy &&arrayItem)
{
   if (!arrayItem.isKeychianOk(false)) {
      return false;
   }
   // everything is ok
   // here we use the pointer to remove
   ArrayItemProxy::KeyType requestKey = arrayItem.m_implPtr->m_requestKey;
   zval *array = arrayItem.m_implPtr->m_array;
   int ret;
   if (requestKey.second) {
      std::string *key = requestKey.second.get();
      ret = zend_hash_str_del(Z_ARRVAL_P(array), key->c_str(), key->length());
   } else {
      ret = zend_hash_index_del(Z_ARRVAL_P(array), requestKey.first);
   }
   return ret == VMAPI_SUCCESS;
}

bool array_isset(ArrayItemProxy &&arrayItem)
{
   bool exist = false;
   if (arrayItem.m_implPtr->m_parent) {
      bool stop = false;
      arrayItem.checkExistRecursive(stop, arrayItem.m_implPtr->m_array,
                                    arrayItem.m_implPtr->m_apiPtr, true);
      exist = !stop;
   } else {
      exist = nullptr != arrayItem.retrieveZvalPtr(true);
   }
   arrayItem.m_implPtr->m_array = nullptr;
   arrayItem.m_implPtr->m_needCheckRequestItem = false;
   return exist;
}

bool empty(const Variant &value)
{
   return value.isNull() || !value.toBoolean();
}

} // vmapi
} // polar
