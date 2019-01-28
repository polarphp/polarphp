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

#include "polarphp/vm/lang/Module.h"
#include "polarphp/vm/lang/Namespace.h"
#include "polarphp/vm/lang/Argument.h"
#include "NativeFunctions.h"
#include "Functions.h"

namespace php {

using polar::vmapi::Namespace;
using polar::vmapi::ValueArgument;
using polar::vmapi::RefArgument;
using polar::vmapi::VariadicArgument;
using polar::vmapi::Type;

void register_functions_hook(Module &module)
{
   module.registerFunction<decltype(php::show_something), php::show_something>("show_something");
   module.registerFunction<decltype(php::get_name), php::get_name>("get_name");
   module.registerFunction<decltype(php::print_name), php::print_name>
         ("print_name", {
             ValueArgument("name", Type::String)
          });
   module.registerFunction<decltype(php::print_name_and_age), php::print_name_and_age>
         ("print_name_and_age", {
             ValueArgument("name", Type::String),
             ValueArgument("age", Type::Long)
          });
   module.registerFunction<decltype(php::add_two_number), php::add_two_number>
         ("add_two_number", {
             ValueArgument("num1", Type::Long),
             ValueArgument("num2", Type::Long)
          });
   module.registerFunction<decltype(php::return_arg), php::return_arg>
         ("return_arg", {
             ValueArgument("number1"),
          });
   // for passby value and reference test
   module.registerFunction<decltype(php::get_value_ref), php::get_value_ref>
         ("get_value_ref", {
             RefArgument("number", Type::Numeric),
          });
   module.registerFunction<decltype(php::passby_value), php::passby_value>
         ("passby_value", {
             ValueArgument("number", Type::Numeric),
          });

   // test for default arguments
   module.registerFunction<decltype(php::say_hello), php::say_hello>
         ("say_hello", {
             ValueArgument("name", Type::String, false)
          });

   // register for namespace
   Namespace *php = module.findNamespace("php");
   Namespace *io = php->findNamespace("io");

   php->registerFunction<decltype(php::get_name), php::get_name>("get_name");
   php->registerFunction<decltype(php::show_something), php::show_something>("show_something");

   io->registerFunction<decltype(php::calculate_sum), php::calculate_sum>
         ("calculate_sum", {
             VariadicArgument("numbers")
          });
   io->registerFunction<decltype(php::print_name), php::print_name>
         ("print_name", {
             ValueArgument("name", Type::String)
          });

   // for test varidic
   io->registerFunction<decltype(php::print_sum), php::print_sum>
         ("print_sum", {
             VariadicArgument("numbers")
          });
   io->registerFunction<decltype(php::show_something), php::show_something>("show_something");
   io->registerFunction<decltype(php::print_something), php::print_something>("print_something");
}

} // php
