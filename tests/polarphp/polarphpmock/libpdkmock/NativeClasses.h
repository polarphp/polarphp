// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/02/07.

#ifndef POLARPHP_STDLIBMOCK_NATIVE_CLASSES_H
#define POLARPHP_STDLIBMOCK_NATIVE_CLASSES_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/ds/NumericVariant.h"
#include "polarphp/vm/ds/Variant.h"
#include "polarphp/vm/ds/StringVariant.h"
#include "polarphp/vm/ds/ArrayVariant.h"
#include "polarphp/vm/StdClass.h"
#include "polarphp/vm/protocol/ArrayAccess.h"
#include "polarphp/vm/protocol/Countable.h"
#include "polarphp/vm/protocol/Serializable.h"
#include "polarphp/vm/protocol/Traversable.h"
#include "polarphp/vm/protocol/AbstractIterator.h"

#include <string>
#include <vector>

namespace php {

using polar::vmapi::StdClass;
using polar::vmapi::Variant;
using polar::vmapi::NumericVariant;
using polar::vmapi::StringVariant;
using polar::vmapi::ArrayVariant;
using polar::vmapi::Parameters;

// normal classes definitions

class Person : public StdClass
{
public:
   Person();
   void showName();
   void print_sum(Parameters &args);
   void setAge(Parameters &args);
   int getAge();

   Variant getName();
   int addTwoNum(Parameters &args);
   int addSum(Parameters &args);
   // access level test method
   void protectedMethod();
   void privateMethod();

   static void staticShowName();
   static StringVariant concatStr(Parameters &args);
   static void staticProtectedMethod();
   static void staticPrivateMethod();
   static void makeNewPerson();
private:
   /**
     *  The initial value
     *  @var    int
     */
   std::string m_name;
   int m_age;
};


class Address : public StdClass
{
public:
   Address();
protected:
   std::string address;
};

class ConstructAndDestruct : public StdClass
{
public:
   void __construct();
   void __destruct();
};

class EmptyClass : public StdClass
{};

// for test class and interface inherit
class A : public StdClass
{
public:
   void printInfo();
   void changeNameByRef(Parameters &args);
   void privateAMethod();
   void protectedAMethod();
};

class B : public StdClass
{
public:
   void printInfo();
   void showSomething();
   void calculateSumByRef(Parameters &args);
   Variant addTwoNumber(Parameters &args);
   void privateBMethod();
   void protectedBMethod();
};

class C : public StdClass
{
public:
   void printInfo();
   void testCallParentPassRefArg();
   void testCallParentWithReturn();
   void testGetObjectVaraintPtr();
   void privateCMethod();
   void protectedCMethod();
   void methodOfA();
   void protectedMethodOfA();
   void privateMethodOfA();
};

class IterateTestClass :
      public StdClass,
      public polar::vmapi::Traversable,
      public polar::vmapi::AbstractIterator,
      public polar::vmapi::Countable,
      public polar::vmapi::ArrayAccess
{
   using IteratorType = std::vector<std::pair<std::string, std::string>>::iterator;
public:
   IterateTestClass();
   virtual AbstractIterator *getIterator();
   virtual bool valid();
   virtual Variant current();
   virtual Variant key();
   virtual void next();
   virtual void rewind();
   virtual vmapi_long count();

   virtual bool offsetExists(Variant offset);
   virtual void offsetSet(Variant offset, Variant value);
   virtual Variant offsetGet(Variant offset);
   virtual void offsetUnset(Variant offset);
   virtual ~IterateTestClass();
protected:
   // save iterator object
   std::shared_ptr<AbstractIterator> m_iterator;
   IteratorType m_currentIter;
   std::vector<std::pair<std::string, std::string>> m_items;
};

class ClosureTestClass : public StdClass
{
public:
   void testClosureCallable();
   Variant getNoArgAndReturnCallable();
   Variant getArgAndReturnCallable();
   ~ClosureTestClass();
};

class VisibilityClass : public StdClass
{
public:
   void publicMethod();
   void protectedMethod();
   void privateMethod();
   void finalMethod();
};

// for class type test

class FinalTestClass : public StdClass
{
public:
   void someMethod();
};

class AbstractTestClass : public StdClass
{
public:
   void normalMethod();
};

// for magic test

class NonMagicMethodClass : public StdClass
{

};

class MagicMethodClass : public StdClass, public polar::vmapi::Serializable
{
public:
   Variant __call(const std::string &method, Parameters &params) const;
   Variant __invoke(Parameters &params) const;
   void __set(const std::string &key, const Variant &value);
   Variant __get(const std::string &key) const;
   bool __isset(const std::string &key) const;
   void __unset(const std::string &key);
   Variant __toString() const;
   Variant __toInteger() const;
   Variant __toDouble() const;
   Variant __toBool() const;
   ArrayVariant __debugInfo() const;
   int __compare(const MagicMethodClass &object) const;
   void __clone();
   Variant getTeamWebsite();
   static Variant __callStatic(const std::string &method, Parameters &params);
   virtual std::string serialize();
   virtual void unserialize(const char *input, size_t size);
   virtual ~MagicMethodClass() noexcept;
private:
   bool m_teamNameUnset = false;
   bool m_teamAddressUnset = true;
   int m_length = 0;
   std::string m_address;
};

// for class properties test
class PropsTestClass : public StdClass
{
private:
   int m_age;
   std::string m_name;
public:
   void setAge(const Variant &age);
   Variant getAge();
   void setName(const Variant &name);
   Variant getName();
};

class ObjectVariantClass : public StdClass
{
public:
   Variant __invoke(Parameters &params) const;
   void forwardInvoke();
   void testInstanceOf();
   void testDerivedFrom();
   void testNoArgCall();
   void testVarArgsCall();
   void printName();
   std::string getName();
   void printSum(Parameters &args);
   int calculateSum(Parameters &args);
   void changeNameByRef(Parameters &args);
};

} // php

#endif // POLARPHP_STDLIBMOCK_NATIVE_CLASSES_H
