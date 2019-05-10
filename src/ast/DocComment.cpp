//===--- DocComment.cpp - Extraction of doc comments ----------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/05.
//===----------------------------------------------------------------------===//

#include "polarphp/ast/Comment.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Types.h"
#include "polarphp/ast/PrettyStackTrace.h"
#include "polarphp/ast/RawComment.h"
#include "polarphp/Markup/Markup.h"

namespace polar::ast {

class AstContext;
class Decl;
class Expr;
class GenericSignature;
class Stmt;
class TypeRepr;

} // polar::ast
