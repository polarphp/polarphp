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

#ifndef POLARPHP_VMAPI_ABSTRACT_MEMBER_H
#define POLARPHP_VMAPI_ABSTRACT_MEMBER_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/basic/adt/StringRef.h"

namespace polar {
namespace vmapi {
namespace internal
{
class AbstractMemberPrivate;
class AbstractClassPrivate;
} // internal

using internal::AbstractClassPrivate;
using internal::AbstractMemberPrivate;
using polar::basic::StringRef;

class AbstractMember
{
public:
   AbstractMember();
   AbstractMember(StringRef name, Modifier flags);
   AbstractMember(const AbstractMember &other);
   AbstractMember(AbstractMember &&other) noexcept;
   virtual ~AbstractMember();
   AbstractMember &operator=(const AbstractMember &other);
   AbstractMember &operator=(AbstractMember &&other) noexcept;
   bool isConstant() const noexcept;
protected:
   AbstractMember(AbstractMemberPrivate *implPtr);
   void initialize(zend_class_entry *entry);
   virtual void setupConstant(zend_class_entry *entry) = 0;
   virtual void setupProperty(zend_class_entry *entry) = 0;

   VMAPI_DECLARE_PRIVATE(AbstractMember);
   std::shared_ptr<AbstractMemberPrivate> m_implPtr;
   friend class AbstractClassPrivate;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_ABSTRACT_MEMBER_H
