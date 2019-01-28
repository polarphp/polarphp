// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/23.

#ifndef POLARPHP_VMAPI_LANG_ARGUMENT_H
#define POLARPHP_VMAPI_LANG_ARGUMENT_H

#include "polarphp/vm/ZendApi.h"
#include <initializer_list>

namespace polar {
namespace vmapi {

namespace internal
{
class ArgumentPrivate;
} // internal

using internal::ArgumentPrivate;

class VMAPI_DECL_EXPORT Argument
{
public:
   Argument(const Argument &other);
   Argument(Argument &&other) noexcept;
   Argument &operator=(const Argument &other);
   Argument &operator=(Argument &&other) noexcept;
   virtual ~Argument();
   bool isNullable() const;
   bool isReference() const;
   bool isRequired() const;
   bool isVariadic() const;
   const char *getName() const;
   Type getType() const;
   const char *getClassName() const;

protected:
   Argument(const char *name, Type type, bool required = true,
            bool byReference = false, bool isVariadic = false);
   Argument(const char *name, const char *className, bool nullable = true,
            bool required = true, bool byReference = false, bool isVariadic = false);
   VMAPI_DECLARE_PRIVATE(Argument);
   std::unique_ptr<ArgumentPrivate> m_implPtr;
};

#if defined(POLAR_CC_MSVC) && POLAR_CC_MSVC < 1800
using Arguments = std::vector<Argument>;
#else
using Arguments = std::initializer_list<Argument>;
#endif

class VMAPI_DECL_EXPORT RefArgument : public Argument
{
public:
   RefArgument(const char *name, Type type = Type::Undefined, bool required = true)
      : Argument(name, type, required, true, false)
   {}

   RefArgument(const char *name, const char *className, bool required = true)
      : Argument(name, className, false, required, true, false)
   {}

   RefArgument(const RefArgument &argument)
      : Argument(argument)
   {}

   RefArgument(RefArgument &&argument) noexcept
      : Argument(std::move(argument))
   {}

   virtual ~RefArgument();
};

class VMAPI_DECL_EXPORT ValueArgument : public Argument
{
public:
   ValueArgument(const char *name, Type type = Type::Undefined,
                 bool required = true)
      : Argument(name, type, required, false, false)
   {}

   ValueArgument(const char *name, const char *className, bool nullable = false,
                 bool required = true)
      : Argument(name, className, nullable, required, false, false)
   {}

   ValueArgument(const ValueArgument &argument)
      : Argument(argument)
   {}

   ValueArgument(ValueArgument &&argument) noexcept
      : Argument(std::move(argument))
   {}

   virtual ~ValueArgument();
};

class VMAPI_DECL_EXPORT VariadicArgument : public Argument
{
public:
   VariadicArgument(const char *name, Type type = Type::Undefined, bool isReference = false)
      : Argument(name, type, false, isReference, true)
   {}

   VariadicArgument(const ValueArgument &argument)
      : Argument(argument)
   {}

   VariadicArgument(ValueArgument &&argument) noexcept
      : Argument(std::move(argument))
   {}

   virtual ~VariadicArgument();
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_ARGUMENT_H
