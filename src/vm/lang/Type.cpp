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

#include "polarphp/vm/lang/Type.h"

namespace polar {
namespace vmapi {

Modifier operator ~(Modifier modifier)
{
   return static_cast<Modifier>(~static_cast<unsigned int>(modifier));
}

Modifier operator |(Modifier lhs, Modifier rhs)
{
   return static_cast<Modifier>(static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs));
}

Modifier operator &(Modifier lhs, Modifier rhs)
{
   return static_cast<Modifier>(static_cast<unsigned int>(lhs) & static_cast<unsigned int>(rhs));
}

Modifier &operator |=(Modifier &lhs, Modifier rhs)
{
   lhs = static_cast<Modifier>(static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs));
   return lhs;
}

Modifier &operator &=(Modifier &lhs, Modifier rhs)
{
   lhs = static_cast<Modifier>(static_cast<unsigned int>(lhs) & static_cast<unsigned int>(rhs));
   return lhs;
}

bool operator ==(unsigned long value, Modifier rhs)
{
   return static_cast<unsigned long>(rhs) == value;
}

bool operator ==(Modifier lhs, unsigned long value)
{
   return static_cast<unsigned long>(lhs) == value;
}

bool operator ==(Modifier lhs, Modifier rhs)
{
   return static_cast<unsigned long>(lhs) == static_cast<unsigned long>(rhs);
}

} // vmapi
} // polar
