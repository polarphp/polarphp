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
   ///
   /// TODO review memory leak
   ///
   m_data.reserve(argc);
   std::unique_ptr<zval[]> arguments(new zval[argc]);
   zend_get_parameters_array_ex(argc, arguments.get());
   bool isRef = false;
   for (uint32_t i = 0; i < argc; i++) {
      zval *arg = &arguments[i];
      zend_uchar type = Z_TYPE_P(arg);
      if (type == IS_REFERENCE) {
         type = Z_TYPE_P(Z_REFVAL_P(arg));
         isRef = true;
      } else {
         isRef = false;
      }
      if (type == IS_LONG) {
         m_data.push_back(NumericVariant(arg, isRef));
      } else if (type == IS_ARRAY) {
         m_data.push_back(ArrayVariant(arg, isRef));
      } else if (type == IS_DOUBLE) {
         m_data.push_back(DoubleVariant(arg, isRef));
      } else if (type == IS_STRING) {
         m_data.push_back(StringVariant(arg, isRef));
      } else if (type == IS_TRUE || type == IS_FALSE) {
         m_data.push_back(BooleanVariant(arg, isRef));
      } else if (type == IS_CALLABLE) {
         m_data.push_back(CallableVariant(arg));
      } else if (type == IS_OBJECT) {
         m_data.push_back(ObjectVariant(arg));
      } else {
         m_data.push_back(Variant(arg, isRef));
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
