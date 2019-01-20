// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 polarboy <polarboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/02/07.

#include "polarphp/vm/lang/Module.h"
#include "polarphp/vm/lang/Class.h"
#include "polarphp/vm/lang/Constant.h"
#include "polarphp/vm/lang/Namespace.h"
#include "Classes.h"
#include "NativeClasses.h"

namespace php {

using polar::vmapi::Modifier;
using polar::vmapi::Constant;
using polar::vmapi::ValueArgument;
using polar::vmapi::VariadicArgument;
using polar::vmapi::RefArgument;
using polar::vmapi::Namespace;
using polar::vmapi::Interface;
using polar::vmapi::Type;
using polar::vmapi::Module;
using polar::vmapi::Class;
using polar::vmapi::ClassType;

namespace {

void register_basic_classes(Module &module);
void register_construct_and_destruct_classes(Module &module);
void register_namespace_classes(Module &module);
void register_inherit_test_classes(Module &module);
void register_iterator_test_classes(Module &module);
void register_closure_test_classes(Module &module);
void register_visibility_test_classes(Module &module);
void register_magic_method_test_classes(Module &module);
void register_props_test_classes(Module &module);
void register_object_variant_test_classes(Module &module);

} // anonymous namespace

void register_classes_hook(Module &module)
{
   register_basic_classes(module);
   register_construct_and_destruct_classes(module);
   register_namespace_classes(module);
   register_inherit_test_classes(module);
   register_iterator_test_classes(module);
   register_closure_test_classes(module);
   register_visibility_test_classes(module);
   register_magic_method_test_classes(module);
   register_props_test_classes(module);
   register_object_variant_test_classes(module);
}

namespace {

void register_basic_classes(Module &module)
{
   Class<Person> personClass("Person");
   personClass.registerConstant("POLARPHP_TEAM", "beijing polarphp team");
   personClass.registerConstant("MY_CONST", "MY_CONST_VALUE");
   personClass.registerConstant(Constant("PI", 3.1415926));
   personClass.registerConstant("HEADER_SIZE", 123);
   personClass.registerConstant("ALLOW_ACL", true);
   personClass.registerProperty("name", "polarboy");
   personClass.registerProperty("staticProp", "beijing", Modifier::Public | Modifier::Static);
   personClass.registerMethod<decltype(&Person::showName), &Person::showName>("showName");
   personClass.registerMethod<decltype(&Person::print_sum), &Person::print_sum>
         ("print_sum",{
             VariadicArgument("numbers")
          });
   personClass.registerMethod<decltype(&Person::setAge), &Person::setAge>
         ("setAge", {
             ValueArgument("age", Type::Long)
          });
   personClass.registerMethod<decltype(&Person::getAge), &Person::getAge>("getAge");
   personClass.registerMethod<decltype(&Person::getName), &Person::getName>("getName");
   personClass.registerMethod<decltype(&Person::addTwoNum), &Person::addTwoNum>
         ("addTwoNum", {
             ValueArgument("num1", Type::Numeric),
             ValueArgument("num2", Type::Numeric)
          });
   personClass.registerMethod<decltype(&Person::addSum), &Person::addSum>
         ("addSum",{
             VariadicArgument("numbers")
          });
   personClass.registerMethod<decltype(&Person::protectedMethod), &Person::protectedMethod>
         ("protectedMethod", Modifier::Protected);
   personClass.registerMethod<decltype(&Person::privateMethod), &Person::privateMethod>
         ("privateMethod", Modifier::Private);

   personClass.registerMethod<decltype(&Person::concatStr), &Person::concatStr>
         ("concatStr", {
             ValueArgument("lhs", Type::String),
             ValueArgument("rhs", Type::String)
          });
   personClass.registerMethod<decltype(&Person::staticShowName), &Person::staticShowName>("staticShowName");
   personClass.registerMethod<decltype(&Person::staticProtectedMethod), &Person::staticProtectedMethod>
         ("staticProtectedMethod", Modifier::Protected);
   personClass.registerMethod<decltype(&Person::staticPrivateMethod), &Person::staticPrivateMethod>
         ("staticPrivateMethod", Modifier::Private);
   personClass.registerMethod<decltype(&Person::makeNewPerson), &Person::makeNewPerson>("makeNewPerson");

   module.registerClass(personClass);
}

void register_construct_and_destruct_classes(Module &module)
{
   Class<ConstructAndDestruct> ConstructAndDestruct("ConstructAndDestruct");
   ConstructAndDestruct.registerMethod
         <decltype(&ConstructAndDestruct::__construct), &ConstructAndDestruct::__construct>("__construct");
   ConstructAndDestruct.registerMethod
         <decltype(&ConstructAndDestruct::__destruct), &ConstructAndDestruct::__construct>("__destruct");
   module.registerClass(ConstructAndDestruct);
}

void register_namespace_classes(Module &module)
{
   Class<Address> addressClass("Address");
   Namespace *php = module.findNamespace("php");
   php->registerClass(addressClass);
   Class<EmptyClass> emptyCls("EmptyClass");
   php->registerClass(emptyCls);
}

void register_inherit_test_classes(Module &module)
{
   Interface interfaceA("InterfaceA");
   Interface interfaceB("InterfaceB");
   Interface interfaceC("InterfaceC");
   interfaceA.registerMethod("methodOfA");
   interfaceA.registerMethod("protectedMethodOfA", Modifier::Protected);
   interfaceA.registerMethod("privateMethodOfA", Modifier::Private);
   interfaceB.registerMethod("methodOfB");
   interfaceB.registerMethod("protectedMethodOfB", Modifier::Protected);
   interfaceB.registerMethod("privateMethodOfB", Modifier::Private);
   interfaceC.registerMethod("methodOfC");
   interfaceC.registerMethod("protectedMethodOfC", Modifier::Protected);
   interfaceC.registerMethod("privateMethodOfC", Modifier::Private);

   interfaceC.registerBaseInterface(interfaceB);
   interfaceB.registerBaseInterface(interfaceA);
   module.registerInterface(interfaceA);
   module.registerInterface(interfaceB);
   module.registerInterface(interfaceC);

   Class<A> a("A");
   Class<B> b("B");
   Class<C> c("C");
   a.registerMethod<decltype(&A::printInfo), &A::printInfo>("printInfo");
   a.registerMethod<decltype(&A::changeNameByRef), &A::changeNameByRef>
         ("changeNameByRef", {
             RefArgument("name", Type::String)
          });
   a.registerMethod<decltype(&A::privateAMethod), &A::privateAMethod>("privateAMethod", Modifier::Private);
   a.registerMethod<decltype(&A::protectedAMethod), &A::protectedAMethod>("protectedAMethod", Modifier::Protected);
   a.registerProperty("name", "polarphp");
   a.registerProperty("protectedName", "protected polarphp", Modifier::Protected);
   a.registerProperty("privateName", "private polarphp", Modifier::Private);
   b.registerMethod<decltype(&B::privateBMethod), &B::privateBMethod>("privateBMethod", Modifier::Private);
   b.registerMethod<decltype(&B::protectedBMethod), &B::protectedBMethod>("protectedBMethod", Modifier::Protected);
   b.registerMethod<decltype(&B::printInfo), &B::printInfo>("printInfo");
   b.registerMethod<decltype(&B::showSomething), &B::showSomething>("showSomething");
   b.registerMethod<decltype(&B::calculateSumByRef), &B::calculateSumByRef>
         ("calculateSumByRef", {
             RefArgument("result", Type::Long),
             VariadicArgument("numbers")
          });
   b.registerMethod<decltype(&B::addTwoNumber), &B::addTwoNumber>
         ("addTwoNumber", {
             ValueArgument("lhs"),
             ValueArgument("rhs")
          });
   b.registerProperty("propsFromB", "polarphp team", Modifier::Protected);
   c.registerMethod<decltype(&C::printInfo), &C::printInfo>("printInfo");
   c.registerMethod<decltype(&C::testCallParentPassRefArg), &C::testCallParentPassRefArg>("testCallParentPassRefArg");
   c.registerMethod<decltype(&C::testCallParentWithReturn), &C::testCallParentWithReturn>("testCallParentWithReturn");
   c.registerMethod<decltype(&C::testGetObjectVaraintPtr), &C::testGetObjectVaraintPtr>("testGetObjectVaraintPtr");
   c.registerMethod<decltype(&C::privateCMethod), &C::privateCMethod>("privateCMethod", Modifier::Private);
   c.registerMethod<decltype(&C::protectedCMethod), &C::protectedCMethod>("protectedCMethod", Modifier::Protected);
   c.registerMethod<decltype(&C::methodOfA), &C::methodOfA>("methodOfA", Modifier::Public);
   c.registerMethod<decltype(&C::protectedMethodOfA), &C::protectedMethodOfA>("protectedMethodOfA", Modifier::Public);
   c.registerMethod<decltype(&C::privateMethodOfA), &C::privateMethodOfA>("privateMethodOfA", Modifier::Public);
   c.registerProperty("address", "beijing", Modifier::Private);
   b.registerBaseClass(a);
   c.registerBaseClass(b);
   c.registerInterface(interfaceA);
   module.registerClass(a);
   module.registerClass(b);
   module.registerClass(c);
}

void register_iterator_test_classes(Module &module)
{
   Class<IterateTestClass> iterateTestClass("IterateTestClass");
   module.registerClass(iterateTestClass);
}

void register_closure_test_classes(Module &module)
{
   Class<ClosureTestClass> closureTestClass("ClosureTestClass");
   closureTestClass.registerMethod<decltype(&ClosureTestClass::testClosureCallable), &ClosureTestClass::testClosureCallable>("testClosureCallable");
   closureTestClass.registerMethod<decltype(&ClosureTestClass::getNoArgAndReturnCallable), &ClosureTestClass::getNoArgAndReturnCallable>("getNoArgAndReturnCallable");
   closureTestClass.registerMethod<decltype(&ClosureTestClass::getArgAndReturnCallable), &ClosureTestClass::getArgAndReturnCallable>("getArgAndReturnCallable");
   module.registerClass(closureTestClass);
}

void register_visibility_test_classes(Module &module)
{
   Class<VisibilityClass> visibilityClass("VisibilityClass");
   visibilityClass.registerMethod<decltype(&VisibilityClass::publicMethod), &VisibilityClass::publicMethod>("publicMethod", Modifier::Public);
   visibilityClass.registerMethod<decltype(&VisibilityClass::protectedMethod), &VisibilityClass::protectedMethod>("protectedMethod", Modifier::Protected);
   visibilityClass.registerMethod<decltype(&VisibilityClass::privateMethod), &VisibilityClass::privateMethod>("privateMethod", Modifier::Private);
   visibilityClass.registerMethod<decltype(&VisibilityClass::finalMethod), &VisibilityClass::finalMethod>("finalMethod", Modifier::Final);
   // register some property
   visibilityClass.registerProperty("publicProp", "propValue", Modifier::Public);
   visibilityClass.registerProperty("protectedProp", "propValue", Modifier::Protected);
   visibilityClass.registerProperty("privateProp", "propValue", Modifier::Private);

   Class<FinalTestClass> finalTestClass("FinalTestClass", ClassType::Final);
   finalTestClass.registerMethod<decltype(&FinalTestClass::someMethod), &FinalTestClass::someMethod>("someMethod");
   module.registerClass(visibilityClass);
   module.registerClass(finalTestClass);
}

void register_magic_method_test_classes(Module &module)
{
   Class<NonMagicMethodClass> nonMagicMethodClass("NonMagicMethodClass");
   Class<MagicMethodClass> magicMethodClass("MagicMethodClass");
   magicMethodClass.registerProperty("teamWebsite", &MagicMethodClass::getTeamWebsite);
   module.registerClass(nonMagicMethodClass);
   module.registerClass(magicMethodClass);
}

void register_props_test_classes(Module &module)
{
   Class<PropsTestClass> propsTestClass("PropsTestClass");
   propsTestClass.registerProperty("nullProp", nullptr);
   propsTestClass.registerProperty("trueProp", true);
   propsTestClass.registerProperty("falseProp", false);
   propsTestClass.registerProperty("numProp", 2017);
   propsTestClass.registerProperty("doubleProp", 3.1415);
   propsTestClass.registerProperty("strProp", "polarphp");
   propsTestClass.registerProperty("str1Prop", std::string("polarphp"));
   propsTestClass.registerProperty("staticNullProp", nullptr, Modifier::Static);
   propsTestClass.registerProperty("staticTrueProp", true, Modifier::Static);
   propsTestClass.registerProperty("staticFalseProp", false, Modifier::Static);
   propsTestClass.registerProperty("staticNumProp", 2012, Modifier::Static);
   propsTestClass.registerProperty("staticDoubleProp", 3.1415, Modifier::Static);
   propsTestClass.registerProperty("staticStrProp", "static polarphp", Modifier::Static);
   propsTestClass.registerProperty("staticStr1Prop", std::string("static polarphp"), Modifier::Static);

   propsTestClass.registerProperty("MATH_PI", 3.14, Modifier::Const);

   propsTestClass.registerProperty("name", &PropsTestClass::getName, &PropsTestClass::setName);
   propsTestClass.registerProperty("age", &PropsTestClass::getAge, &PropsTestClass::setAge);

   module.registerClass(propsTestClass);
}

void register_object_variant_test_classes(Module &module)
{
   Class<ObjectVariantClass> objectVariantClass("ObjectVariantClass");
   objectVariantClass.registerMethod<decltype(&ObjectVariantClass::forwardInvoke), &ObjectVariantClass::forwardInvoke>("forwardInvoke");
   objectVariantClass.registerMethod<decltype(&ObjectVariantClass::testDerivedFrom), &ObjectVariantClass::testDerivedFrom>("testDerivedFrom");
   objectVariantClass.registerMethod<decltype(&ObjectVariantClass::testInstanceOf), &ObjectVariantClass::testInstanceOf>("testInstanceOf");
   objectVariantClass.registerMethod<decltype(&ObjectVariantClass::testNoArgCall), &ObjectVariantClass::testNoArgCall>("testNoArgCall");
   objectVariantClass.registerMethod<decltype(&ObjectVariantClass::testVarArgsCall), &ObjectVariantClass::testVarArgsCall>("testVarArgsCall");
   objectVariantClass.registerMethod<decltype(&ObjectVariantClass::printName), &ObjectVariantClass::printName>("printName");
   objectVariantClass.registerMethod<decltype(&ObjectVariantClass::getName), &ObjectVariantClass::getName>("getName");
   objectVariantClass.registerMethod<decltype(&ObjectVariantClass::printSum), &ObjectVariantClass::printSum>
         ("printSum", {
             VariadicArgument("nums")
          });
   objectVariantClass.registerMethod<decltype(&ObjectVariantClass::calculateSum), &ObjectVariantClass::calculateSum>
         ("calculateSum",{
             VariadicArgument("nums")
          });
   objectVariantClass.registerMethod<decltype(&ObjectVariantClass::changeNameByRef), &ObjectVariantClass::changeNameByRef>
         ("changeNameByRef", {
             RefArgument("name", Type::String)
          });

   module.registerClass(objectVariantClass);
}

} // anonymous namespace

} // php
