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

#ifndef POLARPHP_VMAPI_TYPEDEFS_H
#define POLARPHP_VMAPI_TYPEDEFS_H

#include "polarphp/vm/internal/DepsZendVmHeaders.h"
#include <functional>

struct _zend_execute_data;
struct _zval_struct;

namespace polar {
namespace vmapi {

/// forward declare class
class Parameters;
class StdClass;
class Variant;

using HANDLE = void *;
using Callback = std::function<void()>;
using ZendCallable = void(*)(struct _zend_execute_data *executeData, struct _zval_struct *returnValue);

// native function signature type alias
using NativeCallable0 = void (*)();
using NativeCallable1 = void (*)(Parameters &);
using NativeCallable2 = Variant (*)();
using NativeCallable3 = Variant (*)(Parameters &);

// class method signature type alias
using MethodCallable0  = void (StdClass::*)();
using MethodCallable1  = void (StdClass::*)(Parameters &);
using MethodCallable2  = Variant (StdClass::*)();
using MethodCallable3  = Variant (StdClass::*)(Parameters &);
using MethodCallable4  = void (StdClass::*)() const;
using MethodCallable5  = void (StdClass::*)(Parameters &) const;
using MethodCallable6  = Variant (StdClass::*)() const;
using MethodCallable7  = Variant (StdClass::*)(Parameters &) const;
using MethodCallable8  = NativeCallable0;
using MethodCallable9  = NativeCallable1;
using MethodCallable10 = NativeCallable2;
using MethodCallable11 = NativeCallable3;

// class getter and setter signature type alias
using GetterMethodCallable0 = Variant (StdClass::*)();
using GetterMethodCallable1 = Variant (StdClass::*)() const;
using SetterMethodCallable0 = void (StdClass::*)(const Variant &value);
using SetterMethodCallable1 = void (StdClass::*)(const Variant &value) const;

enum class Error : int
{
   Error            = E_ERROR,
   Warning          = E_WARNING,
   Parse            = E_PARSE,
   Notice           = E_NOTICE,
   CoreError        = E_CORE_ERROR,
   CoreWarning      = E_CORE_WARNING,
   CompileError     = E_COMPILE_ERROR,
   CompileWarning   = E_COMPILE_WARNING,
   UserError        = E_USER_ERROR,
   UserNotice       = E_USER_NOTICE,
   Strict           = E_STRICT,
   RecoverableError = E_RECOVERABLE_ERROR,
   Deprecated       = E_DEPRECATED,
   UserDeprecated   = E_USER_DEPRECATED,

   Core             = (E_CORE_ERROR | E_CORE_WARNING),
   All              = (E_ERROR | E_WARNING | E_PARSE | E_NOTICE | E_CORE_ERROR | E_CORE_WARNING | E_COMPILE_ERROR |
   E_COMPILE_WARNING | E_USER_ERROR | E_USER_NOTICE | E_STRICT | E_RECOVERABLE_ERROR |
   E_DEPRECATED | E_USER_DEPRECATED)
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_TYPEDEFS_H
