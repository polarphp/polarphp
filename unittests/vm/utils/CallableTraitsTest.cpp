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
#include "polarphp/vm/utils/CallableTraits.h"

#include <string>
#include <iostream>
#include <type_traits>

void func_without_params()
{

}

int func_with_two_params(int arg1, int arg2)
{
   return arg1 + arg2;
}

struct Data
{
   std::string processData(std::string data)
   {
      return "welcome: " + data;
   }
};

namespace somenamespace {
void print_info()
{
   std::cout << "hello, polarphp" << std::endl;
}

std::string process_info(int age, std::string address)
{
   std::cout << "age: " << age << " adderss: " << address << std::endl;
   return address;
}
} // somenamespace

using polar::vmapi::CallableInfoTrait;

class PersonInfo
{
public:
   std::string getName()
   {
      return "polarphp";
   }
   void static printAddress()
   {
      std::cout << std::endl;
   }
};

TEST(CallableTraitsTest, testBasic)
{
   ASSERT_TRUE((std::is_same<CallableInfoTrait<decltype(func_without_params)>::ReturnType, void>::value));
   ASSERT_FALSE((CallableInfoTrait<decltype(func_without_params)>::hasReturn));
   ASSERT_TRUE((std::is_same<CallableInfoTrait<decltype(func_with_two_params)>::ReturnType, int>::value));
   ASSERT_TRUE((CallableInfoTrait<decltype(func_with_two_params)>::hasReturn));
   /// test namespace functions
   ASSERT_TRUE((std::is_same<CallableInfoTrait<decltype(somenamespace::print_info)>::ReturnType, void>::value));
   ASSERT_FALSE((CallableInfoTrait<decltype(somenamespace::print_info)>::hasReturn));
   ASSERT_TRUE((std::is_same<CallableInfoTrait<decltype(somenamespace::print_info)>::ReturnType, void>::value));
   ASSERT_TRUE((CallableInfoTrait<decltype(somenamespace::process_info)>::hasReturn));
   /// get args number
   ASSERT_EQ(CallableInfoTrait<decltype(func_without_params)>::argNum, 0);
   ASSERT_EQ(CallableInfoTrait<decltype(func_with_two_params)>::argNum, 2);
   ASSERT_EQ(CallableInfoTrait<decltype(somenamespace::print_info)>::argNum, 0);
   ASSERT_EQ(CallableInfoTrait<decltype(somenamespace::process_info)>::argNum, 2);

   ASSERT_TRUE((std::is_same_v<CallableInfoTrait<decltype(somenamespace::process_info)>::arg<0>::type, int>));
   ASSERT_TRUE((std::is_same_v<CallableInfoTrait<decltype(somenamespace::process_info)>::arg<1>::type, std::string>));
}
