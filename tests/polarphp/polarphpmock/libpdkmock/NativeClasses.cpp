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

#include "NativeClasses.h"
#include "NativeFunctions.h"
#include "polarphp/vm/ds/CallableVariant.h"
#include "polarphp/vm/lang/Parameter.h"

namespace php {

using polar::vmapi::CallableVariant;
using polar::vmapi::ObjectVariant;
using polar::vmapi::AbstractIterator;
using polar::vmapi::Type;

Person::Person()
   : m_name("zzu_softboy"),
     m_age(0)
{}

void Person::showName()
{
   polar::vmapi::out() << "my name is polarphp" << std::endl;
}

void Person::setAge(Parameters &args)
{
   m_age = args.at<NumericVariant>(0).toLong();
}

int Person::getAge()
{
   return m_age;
}

Variant Person::getName()
{
   return m_name;
}

void Person::staticShowName()
{
   polar::vmapi::out() << "static my name is polarphp" << std::endl;
}

StringVariant Person::concatStr(Parameters &args)
{
   StringVariant &lhs = args.at<StringVariant>(0);
   StringVariant &rhs = args.at<StringVariant>(1);
   return lhs + rhs;
}

void Person::staticProtectedMethod()
{

}

void Person::staticPrivateMethod()
{

}

void Person::makeNewPerson()
{
   ObjectVariant obj("Person", std::make_shared<Person>());
}

void Person::print_sum(Parameters &args)
{
   NumericVariant result;
   for (size_t i = 0; i < args.size(); ++i) {
      result += args.at<NumericVariant>(i);
   }
   polar::vmapi::out() << "the sum is " << result << std::endl;
}

int Person::addSum(Parameters &args)
{
   NumericVariant result;
   for (size_t i = 0; i < args.size(); ++i) {
      result += args.at<NumericVariant>(i);
   }
   return result.toLong();
}

void Person::protectedMethod()
{

}

void Person::privateMethod()
{

}

int Person::addTwoNum(Parameters &args)
{
   NumericVariant &num1 = args.at<NumericVariant>(0);
   NumericVariant &num2 = args.at<NumericVariant>(1);
   return num1 + num2;
}

Address::Address()
   : address("beijing")
{}

void ConstructAndDestruct::__construct()
{
   polar::vmapi::out() << "constructor been invoked" << std::endl;
}

void ConstructAndDestruct::__destruct()
{
   polar::vmapi::out() << "destructor been invoked" << std::endl;;
}

// for class and interface inherit classes

void A::printInfo()
{
   polar::vmapi::out() << "A::printInfo been called" << std::endl;
}

void A::changeNameByRef(Parameters &args)
{
   polar::vmapi::out() << "A::changeNameByRef been called" << std::endl;
   StringVariant &name = args.at<StringVariant>(0);
   if (name.getUnDerefType() == Type::Reference) {
      polar::vmapi::out() << "get ref arg" << std::endl;
   }
   name = "hello, polarphp";
}

void A::privateAMethod()
{
   polar::vmapi::out() << "A::privateBMethod been called" << std::endl;
}

void A::protectedAMethod()
{
   polar::vmapi::out() << "A::protectedAMethod been called" << std::endl;
}

void B::printInfo()
{
   polar::vmapi::out() << "B::printInfo been called" << std::endl;
}

void B::showSomething()
{
   polar::vmapi::out() << "B::showSomething been called" << std::endl;

}

void B::calculateSumByRef(Parameters &args)
{
   polar::vmapi::out() << "C::calculateSumByRef been called" << std::endl;
   polar::vmapi::out() << "got " << args.size() << " args" << std::endl;
   NumericVariant &retval = args.at<NumericVariant>(0);
   if (retval.getUnDerefType() == Type::Reference) {
      polar::vmapi::out() << "retval is reference arg" << std::endl;
   }
   for (size_t i = 1; i < args.size(); ++i) {
      retval += args.at<NumericVariant>(i);
   }
}

Variant B::addTwoNumber(Parameters &args)
{
   NumericVariant &lhs = args.at<NumericVariant>(0);
   NumericVariant &rhs = args.at<NumericVariant>(1);
   polar::vmapi::out() << "B::addTwoNumber been called" << std::endl;
   return lhs + rhs;
}

void B::privateBMethod()
{
   polar::vmapi::out() << "B::privateBMethod been called" << std::endl;
}

void B::protectedBMethod()
{
   polar::vmapi::out() << "B::protectedBMethod been called" << std::endl;
   callParent("protectedAMethod");
}

void C::printInfo()
{
   polar::vmapi::out() << "C::printInfo been called" << std::endl;
   callParent("printInfo");
   callParent("showSomething");
}

void C::testCallParentPassRefArg()
{
   polar::vmapi::out() << "C::testCallParentPassRefArg been called" << std::endl;
   Variant str("xxxx");
   polar::vmapi::out() << "before call changeNameByRef : " << str << std::endl;
   callParent("changeNameByRef", Variant(str, true));
   polar::vmapi::out() << "after call changeNameByRef : " << str << std::endl;
   // pass arg when variant args
   NumericVariant ret(0);
   polar::vmapi::out() << "before call calculateSumByRef : " << ret.toLong() << std::endl;
   callParent("calculateSumByRef", ret.makeReferenceByZval(), 12, 2, 33);
   polar::vmapi::out() << "after call calculateSumByRef : " << ret.toLong() << std::endl;

}

void C::testCallParentWithReturn()
{
   polar::vmapi::out() << "C::testCallParentWithReturn been called" << std::endl;
   Variant ret = callParent("addTwoNumber", 1, 23);
   polar::vmapi::out() << "after call addTwoNumber get : " << ret << std::endl;
}

void C::testGetObjectVaraintPtr()
{
   polar::vmapi::out() << "C::testGetObjectVaraintPtr been called" << std::endl;
   ObjectVariant *objZvalPtr = this->getObjectZvalPtr();
   if (objZvalPtr->hasProperty("address")) {
      polar::vmapi::out() << "property C::address exists" << std::endl;
      polar::vmapi::out() << "property value : " << objZvalPtr->getProperty("address") << std::endl;
   }
   if (!objZvalPtr->hasProperty("privateName")) {
      polar::vmapi::out() << "property C::privateName not exists" << std::endl;
   }
   if (objZvalPtr->hasProperty("protectedName")) {
      polar::vmapi::out() << "property C::protectedName exists" << std::endl;
      polar::vmapi::out() << "property value : " << objZvalPtr->getProperty("protectedName") << std::endl;
   }
   if (objZvalPtr->methodExist("showSomething")) {
      polar::vmapi::out() << "method C::showSomething exists" << std::endl;
      objZvalPtr->call("showSomething");
   }
   if (objZvalPtr->methodExist("privateCMethod")) {
      polar::vmapi::out() << "method C::privateCMethod exists" << std::endl;
      objZvalPtr->call("privateCMethod");
   }
   if (objZvalPtr->methodExist("privateAMethod")) {
      polar::vmapi::out() << "method C::privateCMethod exists" << std::endl;
      // objZvalPtr->call("privateAMethod"); fata error
   }
   if (objZvalPtr->methodExist("protectedAMethod")) {
      polar::vmapi::out() << "method C::protectedAMethod exists" << std::endl;
      objZvalPtr->call("protectedAMethod");
   }
   if (objZvalPtr->methodExist("privateBMethod")) {
      polar::vmapi::out() << "method C::privateBMethod exists" << std::endl;
      // objZvalPtr->call("privateAMethod"); fata error
   }
   if (objZvalPtr->methodExist("protectedBMethod")) {
      polar::vmapi::out() << "method C::protectedBMethod exists" << std::endl;
      objZvalPtr->call("protectedBMethod");
   }
}

void C::privateCMethod()
{
   polar::vmapi::out() << "C::privateCMethod been called" << std::endl;
}

void C::protectedCMethod()
{
   polar::vmapi::out() << "C::protectedCMethod been called" << std::endl;
}

void C::methodOfA()
{

}

void C::protectedMethodOfA()
{

}

void C::privateMethodOfA()
{

}

// for test class iterator


IterateTestClass::IterateTestClass()
   : AbstractIterator(this)
{
   m_items.push_back(std::make_pair<std::string, std::string>("key1", "value1"));
   m_items.push_back(std::make_pair<std::string, std::string>("key2", "value2"));
   m_items.push_back(std::make_pair<std::string, std::string>("key3", "value3"));
   m_items.push_back(std::make_pair<std::string, std::string>("key4", "value4"));
   m_currentIter = m_items.begin();
}

AbstractIterator *IterateTestClass::getIterator()
{
   return this;
}

bool IterateTestClass::valid()
{
   polar::vmapi::out() << "IterateTestClass::valid called" << std::endl;
   return m_currentIter != m_items.end();
}

Variant IterateTestClass::current()
{
   polar::vmapi::out() << "IterateTestClass::current called" << std::endl;
   return m_currentIter->second;
}

Variant IterateTestClass::key()
{
   polar::vmapi::out() << "IterateTestClass::key called" << std::endl;
   return m_currentIter->first;
}

void IterateTestClass::next()
{
   polar::vmapi::out() << "IterateTestClass::next called" << std::endl;
   m_currentIter++;
}

void IterateTestClass::rewind()
{
   polar::vmapi::out() << "IterateTestClass::rewind called" << std::endl;
   m_currentIter = m_items.begin();
}

vmapi_long IterateTestClass::count()
{
   polar::vmapi::out() << "IterateTestClass::count called" << std::endl;
   return m_items.size();
}

bool IterateTestClass::offsetExists(Variant offset)
{
   auto begin = m_items.begin();
   auto end = m_items.end();
   std::string key = StringVariant(std::move(offset)).toString();
   while (begin != end) {
      if (begin->first == key) {
         return true;
      }
      begin++;
   }
   return false;
}

void IterateTestClass::offsetSet(Variant offset, Variant value)
{
   auto begin = m_items.begin();
   auto end = m_items.end();
   std::string key = StringVariant(std::move(offset)).toString();
   while (begin != end) {
      if (begin->first == key) {
         begin->second = StringVariant(std::move(value)).toString();
         return;
      }
      begin++;
   }
}

Variant IterateTestClass::offsetGet(Variant offset)
{
   auto begin = m_items.begin();
   auto end = m_items.end();
   std::string key = StringVariant(std::move(offset)).toString();
   while (begin != end) {
      if (begin->first == key) {
         return begin->second;
      }
      begin++;
   }
   return nullptr;
}

void IterateTestClass::offsetUnset(Variant offset)
{
   auto begin = m_items.begin();
   auto end = m_items.end();
   std::string key = StringVariant(std::move(offset)).toString();
   while (begin != end) {
      if (begin->first == key) {
         break;
      }
      begin++;
   }
   if (begin != end) {
      m_items.erase(begin);
   }

}

IterateTestClass::~IterateTestClass()
{}

// for test closure class test

void ClosureTestClass::testClosureCallable()
{
   CallableVariant callableVar(print_something);
   //callableVar();
}

Variant ClosureTestClass::getNoArgAndReturnCallable()
{
   return CallableVariant(print_something);
}

Variant ClosureTestClass::getArgAndReturnCallable()
{
   return CallableVariant(have_ret_and_have_arg);
}

ClosureTestClass::~ClosureTestClass()
{}

// for class type test
void VisibilityClass::publicMethod()
{}

void VisibilityClass::protectedMethod()
{}

void VisibilityClass::privateMethod()
{}

void VisibilityClass::finalMethod()
{}

void FinalTestClass::someMethod()
{
}

void AbstractTestClass::normalMethod()
{

}

// for magic methods test
Variant MagicMethodClass::__call(const std::string &method, Parameters &params) const
{
   polar::vmapi::out() << "MagicMethodClass::__call is called" << std::endl;
   if (method == "calculateSum") {
      NumericVariant sum;
      for (size_t i = 0; i < params.size(); i++) {
         sum += params.at<NumericVariant>(i);
      }
      return sum;
   } else {
      return nullptr;
   }
}

Variant MagicMethodClass::__invoke(Parameters &params) const
{
   polar::vmapi::out() << "MagicMethodClass::__invoke is called" << std::endl;
   NumericVariant sum;
   for (size_t i = 0; i < params.size(); i++) {
      sum += params.at<NumericVariant>(i);
   }
   return sum;
}

void MagicMethodClass::__set(const std::string &key, const Variant &value)
{
   polar::vmapi::out() << "MagicMethodClass::__set is called" << std::endl;
   if (key == "address") {
      m_address = StringVariant(value).toString();
      m_teamAddressUnset = false;
   } else if (key == "length") {
      m_length = NumericVariant(value).toLong();
   }
}

Variant MagicMethodClass::__get(const std::string &key) const
{
   polar::vmapi::out() << "MagicMethodClass::__get is called" << std::endl;
   if (key == "prop1") {
      return "polarphp";
   } else if(key == "teamName" && !m_teamNameUnset) {
      return "polarphp team";
   } else if (key == "address" && !m_teamAddressUnset) {
      return m_address;
   } else if (key == "length") {
      return m_length;
   }
   return nullptr;
}

bool MagicMethodClass::__isset(const std::string &key) const
{
   polar::vmapi::out() << "MagicMethodClass::__isset is called" << std::endl;
   if (key == "prop1") {
      return true;
   } else if (key == "teamName" && !m_teamNameUnset) {
      return true;
   } else if (key == "address" && !m_teamAddressUnset) {
      return true;
   } else if (key == "length") {
      return true;
   }
   return false;
}

void MagicMethodClass::__unset(const std::string &key)
{
   polar::vmapi::out() << "MagicMethodClass::__unset is called" << std::endl;
   if (key == "teamName") {
      m_teamNameUnset = true;
   } else if (key == "address") {
      m_teamAddressUnset = true;
   }
}

Variant MagicMethodClass::__toString() const
{
   polar::vmapi::out() << "MagicMethodClass::__toString is called" << std::endl;
   return "hello, polarphp";
}

Variant MagicMethodClass::__toInteger() const
{
   polar::vmapi::out() << "MagicMethodClass::__toInteger is called" << std::endl;
   return 2017;
}

Variant MagicMethodClass::__toDouble() const
{
   polar::vmapi::out() << "MagicMethodClass::__toDouble is called" << std::endl;
   return 3.14;
}

Variant MagicMethodClass::__toBool() const
{
   polar::vmapi::out() << "MagicMethodClass::__toBool is called" << std::endl;
   return true;
}

ArrayVariant MagicMethodClass::__debugInfo() const
{
   ArrayVariant info;
   info.insert("name", "polarphp");
   info.insert("address", "beijing");
   return info;
}

int MagicMethodClass::__compare(const MagicMethodClass &object) const
{
   polar::vmapi::out() << "MagicMethodClass::__compare is called" << std::endl;
   if (m_length < object.m_length) {
      return -1;
   } else if (m_length == object.m_length) {
      return 0;
   } else {
      return 1;
   }
}

void MagicMethodClass::__clone()
{
   polar::vmapi::out() << "MagicMethodClass::__clone is called" << std::endl;
}

Variant MagicMethodClass::getTeamWebsite()
{
   return "polarphp.org";
}

Variant MagicMethodClass::__callStatic(const std::string &method, Parameters &params)
{
   polar::vmapi::out() << "MagicMethodClass::__callStatic is called" << std::endl;
   if (method == "staticCalculateSum") {
      NumericVariant sum;
      for (size_t i = 0; i < params.size(); i++) {
         sum += params.at<NumericVariant>(i);
      }
      return sum;
   } else {
      StringVariant str("hello, ");
      str += params.at<StringVariant>(0);
      return str;
   }
}

std::string MagicMethodClass::serialize()
{
   polar::vmapi::out() << "MagicMethodClass::serialize is called" << std::endl;
   return "serialize data";
}

void MagicMethodClass::unserialize(const char *input, size_t size)
{
   polar::vmapi::out() << "MagicMethodClass::unserialize is called" << std::endl;
   polar::vmapi::out() << "serialize data : " << input << std::endl;
}

MagicMethodClass::~MagicMethodClass() noexcept
{}

// for properties test
void PropsTestClass::setAge(const Variant &value)
{
   NumericVariant age(value);
   age += 1;
   m_age = age.toLong();
}

Variant PropsTestClass::getAge()
{
   return m_age;
}

void PropsTestClass::setName(const Variant &name)
{
   StringVariant str(name);
   str.prepend("polarphp:");
   m_name = str.toString();
}

Variant PropsTestClass::getName()
{
   return m_name;
}

Variant ObjectVariantClass::__invoke(Parameters &args) const
{
   polar::vmapi::out() << "ObjectVariantClass::__invoke invoked" << std::endl;
   StringVariant str(args.retrieveAsVariant(0).getUnDerefZvalPtr(), true);
   NumericVariant result;
   for (size_t i = 1; i < args.size(); i++) {
      result += args.at<NumericVariant>(i);
   }
   str = "polarphp";
   return str;
}

void ObjectVariantClass::forwardInvoke()
{
   ObjectVariant obj("ObjectVariantClass", std::make_shared<ObjectVariantClass>());
   Variant str("xxx");
   polar::vmapi::out() << "begin invoke ObjectVariant::classInvoke : the text is xxx" << std::endl;
   Variant result = obj(str.makeReferenceByZval(), 123, 456, 222);
   polar::vmapi::out() << "after invoke ObjectVariant::classInvoke : this text is " << result << std::endl;
   ObjectVariant obj1("NonMagicMethodClass", std::make_shared<NonMagicMethodClass>());
  ///  obj1(1, 2); // Fatal error: Function name must be a string
}

void ObjectVariantClass::testInstanceOf()
{
   ObjectVariant objA("A", std::make_shared<A>());
   ObjectVariant objB("B", std::make_shared<B>());
   ObjectVariant objC("C", std::make_shared<C>());
   if (objA.instanceOf("A") && objA.instanceOf(objA)) {
      polar::vmapi::out() << "A is instance of A" << std::endl;
   }
   if (objB.instanceOf("B") && objB.instanceOf(objB)) {
      polar::vmapi::out() << "B is instance of B" << std::endl;
   }
   if (objC.instanceOf("C") && objC.instanceOf(objC)) {
      polar::vmapi::out() << "C is instance of C" << std::endl;
   }
   if (objB.instanceOf("A") && objB.instanceOf(objA)) {
      polar::vmapi::out() << "B is instance of A" << std::endl;
   }
   if (objC.instanceOf("B") && objC.instanceOf(objB)) {
      polar::vmapi::out() << "C is instance of B" << std::endl;
   }
   if (objC.instanceOf("A") && objC.instanceOf(objA)) {
      polar::vmapi::out() << "C is instance of A" << std::endl;
   }

   if (!objA.instanceOf("B") && !objA.instanceOf(objB)) {
      polar::vmapi::out() << "A is not instance of B" << std::endl;
   }
   if (!objB.instanceOf("C") && !objB.instanceOf(objC)) {
      polar::vmapi::out() << "C is not instance of B" << std::endl;
   }
   if (!objA.instanceOf("C") && !objA.instanceOf(objC)) {
      polar::vmapi::out() << "C is not instance of A" << std::endl;
   }
}

void ObjectVariantClass::testDerivedFrom()
{
   ObjectVariant objA("A", std::make_shared<A>());
   ObjectVariant objB("B", std::make_shared<B>());
   ObjectVariant objC("C", std::make_shared<C>());
   if (!objA.derivedFrom("A") && !objA.derivedFrom(objA)) {
      polar::vmapi::out() << "A is not derived from A" << std::endl;
   }
   if (!objB.derivedFrom("B") && !objB.derivedFrom(objB)) {
      polar::vmapi::out() << "B is not derived from B" << std::endl;
   }
   if (!objC.derivedFrom("C") && !objC.derivedFrom(objC)) {
      polar::vmapi::out() << "C is not derived from C" << std::endl;
   }
   if (objB.derivedFrom("A") && objB.derivedFrom(objA)) {
      polar::vmapi::out() << "B is derived from A" << std::endl;
   }
   if (objC.derivedFrom("B") && objC.derivedFrom(objB)) {
      polar::vmapi::out() << "C is derived from B" << std::endl;
   }
   if (objC.derivedFrom("A") && objC.derivedFrom(objA)) {
      polar::vmapi::out() << "C is derived from A" << std::endl;
   }

   if (!objA.derivedFrom("B") && !objA.derivedFrom(objB)) {
      polar::vmapi::out() << "A is not derived from B" << std::endl;
   }
   if (!objB.derivedFrom("C") && !objB.derivedFrom(objC)) {
      polar::vmapi::out() << "C is not derived from B" << std::endl;
   }
   if (!objA.derivedFrom("C") && !objA.derivedFrom(objC)) {
      polar::vmapi::out() << "C is not derived from A" << std::endl;
   }
}

void ObjectVariantClass::testNoArgCall()
{
   ObjectVariant obj("ObjectVariantClass", std::make_shared<ObjectVariantClass>());
   obj.call("printName");
   StringVariant ret = obj.call("getName");
   polar::vmapi::out() << "the result of ObjectVariantClass::getName is " << ret << std::endl;
}

void ObjectVariantClass::testVarArgsCall()
{
   ObjectVariant obj("ObjectVariantClass", std::make_shared<ObjectVariantClass>());
   obj.call("printSum", 12, 12, 12);
   Variant ret = obj.call("calculateSum", 1, 2, 4);
   polar::vmapi::out() << "the result of ObjectVariantClass::calculateSum is " << ret << std::endl;
   // test ref arg
   Variant str("polarphp");
   polar::vmapi::out() << "before call by ref arg " << str << std::endl;
   obj.call("changeNameByRef", str.makeReferenceByZval());
   polar::vmapi::out() << "after call by ref arg " << str << std::endl;
}

void ObjectVariantClass::printName()
{
   polar::vmapi::out() << "ObjectVariantClass::printName been called" << std::endl;
}

std::string ObjectVariantClass::getName()
{
   polar::vmapi::out() << "ObjectVariantClass::getName been called" << std::endl;
   return "hello, polarphp";
}

void ObjectVariantClass::printSum(Parameters &args)
{
   polar::vmapi::out() << "ObjectVariantClass::printSum been called" << std::endl;
   polar::vmapi::out() << "got " << args.size() << " args" << std::endl;
   NumericVariant result;
   for (size_t i = 0; i < args.size(); ++i) {
      result += args.at<NumericVariant>(i);
   }
   polar::vmapi::out() << "the result is " << result << std::endl;
}

int ObjectVariantClass::calculateSum(Parameters &args)
{
   polar::vmapi::out() << "ObjectVariantClass::calculateSum been called" << std::endl;
   polar::vmapi::out() << "got " << args.size() << " args" << std::endl;
   NumericVariant result;
   for (size_t i = 0; i < args.size(); ++i) {
      result += args.at<NumericVariant>(i);
   }
   polar::vmapi::out() << "the result is " << result << std::endl;
   return result;
}


void ObjectVariantClass::changeNameByRef(Parameters &args)
{
   polar::vmapi::out() << "ObjectVariantClass::changeNameByRef been called" << std::endl;
   StringVariant &name = args.at<StringVariant>(0);
   if (name.getUnDerefType() == Type::Reference) {
      polar::vmapi::out() << "get ref arg" << std::endl;
   }
   name = "hello, polarphp";
}

} // php
