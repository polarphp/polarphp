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

#include "polarphp/vm/lang/Module.h"
#include "polarphp/vm/lang/Namespace.h"
#include "polarphp/vm/lang/Class.h"
#include "polarphp/vm/StdClass.h"
#include "polarphp/vm/AbstractClass.h"
#include "gtest/gtest.h"

using polar::vmapi::Module;
using polar::vmapi::Namespace;
using polar::vmapi::Class;
using polar::vmapi::StdClass;
using polar::vmapi::AbstractClass;

class ClassA : public StdClass
{};

class ClassB : public StdClass
{};

TEST(ModuleTest, testFindNamespace)
{
   Module ext("dummyext", "1.0");
   ext.registerNamespace(Namespace("polar"));
   ext.registerNamespace(Namespace("php"));
   ASSERT_EQ(ext.getNamespaceCount(), 2);
   Namespace *result = nullptr;
   result = ext.findNamespace("notexist");
   ASSERT_EQ(result, nullptr);
   result = ext.findNamespace("polar");
   ASSERT_NE(result, nullptr);
   ASSERT_EQ(result->getName(), "polar");
   result = ext.findNamespace("php");
   ASSERT_NE(result, nullptr);
   ASSERT_EQ(result->getName(), "php");
   ext.registerClass(Class<ClassA>("ClassA"));
   ext.registerClass(Class<ClassB>("ClassB"));
   ASSERT_EQ(ext.getClassCount(), 2);
}
