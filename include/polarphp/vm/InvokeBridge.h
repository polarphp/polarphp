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

#include <ostream>
#include <list>
#include <type_traits>
#include <tuple>

struct _zend_execute_data;
struct _zval_struct;

namespace polar {
namespace vmapi {

/// forward declare class
class Variant;
class Parameters;
class StdClass;

namespace
{

void yield(_zval_struct *return_value, const Variant &value)
{
   RETVAL_ZVAL(static_cast<zval *>(value), 1, 0);
}

void yield(_zval_struct *return_value, std::nullptr_t value)
{
   RETVAL_NULL();
}

StdClass *instance(zend_execute_data *execute_data)
{
   return ObjectBinder::retrieveSelfPtr(getThis())->getNativeObject();
}

bool check_invoke_arguments(_zend_execute_data *execute_data, _zval_struct *return_value, size_t funcDefinedArgNumber)
{
   uint32_t required = execute_data->func->common.required_num_args;
   uint32_t argNumer = execute_data->func->common.num_args;
   uint32_t provided = ZEND_NUM_ARGS();
   const char *name = get_active_function_name();
   if (funcDefinedArgNumber > argNumer) {
      vmapi::warning << name << " native cpp callable definition have " << funcDefinedArgNumber << " parameter(s), "
                    << "but register meta info given " << argNumer << " parameter(s)." << std::flush;
      RETVAL_NULL();
      return false;
   }
   // we just check arguments number
   if (provided >= required) {
      return true;
   }
   // TODO
   vmapi::warning << name << "() expects at least " << required << " parameter(s), "
                 << provided << " given" << std::flush;
   RETVAL_NULL();
   return false;
}

class InvokeParamGenerator
{
public:
   InvokeParamGenerator(zval *arguments)
      : m_arguments(arguments)
   {}
   template <typename ParamType>
   typename std::remove_reference<ParamType>::type generate(size_t index)
   {
      using ClassType = typename std::remove_reference<ParamType>::type;
      zval *arg = &m_arguments[index];
      if (!zval_type_is_valid(arg)) {
         ZVAL_NULL(arg);
      }
      if (Z_TYPE_P(arg) == IS_REFERENCE) {
         return ClassType(arg, true);
      }
      return ClassType(arg);
   }

private:
   zval *m_arguments;
};

} // anonymous namespace

template <typename CallableType, CallableType callable,
          bool isMemberFunc, bool HasReturn, bool HasVariableParam>
class InvokeBridgePrivate
{
public:
   virtual ~InvokeBridgePrivate() = default;
private:
};

template <typename CallableType, CallableType callable>
class InvokeBridgePrivate <CallableType, callable, false, false, false>
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
         const size_t argNumber = ZEND_NUM_ARGS();
         std::unique_ptr<zval[]> arguments(new zval[argNumber]);
         zend_get_parameters_array_ex(argNumber, arguments.get());
         InvokeParamGenerator generator(arguments.get());
         auto tuple = gen_tuple_with_type<paramNumber, CallableType>(generator);
         std::apply(callable, tuple);
         yield(return_value, nullptr);
      } catch (Exception &exception) {
         process_exception(exception);
      }
   }
};

template <typename CallableType, CallableType callable>
class InvokeBridgePrivate <CallableType, callable, false, false, true>
{
public:
   static void invoke(zend_execute_data *execute_data, zval *return_value)
   {
      try {
         // variadic params
         constexpr size_t paramNumber = CallableInfoTrait<CallableType>::argNum;
         if (!check_invoke_arguments(execute_data, return_value, paramNumber - 1)) {
            return;
         }
         const size_t argNumber = ZEND_NUM_ARGS();
         zval arguments[16];
         zend_get_parameters_array_ex(argNumber, arguments);
         // 15 arguments is enough ?
         auto tuple = gen_tuple<16>(
                  [&arguments, argNumber](size_t index){
            if (index == 0) {
               zval temp;
               ZVAL_LONG(&temp, static_cast<int32_t>(argNumber));
               return temp;
            } else if (index <= argNumber + 1){
               return arguments[index - 1];
            } else {
               zval temp;
               ZVAL_NULL(&temp);
               return temp;
            }
         });
         std::apply(callable, tuple);
         yield(return_value, nullptr);
      } catch (Exception &exception) {
         process_exception(exception);
      }
   }
};

template <typename CallableType, CallableType callable>
class InvokeBridgePrivate <CallableType, callable, false, true, false>
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
         const size_t argNumber = ZEND_NUM_ARGS();
         std::unique_ptr<zval[]> arguments(new zval[argNumber]);
         zend_get_parameters_array_ex(argNumber, arguments.get());
         InvokeParamGenerator generator(arguments.get());
         auto tuple = gen_tuple_with_type<paramNumber, CallableType>(generator);
         yield(return_value, std::apply(callable, tuple));
      } catch (Exception &exception) {
         process_exception(exception);
      }
   }
};

template <typename CallableType, CallableType callable>
class InvokeBridgePrivate <CallableType, callable, false, true, true>
{
public:
   static void invoke(zend_execute_data *execute_data, zval *return_value)
   {
      try {
         // variadic params
         constexpr size_t paramNumber = CallableInfoTrait<CallableType>::argNum;
         // for the first marker param
         if (!check_invoke_arguments(execute_data, return_value, paramNumber - 1)) {
            return;
         }
         const size_t argNumber = ZEND_NUM_ARGS();
         zval arguments[16];
         zend_get_parameters_array_ex(argNumber, arguments);
         // 15 arguments is enough ?
         auto tuple = gen_tuple<16>(
                  [&arguments, argNumber](size_t index){
            if (index == 0) {
               zval temp;
               ZVAL_LONG(&temp, static_cast<int32_t>(argNumber));
               return temp;
            } else if (index <= argNumber + 1){
               return arguments[index - 1];
            } else {
               zval temp;
               ZVAL_NULL(&temp);
               return temp;
            }
         });
         yield(return_value,  apply(callable, tuple));
      } catch (Exception &exception) {
         process_exception(exception);
      }
   }
};

template <typename CallableType, CallableType callable>
class InvokeBridgePrivate <CallableType, callable, true, false, false>
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
         const size_t argNumber = ZEND_NUM_ARGS();
         std::unique_ptr<zval[]> arguments(new zval[argNumber]);
         zend_get_parameters_array_ex(argNumber, arguments.get());
         // for class object
         InvokeParamGenerator generator(arguments.get());
         auto objectTuple = std::make_tuple(static_cast<ClassType *>(nativeObject));
         auto tuple = std::tuple_cat(objectTuple, gen_tuple_with_type<paramNumber, CallableType>(generator));
         std::apply(callable, tuple);
         yield(return_value, nullptr);
      } catch (Exception &exception) {
         process_exception(exception);
      }
   }
};

template <typename CallableType, CallableType callable>
class InvokeBridgePrivate <CallableType, callable, true, false, true>
{
public:
   static void invoke(zend_execute_data *execute_data, zval *return_value)
   {
      try {
         // variable params
         constexpr size_t paramNumber = CallableInfoTrait<CallableType>::argNum;
         if (!check_invoke_arguments(execute_data, return_value, paramNumber - 1)) {
            return;
         }
         using ClassType = typename std::decay<typename member_pointer_traits<CallableType>::ClassType>::type;
         StdClass *nativeObject = ObjectBinder::retrieveSelfPtr(getThis())->getNativeObject();
         const size_t argNumber = ZEND_NUM_ARGS();
         zval arguments[16];
         zend_get_parameters_array_ex(argNumber, arguments);
         // for class object
         auto objectTuple = std::make_tuple(static_cast<ClassType *>(nativeObject));
         // 15 arguments is enough ?
         auto tuple = std::tuple_cat(objectTuple, gen_tuple<16>(
                                        [&arguments, argNumber](size_t index){
            if (index == 0) {
               zval temp;
               ZVAL_LONG(&temp, static_cast<int32_t>(argNumber));
               return temp;
            } else if (index <= argNumber + 1){
               return arguments[index - 1];
            } else {
               zval temp;
               ZVAL_NULL(&temp);
               return temp;
            }
         }));
         apply(callable, tuple);
         yield(return_value, nullptr);
      } catch (Exception &exception) {
         process_exception(exception);
      }
   }
};

template <typename CallableType, CallableType callable>
class InvokeBridgePrivate <CallableType, callable, true, true, false>
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
         const size_t argNumber = ZEND_NUM_ARGS();
         std::unique_ptr<zval[]> arguments(new zval[argNumber]);
         zend_get_parameters_array_ex(argNumber, arguments.get());
         // for class object
         InvokeParamGenerator generator(arguments.get());
         auto objectTuple = std::make_tuple(static_cast<ClassType *>(nativeObject));
         auto tuple = std::tuple_cat(objectTuple, gen_tuple_with_type<paramNumber, CallableType>(generator));
         yield(return_value, apply(callable, tuple));
      } catch (Exception &exception) {
         process_exception(exception);
      }
   }
};

template <typename CallableType, CallableType callable>
class InvokeBridgePrivate <CallableType, callable, true, true, true>
{
public:
   static void invoke(zend_execute_data *execute_data, zval *return_value)
   {
      try {
         // variable params
         constexpr size_t paramNumber = CallableInfoTrait<CallableType>::argNum;
         if (!check_invoke_arguments(execute_data, return_value, paramNumber - 1)) {
            return;
         }
         using ClassType = typename std::decay<typename member_pointer_traits<CallableType>::ClassType>::type;
         StdClass *nativeObject = ObjectBinder::retrieveSelfPtr(getThis())->getNativeObject();
         const size_t argNumber = ZEND_NUM_ARGS();
         zval arguments[16];
         zend_get_parameters_array_ex(argNumber, arguments);
         // for class object
         auto objectTuple = std::make_tuple(static_cast<ClassType *>(nativeObject));
         // 15 arguments is enough ?
         auto tuple = std::tuple_cat(objectTuple, gen_tuple<16>(
                                        [&arguments, argNumber](size_t index){
            if (index == 0) {
               zval temp;
               ZVAL_LONG(&temp, static_cast<int32_t>(argNumber));
               return temp;
            } else if (index <= argNumber + 1){
               return arguments[index - 1];
            } else {
               zval temp;
               ZVAL_NULL(&temp);
               return temp;
            }
         }));
         yield(return_value, apply(callable, tuple));
      } catch (Exception &exception) {
         process_exception(exception);
      }
   }
};

// for normal function and static method
template <typename CallableType,
          typename std::decay<CallableType>::type callable,
          typename DecayCallableType = typename std::decay<CallableType>::type>
class InvokeBridge : public InvokeBridgePrivate<DecayCallableType, callable,
      CallableInfoTrait<DecayCallableType>::isMemberCallable,
      CallableHasReturn<DecayCallableType>::value,
      CallableInfoTrait<DecayCallableType>::hasVaridicParams>
{};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_INVOKE_BRIDGE_H
