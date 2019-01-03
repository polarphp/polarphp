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

#include "polarphp/vm/lang/Namespace.h"
#include "polarphp/vm/lang/Class.h"
#include "polarphp/vm/StdClass.h"
#include "polarphp/vm/AbstractClass.h"

#include "gtest/gtest.h"

using polar::vmapi::Namespace;
using polar::vmapi::Class;
using polar::vmapi::StdClass;
using polar::vmapi::AbstractClass;

class ClassA : public StdClass
{};

class ClassB : public StdClass
{};

TEST(NamespaceTest, testFindNamespace)
{
   Namespace polar("polar");
   Namespace kernel("kernel");
   Namespace net("net");
   polar.registerNamespace(kernel);
   polar.registerNamespace(net);
   Namespace *result = nullptr;
   result = polar.findNamespace("NotExistNamespace");
   ASSERT_EQ(result, nullptr);
   result = polar.findNamespace("kernel");
   ASSERT_NE(result, nullptr);
   ASSERT_EQ(result->getName(), "kernel");
   result = nullptr;
   result = polar.findNamespace("net");
   ASSERT_NE(result, nullptr);
   ASSERT_EQ(result->getName(), "net");
}

TEST(NamespaceTest, testFindClass)
{
   Namespace polarphp("polarphp");
   Class<ClassA> classA("ClassA");
   Class<ClassB> classB("ClassB");
   polarphp.registerClass(classA);
   polarphp.registerClass(classB);
   AbstractClass *resultCls = nullptr;
   resultCls = polarphp.findClass("NotExistClass");
   ASSERT_EQ(resultCls, nullptr);
   resultCls = polarphp.findClass("ClassA");
   ASSERT_NE(resultCls, nullptr);
   ASSERT_EQ(resultCls->getClassName(), "ClassA");
   resultCls = nullptr;
   resultCls = polarphp.findClass("ClassB");
   ASSERT_NE(resultCls, nullptr);
   ASSERT_EQ(resultCls->getClassName(), "ClassB");
}

