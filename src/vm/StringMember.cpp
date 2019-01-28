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

#include "polarphp/vm/StringMember.h"
#include "polarphp/vm/internal/AbstractMemberPrivate.h"
#include <cstring>

namespace polar {
namespace vmapi {

using internal::AbstractMemberPrivate;

namespace internal
{
class StringMemberPrivate : public AbstractMemberPrivate
{
public:
   StringMemberPrivate(StringRef name, const char *value, size_t size, Modifier flags)
      : AbstractMemberPrivate(name, flags),
        m_value(value, size)
   {}

   StringMemberPrivate(StringRef name, const char *value, Modifier flags)
      : AbstractMemberPrivate(name, flags),
        m_value(value, std::strlen(value))
   {}

   StringMemberPrivate(StringRef name, const std::string &value, Modifier flags)
      : AbstractMemberPrivate(name, flags),
        m_value(value)
   {}

   std::string m_value;
};

} // internal

using internal::StringMemberPrivate;

StringMember::StringMember(StringRef name, StringRef value, Modifier flags)
   : AbstractMember(new StringMemberPrivate(name, value, flags))
{}

StringMember::StringMember(const StringMember &other)
   : AbstractMember(other)
{}

StringMember &StringMember::operator=(const StringMember &other)
{
   if (this != &other) {
      AbstractMember::operator=(other);
   }
   return *this;
}

StringMember::~StringMember()
{}

void StringMember::setupConstant(zend_class_entry *entry)
{
   VMAPI_D(StringMember);
   zend_declare_class_constant_stringl(entry, implPtr->m_name.c_str(), implPtr->m_name.size(),
                                       implPtr->m_value.c_str(), implPtr->m_value.size());
}

void StringMember::setupProperty(zend_class_entry *entry)
{
   VMAPI_D(StringMember);
   zend_declare_property_stringl(entry, implPtr->m_name.c_str(), implPtr->m_name.size(),
                                 implPtr->m_value.c_str(), implPtr->m_value.size(), static_cast<int>(implPtr->m_flags));
}

} // vmapi
} // polar
