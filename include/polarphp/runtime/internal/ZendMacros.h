// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/02/13.

#ifndef POLARPHP_RUNTIME_INTERNAL_ZEND_HASH_MACROS_H
#define POLARPHP_RUNTIME_INTERNAL_ZEND_HASH_MACROS_H

/// use cpp cast
///
#define POLAR_ZVAL_EMPTY_ARRAY(z) do {						\
      zval *__z = (z);								\
      Z_ARR_P(__z) = const_cast<zend_array*>(&zend_empty_array);	\
      Z_TYPE_INFO_P(__z) = IS_ARRAY; \
   } while (0)

#define POLAR_HASH_FOREACH_PTR_WITH_TYPE(ht, _type, _ptr) \
   ZEND_HASH_FOREACH(ht, 0); \
   _ptr = reinterpret_cast<_type>(Z_PTR_P(_z));

#define POLAR_HASH_FOREACH_STR_KEY_PTR_WITH_TYPE(ht, _key, _type, _ptr) \
   ZEND_HASH_FOREACH(ht, 0); \
   _key = _p->key; \
   _ptr = reinterpret_cast<_type>(Z_PTR_P(_z));

#define POLAR_HASH_FOREACH_END ZEND_HASH_FOREACH_END

#endif // POLARPHP_RUNTIME_INTERNAL_ZEND_HASH_MACROS_H
