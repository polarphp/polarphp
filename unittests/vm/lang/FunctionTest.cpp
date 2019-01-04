// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/28.

#include "gtest/gtest.h"
#include "polarphp/vm/lang/Function.h"

using polar::vmapi::Function;
using polar::vmapi::Arguments;
using polar::vmapi::Type;
using polar::vmapi::ValueArgument;
using polar::vmapi::RefArgument;
using polar::vmapi::VariadicArgument;

namespace {
void dummy_func(struct _zend_execute_data *executeData, struct _zval_struct *returnValue)
{}
} // anonymous namespace

TEST(FunctionTest, testConstructor)
{
   Function func("polarphp_version", dummy_func);
   zend_function_entry funcEntry = func.buildCallableEntry();
   ASSERT_STREQ(funcEntry.fname, "polarphp_version");
   ASSERT_EQ(funcEntry.handler, &dummy_func);
   ASSERT_EQ(funcEntry.num_args, 0);
   ASSERT_EQ(funcEntry.flags, 0);
   func.markDeprecated();
   funcEntry = func.buildCallableEntry();
   ASSERT_TRUE(funcEntry.flags & ZEND_ACC_DEPRECATED);
   /// test basic arg info
   const zend_internal_function_info *argInfo = reinterpret_cast<const zend_internal_function_info *>(funcEntry.arg_info);
   ASSERT_FALSE(argInfo->_is_variadic);
   ASSERT_EQ(argInfo->type, ZEND_TYPE_ENCODE(IS_UNDEF, 1));
   ASSERT_FALSE(argInfo->return_reference);
   ASSERT_EQ(argInfo->required_num_args, 0);
   /// self pointer info
   const zend_arg_info *selfInfo = reinterpret_cast<const zend_arg_info *>(funcEntry.arg_info + 1);
   ASSERT_EQ(selfInfo->name, nullptr);
}

TEST(FunctionTest, testArguments)
{
   Function func("some_func", dummy_func, {ValueArgument("name", Type::String, true),
                                           RefArgument("ret", Type::Long, false),
                                           VariadicArgument("extraArgs")});
   func.setReturnType(Type::Boolean);
   zend_function_entry funcEntry = func.buildCallableEntry();
   ASSERT_STREQ(funcEntry.fname, "some_func");
   ASSERT_EQ(funcEntry.handler, &dummy_func);
   ASSERT_EQ(funcEntry.flags, 0);
   ASSERT_EQ(funcEntry.num_args, 3);

   /// test basic arg info
   const zend_internal_function_info *argInfo = reinterpret_cast<const zend_internal_function_info *>(funcEntry.arg_info);
   ASSERT_FALSE(argInfo->_is_variadic);
   ASSERT_EQ(argInfo->type, ZEND_TYPE_ENCODE(_IS_BOOL, 1));
   ASSERT_FALSE(argInfo->return_reference);
   ASSERT_EQ(argInfo->required_num_args, 1);
   /// test arg
   const zend_internal_arg_info *arg = funcEntry.arg_info + 1;
   ASSERT_FALSE(arg->is_variadic);
   ASSERT_FALSE(arg->pass_by_reference);
   ASSERT_STREQ(arg->name, "name");
   ASSERT_EQ(arg->type, ZEND_TYPE_ENCODE(IS_STRING, 0));

   arg = funcEntry.arg_info + 2;
   ASSERT_FALSE(arg->is_variadic);
   ASSERT_TRUE(arg->pass_by_reference);
   ASSERT_STREQ(arg->name, "ret");
   ASSERT_EQ(arg->type, ZEND_TYPE_ENCODE(IS_LONG, 0));

   arg = funcEntry.arg_info + 3;
   ASSERT_TRUE(arg->is_variadic);
   ASSERT_FALSE(arg->pass_by_reference);
   ASSERT_STREQ(arg->name, "extraArgs");
   ASSERT_EQ(arg->type, 0);

   func.setReturnType("Person");
   funcEntry = func.buildCallableEntry();
   argInfo = reinterpret_cast<const zend_internal_function_info *>(funcEntry.arg_info);
   ASSERT_FALSE(argInfo->_is_variadic);
   ASSERT_STREQ(reinterpret_cast<char *>(argInfo->type & ~0x1), "Person");
}

TEST(FunctionTest, testFunctionReturnType)
{
   Function func("some_func", dummy_func, {ValueArgument("name", Type::String, true),
                                           RefArgument("ret", Type::Long, false),
                                           VariadicArgument("extraArgs")});
   func.setReturnType(Type::Boolean);
   zend_function_entry funcEntry = func.buildCallableEntry();
   const zend_internal_function_info *argInfo = reinterpret_cast<const zend_internal_function_info *>(funcEntry.arg_info);
   ASSERT_EQ(argInfo->type & ~0x1, _IS_BOOL);

   func.setReturnType(Type::String, false);
   funcEntry = func.buildCallableEntry();
   argInfo = reinterpret_cast<const zend_internal_function_info *>(funcEntry.arg_info);
   ASSERT_EQ(argInfo->type & ~0x0, IS_STRING);

   func.setReturnType("SomeClass", false);
   funcEntry = func.buildCallableEntry();
   argInfo = reinterpret_cast<const zend_internal_function_info *>(funcEntry.arg_info);
   ASSERT_STREQ(reinterpret_cast<char *>(argInfo->type & ~0x0), "SomeClass");

}
