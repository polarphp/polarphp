// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by softboy on 2018/12/23.

#ifndef  POLARPHP_VM_ZENDAPI_H
#define  POLARPHP_VM_ZENDAPI_H

#include "polarphp/global/Global.h"
#include "polarphp/vm/lang/Type.h"
#include "polarphp/vm/TypeDefs.h"

#define VMAPI_DECLARE_PRIVATE POLAR_DECLARE_PRIVATE
#define WMAPI_DECLARE_PUBLIC  POLAR_DECLARE_PUBLIC

#define VMAPI_D POLAR_D
#define VMAPI_Q POLAR_Q
#define VMAPI_DECL_EXPORT POLAR_DECL_EXPORT

namespace polar {
namespace vmapi {

extern thread_local VMAPI_DECL_EXPORT std::ostream out;
extern thread_local VMAPI_DECL_EXPORT std::ostream error;
extern thread_local VMAPI_DECL_EXPORT std::ostream notice;
extern thread_local VMAPI_DECL_EXPORT std::ostream warning;
extern thread_local VMAPI_DECL_EXPORT std::ostream deprecated;

using VmApiVaridicItemType = zval;

} // vmapi
} // polar

#define VMAPI_ASSERT ZEND_ASSERT

#define VMAPI_SUCCESS SUCCESS
#define VMAPI_FAILURE FAILURE

// define some zend macro
#define vmapi_bailout() _zend_bailout(const_cast<char *>(static_cast<const char *>(__FILE__)), __LINE__)

#endif // POLARPHP_VM_ZENDAPI_H
