// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/19.

#include "polarphp/syntax/serialization/SyntaxJsonSerialization.h"
#include "polarphp/syntax/Syntax.h"

namespace polar::syntax {

void to_json(json &jsonObject, const Syntax &syntax)
{
   jsonObject["kind"] = syntax.getKind();
   jsonObject["hasChild"] = syntax.getNumChildren() != 0;
   jsonObject["childCount"] = syntax.getNumChildren();
}

} // polar::syntax

