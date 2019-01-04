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

#include "polarphp/vm/lang/Argument.h"

namespace polar {
namespace vmapi {

namespace internal
{
class ArgumentPrivate
{
public:
   ArgumentPrivate(const char *name, Type type, bool required = true, bool byReference = false,
                   bool variadic = false);
   ArgumentPrivate(const char *name, const char *className, bool nullable = true,
                   bool required = true, bool byReference = false, bool variadic = false);
   ArgumentPrivate(const ArgumentPrivate &other);
   Type m_type = Type::Null;
   bool m_nullable = false;
   bool m_required = true;
   bool m_byReference = false;
   bool m_variadic = false;
   const char *m_name = nullptr;
   const char *m_className = nullptr;
};

ArgumentPrivate::ArgumentPrivate(const char *name, Type type, bool required,
                                 bool byReference, bool isVariadic)
   : m_type(type),
     m_required(required),
     m_byReference(byReference),
     m_variadic(isVariadic),
     m_name(name)
{}

ArgumentPrivate::ArgumentPrivate(const char *name, const char *className, bool nullable,
                                 bool required, bool byReference, bool isVariadic)
   : m_nullable(nullable),
     m_required(required),
     m_byReference(byReference),
     m_variadic(isVariadic),
     m_name(name),
     m_className(className)
{}

ArgumentPrivate::ArgumentPrivate(const ArgumentPrivate &other)
   : m_nullable(other.m_nullable),
     m_required(other.m_required),
     m_byReference(other.m_byReference),
     m_variadic(other.m_variadic),
     m_name(other.m_name),
     m_className(other.m_className)
{}
} // internal

Argument::Argument(const char *name, Type type, bool required,
                   bool byReference, bool isVariadic)
   : m_implPtr(new ArgumentPrivate(name, type, required, byReference, isVariadic))
{}

Argument::Argument(const char *name, const char *className, bool nullable,
                   bool required, bool byReference, bool isVariadic)
   : m_implPtr(new ArgumentPrivate(name, className, nullable, required, byReference, isVariadic))
{}

Argument::Argument(const Argument &other)
   : m_implPtr(new ArgumentPrivate(*other.m_implPtr))
{
}

Argument::Argument(Argument &&other) noexcept
   : m_implPtr(std::move(other.m_implPtr))
{
}

Argument::~Argument()
{}

Argument &Argument::operator=(const Argument &other)
{
   if (this != &other) {
      m_implPtr.reset(new ArgumentPrivate(*other.m_implPtr));
   }
   return *this;
}

Argument &Argument::operator=(Argument &&other) noexcept
{
   assert(this != &other);
   m_implPtr = std::move(other.m_implPtr);
   return *this;
}

bool Argument::isNullable() const
{
   VMAPI_D(const Argument);
   return implPtr->m_nullable;
}

bool Argument::isReference() const
{
   VMAPI_D(const Argument);
   return implPtr->m_byReference;
}

bool Argument::isRequired() const
{
   VMAPI_D(const Argument);
   return implPtr->m_required;
}

bool Argument::isVariadic() const
{
   VMAPI_D(const Argument);
   return implPtr->m_variadic;
}

const char *Argument::getName() const
{
   VMAPI_D(const Argument);
   return implPtr->m_name;
}

Type Argument::getType() const
{
   VMAPI_D(const Argument);
   return implPtr->m_type;
}

const char *Argument::getClassName() const
{
   VMAPI_D(const Argument);
   return implPtr->m_className;
}

ValueArgument::~ValueArgument()
{}

RefArgument::~RefArgument()
{}

VariadicArgument::~VariadicArgument()
{}

} // vmapi
} // polar
