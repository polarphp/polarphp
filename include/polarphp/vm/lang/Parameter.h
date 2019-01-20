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

#ifndef POLARPHP_VMAPI_LANG_PARAMETER_H
#define POLARPHP_VMAPI_LANG_PARAMETER_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/ds/ArrayVariant.h"
#include "polarphp/vm/ds/StringVariant.h"
#include "polarphp/vm/ds/BooleanVariant.h"
#include "polarphp/vm/ds/DoubleVariant.h"
#include "polarphp/vm/ds/NumericVariant.h"
#include "polarphp/vm/ds/ObjectVariant.h"
#include "polarphp/vm/ds/CallableVariant.h"
#include <vector>
#include <any>

namespace polar {
namespace vmapi {

class StdClass;

using polar::vmapi::StringVariant;
using polar::vmapi::ArrayVariant;
using polar::vmapi::BooleanVariant;
using polar::vmapi::DoubleVariant;
using polar::vmapi::NumericVariant;
using polar::vmapi::ObjectVariant;
using polar::vmapi::CallableVariant;

/**
 * now this is very bad implemention of parameters class, but it works
 */
class VMAPI_DECL_EXPORT Parameters final
{
public:
   using ParamCollectionType = std::vector<std::any>;
   using ValueType = ParamCollectionType::value_type;
   using SizeType = ParamCollectionType::size_type;
   using DifferenceType = ParamCollectionType::difference_type;
   using Reference = ParamCollectionType::reference;
   using ConstReference = ParamCollectionType::const_reference;
   using Pointer = ParamCollectionType::pointer;
   using ConstPointer = ParamCollectionType::const_pointer;

public:
   Parameters(std::initializer_list<std::any> items)
      : m_data(items)
   {}

   Parameters(const Parameters &other)
      : m_object(other.m_object), m_data(other.m_data)
   {
      std::cout << "copy" << std::endl;
      std::cout << "copy" << std::endl;
   }

   Parameters(const ParamCollectionType::iterator begin,
              const ParamCollectionType::iterator end)
      : m_data(begin, end)
   {}

   Parameters(Parameters &&params) noexcept
      : m_object(params.m_object),
        m_data(std::move(params.m_data))
   {}

   Parameters(StdClass *object)
      : m_object(object)
   {}
   Parameters(zval *thisPtr, uint32_t argc);

public:

   StdClass *getObject() const
   {
      return m_object;
   }

   ///
   /// TODO add debug bad cast string
   ///
   template <typename T,
             typename DecayVariantType = typename std::decay<T>::type,
             typename std::enable_if<std::is_same<T, Variant>::value ||
                                     std::is_same<T, StringVariant>::value ||
                                     std::is_same<T, NumericVariant>::value ||
                                     std::is_same<T, BooleanVariant>::value ||
                                     std::is_same<T, ObjectVariant>::value ||
                                     std::is_same<T, DoubleVariant>::value ||
                                     std::is_same<T, ArrayVariant>::value ||
                                     std::is_same<T, CallableVariant>::value, T>::type * = nullptr>
   T &at(SizeType pos)
   {
      std::any &arg = m_data.at(pos);
      return std::any_cast<T &>(arg);
   }

   template <typename T,
             typename DecayVariantType = typename std::decay<T>::type,
             typename std::enable_if<std::is_same<T, Variant>::value ||
                                     std::is_same<T, StringVariant>::value ||
                                     std::is_same<T, NumericVariant>::value ||
                                     std::is_same<T, BooleanVariant>::value ||
                                     std::is_same<T, ObjectVariant>::value ||
                                     std::is_same<T, DoubleVariant>::value ||
                                     std::is_same<T, ArrayVariant>::value ||
                                     std::is_same<T, CallableVariant>::value, T>::type * = nullptr>
   const T &at(SizeType pos) const
   {
      const std::any &arg = m_data.at(pos);
      return std::any_cast<const T &>(arg);
   }

   Variant retrieveAsVariant(SizeType pos);
   Variant retrieveAsVariant(SizeType pos) const;

   bool empty() const noexcept
   {
      return m_data.empty();
   }

   SizeType size() const noexcept
   {
      return m_data.size();
   }

private:
   /**
    *  The base object
    *  @var Base
    */
   StdClass *m_object = nullptr;
   std::vector<std::any> m_data;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_PARAMETER_H
