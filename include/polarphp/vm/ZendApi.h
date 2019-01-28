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

#ifndef  POLARPHP_VM_ZENDAPI_H
#define  POLARPHP_VM_ZENDAPI_H

#include "polarphp/global/Global.h"
#include "polarphp/vm/lang/Type.h"
#include "polarphp/vm/TypeDefs.h"

#define VMAPI_DECLARE_PRIVATE POLAR_DECLARE_PRIVATE
#define VMAPI_DECLARE_PUBLIC  POLAR_DECLARE_PUBLIC

#define VMAPI_D POLAR_D
#define VMAPI_Q POLAR_Q
#define VMAPI_DECL_EXPORT POLAR_DECL_EXPORT

#define VMAPI_DECLARE_MODULE_GLOBALS(module_name)							\
   ts_rsrc_id module_name##_globals_id
#define VMAPI_EXTERN_MODULE_GLOBALS(module_name)								\
   extern ts_rsrc_id module_name##_globals_id
#define VMAPI_INIT_MODULE_GLOBALS(module_name, globals_ctor, globals_dtor)	\
   ts_allocate_id(&module_name##_globals_id, sizeof(zend_##module_name##_globals), (ts_allocate_ctor) globals_ctor, (ts_allocate_dtor) globals_dtor)
#define VMAPI_MODULE_GLOBALS_ACCESSOR(module_name, v) ZEND_TSRMG(module_name##_globals_id, zend_##module_name##_globals *, v)
#if ZEND_ENABLE_STATIC_TSRMLS_CACHE
#define VMAPI_MODULE_GLOBALS_BULK(module_name) TSRMG_BULK_STATIC(module_name##_globals_id, zend_##module_name##_globals *)
#else
#define VMAPI_MODULE_GLOBALS_BULK(module_name) TSRMG_BULK(module_name##_globals_id, zend_##module_name##_globals *)
#endif


namespace polar {
namespace vmapi {

VMAPI_DECL_EXPORT std::ostream &out();
VMAPI_DECL_EXPORT std::ostream &error();
VMAPI_DECL_EXPORT std::ostream &notice();
VMAPI_DECL_EXPORT std::ostream &warning();
VMAPI_DECL_EXPORT std::ostream &deprecated();

using VmApiVaridicItemType = zval;

VMAPI_DECL_EXPORT Modifier operator~(Modifier modifier);
VMAPI_DECL_EXPORT Modifier operator|(Modifier lhs, Modifier rhs);
VMAPI_DECL_EXPORT Modifier operator&(Modifier lhs, Modifier rhs);
VMAPI_DECL_EXPORT Modifier &operator|=(Modifier &lhs, Modifier rhs);
VMAPI_DECL_EXPORT Modifier &operator&=(Modifier &lhs, Modifier rhs);
VMAPI_DECL_EXPORT bool operator==(const Modifier lhs, unsigned long rhs);
VMAPI_DECL_EXPORT bool operator==(unsigned long lhs, const Modifier &rhs);
VMAPI_DECL_EXPORT bool operator==(Modifier lhs, Modifier rhs);

class Module;
bool register_module_to_zendvm(const Module &module);

} // vmapi
} // polar

#define vmapi_long  zend_long
#define vmapi_ulong zend_ulong

#define VMAPI_ASSERT   POLAR_ASSERT
#define VMAPI_ASSERT_X POLAR_ASSERT_X

#define VMAPI_SUCCESS SUCCESS
#define VMAPI_FAILURE FAILURE

#define BOOL2SUCCESS(b) ((b) ? VMAPI_SUCCESS : VMAPI_FAILURE)

#define POLAR_HASH_FOREACH(_ht, indirect) do { \
      ::HashTable *__ht = (_ht); \
      ::Bucket *_p = __ht->arData; \
      ::Bucket *_end = _p + __ht->nNumUsed; \
      for (; _p != _end; _p++) { \
         zval *_z = &_p->val; \
         if (indirect && Z_TYPE_P(_z) == IS_INDIRECT) { \
            _z = Z_INDIRECT_P(_z); \
         } \
         if (UNEXPECTED(Z_TYPE_P(_z) == IS_UNDEF)) continue;

#define POLAR_HASH_REVERSE_FOREACH(_ht, indirect) do { \
      ::HashTable *__ht = (_ht); \
      uint32_t _idx = __ht->nNumUsed; \
      ::Bucket *_p = __ht->arData + _idx; \
      zval *_z; \
      for (_idx = __ht->nNumUsed; _idx > 0; _idx--) { \
         _p--; \
         _z = &_p->val; \
         if (indirect && Z_TYPE_P(_z) == IS_INDIRECT) { \
            _z = Z_INDIRECT_P(_z); \
         } \
         if (UNEXPECTED(Z_TYPE_P(_z) == IS_UNDEF)) continue;

#define POLAR_HASH_FOREACH_BUCKET(ht, _bucket) \
   POLAR_HASH_FOREACH(ht, 0); \
   _bucket = _p;

#define POLAR_HASH_FOREACH_VAL(ht, _val) \
   POLAR_HASH_FOREACH(ht, 0); \
   _val = _z;

#define POLAR_HASH_FOREACH_VAL_IND(ht, _val) \
   POLAR_HASH_FOREACH(ht, 1); \
   _val = _z;

#define POLAR_HASH_FOREACH_PTR(ht, _ptr) \
   POLAR_HASH_FOREACH(ht, 0); \
   _ptr = Z_PTR_P(_z);

#define POLAR_HASH_FOREACH_NUM_KEY(ht, _h) \
   POLAR_HASH_FOREACH(ht, 0); \
   _h = _p->h;

#define POLAR_HASH_FOREACH_STR_KEY(ht, _key) \
   POLAR_HASH_FOREACH(ht, 0); \
   _key = _p->key;

#define POLAR_HASH_FOREACH_KEY(ht, _h, _key) \
   POLAR_HASH_FOREACH(ht, 0); \
   _h = _p->h; \
   _key = _p->key;

#define POLAR_HASH_FOREACH_NUM_KEY_VAL(ht, _h, _val) \
   POLAR_HASH_FOREACH(ht, 0); \
   _h = _p->h; \
   _val = _z;

#define POLAR_HASH_FOREACH_STR_KEY_VAL(ht, _key, _val) \
   POLAR_HASH_FOREACH(ht, 0); \
   _key = _p->key; \
   _val = _z;

#define POLAR_HASH_FOREACH_KEY_VAL(ht, _h, _key, _val) \
   POLAR_HASH_FOREACH(ht, 0); \
   _h = _p->h; \
   _key = _p->key; \
   _val = _z;

#define POLAR_HASH_FOREACH_STR_KEY_VAL_IND(ht, _key, _val) \
   POLAR_HASH_FOREACH(ht, 1); \
   _key = _p->key; \
   _val = _z;

#define POLAR_HASH_FOREACH_KEY_VAL_IND(ht, _h, _key, _val) \
   POLAR_HASH_FOREACH(ht, 1); \
   _h = _p->h; \
   _key = _p->key; \
   _val = _z;

#define POLAR_HASH_FOREACH_NUM_KEY_PTR(ht, _h, _ptr) \
   POLAR_HASH_FOREACH(ht, 0); \
   _h = _p->h; \
   _ptr = Z_PTR_P(_z);

#define POLAR_HASH_FOREACH_STR_KEY_PTR(ht, _key, _ptr) \
   POLAR_HASH_FOREACH(ht, 0); \
   _key = _p->key; \
   _ptr = Z_PTR_P(_z);

#define POLAR_HASH_FOREACH_KEY_PTR(ht, _h, _key, _ptr) \
   POLAR_HASH_FOREACH(ht, 0); \
   _h = _p->h; \
   _key = _p->key; \
   _ptr = Z_PTR_P(_z);

#define POLAR_HASH_REVERSE_FOREACH_BUCKET(ht, _bucket) \
   POLAR_HASH_REVERSE_FOREACH(ht, 0); \
   _bucket = _p;

#define POLAR_HASH_REVERSE_FOREACH_VAL(ht, _val) \
   POLAR_HASH_REVERSE_FOREACH(ht, 0); \
   _val = _z;

#define POLAR_HASH_REVERSE_FOREACH_PTR(ht, _ptr) \
   POLAR_HASH_REVERSE_FOREACH(ht, 0); \
   _ptr = Z_PTR_P(_z);

#define POLAR_HASH_REVERSE_FOREACH_VAL_IND(ht, _val) \
   POLAR_HASH_REVERSE_FOREACH(ht, 1); \
   _val = _z;

#define POLAR_HASH_REVERSE_FOREACH_STR_KEY_VAL(ht, _key, _val) \
   POLAR_HASH_REVERSE_FOREACH(ht, 0); \
   _key = _p->key; \
   _val = _z;

#define POLAR_HASH_REVERSE_FOREACH_KEY_VAL(ht, _h, _key, _val) \
   POLAR_HASH_REVERSE_FOREACH(ht, 0); \
   _h = _p->h; \
   _key = _p->key; \
   _val = _z;

#define POLAR_HASH_REVERSE_FOREACH_KEY_VAL_IND(ht, _h, _key, _val) \
   POLAR_HASH_REVERSE_FOREACH(ht, 1); \
   _h = _p->h; \
   _key = _p->key; \
   _val = _z;

#define POLAR_HASH_FOREACH_END ZEND_HASH_FOREACH_END
#define POLAR_HASH_FOREACH_END_DEL ZEND_HASH_FOREACH_END_DEL

// define some zend macro
#define vmapi_bailout() _zend_bailout(const_cast<char *>(static_cast<const char *>(__FILE__)), __LINE__)

#define VMAPI_API_VERSION 0x000001

#endif // POLARPHP_VM_ZENDAPI_H
