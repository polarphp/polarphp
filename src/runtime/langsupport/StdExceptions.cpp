// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/01/15.

#include "polarphp/runtime/langsupport/StdExceptions.h"
#include "polarphp/runtime/langsupport/ClassLoader.h"

namespace polar {
namespace runtime {

zend_class_entry *g_LogicException = nullptr;
zend_class_entry *g_BadFunctionCallException = nullptr;
zend_class_entry *g_BadMethodCallException = nullptr;
zend_class_entry *g_DomainException = nullptr;
zend_class_entry *g_InvalidArgumentException = nullptr;
zend_class_entry *g_LengthException = nullptr;
zend_class_entry *g_OutOfRangeException = nullptr;

zend_class_entry *g_RuntimeException = nullptr;
zend_class_entry *g_OutOfBoundsException = nullptr;
zend_class_entry *g_OverflowException = nullptr;
zend_class_entry *g_RangeException = nullptr;
zend_class_entry *g_UnderflowException = nullptr;
zend_class_entry *g_UnexpectedValueException = nullptr;

#define g_Exception zend_ce_exception

PHP_MINIT_FUNCTION(stdexceptions)
{
   RT_REGISTER_SUB_CLASS_EX(LogicException,           Exception,        nullptr, nullptr);
   RT_REGISTER_SUB_CLASS_EX(BadFunctionCallException, LogicException,   nullptr, nullptr);
   RT_REGISTER_SUB_CLASS_EX(BadMethodCallException,   BadFunctionCallException,   nullptr, nullptr);
   RT_REGISTER_SUB_CLASS_EX(DomainException,          LogicException,   nullptr, nullptr);
   RT_REGISTER_SUB_CLASS_EX(InvalidArgumentException, LogicException,   nullptr, nullptr);
   RT_REGISTER_SUB_CLASS_EX(LengthException,          LogicException,   nullptr, nullptr);
   RT_REGISTER_SUB_CLASS_EX(OutOfRangeException,      LogicException,   nullptr, nullptr);

   RT_REGISTER_SUB_CLASS_EX(RuntimeException,         Exception,        nullptr, nullptr);
   RT_REGISTER_SUB_CLASS_EX(OutOfBoundsException,     RuntimeException, nullptr, nullptr);
   RT_REGISTER_SUB_CLASS_EX(OverflowException,        RuntimeException, nullptr, nullptr);
   RT_REGISTER_SUB_CLASS_EX(RangeException,           RuntimeException, nullptr, nullptr);
   RT_REGISTER_SUB_CLASS_EX(UnderflowException,       RuntimeException, nullptr, nullptr);
   RT_REGISTER_SUB_CLASS_EX(UnexpectedValueException, RuntimeException, nullptr, nullptr);

   return SUCCESS;
}

#undef g_exception

} // runtime
} // polar
