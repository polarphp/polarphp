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

#ifndef POLARPHP_VMAPI_LANG_TYPE_H
#define POLARPHP_VMAPI_LANG_TYPE_H

#include "polarphp/vm/internal/DepsZendVmHeaders.h"
#include "polarphp/global/CompilerDetection.h"

namespace polar {
namespace vmapi {

enum class Type : unsigned char
{
   Undefined   = IS_UNDEF,  // Variable is not set
   Null        = IS_NULL,  // Null will allow any type
   False       = IS_FALSE,  // Boolean false
   True        = IS_TRUE,  // Boolean true
   Long        = IS_LONG,  // Integer type
   Numeric     = IS_LONG,  // alias for Type::Long
   Float       = IS_DOUBLE, // alias for Type::Double
   Double      = IS_DOUBLE,  // Floating point type
   String      = IS_STRING,  // A string obviously
   Array       = IS_ARRAY,  // An array of things
   Object      = IS_OBJECT,  // An object
   Resource    = IS_RESOURCE,  // A resource
   Reference   = IS_REFERENCE, // Reference to another value (can be any type!)
   ConstantAST = IS_CONSTANT_AST, // I think an Abstract Syntax tree, not quite sure
   // "fake types", not quite sure what that means
   Boolean     = _IS_BOOL, // You will never get this back as a type
   Callable    = IS_CALLABLE, // I don't know why this is a "fake" type
   // internal types
   Indirect    = IS_INDIRECT,
   Ptr         = IS_PTR,
   Error       = _IS_ERROR
};


enum class ClassType : unsigned int
{
   Regular   = 0x00,
   Abstract  = ZEND_ACC_EXPLICIT_ABSTRACT_CLASS, //0x20,
   Final     = ZEND_ACC_FINAL, //0x04,
   Interface = ZEND_ACC_INTERFACE, //0x40
   Trait     = ZEND_ACC_TRAIT // 0x80
};

enum class  Modifier : unsigned long
{
   None              = 0,
   Static            = ZEND_ACC_STATIC,
   Abstract          = ZEND_ACC_ABSTRACT,
   Final             = ZEND_ACC_FINAL,
   Public            = ZEND_ACC_PUBLIC,
   Protected         = ZEND_ACC_PROTECTED,
   Deprecated        = ZEND_ACC_DEPRECATED,
   Private           = ZEND_ACC_PRIVATE,
   Const             = 0x10000, // we define this number ourself
   MethodModifiers   = Final | Public | Protected | Private | Static,
   PropertyModifiers = Final | Public | Protected | Private | Const | Static,
   // special method type
   Constructor       = ZEND_ACC_CTOR,
   Destructor        = ZEND_ACC_DTOR,
};

POLAR_DECL_EXPORT Modifier operator ~(Modifier modifier);
POLAR_DECL_EXPORT Modifier operator |(Modifier left, Modifier right);
POLAR_DECL_EXPORT Modifier operator &(Modifier left, Modifier right);
POLAR_DECL_EXPORT Modifier &operator |=(Modifier &left, Modifier right);
POLAR_DECL_EXPORT Modifier &operator &=(Modifier &left, Modifier right);
POLAR_DECL_EXPORT bool operator ==(const Modifier left, unsigned long value);
POLAR_DECL_EXPORT bool operator ==(unsigned long value, const Modifier &right);
POLAR_DECL_EXPORT bool operator ==(Modifier left, Modifier right);

using HashTableDataDeleter = dtor_func_t;

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_TYPE_H
