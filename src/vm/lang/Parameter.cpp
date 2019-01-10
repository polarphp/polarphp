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
#include "polarphp/vm/ds/Variant.h"
#include "polarphp/vm/ds/NumericVariant.h"
#include "polarphp/vm/ds/StringVariant.h"
#include "polarphp/vm/ds/ObjectVariant.h"
#include "polarphp/vm/ds/BooleanVariant.h"
#include "polarphp/vm/ds/CallableVariant.h"
#include "polarphp/vm/ds/ArrayVariant.h"
#include "polarphp/vm/ds/DoubleVariant.h"

namespace polar {
namespace vmapi {

Parameters::Parameters(zval *thisPtr, uint32_t argc)
   : Parameters(nullptr != thisPtr ? ObjectBinder::retrieveSelfPtr(thisPtr)->getNativeObject() : nullptr)
{
   ///
   /// TODO review memory leak
   ///
   m_data.reserve(argc);
   std::unique_ptr<zval[]> arguments(new zval[argc]);
   zend_get_parameters_array_ex(argc, arguments.get());
   for (uint32_t i = 0; i < argc; i++)
   {
      zval *arg = &arguments[i];
      zend_uchar type = Z_TYPE_P(arg);
      if (type == IS_LONG) {
         m_data.emplace_back(NumericVariant(arg));
      } else if (type == IS_ARRAY) {
         m_data.emplace_back(ArrayVariant(arg));
      } else if (type == IS_DOUBLE) {
         m_data.emplace_back(DoubleVariant(arg));
      } else if (type == IS_STRING) {
         m_data.emplace_back(StringVariant(arg));
      } else if (type == IS_TRUE || type == IS_FALSE) {
         m_data.emplace_back(BooleanVariant(arg));
      } else if (type == IS_CALLABLE) {
         m_data.emplace_back(CallableVariant(arg));
      } else if (type == IS_OBJECT) {
         m_data.emplace_back(ObjectVariant(arg));
      } else {
         m_data.emplace_back(Variant(arg));
      }
   }
}

Variant Parameters::retrieveAsVariant(SizeType pos)
{
   return static_cast<const Parameters *>(this)->retrieveAsVariant(pos);
}

Variant Parameters::retrieveAsVariant(SizeType pos) const
{
   const std::any &arg = m_data.at(pos);
   const std::type_info &type = arg.type();
   if (type == typeid(NumericVariant)) {
      return at<NumericVariant>(pos);
   } else if (type == typeid(ArrayVariant)) {
      return at<ArrayVariant>(pos);
   } else if (type == typeid(DoubleVariant)) {
      return at<DoubleVariant>(pos);
   } else if (type == typeid(StringVariant)) {
      return at<StringVariant>(pos);
   } else if (type == typeid(BooleanVariant)) {
      return at<BooleanVariant>(pos);
   } else if (type == typeid(CallableVariant)) {
      return at<CallableVariant>(pos);
   } else if (type == typeid(ObjectVariant)) {
      return at<ObjectVariant>(pos);
   } else {
      return at<Variant>(pos);
   }
}

} // vmapi
} // polar
