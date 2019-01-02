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

#ifndef POLARPHP_VMAPI_DS_INTERNAL_VARIANT_PRIVATE_H
#define POLARPHP_VMAPI_DS_INTERNAL_VARIANT_PRIVATE_H

#include "polarphp/vm/internal/DepsZendVmHeaders.h"
#include "polarphp/global/Config.h"

#include <type_traits>
#include <memory>
#include <cstring>

namespace polar {
namespace vmapi {
namespace internal {

class VariantPrivate
{
public:
   VariantPrivate()
   {
      std::memset(&m_buffer, 0, sizeof(m_buffer));
   }

   operator _zval_struct *() const &;
   operator const _zval_struct *() const &;
   _zval_struct &operator*() const &;
   _zval_struct *dereference() const;
#ifndef POLAR_DEBUG_BUILD
   std::aligned_storage<16>::type m_buffer;
#else
   zval m_buffer;
#endif

};

} // internal
} // vmapi
} // polar

#endif // POLARPHP_VMAPI_DS_INTERNAL_VARIANT_PRIVATE_H
