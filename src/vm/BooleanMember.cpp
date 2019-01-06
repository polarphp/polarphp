// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/26.

#include "polarphp/vm/BooleanMember.h"
#include "polarphp/vm/internal/AbstractMemberPrivate.h"

namespace polar {
namespace vmapi {

namespace internal
{
class BooleanMemberPrivate : public AbstractMemberPrivate
{
public:
   BooleanMemberPrivate(StringRef name, double value, Modifier flags)
      : AbstractMemberPrivate(name, flags),
        m_value(value)
   {}
   BooleanMemberPrivate(const BooleanMemberPrivate &other) = default;
   bool m_value;
};

} // internal

using internal::BooleanMemberPrivate;

BooleanMember::BooleanMember(StringRef name, double value, Modifier flags)
   : AbstractMember(new BooleanMemberPrivate(name, value, flags))
{}

BooleanMember::BooleanMember(const BooleanMember &other)
   : AbstractMember(other)
{}

BooleanMember &BooleanMember::operator=(const BooleanMember &other)
{
   if (this != &other) {
      AbstractMember::operator=(other);
   }
   return *this;
}

void BooleanMember::setupConstant(zend_class_entry *entry)
{
   VMAPI_D(BooleanMember);
   zend_declare_class_constant_bool(entry, implPtr->m_name.c_str(), implPtr->m_name.size(),
                                    implPtr->m_value);
}

void BooleanMember::setupProperty(zend_class_entry *entry)
{
   VMAPI_D(BooleanMember);
   zend_declare_property_bool(entry, implPtr->m_name.c_str(), implPtr->m_name.size(),
                              implPtr->m_value, static_cast<int>(implPtr->m_flags));
}

BooleanMember::~BooleanMember()
{}

} // vmapi
} // polar
