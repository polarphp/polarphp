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

#include "polarphp/vm/NumericMember.h"
#include "polarphp/vm/internal/AbstractMemberPrivate.h"

namespace polar {
namespace vmapi {

namespace internal
{
class NumericMemberPrivate : public AbstractMemberPrivate
{
public:
   NumericMemberPrivate(StringRef name, double value, Modifier flags)
      : AbstractMemberPrivate(name, flags),
        m_value(value)
   {}
   NumericMemberPrivate(const NumericMemberPrivate &other) = default;
   long m_value;
};
} // internal

using internal::NumericMemberPrivate;

NumericMember::NumericMember(StringRef name, double value, Modifier flags)
   : AbstractMember(new NumericMemberPrivate(name, value, flags))
{}

NumericMember::NumericMember(const NumericMember &other)
   : AbstractMember(new NumericMemberPrivate(*static_cast<NumericMemberPrivate *>(other.m_implPtr.get())))
{}

NumericMember &NumericMember::operator=(const NumericMember &other)
{
   if (this != &other) {
      AbstractMember::operator=(other);
   }
   return *this;
}

void NumericMember::setupConstant(zend_class_entry *entry)
{
   VMAPI_D(NumericMember);
   zend_declare_class_constant_long(entry, implPtr->m_name.c_str(), implPtr->m_name.size(),
                                    implPtr->m_value);
}

void NumericMember::setupProperty(zend_class_entry *entry)
{
   VMAPI_D(NumericMember);
   zend_declare_property_long(entry, implPtr->m_name.c_str(), implPtr->m_name.size(),
                              implPtr->m_value, static_cast<int>(implPtr->m_flags));
}

NumericMember::~NumericMember()
{}

} // vmapi
} // polar
