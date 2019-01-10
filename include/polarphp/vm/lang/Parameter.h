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
#include "polarphp/vm/ds/Variant.h"

#include <vector>

namespace polar {
namespace vmapi {

class StdClass;

/**
 * now this is very bad implemention of parameters class, but it works
 */
class VMAPI_DECL_EXPORT Parameters final
{
public:
   using ParamCollectionType = std::vector<Variant>;
   using ValueType = ParamCollectionType::value_type;
   using SizeType = ParamCollectionType::size_type;
   using DifferenceType = ParamCollectionType::difference_type;
   using Reference = ParamCollectionType::reference;
   using ConstReference = ParamCollectionType::const_reference;
   using Pointer = ParamCollectionType::pointer;
   using ConstPointer = ParamCollectionType::const_pointer;
   using Iterator = ParamCollectionType::iterator;
   using ConstIterator = ParamCollectionType::const_iterator;
   using ReverseIterator = ParamCollectionType::reverse_iterator;
   using ConstReverseIterator = ParamCollectionType::const_reverse_iterator;
public:
   Parameters(std::initializer_list<Variant> items)
      : m_data(items)
   {}

   Parameters(const Parameters &other)
      : m_object(other.m_object), m_data(other.m_data)
   {}

   Parameters(const ParamCollectionType::iterator begin,
              const ParamCollectionType::iterator end)
      : m_data(begin, end)
   {}

   Parameters(Parameters &&params) noexcept
      : m_object(params.m_object), m_data(std::move(params.m_data))
   {}

   Parameters(StdClass *object) : m_object(object)
   {}

   Parameters(zval *thisPtr, uint32_t argc);

public:

   StdClass *getObject() const
   {
      return m_object;
   }

   Reference at(SizeType pos);
   ConstReference at(SizeType pos) const;
   bool empty() const noexcept;
   SizeType size() const noexcept;
private:
   /**
    *  The base object
    *  @var Base
    */
   StdClass *m_object = nullptr;
   std::vector<Variant> m_data;
};

class VariadicParameters : private std::vector<Variant>
{
   using Vector = std::vector<Variant>;
public:
   using Vector::at;
   using Vector::operator[];
   using Vector::empty;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_PARAMETER_H
