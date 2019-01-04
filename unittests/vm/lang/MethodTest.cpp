// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/02/04.

#include "gtest/gtest.h"
#include "polarphp/vm/lang/Method.h"

using polar::vmapi::Method;
using polar::vmapi::Arguments;
using polar::vmapi::Type;
using polar::vmapi::ValueArgument;
using polar::vmapi::RefArgument;
using polar::vmapi::VariadicArgument;
using polar::vmapi::Modifier;

namespace {
void dummy_func(struct _zend_execute_data *executeData, struct _zval_struct *returnValue)
{}
} // anonymous namespace

TEST(MethodTest, testMethodFlags)
{
   {
      Method method("getInfo", dummy_func);
      zend_function_entry methodEntry = method.buildCallableEntry();
      ASSERT_STREQ(methodEntry.fname, "getInfo");
      ASSERT_EQ(methodEntry.handler, &dummy_func);
      ASSERT_EQ(methodEntry.num_args, 0);
      ASSERT_EQ(methodEntry.flags, ZEND_ACC_PUBLIC);
   }
   {
      Method method("getInfo", dummy_func, Modifier::Abstract | Modifier::Public | Modifier::Final);
      zend_function_entry methodEntry = method.buildCallableEntry();
      ASSERT_STREQ(methodEntry.fname, "getInfo");
      ASSERT_EQ(methodEntry.handler, &dummy_func);
      ASSERT_EQ(methodEntry.num_args, 0);
      ASSERT_EQ(methodEntry.flags, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT | ZEND_ACC_PUBLIC | ZEND_ACC_FINAL);
   }
}

TEST(MethodTest, testMethodClassName)
{
   Method method("getInfo", dummy_func);
   zend_function_entry methodEntry = method.buildCallableEntry("Person");
   ASSERT_STREQ(methodEntry.fname, "getInfo");
   ASSERT_EQ(methodEntry.handler, &dummy_func);
   ASSERT_EQ(methodEntry.num_args, 0);
   ASSERT_EQ(methodEntry.flags, ZEND_ACC_PUBLIC);
}
