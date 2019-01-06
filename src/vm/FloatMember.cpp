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

#include "polarphp/vm/FloatMember.h"
#include "polarphp/vm/internal/AbstractMemberPrivate.h"

namespace polar {
namespace vmapi {

namespace internal
{
class FloatMemberPrivate : public AbstractMemberPrivate
{
public:
   FloatMemberPrivate(StringRef name, double value, Modifier flags)
      : AbstractMemberPrivate(name, flags),
        m_value(value)
   {}
   double m_value;
};

} // internal

using internal::FloatMemberPrivate;

FloatMember::FloatMember(StringRef name, double value, Modifier flags)
   : AbstractMember(new FloatMemberPrivate(name, value, flags))
{}

FloatMember::FloatMember(const FloatMember &other)
   : AbstractMember(other)
{}

FloatMember &FloatMember::operator=(const FloatMember &other)
{
   if (this != &other) {
      AbstractMember::operator=(other);
   }
   return *this;
}

void FloatMember::setupConstant(zend_class_entry *entry)
{
   VMAPI_D(FloatMember);
   zend_declare_class_constant_double(entry, implPtr->m_name.c_str(), implPtr->m_name.size(),
                                      implPtr->m_value);
}

void FloatMember::setupProperty(zend_class_entry *entry)
{
   VMAPI_D(FloatMember);
   zend_declare_property_double(entry, implPtr->m_name.c_str(), implPtr->m_name.size(),
                                implPtr->m_value, static_cast<int>(implPtr->m_flags));
}

FloatMember::~FloatMember()
{}

} // vmapi
} // polar
