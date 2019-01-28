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

#ifndef POLARPHP_VMAPI_PROTOCOL_ABSTRACT_ITERATOR_H
#define POLARPHP_VMAPI_PROTOCOL_ABSTRACT_ITERATOR_H

#include "polarphp/vm/ZendApi.h"
#include <memory>

namespace polar {
namespace vmapi {

/// forward declare class with namespace
namespace internal {
class AbstractIteratorPrivate;
} // internal

using internal::AbstractIteratorPrivate;

class VMAPI_DECL_EXPORT AbstractIterator
{
public:
   AbstractIterator(StdClass *nativeObject);
   virtual ~AbstractIterator();

   virtual bool valid() = 0;
   virtual Variant current() = 0;
   virtual Variant key() = 0;
   virtual void next() = 0;
   virtual void rewind() = 0;

protected:
   VMAPI_DECLARE_PRIVATE(AbstractIterator);
   std::unique_ptr<AbstractIteratorPrivate> m_implPtr;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_PROTOCOL_ABSTRACT_ITERATOR_H
