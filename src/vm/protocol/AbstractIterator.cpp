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

#include "polarphp/vm/protocol/AbstractIterator.h"
#include "polarphp/vm/ds/Variant.h"
#include "polarphp/vm/StdClass.h"

namespace polar {
namespace vmapi {

namespace internal
{

class AbstractIteratorPrivate
{
public:
   AbstractIteratorPrivate(StdClass *nativeObject)
      : m_object(nativeObject)
   {}
   // here we just prevent during iterator zend_object being destroy
   // any better method to do this
   Variant m_object;
};

} // internal

AbstractIterator::AbstractIterator(StdClass *nativeObject)
   : m_implPtr(new AbstractIteratorPrivate(nativeObject))
{}

AbstractIterator::~AbstractIterator()
{}

} // vmapi
} // polar
