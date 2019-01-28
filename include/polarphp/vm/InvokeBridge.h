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

#ifndef POLARPHP_VMAPI_INVOKE_BRIDGE_H
#define POLARPHP_VMAPI_INVOKE_BRIDGE_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/utils/Exception.h"
#include "polarphp/vm/utils/OrigException.h"
#include "polarphp/vm/lang/Parameter.h"
#include "polarphp/vm/lang/Argument.h"
#include "polarphp/vm/ds/Variant.h"
#include "polarphp/vm/InvokeBridge.h"
#include "polarphp/vm/ObjectBinder.h"
#include "polarphp/vm/utils/Funcs.h"
#include "polarphp/vm/utils/CallableTraits.h"
#include "polarphp/vm/utils/ZendVMInvokerTypeTrait.h"

#include <tuple>
#include <functional>

struct _zend_execute_data;
struct _zval_struct;

namespace polar {
namespace vmapi {

/// forward declare class
class Variant;
class StdClass;

namespace
{

POLAR_DECL_UNUSED void yield(_zval_struct *return_value, Variant &&value)
{
   RETVAL_ZVAL(static_cast<zval *>(value), 1, 0);
}

POLAR_DECL_UNUSED void yield(_zval_struct *return_value, std::nullptr_t value)
{
   RETVAL_NULL();
}

POLAR_DECL_UNUSED StdClass *instance(zend_execute_data *execute_data)
{
   return ObjectBinder::retrieveSelfPtr(getThis())->getNativeObject();
}

bool check_invoke_arguments(_zend_execute_data *execute_data, _zval_struct *return_value, size_t funcDefinedArgNumber)
{
   uint32_t required = execute_data->func->common.required_num_args;
   uint32_t argNumer = execute_data->func->common.num_args;
   bool hasVariadic = execute_data->func->common.fn_flags & ZEND_ACC_VARIADIC;
   uint32_t provided = ZEND_NUM_ARGS();
   const char *name = get_active_function_name();
   if (hasVariadic) {
      ++argNumer;
   }
   if (funcDefinedArgNumber > argNumer) {
      vmapi::warning() << name << " native cpp callable definition have " << funcDefinedArgNumber << " parameter(s), "
                       << "but register meta info given " << argNumer << " parameter(s)." << std::flush;
      RETVAL_NULL();
      return false;
   }
   // we just check arguments number
   if (provided >= required) {
      return true;
   }
   vmapi::warning() << name << "() expects at least " << required << " parameter(s), "
                    << provided << " given" << std::flush;
   RETVAL_NULL();
   return false;
}

template <typename CallableType, CallableType callable,
          bool isMemberFunc, bool HasReturn>
class InvokeBridgePrivate
{
public:
   virtual ~InvokeBridgePrivate() = default;
private:
};

template <typename CallableType, CallableType callable>
class InvokeBridgePrivate <CallableType, callable, false, false>
{
public:
   static void invoke(zend_execute_data *execute_data, zval *return_value)
   {
      try {
         // no variable param
         constexpr size_t paramNumber = CallableInfoTrait<CallableType>::argNum;
         if (!check_invoke_arguments(execute_data, return_value, paramNumber)) {
            return;
         }
         if constexpr(paramNumber == 0) {
            std::invoke(callable);
         } else {
            const size_t argNumber = ZEND_NUM_ARGS();
            Parameters arguments(nullptr, argNumber);
            std::invoke(callable, std::ref(arguments));
         }
         yield(return_value, nullptr);
      } catch (Exception &exception) {
         process_exception(exception);
      }
   }
};

template <typename CallableType, CallableType callable>
class InvokeBridgePrivate <CallableType, callable, false, true>
{
public:
   static void invoke(zend_execute_data *execute_data, zval *return_value)
   {
      try {
         // no variable param
         constexpr size_t paramNumber = CallableInfoTrait<CallableType>::argNum;
         if (!check_invoke_arguments(execute_data, return_value, paramNumber)) {
            return;
         }
         if constexpr (paramNumber == 0) {
            yield(return_value, std::invoke(callable));
         } else {
            const size_t argNumber = ZEND_NUM_ARGS();
            Parameters arguments(nullptr, argNumber);
            yield(return_value, std::invoke(callable, std::ref(arguments)));
         }
      } catch (Exception &exception) {
         process_exception(exception);
      }
   }
};

template <typename CallableType, CallableType callable>
class InvokeBridgePrivate <CallableType, callable, true, false>
{
public:
   static void invoke(zend_execute_data *execute_data, zval *return_value)
   {
      try {
         // no variable param
         constexpr size_t paramNumber = CallableInfoTrait<CallableType>::argNum;
         if (!check_invoke_arguments(execute_data, return_value, paramNumber)) {
            return;
         }
         using ClassType = typename std::decay<typename member_pointer_traits<CallableType>::ClassType>::type;
         StdClass *nativeObject = ObjectBinder::retrieveSelfPtr(getThis())->getNativeObject();
         if constexpr(paramNumber == 0) {
            std::invoke(callable, static_cast<ClassType *>(nativeObject));
         } else {
            const size_t argNumber = ZEND_NUM_ARGS();
            Parameters arguments(getThis(), argNumber);
            // for class object
            std::apply(callable, std::make_tuple(static_cast<ClassType *>(nativeObject), std::ref(arguments)));
         }
         yield(return_value, nullptr);
      } catch (Exception &exception) {
         process_exception(exception);
      }
   }
};

template <typename CallableType, CallableType callable>
class InvokeBridgePrivate <CallableType, callable, true, true>
{
public:
   static void invoke(zend_execute_data *execute_data, zval *return_value)
   {
      try {
         // no variable param
         constexpr size_t paramNumber = CallableInfoTrait<CallableType>::argNum;
         if (!check_invoke_arguments(execute_data, return_value, paramNumber)) {
            return;
         }
         using ClassType = typename std::decay<typename member_pointer_traits<CallableType>::ClassType>::type;
         StdClass *nativeObject = ObjectBinder::retrieveSelfPtr(getThis())->getNativeObject();
         if constexpr(paramNumber == 0) {
            yield(return_value, std::invoke(callable, static_cast<ClassType *>(nativeObject)));
         } else {
            const size_t argNumber = ZEND_NUM_ARGS();
            Parameters arguments(getThis(), argNumber);
            // for class object
            auto tuple = std::make_tuple(static_cast<ClassType *>(nativeObject), std::ref(arguments));
            yield(return_value, std::apply(callable, tuple));
         }
      } catch (Exception &exception) {
         process_exception(exception);
      }
   }
};

} // anonymous namespace

// for normal function and static method
template <typename CallableType,
          typename std::decay<CallableType>::type callable,
          typename DecayCallableType = typename std::decay<CallableType>::type,
          typename std::enable_if<method_callable_prototype_checker<DecayCallableType>::value, DecayCallableType>::type * = nullptr>
class InvokeBridge : public InvokeBridgePrivate<DecayCallableType, callable,
      CallableInfoTrait<DecayCallableType>::isMemberCallable,
      CallableHasReturn<DecayCallableType>::value>
{};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_INVOKE_BRIDGE_H
