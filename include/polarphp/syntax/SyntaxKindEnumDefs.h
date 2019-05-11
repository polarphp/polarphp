// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/11.

#ifndef POLARPHP_SYNTAX_KIND_ENUM_DEFS_H
#define POLARPHP_SYNTAX_KIND_ENUM_DEFS_H

namespace polar::syntax {

enum class SyntaxKind {
   Token,
   // NOTE: Unknown must be the last kind.
   Unknown,
};


} // polar::syntax

#endif // POLARPHP_SYNTAX_KIND_ENUM_DEFS_H
