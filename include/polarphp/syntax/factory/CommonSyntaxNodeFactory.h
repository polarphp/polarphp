// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/14.

#ifndef POLARPHP_SYNTAX_FACTORY_COMMON_SYNTAX_NODE_FACTORY_H
#define POLARPHP_SYNTAX_FACTORY_COMMON_SYNTAX_NODE_FACTORY_H

#include "polarphp/syntax/AbstractFactory.h"

namespace polar::syntax {

class CommonSyntaxNodeFactory final : public AbstractFactory
{
public:
   static Syntax makeBlankCollection(SyntaxKind kind);
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_FACTORY_COMMON_SYNTAX_NODE_FACTORY_H
