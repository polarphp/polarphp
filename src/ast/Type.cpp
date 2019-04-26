//===--- Type.cpp - Swift Language Type ASTs ------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/25.
//===----------------------------------------------------------------------===//
//
//  This file implements the Type class and subclasses.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/Types.h"
//#include "polarphp/ast/AstContext.h"
//#include "polarphp/ast/GenericSignatureBuilder.h"
//#include "polarphp/ast/ReferenceCounting.h"
#include "polarphp/ast/TypeVisitor.h"
#include "polarphp/ast/TypeWalker.h"
#include "polarphp/ast/Decl.h"
//#include "polarphp/ast/GenericEnvironment.h"
//#include "polarphp/ast/LazyResolver.h"
//#include "polarphp/ast/ParameterList.h"
//#include "polarphp/ast/ProtocolConformance.h"
//#include "polarphp/ast/SubstitutionMap.h"
#include "polarphp/ast/TypeLoc.h"
//#include "polarphp/ast/TypeRepr.h"
#include "polarphp/basic/adt/APFloat.h"
#include "polarphp/basic/adt/SmallPtrSet.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/utils/RawOutStream.h"
#include <algorithm>
#include <functional>
#include <iterator>

namespace polar::ast {

} // polar::ast
