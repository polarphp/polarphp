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

#include "polarphp/vm/ZvalMember.h"
#include "polarphp/vm/internal/AbstractMemberPrivate.h"

namespace polar {
namespace vmapi {

using internal::AbstractMemberPrivate;

namespace internal
{

class ZvalMemberPrivate : public AbstractMemberPrivate
{
public:
   ZvalMemberPrivate(const char *name, zval *value, Modifier flags)
      : AbstractMemberPrivate(name, flags),
        m_value(value)
   {}
   zval *m_value;
};

} // internal

using internal::ZvalMemberPrivate;

ZvalMember::ZvalMember(const char *name, zval *value, Modifier flags)
   : AbstractMember(new ZvalMemberPrivate(name, value, flags))
{}

ZvalMember::ZvalMember(const ZvalMember &other)
   : AbstractMember(other)
{
   VMAPI_D(ZvalMember);
   Z_TRY_ADDREF_P(implPtr->m_value);
}

ZvalMember &ZvalMember::operator=(const ZvalMember &other)
{
   if (this != &other) {
      AbstractMember::operator=(other);
   }
   return *this;
}

ZvalMember::~ZvalMember()
{}

void ZvalMember::setupConstant(zend_class_entry *entry)
{
   VMAPI_D(ZvalMember);
   zend_declare_class_constant(entry, implPtr->m_name.c_str(), implPtr->m_name.size(), implPtr->m_value);
}

void ZvalMember::setupProperty(zend_class_entry *entry)
{
   VMAPI_D(ZvalMember);
   zend_declare_property(entry, implPtr->m_name.c_str(), implPtr->m_name.size(),
                         implPtr->m_value, static_cast<int>(implPtr->m_flags));
}

} // vmapi
} // polar
