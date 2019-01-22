// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/01/20.

#ifndef POLARPHP_RUNTIME_LANG_SUPPORT_SERIALIZE_FUNC_H
#define POLARPHP_RUNTIME_LANG_SUPPORT_SERIALIZE_FUNC_H

#include "polarphp/runtime/RtDefs.h"

namespace polar {
namespace runtime {

struct SerializeData
{
   ::HashTable ht;
   uint32_t n;
};

struct UnserializeData
{
   void *first;
   void *last;
   void *firstDtor;
   void *lastDtor;
   ::HashTable *allowedClasses;
};

PHP_FUNCTION(serialize);
PHP_FUNCTION(unserialize);

POLAR_DECL_EXPORT void var_serialize(smart_str *buf, zval *struc, SerializeData **data);
POLAR_DECL_EXPORT int var_unserialize(zval *rval, const unsigned char **p, const unsigned char *max, UnserializeData **varHash);
POLAR_DECL_EXPORT int var_unserialize_ref(zval *rval, const unsigned char **p, const unsigned char *max, UnserializeData **varHash);
POLAR_DECL_EXPORT int var_unserialize_intern(zval *rval, const unsigned char **p, const unsigned char *max, UnserializeData **varHash);

POLAR_DECL_EXPORT SerializeData *var_serialize_init(void);
POLAR_DECL_EXPORT void var_serialize_destroy(SerializeData *d);
POLAR_DECL_EXPORT UnserializeData *var_unserialize_init(void);
POLAR_DECL_EXPORT void var_unserialize_destroy(UnserializeData *d);
POLAR_DECL_EXPORT HashTable *var_unserialize_get_allowed_classes(UnserializeData *d);
POLAR_DECL_EXPORT void var_unserialize_set_allowed_classes(UnserializeData *d, HashTable *classes);

#define PHP_VAR_SERIALIZE_INIT(d) \
   (d) = var_serialize_init()

#define PHP_VAR_SERIALIZE_DESTROY(d) \
   var_serialize_destroy(d)

#define PHP_VAR_UNSERIALIZE_INIT(d) \
   (d) = var_unserialize_init()

#define PHP_VAR_UNSERIALIZE_DESTROY(d) \
   var_unserialize_destroy(d)

POLAR_DECL_EXPORT void var_replace(UnserializeData **varHash, zval *ozval, zval *nzval);
POLAR_DECL_EXPORT void var_push_dtor(UnserializeData **varHash, zval *val);
POLAR_DECL_EXPORT zval *var_tmp_var(UnserializeData **varHash);
POLAR_DECL_EXPORT void var_destroy(UnserializeData **varHash);

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_LANG_SUPPORT_SERIALIZE_FUNC_H
