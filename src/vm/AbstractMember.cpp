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

#include "polarphp/vm/AbstractMember.h"
#include "polarphp/vm/internal/AbstractMemberPrivate.h"

namespace polar {
namespace vmapi {

AbstractMember::AbstractMember()
{}

AbstractMember::AbstractMember(StringRef name, Modifier flags)
   : m_implPtr(new AbstractMemberPrivate(name, flags))
{}

AbstractMember::AbstractMember(const AbstractMember &other)
   : m_implPtr(other.m_implPtr)
{}

AbstractMember::AbstractMember(AbstractMember &&other) noexcept
   : m_implPtr(std::move(other.m_implPtr))
{}

AbstractMember::AbstractMember(AbstractMemberPrivate *implPtr)
   : m_implPtr(implPtr)
{}

AbstractMember &AbstractMember::operator=(const AbstractMember &other)
{
   if (this != &other) {
      m_implPtr = other.m_implPtr;
   }
   return *this;
}

AbstractMember &AbstractMember::operator=(AbstractMember &&other) noexcept
{
   assert(this != &other);
   m_implPtr = std::move(other.m_implPtr);
   return *this;
}

bool AbstractMember::isConstant() const noexcept
{
   return (m_implPtr->m_flags & Modifier::Const) == Modifier::Const;
}

void AbstractMember::initialize(zend_class_entry *entry)
{
   VMAPI_D(AbstractMember);
   if (implPtr->m_flags == Modifier::Const) {
      setupConstant(entry);
   } else {
      setupProperty(entry);
   }
}

AbstractMember::~AbstractMember()
{}

} // vmapi
} // polar
