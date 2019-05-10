// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/08.

#ifndef POLARPHP_SYNTAX_NODES_H
#define POLARPHP_SYNTAX_NODES_H

#include "polarphp/syntax/syntaxnode/CommonNodes.h"
#include "polarphp/syntax/syntaxnode/ExprNodes.h"
#include "polarphp/syntax/syntaxnode/StmtNodes.h"

namespace polar::syntax {

/// Calculating an identifier for all syntax nodes' structures for verification
/// purposes.
const char* get_syntax_structure_versioning_identifier();

} // polar::syntax

#endif // POLARPHP_SYNTAX_NODES_H
