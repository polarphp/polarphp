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

#include "polarphp/vm/lang/Parameter.h"
#include "polarphp/vm/ObjectBinder.h"

namespace polar {
namespace vmapi {

Parameters::Parameters(zval *thisPtr, uint32_t argc)
   : Parameters(nullptr != thisPtr ? ObjectBinder::retrieveSelfPtr(thisPtr)->getNativeObject() : nullptr)
{
   m_data.reserve(argc);
   std::unique_ptr<zval[]> arguments(new zval[argc]);
   zend_get_parameters_array_ex(argc, arguments.get());
   for (uint32_t i = 0; i < argc; i++)
   {
      // append value
      m_data.emplace_back(&arguments[i]);
   }
}

Parameters::Reference Parameters::at(SizeType pos)
{
   return m_data.at(pos);
}

Parameters::ConstReference Parameters::at(SizeType pos) const
{
   return m_data.at(pos);
}

bool Parameters::empty() const noexcept
{
   return m_data.empty();
}

Parameters::SizeType Parameters::size() const noexcept
{
   return m_data.size();
}

} // vmapi
} // polar
