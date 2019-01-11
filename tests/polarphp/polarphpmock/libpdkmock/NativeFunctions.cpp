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
   polar::vmapi::out() << "hello world, polarphp" << std::flush;
}

void get_value_ref(Parameters &args)
{
   args.at<NumericVariant>(0) = 321;
}

void passby_value(Parameters &args)
{
   // have no effect
   args.at<NumericVariant>(0) = 321;
}

Variant get_name()
{
   return "polarboy";
}

void print_name(Parameters &args)
{
   polar::vmapi::out() << args.at<StringVariant>(0) << std::flush;
}

void print_sum(Parameters &args)
{
   NumericVariant result;
   for (size_t i = 0; i < args.size(); ++i) {
      result += args.at<NumericVariant>(i);
   }
   polar::vmapi::out() << result << std::flush;
}

Variant calculate_sum(Parameters &args)
{
   NumericVariant result;
   for (size_t i = 0; i < args.size(); ++i) {
      result += args.at<NumericVariant>(i);
   }
   return result;
}

void print_name_and_age(Parameters &args)
{
   polar::vmapi::out() << "name: " << args.at<StringVariant>(0)
                       << " age: " << args.at<NumericVariant>(1) << std::flush;
}

Variant add_two_number(Parameters &args)
{
   NumericVariant &num1 = args.at<NumericVariant>(0);
   NumericVariant &num2 = args.at<NumericVariant>(1);
   return num1 + num2;
}

void say_hello(Parameters &args)
{
   std::string name;
   if (args.size() == 0) {
      name = "polarphp";
   } else {
      name = args.at<StringVariant>(0).toString();
   }
   polar::vmapi::out() << "hello, " << name << std::endl;
}

Variant return_arg(Parameters &args)
{
   return args.retrieveAsVariant(0);
}

// for test closure
Variant print_something()
{
   polar::vmapi::out() << "print_something called" << std::endl;
   return "print_some";
}

Variant have_ret_and_have_arg(Parameters &args)
{
   polar::vmapi::out() << "have_ret_and_have_arg called" << std::endl;
   if (args.empty()) {
      return "have_ret_and_have_arg";
   } else {
      return args.retrieveAsVariant(0);
   }
}

} // php
