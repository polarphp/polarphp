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

#ifndef POLARPHP_STDLIBMOCK_NATIVE_FUNCTIONS_H
#define POLARPHP_STDLIBMOCK_NATIVE_FUNCTIONS_H

namespace polar {
namespace vmapi {
class NumericVariant;
class Variant;
class StringVariant;
class Parameters;
} // vmapi
} // polar

namespace php {

using polar::vmapi::NumericVariant;
using polar::vmapi::Variant;
using polar::vmapi::StringVariant;
using polar::vmapi::Parameters;

Variant return_arg(Parameters &args);
void show_something();
Variant get_name();
void get_value_ref(Parameters &args);
void passby_value(Parameters &args);
void print_sum(Parameters &args);
void print_name(Parameters &args);
void print_name_and_age(Parameters &args);
Variant calculate_sum(Parameters &args);
Variant add_two_number(Parameters &args);
void say_hello(Parameters &args);
Variant print_something();
Variant have_ret_and_have_arg(Parameters &params);

} // php

#endif // POLARPHP_STDLIBMOCK_NATIVE_FUNCTIONS_H
