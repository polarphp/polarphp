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

#include "NativeFunctions.h"
#include "polarphp/vm/lang/Parameter.h"
#include "polarphp/vm/ds/NumericVariant.h"
#include "polarphp/vm/ds/StringVariant.h"
#include "polarphp/vm/ds/Variant.h"

namespace php {


void show_something()
{
   polar::vmapi::out() << "hello world, zapi" << std::flush;
}

void get_value_ref(NumericVariant &number)
{
   number = 321;
}

void passby_value(NumericVariant &number)
{
   // have no effect
   number = 321;
}

Variant get_name()
{
   return "polarboy";
}

void print_name(const StringVariant &name)
{
   polar::vmapi::out() << name << std::flush;
}

void print_sum(NumericVariant argQuantity, ...)
{
   va_list args;
   va_start(args, argQuantity);
   NumericVariant result;
   for (int i = 0; i < argQuantity; ++i) {
      result += NumericVariant(va_arg(args, polar::vmapi::VmApiVaridicItemType), false);
   }
   polar::vmapi::out() << result << std::flush;
}

Variant calculate_sum(NumericVariant argQuantity, ...)
{
   va_list args;
   va_start(args, argQuantity);
   NumericVariant result;
   for (int i = 0; i < argQuantity; ++i) {
      result += NumericVariant(va_arg(args, polar::vmapi::VmApiVaridicItemType), false);
   }
   return result;
}

void print_name_and_age(const StringVariant &name, const NumericVariant &age)
{
   polar::vmapi::out() << "name: " << name << " age: " << age << std::flush;
}

Variant add_two_number(const NumericVariant &num1, const NumericVariant &num2)
{
   return num1 + num2;
}

void say_hello(StringVariant &name)
{
   if (name.getSize() == 0) {
      name = "zapi";
   }
   polar::vmapi::out() << "hello, " << name << std::endl;
}

Variant return_arg(Variant &value)
{
   return value;
}

// for test closure
Variant print_something()
{
   polar::vmapi::out() << "print_something called" << std::endl;
   return "print_some";
}

Variant have_ret_and_have_arg(Parameters &params)
{
   polar::vmapi::out() << "have_ret_and_have_arg called" << std::endl;
   if (params.empty()) {
      return "have_ret_and_have_arg";
   } else {
      return params.at(0);
   }
}

} // php
