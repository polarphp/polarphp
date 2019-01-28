// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 polarboy <polarboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/28.

#include "gtest/gtest.h"

#include "polarphp/vm/ds/StringVariant.h"
#include "polarphp/vm/ds/Variant.h"

#include <string>
#include <vector>

using polar::vmapi::StringVariant;
using polar::vmapi::Variant;
using polar::vmapi::Type;


namespace {
class ScopeZvalDeleter
{
public:
   ScopeZvalDeleter(zval *data)
      : m_zval(data)
   {
   }
   ~ScopeZvalDeleter()
   {
      zval_ptr_dtor(m_zval);
   }
private:
   zval *m_zval;
};
}

TEST(StringVariantTest, testConstructors)
{
   StringVariant str("polarboy");
   StringVariant emptyStr;
   ASSERT_TRUE(emptyStr.isEmpty());
   ASSERT_EQ(emptyStr.getCapacity(), 0);
   ASSERT_EQ(emptyStr.getSize(), 0);
   emptyStr = 1;
   //   //std::cout << emptyStr << std::endl;
   emptyStr.append('C');
   ASSERT_EQ(emptyStr.getSize(), 2);
   ASSERT_EQ(emptyStr.getCapacity(), 191);
   ASSERT_EQ(emptyStr.at(0), '1');
   emptyStr.clear();
   ASSERT_EQ(emptyStr.getSize(), 0);
   ASSERT_EQ(emptyStr.getCapacity(), 0);
   emptyStr = str;
   ASSERT_EQ(emptyStr.getSize(), 8);
   ASSERT_EQ(emptyStr.getCapacity(), 8);
   ASSERT_EQ(emptyStr.getRefCount(), 2);
   ASSERT_EQ(str.getRefCount(), 2);
   emptyStr.clear();
   ASSERT_EQ(emptyStr.getSize(), 0);
   ASSERT_EQ(emptyStr.getCapacity(), 0);
   emptyStr = Variant("polarphp");
   ASSERT_EQ(emptyStr.getSize(), 8);
   ASSERT_EQ(emptyStr.getCapacity(), 8);
   ASSERT_EQ(emptyStr.getRefCount(), 1);
   emptyStr.clear();
   Variant gvar("polarphp");
   emptyStr = gvar;
   ASSERT_EQ(emptyStr.getSize(), 8);
   ASSERT_EQ(emptyStr.getCapacity(), 8);
   ASSERT_EQ(emptyStr.getRefCount(), 2);
}

TEST(StringVariantTest, testRefConstruct)
{
   {
      zval rawStrVar;
      ZVAL_STRING(&rawStrVar, "polarphp");
      zend_string *rawZendString = Z_STR(rawStrVar);
      ASSERT_EQ(zend_string_refcount(rawZendString), 1);
      ScopeZvalDeleter deleter(&rawStrVar);
      ASSERT_STREQ(Z_STRVAL_P(&rawStrVar), "polarphp");
      StringVariant strVariant(rawStrVar, false);
      ASSERT_EQ(zend_string_refcount(rawZendString), 2);
      ASSERT_EQ(strVariant.getCapacity(), 8);
      ASSERT_EQ(strVariant.getLength(), 8);
      ASSERT_EQ(strVariant.getRefCount(), 2);
      ASSERT_TRUE(strVariant.getUnDerefType() == Type::String);
      ASSERT_TRUE(strVariant.getType() == Type::String);
      StringVariant refStrVariant(rawStrVar, true);
      ASSERT_EQ(zend_string_refcount(rawZendString), 1);
      ASSERT_EQ(strVariant.getRefCount(), 1);
      zval *rval = &rawStrVar;
      ZVAL_DEREF(rval);
      ASSERT_TRUE(Z_TYPE_P(rval) == IS_STRING);
      ASSERT_TRUE(Z_TYPE_P(&rawStrVar) == IS_REFERENCE);
      ASSERT_TRUE(refStrVariant.getUnDerefType() == Type::Reference);
      ASSERT_TRUE(refStrVariant.getType() == Type::String);
      ASSERT_EQ(refStrVariant.getRefCount(), 2);
      ASSERT_STREQ(refStrVariant.getCStr(), "polarphp");

      ASSERT_EQ(refStrVariant.getCapacity(), 8);
      ASSERT_EQ(refStrVariant.getSize(), 8);
      refStrVariant += "x";
      ASSERT_STREQ(strVariant.getCStr(), "polarphp");
      ASSERT_STREQ(refStrVariant.getCStr(), "polarphpx");
      ASSERT_STREQ(Z_STRVAL_P(rval), "polarphpx");
   }
   {
      zval rawVar;
      ZVAL_LONG(&rawVar, 123);
      StringVariant strVariant(rawVar, false);
      ASSERT_EQ(strVariant.getLength(), 3);
      ASSERT_EQ(strVariant.getRefCount(), 1);
      StringVariant strVariantRef(rawVar, true);
      ASSERT_EQ(strVariantRef.getLength(), 3);
      ASSERT_EQ(strVariantRef.getRefCount(), 1);
   }
   {
      StringVariant str1("polarphp");
      StringVariant str2(str1, false);
      ASSERT_EQ(str1.getRefCount(), 2);
      ASSERT_EQ(str2.getRefCount(), 2);
      ASSERT_TRUE(str1.getUnDerefType() == Type::String);
      ASSERT_TRUE(str1.getType() == Type::String);
      ASSERT_TRUE(str2.getUnDerefType() == Type::String);
      ASSERT_TRUE(str2.getType() == Type::String);
      ASSERT_STREQ(str1.getCStr(), "polarphp");
      ASSERT_STREQ(str2.getCStr(), "polarphp");
   }
   {
      StringVariant str1("polarphp");
      StringVariant str2(str1, true);
      ASSERT_EQ(str1.getRefCount(), 2);
      ASSERT_EQ(str2.getRefCount(), 2);
      ASSERT_TRUE(str1.getUnDerefType() == Type::Reference);
      ASSERT_TRUE(str1.getType() == Type::String);
      ASSERT_TRUE(str2.getUnDerefType() == Type::Reference);
      ASSERT_TRUE(str2.getType() == Type::String);
      ASSERT_EQ(str1.getSize(), 8);
      ASSERT_EQ(str2.getSize(), 8);
      {
         StringVariant str3(str2, true);
         ASSERT_EQ(str1.getRefCount(), 3);
         ASSERT_EQ(str2.getRefCount(), 3);
         StringVariant str4(str3, false);
         ASSERT_EQ(str4.getRefCount(), 2);
         StringVariant str5(str4);
         ASSERT_EQ(str4.getRefCount(), 3);
         ASSERT_EQ(str5.getRefCount(), 3);
         ASSERT_EQ(str3.getSize(), 8);
         ASSERT_EQ(str4.getSize(), 8);
         ASSERT_EQ(str5.getSize(), 8);
         ASSERT_STREQ(str3.getCStr(), "polarphp");
         ASSERT_STREQ(str4.getCStr(), "polarphp");
         ASSERT_STREQ(str5.getCStr(), "polarphp");
      }
      ASSERT_EQ(str1.getRefCount(), 2);
      ASSERT_EQ(str2.getRefCount(), 2);
      ASSERT_TRUE(str1.getUnDerefType() == Type::Reference);
      ASSERT_TRUE(str1.getType() == Type::String);
      ASSERT_TRUE(str2.getUnDerefType() == Type::Reference);
      ASSERT_TRUE(str2.getType() == Type::String);
   }
   {
      // mixed ref and not ref
      // the ref and not ref will separate
      StringVariant str1("my name is polarboy, i think php is the best programming language in the world. php is the best!");
      StringVariant str2(str1, true);
      StringVariant str3(str2, true);
      StringVariant str4(str3, true);
      ASSERT_EQ(str1.getRefCount(), 4);
      ASSERT_EQ(str2.getRefCount(), 4);
      ASSERT_EQ(str3.getRefCount(), 4);
      ASSERT_EQ(str4.getRefCount(), 4);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str1.getZvalPtr())), 1);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str2.getZvalPtr())), 1);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str3.getZvalPtr())), 1);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str4.getZvalPtr())), 1);

      ASSERT_EQ(str1.getUnDerefType(), Type::Reference);
      ASSERT_EQ(str2.getUnDerefType(), Type::Reference);
      ASSERT_EQ(str3.getUnDerefType(), Type::Reference);
      ASSERT_EQ(str4.getUnDerefType(), Type::Reference);
      ASSERT_EQ(Z_REFCOUNT_P(str1.getZvalPtr()), 1);
      ASSERT_EQ(Z_REFCOUNT_P(str2.getZvalPtr()), 1);
      ASSERT_EQ(Z_REFCOUNT_P(str3.getZvalPtr()), 1);
      ASSERT_EQ(Z_REFCOUNT_P(str4.getZvalPtr()), 1);
      ASSERT_EQ(str1.getZvalPtr(), str2.getZvalPtr());
      ASSERT_EQ(str2.getZvalPtr(), str3.getZvalPtr());
      StringVariant str5(str4, false);
      StringVariant str6(str5, false);
      ASSERT_EQ(Z_REFCOUNT_P(str1.getZvalPtr()), 3);
      ASSERT_EQ(Z_REFCOUNT_P(str2.getZvalPtr()), 3);
      ASSERT_EQ(Z_REFCOUNT_P(str3.getZvalPtr()), 3);
      ASSERT_EQ(Z_REFCOUNT_P(str4.getZvalPtr()), 3);
      ASSERT_EQ(str1.getZvalPtr(), str2.getZvalPtr());
      ASSERT_EQ(str2.getZvalPtr(), str3.getZvalPtr());
      ASSERT_EQ(str3.getZvalPtr(), str4.getZvalPtr());

      ASSERT_NE(str4.getZvalPtr(), str5.getZvalPtr());
      ASSERT_EQ(Z_REFCOUNT_P(str5.getZvalPtr()), 3);
      ASSERT_EQ(Z_REFCOUNT_P(str6.getZvalPtr()), 3);
      ASSERT_NE(str5.getZvalPtr(), str6.getZvalPtr());
      ASSERT_EQ(str5.getCStr(), str6.getCStr());

      ASSERT_EQ(str5.getUnDerefType(), Type::String);
      ASSERT_EQ(str6.getUnDerefType(), Type::String);
   }
   {
      // test raw zval string separate
      zval rawStrVar;
      ZVAL_STRING(&rawStrVar, "polarphp");
      ScopeZvalDeleter deleter1(&rawStrVar);
      zval anotherStr;
      ZVAL_COPY(&anotherStr, &rawStrVar);
      ScopeZvalDeleter deleter2(&anotherStr);
      StringVariant str(rawStrVar, true);
      zval *rval = &rawStrVar;
      ZVAL_DEREF(rval);
      ASSERT_EQ(Z_STRVAL_P(str.getZvalPtr()), Z_STRVAL_P(rval));
   }
   {
      StringVariant str1("polarphp");
      StringVariant str2(str1, true);
      StringVariant str3(str2, true);
      StringVariant str4(str3);
      StringVariant str5(str4);
      StringVariant str6(str5, true);
      ASSERT_EQ(str1.getRefCount(), 3);
      ASSERT_EQ(str2.getRefCount(), 3);
      ASSERT_EQ(str3.getRefCount(), 3);
      ASSERT_EQ(str4.getRefCount(), 3);
      ASSERT_EQ(str5.getRefCount(), 2);
      ASSERT_EQ(str6.getRefCount(), 2);

      ASSERT_EQ(zend_string_refcount(Z_STR_P(str1.getZvalPtr())), 3);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str2.getZvalPtr())), 3);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str3.getZvalPtr())), 3);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str4.getZvalPtr())), 3);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str5.getZvalPtr())), 3);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str6.getZvalPtr())), 3);

      ASSERT_STREQ(str1.getCStr(), "polarphp");
      ASSERT_STREQ(str2.getCStr(), "polarphp");
      ASSERT_STREQ(str3.getCStr(), "polarphp");
      ASSERT_STREQ(str4.getCStr(), "polarphp");
      ASSERT_STREQ(str5.getCStr(), "polarphp");
      ASSERT_STREQ(str6.getCStr(), "polarphp");
      str1 += 'x';

      ASSERT_EQ(zend_string_refcount(Z_STR_P(str1.getZvalPtr())), 1);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str2.getZvalPtr())), 1);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str3.getZvalPtr())), 1);

      ASSERT_EQ(str1.getZvalPtr(), str2.getZvalPtr());
      ASSERT_EQ(str2.getZvalPtr(), str3.getZvalPtr());
      ASSERT_EQ(Z_STR_P(str1.getZvalPtr()), Z_STR_P(str2.getZvalPtr()));
      ASSERT_EQ(Z_STR_P(str2.getZvalPtr()), Z_STR_P(str3.getZvalPtr()));

      ASSERT_EQ(zend_string_refcount(Z_STR_P(str4.getZvalPtr())), 2);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str5.getZvalPtr())), 2);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str6.getZvalPtr())), 2);

      ASSERT_STREQ(str1.getCStr(), "polarphpx");
      ASSERT_STREQ(str2.getCStr(), "polarphpx");
      ASSERT_STREQ(str3.getCStr(), "polarphpx");
      ASSERT_STREQ(str4.getCStr(), "polarphp");
      ASSERT_STREQ(str5.getCStr(), "polarphp");
      ASSERT_STREQ(str6.getCStr(), "polarphp");
      str4 = "beijing";

      ASSERT_EQ(zend_string_refcount(Z_STR_P(str1.getZvalPtr())), 1);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str2.getZvalPtr())), 1);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str3.getZvalPtr())), 1);

      ASSERT_EQ(zend_string_refcount(Z_STR_P(str4.getZvalPtr())), 1);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str5.getZvalPtr())), 1);
      ASSERT_EQ(zend_string_refcount(Z_STR_P(str6.getZvalPtr())), 1);

      ASSERT_NE(Z_STR_P(str4.getZvalPtr()), Z_STR_P(str5.getZvalPtr()));
      ASSERT_EQ(Z_STR_P(str5.getZvalPtr()), Z_STR_P(str6.getZvalPtr()));

      ASSERT_STREQ(str1.getCStr(), "polarphpx");
      ASSERT_STREQ(str2.getCStr(), "polarphpx");
      ASSERT_STREQ(str3.getCStr(), "polarphpx");
      ASSERT_STREQ(str4.getCStr(), "beijing");
      ASSERT_STREQ(str5.getCStr(), "polarphp");
      ASSERT_STREQ(str6.getCStr(), "polarphp");
      str6 = "polarboy";
      ASSERT_STREQ(str1.getCStr(), "polarphpx");
      ASSERT_STREQ(str2.getCStr(), "polarphpx");
      ASSERT_STREQ(str3.getCStr(), "polarphpx");
      ASSERT_STREQ(str4.getCStr(), "beijing");
      ASSERT_STREQ(str5.getCStr(), "polarboy");
      ASSERT_STREQ(str6.getCStr(), "polarboy");
   }
}

TEST(StringVariantTest, testRefMidify)
{
   {
      StringVariant str1("polarphp");
      StringVariant str2(str1, true);
      ASSERT_EQ(str1.getRefCount(), 2);
      ASSERT_EQ(str2.getRefCount(), 2);
      StringVariant str3(str2);
      StringVariant str4(str3, true);
      ASSERT_EQ(str3.getRefCount(), 2);
      ASSERT_EQ(str4.getRefCount(), 2);
      StringVariant str5(str4);
      ASSERT_EQ(str5.getRefCount(), 3);
      ASSERT_TRUE(str1 == "polarphp");
      ASSERT_TRUE(str2 == "polarphp");
      ASSERT_TRUE(str3 == "polarphp");
      ASSERT_TRUE(str4 == "polarphp");
      ASSERT_TRUE(str5 == "polarphp");

      str1 += ", beijing";
      ASSERT_TRUE(str1 == "polarphp, beijing");
      ASSERT_TRUE(str2 == "polarphp, beijing");
      ASSERT_TRUE(str3 == "polarphp");
      ASSERT_TRUE(str4 == "polarphp");
      ASSERT_TRUE(str5 == "polarphp");
      str3 += "-x";
      ASSERT_TRUE(str3 == "polarphp-x");
      ASSERT_TRUE(str4 == "polarphp-x");
      ASSERT_TRUE(str5 == "polarphp");

      StringVariant str6(str1 + str3);
      ASSERT_TRUE(str6 == str1 + str3);
   }
   {
      StringVariant str1("polarphp");
      StringVariant str2(str1, true);
      ASSERT_EQ(str1.getRefCount(), 2);
      ASSERT_EQ(str2.getRefCount(), 2);
      StringVariant str3(str2);
      ASSERT_EQ(str3.getRefCount(), 2);
      str1.append("x");
      ASSERT_TRUE(str1 == "polarphpx");
      ASSERT_TRUE(str2 == "polarphpx");
      ASSERT_EQ(str3, "polarphp");
   }
}

TEST(StringVariantTest, testConstructFromVariant)
{
   Variant strVariant("polarphp is the best!");
   Variant numericVariant(123);
   StringVariant str(strVariant);
   StringVariant str1(numericVariant);
   ASSERT_EQ(str.getRefCount(), 2);
   ASSERT_EQ(strVariant.getRefCount(), 2);
   ASSERT_EQ(str1.getRefCount(), 1);
   ASSERT_EQ(numericVariant.getRefCount(), 0);
}

TEST(StringVariantTest, testConstructFromStringVariant)
{
   StringVariant str1("hello polarphp");
   StringVariant strCopy(str1);
   StringVariant strRef(str1, true);
   ASSERT_TRUE(str1.isReference());
   ASSERT_FALSE(strCopy.isReference());
   ASSERT_TRUE(strRef.isReference());
   str1.append(", beijing");
   ASSERT_STREQ(str1.getCStr(), "hello polarphp, beijing");
   ASSERT_STREQ(strCopy.getCStr(), "hello polarphp");
   ASSERT_STREQ(strRef.getCStr(), "hello polarphp, beijing");
   strCopy.append('X');
   ASSERT_STREQ(strCopy.getCStr(), "hello polarphpX");
   ASSERT_STREQ(str1.getCStr(), "hello polarphp, beijing");
   ASSERT_STREQ(strRef.getCStr(), "hello polarphp, beijing");
   strRef.append("BB");
   ASSERT_STREQ(strCopy.getCStr(), "hello polarphpX");
   ASSERT_STREQ(str1.getCStr(), "hello polarphp, beijingBB");
   ASSERT_STREQ(strRef.getCStr(), "hello polarphp, beijingBB");
}

TEST(StringVariantTest, testMoveConstruct)
{
   StringVariant strVariant(Variant("polarphp"));
   ASSERT_STREQ(strVariant.getCStr(), "polarphp");
   Variant gvar("hello polarboy");
   StringVariant str1(std::move(gvar));
   ASSERT_STREQ(str1.getCStr(), "hello polarboy");
   StringVariant str2(std::move(str1));
   ASSERT_STREQ(str2.getCStr(), "hello polarboy");
   StringVariant str3(str2, true);
   ASSERT_EQ(str3.getUnDerefType(), Type::Reference);
   ASSERT_EQ(str2.getUnDerefType(), Type::Reference);
   // test for reference
   str2.append(", hello polarfoundation");
   ASSERT_STREQ(str2.getCStr(), "hello polarboy, hello polarfoundation");
   ASSERT_STREQ(str3.getCStr(), "hello polarboy, hello polarfoundation");
   ASSERT_EQ(str2.getSize(), str3.getSize());
   ASSERT_EQ(str2.getCapacity(), str3.getCapacity());
   // move construct will transfer reference
   StringVariant str4(std::move(str3));
   ASSERT_STREQ(str4.getCStr(), "hello polarboy, hello polarfoundation");
   str4.append("XX");
   ASSERT_STREQ(str4.getCStr(), "hello polarboy, hello polarfoundationXX");
   ASSERT_STREQ(str2.getCStr(), "hello polarboy, hello polarfoundationXX");
}

TEST(StringVariantTest, testAssignOperators)
{
   StringVariant str1("polarphp");
   // test same type
   ASSERT_EQ(str1.getRefCount(), 1);
   StringVariant str2 = str1;
   ASSERT_EQ(str1.getRefCount(), 2);
   ASSERT_EQ(str2.getRefCount(), 2);
   ASSERT_STREQ(str2.getCStr(), "polarphp");
   str1.append('X');
   ASSERT_EQ(str1.getRefCount(), 1);
   ASSERT_EQ(str2.getRefCount(), 1);
   ASSERT_STREQ(str1.getCStr(), "polarphpX");
   ASSERT_STREQ(str2.getCStr(), "polarphp");
   str2 = str1;
   ASSERT_EQ(str1.getRefCount(), 2);
   ASSERT_EQ(str2.getRefCount(), 2);
   ASSERT_STREQ(str2.getCStr(), "polarphpX");
   StringVariant str3("xxx");
   ASSERT_EQ(str3.getRefCount(), 1);
   str3 = str2;
   ASSERT_EQ(str1.getRefCount(), 3);
   ASSERT_EQ(str2.getRefCount(), 3);
   ASSERT_EQ(str3.getRefCount(), 3);
   str1.append("C");
   ASSERT_EQ(str1.getRefCount(), 1);
   ASSERT_EQ(str2.getRefCount(), 2);
   ASSERT_EQ(str3.getRefCount(), 2);
   str3 = str1;
   ASSERT_EQ(str1.getRefCount(), 2);
   ASSERT_EQ(str2.getRefCount(), 1);
   ASSERT_EQ(str3.getRefCount(), 2);
   // test Variant type
   Variant gvar("polarboy");
   str1 = gvar;
   ASSERT_EQ(str1.getRefCount(), 2);
   ASSERT_EQ(gvar.getRefCount(), 2);
   ASSERT_EQ(str2.getRefCount(), 1);
   ASSERT_EQ(str3.getRefCount(), 1);
   ASSERT_STREQ(str1.getCStr(), "polarboy");
   str1.append("XX");
   ASSERT_EQ(gvar.getRefCount(), 1);
   ASSERT_EQ(str1.getRefCount(), 1);
   Variant numVar(123);
   str1 = numVar;
   ASSERT_EQ(str1.getRefCount(), 1);
   ASSERT_EQ(numVar.getRefCount(), 0);
   str1 = Variant("312");
   ASSERT_EQ(str1.getRefCount(), 1);
   ASSERT_STREQ(str1.getCStr(), "312");
   str1 = std::move(numVar);
   ASSERT_EQ(str1.getRefCount(), 1);
   ASSERT_STREQ(str1.getCStr(), "123");
   str1 = StringVariant("polarboy");
   ASSERT_STREQ(str1.getCStr(), "polarboy");
   ASSERT_EQ(str1.getRefCount(), 1);
   str1 = 123456778;
   ASSERT_STREQ(str1.getCStr(), "123456778");
   ASSERT_EQ(str1.getRefCount(), 1);
   ASSERT_EQ(str1.getLength(), 9);
   str1 = std::string("polarboy");
   ASSERT_STREQ(str1.getCStr(), "polarboy");
   ASSERT_EQ(str1.getRefCount(), 1);
   ASSERT_EQ(str1.getLength(), 8);
   str1 = 'c';
   ASSERT_STREQ(str1.getCStr(), "c");
   ASSERT_EQ(str1.getRefCount(), 1);
   ASSERT_EQ(str1.getLength(), 1);
   {
      Variant str("polarphp");
      StringVariant str1(str.makeReferenceByZval());
      ASSERT_EQ(str1.getUnDerefType(), Type::Reference);

      ASSERT_EQ(str.toString(), "polarphp");
      //      std::cout << str << std::endl;
      str1 = "hello, polarphp";
      ASSERT_EQ(str.toString(), "hello, polarphp");
      ASSERT_EQ(str1.toString(), "hello, polarphp");
      Variant str2(str1);
      ASSERT_EQ(str2.toString(), "hello, polarphp");
   }
}

TEST(StringVariantTest, testplusAssignOperators)
{
   StringVariant str;
   ASSERT_EQ(str.getSize(), 0);
   str += "polarphp";
   ASSERT_STREQ(str.getCStr(), "polarphp");
   str += std::string("--");
   ASSERT_STREQ(str.getCStr(), "polarphp--");
   str += StringVariant("php");
   ASSERT_STREQ(str.getCStr(), "polarphp--php");
   char append[] = {'z', 'z', 'u'};
   str += append;
   ASSERT_STREQ(str.getCStr(), "polarphp--phpzzu");
   // std::cout << str << std::endl;
}

TEST(StringVariantTest, testEqOperators)
{
   StringVariant str("polarphp");
   ASSERT_TRUE(str == "polarphp");
   ASSERT_FALSE(str == "polarphp1");
   ASSERT_TRUE(str == std::string("polarphp"));
   ASSERT_TRUE(str == StringVariant("polarphp"));
   char buffer[] = "polarphp";
   ASSERT_TRUE(str == buffer);
   char buffer1[] = {'p', 'o', 'l', 'a', 'r', 'p', 'h', 'p'};
   ASSERT_TRUE(str == buffer1);

   ASSERT_TRUE("polarphp" == str);
   ASSERT_FALSE("polarphp1" == str);
   ASSERT_TRUE(std::string("polarphp") == str);
   ASSERT_TRUE(StringVariant("polarphp") == str);
   char buffer2[] = "polarphp";
   ASSERT_TRUE(buffer2 == str);
   char buffer3[] = {'p', 'o', 'l', 'a', 'r', 'p', 'h', 'p'};
   ASSERT_TRUE(buffer3 == str);
}

TEST(StringVariantTest, testNotEqOperators)
{
   StringVariant str("polarphp");
   ASSERT_TRUE(str != "php");
   ASSERT_FALSE(str != "polarphp");
   ASSERT_TRUE(str != std::string("php"));
   ASSERT_TRUE(str != StringVariant("php"));
   char buffer[] = "php";
   ASSERT_TRUE(str != buffer);
   char buffer1[] = {'p', 'h', 'p'};
   ASSERT_TRUE(str != buffer1);

   ASSERT_TRUE("polarphpphp" != str);
   ASSERT_FALSE("polarphp" != str);
   ASSERT_TRUE(std::string("php") != str);
   ASSERT_TRUE(StringVariant("polarphpphp") != str);
   ASSERT_TRUE(buffer != str);
   ASSERT_TRUE(buffer1 != str);
}

TEST(StringVariantTest, testLtOperator)
{
   StringVariant str("polarphp");
   ASSERT_TRUE(str < "zbpi");
   ASSERT_FALSE(str < "abc");
   ASSERT_TRUE(str < std::string("zbpi"));
   ASSERT_TRUE(str < StringVariant("zbpi"));
   char buffer[] = "zbpi";
   ASSERT_TRUE(str < buffer);
   char buffer1[] = {'z', 'b', 'p', 'i'};
   ASSERT_TRUE(str < buffer1);

   ASSERT_TRUE("zbpi" > str);
   ASSERT_TRUE("polarphpx" > str);
   ASSERT_FALSE("abc" > str);
   ASSERT_TRUE(std::string("zbpi") > str);
   ASSERT_TRUE(StringVariant("zbpi") > str);
   ASSERT_TRUE(buffer > str);
   ASSERT_TRUE(buffer1 > str);
}

TEST(StringVariantTest, testLtEqOperator)
{
   StringVariant str("polarphp");
   ASSERT_TRUE(str <= "zbpi");
   ASSERT_TRUE(str <= "polarphp");
   ASSERT_TRUE(str <= std::string("zbpi"));
   ASSERT_TRUE(str <= std::string("polarphp"));
   ASSERT_TRUE(str <= StringVariant("zbpi"));
   ASSERT_TRUE(str <= StringVariant("polarphp"));
   char buffer[] = "polarphp";
   ASSERT_FALSE(str <= "abc");
   ASSERT_TRUE(str <= buffer);
   ASSERT_TRUE("polarphpx" >= str);
   ASSERT_TRUE("zbpi" >= str);
   ASSERT_FALSE("abc" >= str);
   ASSERT_TRUE(std::string("zbpi") >= str);
   ASSERT_TRUE(StringVariant("zbpi") >= str);
   ASSERT_TRUE(StringVariant("polarphp") >= str);
   ASSERT_TRUE(buffer >= str);
}

TEST(StringVariantTest, testGtOperator)
{
   StringVariant str("polarphp");
   ASSERT_TRUE(str > "abc");
   ASSERT_TRUE(str > std::string("abc"));
   ASSERT_TRUE(str > StringVariant("abc"));
   ASSERT_FALSE(str > "zbpi");
   char buffer[] = "abc";
   ASSERT_TRUE(str > buffer);

   ASSERT_TRUE("abcbdf" < str);
   ASSERT_TRUE(std::string("abc") < str);
   ASSERT_TRUE(StringVariant("abc") < str);
   ASSERT_TRUE(buffer < str);
}

TEST(StringVariantTest, testGtEqOperator)
{
   StringVariant str("polarphp");
   ASSERT_TRUE(str >= "abc");
   ASSERT_TRUE(str >= std::string("abc"));
   ASSERT_TRUE(str >= StringVariant("abc"));
   ASSERT_FALSE(str >= "zbpi");
   char buffer[] = "abc";
   ASSERT_TRUE(str >= buffer);

   ASSERT_TRUE("abc" <= str);
   ASSERT_TRUE(std::string("abc") <= str);
   ASSERT_TRUE(StringVariant("abc") <= str);
   ASSERT_TRUE(buffer <= str);
}

TEST(StringVariantTest, testAccessOperators)
{
   StringVariant str("polarphp");
   char &ch1 = str[0];
   ASSERT_EQ(ch1, 'p');
   ch1 = 'x';
   ASSERT_STREQ(str.getCStr(), "xolarphp");
   const StringVariant str1("polarphp");
   const char &ch2 = str1[0];
   ASSERT_EQ(ch2, 'p');
}

TEST(StringVariantTest, testClear)
{
   {
      StringVariant str("0123456789a123456789b1234A56789c");
      ASSERT_STREQ(str.getCStr(), "0123456789a123456789b1234A56789c");
      str.clear();
      ASSERT_EQ(str.getLength(), 0);
      ASSERT_EQ(str.getCapacity(), 0);
      str.append('c');
      ASSERT_STREQ(str.getCStr(), "c");
      ASSERT_EQ(str.getLength(), 1);
      ASSERT_EQ(str.getCapacity(), 191);
   }
   {
      StringVariant str("polarphp");
      StringVariant refStr(str, true);
      StringVariant refStr1(refStr, true);
      StringVariant anotherStr(str);
      ASSERT_STREQ(str.getCStr(), "polarphp");
      ASSERT_STREQ(refStr.getCStr(), "polarphp");
      ASSERT_STREQ(refStr1.getCStr(), "polarphp");
      ASSERT_STREQ(anotherStr.getCStr(), "polarphp");
      ASSERT_EQ(str.getRefCount(), 3);
      ASSERT_EQ(refStr.getRefCount(), 3);
      ASSERT_EQ(anotherStr.getRefCount(), 2);
      refStr.clear();
      ASSERT_TRUE(str.isEmpty());
      ASSERT_TRUE(refStr.isEmpty());
      ASSERT_STREQ(anotherStr.getCStr(), "polarphp");
   }
   {
      StringVariant str1("polarphp");
      StringVariant str2(str1);
      StringVariant str3(str2);
      ASSERT_EQ(str1.getRefCount(), 3);
      ASSERT_EQ(str2.getRefCount(), 3);
      ASSERT_EQ(str3.getRefCount(), 3);
      ASSERT_STREQ(str1.getCStr(), "polarphp");
      ASSERT_STREQ(str2.getCStr(), "polarphp");
      ASSERT_STREQ(str3.getCStr(), "polarphp");
      str1.clear();
      ASSERT_TRUE(str1.isEmpty());
      ASSERT_FALSE(str2.isEmpty());
      ASSERT_FALSE(str3.isEmpty());
      str2.clear();
      ASSERT_TRUE(str1.isEmpty());
      ASSERT_TRUE(str2.isEmpty());
      ASSERT_FALSE(str3.isEmpty());
      str3.clear();
      ASSERT_TRUE(str1.isEmpty());
      ASSERT_TRUE(str2.isEmpty());
      ASSERT_TRUE(str3.isEmpty());
   }
}

TEST(StringVariantTest, testResize)
{
   {
      StringVariant str("my name is polarboy, i think php is the best programming language in the world. php is the best!");
      ASSERT_EQ(str.getCapacity(), 96);
      ASSERT_EQ(str.getSize(), 96);
      str.resize(32);
      ASSERT_EQ(str.getCapacity(), 32);
      ASSERT_EQ(str.getSize(), 32);
      StringVariant str1 = "polarphp";
      ASSERT_EQ(str1.getRefCount(), 1);
      str = str1;
      ASSERT_EQ(str.getRefCount(), 2);
      ASSERT_EQ(str1.getRefCount(), 2);
      str.resize(32);
      ASSERT_EQ(str.getRefCount(), 1);
      ASSERT_EQ(str1.getRefCount(), 1);
      str = "my name is polarboy, i think php is the best programming language in the world. php is the best!";
      str.resize(12);
      ASSERT_STREQ(str.getCStr(), "my name is p");
      str.clear();
      ASSERT_EQ(str.getCapacity(), 0);
      ASSERT_EQ(str.getSize(), 0);
      str.resize(12);
      ASSERT_EQ(str.getCapacity(), 12);
      ASSERT_EQ(str.getSize(), 12);
      str = "polarphp";
      ASSERT_EQ(str.getCapacity(), 8);
      ASSERT_EQ(str.getSize(), 8);
      str.resize(12, '-');
      ASSERT_STREQ(str.getCStr(), "polarphp----");
   }
   // std::cout << str << std::endl;
   {
      // test str reference
      StringVariant str1("my name is polarboy, i think php is the best programming language in the world. php is the best!");
      StringVariant str2(str1, true);
      StringVariant str3(str2, true);
      StringVariant str4(str3, true);
      StringVariant str5(str4);
      StringVariant str6(str5);
      ASSERT_EQ(str1.getRefCount(), 4);
      ASSERT_EQ(str2.getRefCount(), 4);
      ASSERT_EQ(str3.getRefCount(), 4);
      ASSERT_EQ(str4.getRefCount(), 4);
      ASSERT_EQ(str1.getUnDerefType(), Type::Reference);
      ASSERT_EQ(str2.getUnDerefType(), Type::Reference);
      ASSERT_EQ(str3.getUnDerefType(), Type::Reference);
      ASSERT_EQ(str4.getUnDerefType(), Type::Reference);
      ASSERT_EQ(str5.getUnDerefType(), Type::String);
      ASSERT_EQ(str6.getUnDerefType(), Type::String);
      ASSERT_EQ(str1.getCapacity(), 96);
      ASSERT_EQ(str1.getSize(), 96);
      ASSERT_EQ(str2.getCapacity(), 96);
      ASSERT_EQ(str2.getSize(), 96);
      ASSERT_EQ(str3.getCapacity(), 96);
      ASSERT_EQ(str3.getSize(), 96);
      ASSERT_EQ(str4.getCapacity(), 96);
      ASSERT_EQ(str4.getSize(), 96);
      ASSERT_EQ(str5.getCapacity(), 96);
      ASSERT_EQ(str5.getSize(), 96);
      ASSERT_EQ(str6.getCapacity(), 96);
      ASSERT_EQ(str6.getSize(), 96);
      str1.resize(32);
      ASSERT_EQ(str1.getCapacity(), 32);
      ASSERT_EQ(str1.getSize(), 32);
      ASSERT_EQ(str2.getCapacity(), 32);
      ASSERT_EQ(str2.getSize(), 32);
      ASSERT_EQ(str5.getCapacity(), 96);
      ASSERT_EQ(str5.getSize(), 96);
      ASSERT_EQ(str6.getCapacity(), 96);
      ASSERT_EQ(str6.getSize(), 96);
   }
}

TEST(StringVariantTest, testContains)
{
   StringVariant str = "my name is polarboy, i think php is the best programming language in the world. php is the best!";
   ASSERT_TRUE(str.contains("name"));
   char searchArray[4] = {'b', 'e', 's', 't'};
   ASSERT_TRUE(str.contains(searchArray, 4));
   ASSERT_TRUE(str.contains(searchArray));
   ASSERT_FALSE(str.contains("PHP"));
   ASSERT_TRUE(str.contains("PHP", false));
}

TEST(StringVariantTest, testIndexOf)
{
   StringVariant str("my name is polarboy, i think php is the best programming language in the world. php is the best!");
   char subStr[] = {'p','h', 'p', 'i', 's', 't', 'h', 'e'};
   int pos = str.indexOf(subStr, 3);
   ASSERT_EQ(pos, 29);
   pos = str.indexOf(subStr, 4);
   ASSERT_EQ(pos, -1);
   pos = str.indexOf("php");
   ASSERT_EQ(pos, 29);
   pos = str.indexOf("PhP");
   ASSERT_EQ(pos, -1);
   pos = str.indexOf(std::string("php"));
   ASSERT_EQ(pos, 29);
   pos = str.indexOf('n');
   ASSERT_EQ(pos, 3);
   pos = str.indexOf("php", 33);
   ASSERT_EQ(pos, 80);
   pos = str.indexOf("PhP", 0, false);
   ASSERT_EQ(pos, 29);
   pos = str.indexOf("pHP", 0, false);
   ASSERT_EQ(pos, 29);
   pos = str.indexOf("POLARBOY", 0, false);
   ASSERT_EQ(pos, 11);
   char phpArr[] = {'p', 'h', 'p'};
   pos = str.indexOf(phpArr);
   ASSERT_EQ(pos, 29);
}

TEST(StringVariantTest, testLastIndexOf)
{
   // from php online manual
   StringVariant str("0123456789a123456789b1234A56789c");
   int pos = str.lastIndexOf('7', -5);
   ASSERT_EQ(pos, 17);
   pos = str.lastIndexOf('7', 20);
   ASSERT_EQ(pos, 28);
   pos = str.lastIndexOf('7', 29);
   ASSERT_EQ(pos, -1);
   pos = str.lastIndexOf('a', 0, false);
   ASSERT_EQ(pos, 25);
   pos = str.lastIndexOf('a', -7, false);
   ASSERT_EQ(pos, 25);
   pos = str.lastIndexOf('A', 0, false);
   ASSERT_EQ(pos, 25);
   pos = str.lastIndexOf('A', -7, false);
   ASSERT_EQ(pos, 25);
   pos = str.lastIndexOf('A', -8, false);
   ASSERT_EQ(pos, 10);
   pos = str.lastIndexOf('a', -8, false);
   ASSERT_EQ(pos, 10);
   char arr[] = {'4', '5', '6'};
   pos = str.lastIndexOf(arr);
   ASSERT_EQ(pos, 14);
}

TEST(StringVariantTest, testStartWiths)
{
   StringVariant str = "my name is polarboy, i think php is the best programming language in the world. php is the best!";
   ASSERT_TRUE(str.startsWith("my name is polarboy"));
   ASSERT_FALSE(str.startsWith("my name is zzu_softboy"));
   ASSERT_TRUE(str.startsWith("my name is PolarBoy", false));
   char search[7] = {'m', 'y', ' ', 'n', 'a', 'm', 'e'};
   ASSERT_TRUE(str.startsWith(search, 7));
   char search1[7] = {'m', 'y', ' ', 'N', 'a', 'm', 'e'};
   ASSERT_FALSE(str.startsWith(search1, 7));
   ASSERT_TRUE(str.startsWith(search1, 7, false));
}

TEST(StringVariantTest, testEndWiths)
{
   StringVariant str = "my name is polarboy, i think php is the best programming language in the world. php is the best!";
   ASSERT_TRUE(str.endsWith("php is the best!"));
   ASSERT_FALSE(str.endsWith("php Is The best!"));
   ASSERT_TRUE(str.endsWith("php Is The best!", false));
   char endSearch[5] = {'b', 'e', 's', 't', '!'};
   ASSERT_TRUE(str.endsWith(endSearch, 5));
   char endSearch1[5] = {'b', 'e', 's', 'T', '!'};
   ASSERT_FALSE(str.endsWith(endSearch1, 5));
   ASSERT_TRUE(str.endsWith(endSearch1, 5, false));
}

TEST(StringVariantTest, testLeft)
{
   StringVariant str = "my name is polarboy, i think php is the best programming language in the world. php is the best!";
   ASSERT_STREQ(str.left(2).c_str(), "my");
   ASSERT_STREQ(str.left(111).c_str(), "my name is polarboy, i think php is the best programming language in the world. php is the best!");
}

TEST(StringVariantTest, testRight)
{
   StringVariant str = "my name is polarboy, i think php is the best programming language in the world. php is the best!";
   ASSERT_STREQ(str.right(2).c_str(), "t!");
   ASSERT_STREQ(str.right(12).c_str(), "is the best!");
   ASSERT_STREQ(str.right(111).c_str(), "my name is polarboy, i think php is the best programming language in the world. php is the best!");
}

TEST(StringVariantTest, testJustify)
{
   StringVariant str("polarphp");
   ASSERT_STREQ(str.leftJustified(2, '.').c_str(), "po");
   ASSERT_STREQ(str.leftJustified(12, '.').c_str(), "polarphp....");
   //std::cout << str.leftJustified(8, '.').c_str() << std::endl;
   ASSERT_STREQ(str.rightJustified(2, '.').c_str(), "po");
   ASSERT_STREQ(str.rightJustified(12, '.').c_str(), "....polarphp");
   //std::cout << str.rightJustified(8, '.').c_str() << std::endl;
}

TEST(StringVariantTest, testSubString)
{
   StringVariant str = "my name is zzu_Softboy, i think php is the best programming language in the world. php is the best!";
   ASSERT_STREQ(str.substring(0, 6).c_str(), "my nam");
   ASSERT_STREQ(str.substring(3, 6).c_str(), "name i");
   ASSERT_STREQ(str.substring(20).c_str(), "oy, i think php is the best programming language in the world. php is the best!");
   ASSERT_THROW(str.substring(222), std::out_of_range);
   //std::cout << str.substring(3, 6).c_str() << std::endl;
}

TEST(StringVariantTest, testToLowerCaseAndToUpperCase)
{
   StringVariant str("PolarBOY");
   ASSERT_STREQ(str.toLowerCase().c_str(), "polarboy");
   ASSERT_STREQ(str.toUpperCase().c_str(), "POLARBOY");
}

TEST(StringVariantTest, testAppendAndPrepend)
{
   {
      StringVariant str("polarphp");
      ASSERT_STREQ(str.getCStr(), "polarphp");
      ASSERT_EQ(str.getLength(), 8);
      str.append(1);
      ASSERT_EQ(str.getLength(), 9);
      ASSERT_STREQ(str.getCStr(), "polarphp1");
      char needAppend[] = {'p', 'h', 'p'};
      str.append(needAppend, 3);
      ASSERT_EQ(str.getLength(), 12);
      ASSERT_STREQ(str.getCStr(), "polarphp1php");
      str.append("cpp");
      ASSERT_EQ(str.getLength(), 15);
      ASSERT_STREQ(str.getCStr(), "polarphp1phpcpp");
      StringVariant str1 = "hello";
      str.append(str1);
      ASSERT_EQ(str.getLength(), 20);
      ASSERT_STREQ(str.getCStr(), "polarphp1phpcpphello");
      str.append(needAppend);
      ASSERT_STREQ(str.getCStr(), "polarphp1phpcpphellophp");
   }
   {
      StringVariant str("polarphp");
      ASSERT_STREQ(str.getCStr(), "polarphp");
      ASSERT_EQ(str.getLength(), 8);
      str.prepend(1);
      ASSERT_EQ(str.getLength(), 9);
      ASSERT_STREQ(str.getCStr(), "1polarphp");
      char needPrepend[] = {'p', 'h', 'p'};
      str.prepend(needPrepend, 3);
      ASSERT_EQ(str.getLength(), 12);
      ASSERT_STREQ(str.getCStr(), "php1polarphp");
      str.prepend("cpp");
      ASSERT_EQ(str.getLength(), 15);
      ASSERT_STREQ(str.getCStr(), "cppphp1polarphp");
      StringVariant str1 = "hello";
      str.prepend(str1);
      ASSERT_EQ(str.getLength(), 20);
      ASSERT_STREQ(str.getCStr(), "hellocppphp1polarphp");
      str.prepend(needPrepend);
      ASSERT_STREQ(str.getCStr(), "phphellocppphp1polarphp");
   }
}

TEST(StringVariantTest, testRemove)
{
   StringVariant str = "my name is zzu_Softboy, i think php is the best programming language in the world. php is the best!";
   size_t oldLength = str.getLength();
   str.remove(2, 4);
   ASSERT_EQ(str.getLength(), oldLength - 4);
   ASSERT_STREQ(str.getCStr(), "mye is zzu_Softboy, i think php is the best programming language in the world. php is the best!");
   ASSERT_THROW(str.remove(111, 4), std::out_of_range);
   oldLength = str.getLength();
   str.remove(0);
   ASSERT_STREQ(str.getCStr(), "ye is zzu_Softboy, i think php is the best programming language in the world. php is the best!");
   ASSERT_EQ(str.getLength(), oldLength - 1);
   // test negative pos
   oldLength = str.getLength();
   str.remove(-1);
   ASSERT_STREQ(str.getCStr(), "ye is zzu_Softboy, i think php is the best programming language in the world. php is the best");
   ASSERT_EQ(str.getLength(), oldLength - 1);
   oldLength = str.getLength();
   str.remove(-4, 4);
   ASSERT_STREQ(str.getCStr(), "ye is zzu_Softboy, i think php is the best programming language in the world. php is the ");
   ASSERT_EQ(str.getLength(), oldLength - 4);
   ASSERT_THROW(str.remove(-100, 4), std::out_of_range);
   str = "my name is zzu_Softboy, i think php is the best programming language in the world. php is the best! But PHP a little slow";
   str.remove("php");
   ASSERT_STREQ(str.getCStr(), "my name is zzu_Softboy, i think  is the best programming language in the world.  is the best! But PHP a little slow");
   str.remove("php", false);
   ASSERT_STREQ(str.getCStr(), "my name is zzu_Softboy, i think  is the best programming language in the world.  is the best! But  a little slow");
   str.remove('z');
   ASSERT_STREQ(str.getCStr(), "my name is u_Softboy, i think  is the best programming language in the world.  is the best! But  a little slow");
   StringVariant emptyStr;
   ASSERT_THROW(emptyStr.remove(1, 1), std::out_of_range);
   emptyStr = str;
   ASSERT_EQ(emptyStr.getRefCount(), 2);
   ASSERT_EQ(str.getRefCount(), 2);
   emptyStr.remove(1);
   ASSERT_STREQ(emptyStr.getCStr(), "m name is u_Softboy, i think  is the best programming language in the world.  is the best! But  a little slow");
   ASSERT_EQ(emptyStr.getRefCount(), 1);
   ASSERT_EQ(str.getRefCount(), 1);
}

TEST(StringVariantTest, testStrInsert)
{
   StringVariant str("polarphp");
   ASSERT_STREQ(str.getCStr(), "polarphp");
   ASSERT_EQ(str.getLength(), 8);
   str.insert(1, "x");
   ASSERT_EQ(str.getLength(), 9);
   ASSERT_STREQ(str.getCStr(), "pxolarphp");
   str.insert(0, "x");
   ASSERT_EQ(str.getLength(), 10);
   ASSERT_STREQ(str.getCStr(), "xpxolarphp");
   str.insert(6, "ab");
   ASSERT_EQ(str.getLength(), 12);
   ASSERT_STREQ(str.getCStr(), "xpxolaabrphp");
   ASSERT_THROW(str.insert(13, "ab"), std::out_of_range);
   str.insert(8, 123);
   ASSERT_STREQ(str.getCStr(), "xpxolaab123rphp");
   str.clear();
   ASSERT_EQ(str.getLength(), 0);
   ASSERT_EQ(str.getCapacity(), 0);
   str.insert(0, "abc");
   ASSERT_EQ(str.getLength(), 3);
   ASSERT_STREQ(str.getCStr(), "abc");
   // negative pos
   str.insert(-1, 'x');
   str.insert(-1, 123);
   ASSERT_EQ(str.getLength(), 7);
   ASSERT_STREQ(str.getCStr(), "abx123c");
   ASSERT_THROW(str.insert(-8, "xx"), std::out_of_range);
   str.insert(-7, std::string("x"));
   ASSERT_EQ(str.getLength(), 8);
   int8_t pos1 = -2;
   str.insert(pos1, StringVariant("vv"));
   ASSERT_STREQ(str.getCStr(), "xabx12vv3c");
   // insert array
   char arr[] = {'p', 'h', 'p'};
   str.insert(1, arr, 3);
   ASSERT_STREQ(str.getCStr(), "xphpabx12vv3c");
   str.insert(1, arr);
   ASSERT_STREQ(str.getCStr(), "xphpphpabx12vv3c");
   str.insert(-1, arr);
   ASSERT_STREQ(str.getCStr(), "xphpphpabx12vv3phpc");
   int8_t pos2 = -2;
   str.insert(pos2, arr);
   ASSERT_STREQ(str.getCStr(), "xphpphpabx12vv3phphppc");
   str.insert(0, arr, -1);
   ASSERT_STREQ(str.getCStr(), "phpxphpphpabx12vv3phphppc");
   //   std::cout << str << std::endl;
}

TEST(StringVariantTest, testRepeated)
{
   StringVariant str;
   std::string repeatedStr = str.repeated(1);
   ASSERT_STREQ(repeatedStr.c_str(), "");
   str = "polarphp";
   repeatedStr = str.repeated(1);
   ASSERT_STREQ(repeatedStr.c_str(), "polarphp");
   repeatedStr = str.repeated(3);
   ASSERT_STREQ(repeatedStr.c_str(), "polarphppolarphppolarphp");
}

TEST(StringVariantTest, testSplits)
{
   std::vector<std::string> expected;
   StringVariant text("aaa||bbb||ccc||ddd||eee");
   std::vector<std::string> parts = text.split("||");
   expected = {"aaa", "bbb", "ccc", "ddd", "eee"};
   ASSERT_EQ(parts, expected);
   text = "||aaa||bbb||||ccc||ddd||";
   expected = {"", "aaa", "bbb", "", "ccc", "ddd", ""};
   parts = text.split("||");
   ASSERT_EQ(parts, expected);
   expected = {"", "", "", "", "", ""};
   text = "||||||||||";
   parts = text.split("||");
   ASSERT_EQ(parts, expected);
   text = "ashgdahsd";
   expected = {"ashgdahsd"};
   parts = text.split("||");
   ASSERT_EQ(parts, expected);

   text = "||aaa||bbb||||ccc||ddd||";
   expected = {"aaa", "bbb", "ccc", "ddd"};
   parts = text.split("||", false);
   ASSERT_EQ(parts, expected);
   text = "||||||||||";
   expected = {};
   parts = text.split("||", false);
   ASSERT_EQ(parts, expected);

   text = "aaaXXbbbxxcccXXdddXXeee";
   expected = {"aaa", "bbbxxccc", "ddd", "eee"};
   parts = text.split("XX", false, true);
   ASSERT_EQ(parts, expected);
   text = "aaaXXbbbxxcccXXdddXXeee";
   expected = {"aaa", "bbb", "ccc", "ddd", "eee"};
   parts = text.split("Xx", false, false);
   ASSERT_EQ(parts, expected);
}

TEST(StringVariantTest, testReplace)
{
   StringVariant str("my name is zzu_softboy, i love php");
   str.replace(0, 2, "MY");
   ASSERT_STREQ(str.getCStr(), "MY name is zzu_softboy, i love php");
   str.replace(3, 4, "NAME");
   ASSERT_STREQ(str.getCStr(), "MY NAME is zzu_softboy, i love php");
   str.replace(str.getLength() - 3, 4, "PHP");
   ASSERT_STREQ(str.getCStr(), "MY NAME is zzu_softboy, i love PHP");
   char replaceArr[] = {'p', 'o', 'l', 'a', 'r', 'p', 'h', 'p'};
   str.replace(0, 2, replaceArr);
   ASSERT_STREQ(str.getCStr(), "polarphp NAME is zzu_softboy, i love PHP");
   str = "MY NAME is zzu_softboy, i love PHP";
   str.replace(0, 2, replaceArr, -1);
   ASSERT_STREQ(str.getCStr(), "polarphp NAME is zzu_softboy, i love PHP");
   str = "MY NAME is zzu_softboy, i love PHP";
   str.replace(0, 2, replaceArr, 2);
   ASSERT_STREQ(str.getCStr(), "po NAME is zzu_softboy, i love PHP");
   //std::cout << str << std::endl;
   str.replace(-3, 4, "php");
   ASSERT_STREQ(str.getCStr(), "po NAME is zzu_softboy, i love php");
   str = "MY NAME is zzu_softboy, i love PHP";
   str.replace(-3, -4, "php");
   ASSERT_STREQ(str.getCStr(), "MY NAME is zzu_softboy, i love php");
   str.replace(-3, 3, replaceArr);
   ASSERT_STREQ(str.getCStr(), "MY NAME is zzu_softboy, i love polarphp");
   str = "my name is zzu_Softboy, i think php is the best programming language in the world. php is the best! pHp is very fast!";
   str.replace("php", "PHP");
   ASSERT_STREQ(str.getCStr(), "my name is zzu_Softboy, i think PHP is the best programming language in the world. PHP is the best! pHp is very fast!");
   //std::cout << str << std::endl;
   str = "my name is zzu_Softboy, i think php is the best programming language in the world. php is the best! pHp is very fast!";
   str.replace("php", "PHP", false);
   ASSERT_STREQ(str.getCStr(), "my name is zzu_Softboy, i think PHP is the best programming language in the world. PHP is the best! PHP is very fast!");
   str = "my name is zzu_Softboy, i think php is the best programming language in the world. php is the best! pHp is very fast!";
   str.replace('p', '_');
   ASSERT_STREQ(str.getCStr(), "my name is zzu_Softboy, i think _h_ is the best _rogramming language in the world. _h_ is the best! _H_ is very fast!");
   //std::cout << str << std::endl;
}

TEST(StringVariantTest, testPlusOperator)
{
   StringVariant str("polarphp");
   std::string ret = str + "-php";
   ASSERT_STREQ(ret.c_str(), "polarphp-php");
   ret = "php-" + str;
   ASSERT_STREQ(ret.c_str(), "php-polarphp");
   ret = str + std::string("-php");
   ASSERT_STREQ(ret.c_str(), "polarphp-php");
   ret = std::string("php-") + str;
   ASSERT_STREQ(ret.c_str(), "php-polarphp");
   ret = str + StringVariant("-php");
   ASSERT_STREQ(ret.c_str(), "polarphp-php");
   ret = StringVariant("php-") + str;
   ASSERT_STREQ(ret.c_str(), "php-polarphp");
   char buffer[] = "-php";
   ret = str + buffer;
   ASSERT_STREQ(ret.c_str(), "polarphp-php");
   ret = buffer + str;
   ASSERT_STREQ(ret.c_str(), "-phppolarphp");
   char buffer1[] = {'-', 'p', 'h', 'p'};
   ret = str + buffer1;
   ASSERT_STREQ(ret.c_str(), "polarphp-php");
   ret = buffer1 + str;
   ASSERT_STREQ(ret.c_str(), "-phppolarphp");
   ret = str + 'c';
   ASSERT_STREQ(ret.c_str(), "polarphpc");
   ret = 'c' + str;
   ASSERT_STREQ(ret.c_str(), "cpolarphp");
}

TEST(StringVariantTest, testEmptyStr)
{
   StringVariant emptyStr;
   ASSERT_EQ(emptyStr.getLength(), 0);
   ASSERT_EQ(emptyStr.getCStr(), nullptr);
   ASSERT_EQ(emptyStr.getData(), nullptr);
   ASSERT_FALSE(emptyStr.startsWith("x"));
   ASSERT_FALSE(emptyStr.endsWith("x"));
   ASSERT_EQ(emptyStr.indexOf("x"), -1);
   ASSERT_EQ(emptyStr.lastIndexOf("x"), -1);
   emptyStr.clear();
   ASSERT_EQ(emptyStr.getLength(), 0);
   ASSERT_EQ(emptyStr.getCStr(), nullptr);
   ASSERT_EQ(emptyStr.getData(), nullptr);
   ASSERT_FALSE(emptyStr.startsWith("x"));
   ASSERT_FALSE(emptyStr.endsWith("x"));
   ASSERT_EQ(emptyStr.indexOf("x"), -1);
   ASSERT_EQ(emptyStr.lastIndexOf("x"), -1);
}
