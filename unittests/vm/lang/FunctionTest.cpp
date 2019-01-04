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

void dummy_func(struct _zend_execute_data *executeData, struct _zval_struct *returnValue)
{}

TEST(FunctionTest, testConstructor)
{
   {
//      Function func("polarphp_version", dummy_func);
//      zend_function_entry funcEntry;
//      func.initialize(&funcEntry);
//      ASSERT_STREQ(funcEntry.fname, "zapi_version");
//      ASSERT_EQ(funcEntry.handler, &dummy_func);
//      ASSERT_EQ(funcEntry.num_args, 0);
//      ASSERT_EQ(funcEntry.flags, 0);
   }
}
