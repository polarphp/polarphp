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

#include "polarphp/vm/NullMember.h"
#include "polarphp/vm/internal/AbstractMemberPrivate.h"

namespace polar {
namespace vmapi {

using internal::AbstractMemberPrivate;

NullMember::NullMember(StringRef name, Modifier flags)
   : AbstractMember(name, flags)
{}

void NullMember::setupConstant(zend_class_entry *entry)
{
   zend_declare_class_constant_null(entry, m_implPtr->m_name.c_str(), m_implPtr->m_name.size());
}

void NullMember::setupProperty(zend_class_entry *entry)
{
   zend_declare_property_null(entry, m_implPtr->m_name.c_str(), m_implPtr->m_name.size(),
                              static_cast<unsigned long>(m_implPtr->m_flags));
}

NullMember::~NullMember()
{}

} // vmapi
} // polar
