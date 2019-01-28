// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/02/06.

#include "gtest/gtest.h"
#include "polarphp/vm/lang/Class.h"
#include "polarphp/vm/ds/StringVariant.h"
#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/StdClass.h"
#include "polarphp/vm/ds/ObjectVariant.h"

using polar::vmapi::Type;
using polar::vmapi::Class;
using polar::vmapi::StringVariant;
using polar::vmapi::ObjectVariant;
using polar::vmapi::NumericVariant;
using polar::vmapi::StdClass;
using polar::vmapi::Variant;
using polar::vmapi::VmApiVaridicItemType;
using polar::vmapi::ClassType;

namespace {
class Person : public StdClass
{
public:
   Person()
      : m_name("polarboy"),
        m_age(0)
   {}

   void showName()
   {
      polar::vmapi::out() << "my name is polarboy" << std::endl;
   }

   void print_sum(NumericVariant argCount, ...)
   {
      va_list args;
      va_start(args, argCount);
      NumericVariant result;
      for (int i = 0; i < argCount; ++i) {
         result += NumericVariant(va_arg(args, VmApiVaridicItemType), false);
      }
      polar::vmapi::out() << "the sum is " << result << std::endl;
   }

   void setAge(const NumericVariant &age)
   {
      m_age = age.toLong();
   }

   int getAge()
   {
      return m_age;
   }

   Variant getName()
   {
      return m_name;
   }

   int addTwoNum(const NumericVariant &num1, const NumericVariant &num2)
   {
      return num1 + num2;
   }

   int addSum(NumericVariant argCount, ...)
   {
      va_list args;
      va_start(args, argCount);
      NumericVariant result;
      for (int i = 0; i < argCount; ++i) {
         result += NumericVariant(va_arg(args, VmApiVaridicItemType), false);
      }
      return result.toLong();
   }

   // access level test method
   void protectedMethod()
   {}

   void privateMethod()
   {}

   static void staticShowName()
   {
      polar::vmapi::out() << "static my name is polarphp" << std::endl;
   }

   static StringVariant concatStr(const StringVariant &lhs, const StringVariant &rhs)
   {
      return lhs + rhs;
   }

   static void staticProtectedMethod()
   {}

   static void staticPrivateMethod()
   {}

   static void makeNewPerson()
   {
      ObjectVariant obj("Person", std::make_shared<Person>());
   }
private:
   /**
     *  The initial value
     *  @var    int
     */
   std::string m_name;
   int m_age;
};
}

TEST(ClassTest, testConstructor)
{
   Class<Person> personClass("Person");
   zend_class_entry *ce = personClass.buildClassEntry("", 0);
   ASSERT_STREQ(ZSTR_VAL(ce->name), "Person");
   ASSERT_EQ(ce->ce_flags, static_cast<uint32_t>(ClassType::Regular));
   ASSERT_EQ(personClass.getConstantCount(), 0);
   ASSERT_EQ(personClass.getMethodCount(), 0);
   ASSERT_EQ(personClass.getInterfaceCount(), 0);
   ASSERT_EQ(personClass.getPropertyCount(), 0);
}
