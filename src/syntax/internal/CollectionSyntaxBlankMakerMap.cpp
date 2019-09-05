// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/09/05.

#include <map>

#include "polarphp/syntax/SyntaxNodeFactories.h"
#include "polarphp/syntax/SyntaxKindEnumDefs.h"

namespace polar::syntax::internal {
namespace {

static const std::map<SyntaxKind, int> scg_collectionBlankMakerMap{
   { }
};

} // anonymous namespace
} // polar::syntax::internal
