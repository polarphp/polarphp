// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/24.

#include "polarphp/vm/lang/Constant.h"
#include "polarphp/vm/internal/DepsZendVmHeaders.h"

#include <string>
#include <iostream>
#include <cstring>

namespace polar {
namespace vmapi {

namespace internal
{

class ConstantPrivate
{
public:
   ConstantPrivate(const char *name)
      : m_name(name)
   {}
   ~ConstantPrivate();
   void initialize(const std::string &prefix, int moduleNumber);
   std::string m_name;
   zend_constant m_constant;
   bool m_initialized = false;
};

void ConstantPrivate::initialize(const std::string &prefix, int moduleNumber)
{
   if (!prefix.empty()) {
      m_constant.name = zend_string_alloc(prefix.size() + 1 + m_name.size(), 1);
      std::strncpy(ZSTR_VAL(m_constant.name), prefix.c_str(), prefix.size());
      std::strncpy(ZSTR_VAL(m_constant.name) + prefix.size(), "\\", 1);
      std::strncpy(ZSTR_VAL(m_constant.name) + prefix.size() + 1, m_name.c_str(), m_name.size() + 1);
   } else {
      m_constant.name = zend_string_init(m_name.c_str(), m_name.size(), 1);
   }
   m_constant.flags = CONST_CS | CONST_PERSISTENT;
   m_constant.module_number = moduleNumber;
   zval_add_ref(&m_constant.value);
   zend_register_constant(&m_constant);
   m_initialized = true;
}

ConstantPrivate::~ConstantPrivate()
{
   zval_ptr_dtor(&m_constant.value);
}

} // internal

using internal::ConstantPrivate;

Constant::Constant(const char *name, std::nullptr_t)
   : m_implPtr(new ConstantPrivate(name))
{
   VMAPI_D(Constant);
   ZVAL_NULL(&implPtr->m_constant.value);
}

Constant::Constant(const char *name, bool value)
   : m_implPtr(new ConstantPrivate(name))
{
   VMAPI_D(Constant);
   ZVAL_BOOL(&implPtr->m_constant.value, value);
}

Constant::Constant(const char *name, int32_t value)
   : m_implPtr(new ConstantPrivate(name))
{
   VMAPI_D(Constant);
   ZVAL_LONG(&implPtr->m_constant.value, value);
}

Constant::Constant(const char *name, int64_t value)
   : m_implPtr(new ConstantPrivate(name))
{
   VMAPI_D(Constant);
   ZVAL_LONG(&implPtr->m_constant.value, value);
}

Constant::Constant(const char *name, double value)
   : m_implPtr(new ConstantPrivate(name))
{
   VMAPI_D(Constant);
   ZVAL_DOUBLE(&implPtr->m_constant.value, value);
}

Constant::Constant(const char *name, const char *value)
   : m_implPtr(new ConstantPrivate(name))
{
   VMAPI_D(Constant);
   ZVAL_PSTRINGL(&implPtr->m_constant.value, value, ::strlen(value));
}

Constant::Constant(const char *name, const char *value, size_t size)
   : m_implPtr(new ConstantPrivate(name))
{
   VMAPI_D(Constant);
   ZVAL_PSTRINGL(&implPtr->m_constant.value, value, size);
}

Constant::Constant(const char *name, const std::string &value)
   : m_implPtr(new ConstantPrivate(name))
{
   VMAPI_D(Constant);
   ZVAL_PSTRINGL(&implPtr->m_constant.value, value.c_str(), value.size());
}

Constant::Constant(const Constant &other)
   : m_implPtr(other.m_implPtr)
{}

Constant::Constant(Constant &&other) noexcept
   : m_implPtr(std::move(other.m_implPtr))
{}

Constant& Constant::operator=(const Constant &other)
{
   if (this != &other) {
      m_implPtr = other.m_implPtr;
   }
   return *this;
}

Constant& Constant::operator=(Constant &&other) noexcept
{
   assert(this != &other);
   m_implPtr = std::move(other.m_implPtr);
   return *this;
}

Constant::~Constant()
{
}

void Constant::initialize(const std::string &prefix, int moduleNumber)
{
   getImplPtr()->initialize(prefix, moduleNumber);
}

void Constant::initialize(int moduleNumber)
{
   getImplPtr()->initialize("", moduleNumber);
}

const zend_constant &Constant::getZendConstant() const
{
   VMAPI_D(const Constant);
   return implPtr->m_constant;
}

const std::string &Constant::getName() const
{
   VMAPI_D(const Constant);
   return implPtr->m_name;
}

} // vmapi
} // polar
