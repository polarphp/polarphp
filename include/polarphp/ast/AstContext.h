//===--- ASTContext.h - AST Context Object ----------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
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
// Created by polarboy on 2019/04/26.
//===----------------------------------------------------------------------===//
//
// This file defines the ASTContext interface.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_ASTCONTEXT_H
#define POLARPHP_AST_ASTCONTEXT_H

#include "polarphp/global/DataTypes.h"
#include "polarphp/ast/Identifier.h"
//#include "polarphp/ast/SearchPathOptions.h"
#include "polarphp/ast/Type.h"
#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/basic/Malloc.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/MapVector.h"
#include "polarphp/basic/adt/IntrusiveRefCountPtr.h"
#include "polarphp/basic/adt/PointerIntPair.h"
#include "polarphp/basic/adt/SetVector.h"
#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/basic/adt/TinyPtrVector.h"
#include "polarphp/utils/Allocator.h"
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace polar::ast {
class ASTContext;
enum class Associativity : unsigned char;
class BoundGenericType;
class ConstructorDecl;
class Decl;
class DeclContext;
class DefaultArgumentInitializer;
class FuncDecl;
class GenericContext;
class InFlightDiagnostic;
class IterableDeclContext;
class SourceFile;
class SourceLoc;
class Type;
class TypeVariableType;
class TupleType;
class FunctionType;
class GenericSignatureBuilder;
class ArchetypeType;
class Identifier;
class InheritedNameSet;
class NominalTypeDecl;
enum PointerTypeKind : unsigned;
class PrecedenceGroupDecl;
class TupleTypeElt;
class EnumElementDecl;
class ProtocolDecl;
class SubstitutableType;
class SourceManager;
class ValueDecl;
class DiagnosticEngine;
struct RawComment;
class DocComment;
class TypeAliasDecl;
class VarDecl;
enum class KnownProtocolKind : uint8_t;

class ASTContext final
{
   ASTContext(const ASTContext&) = delete;
   void operator=(const ASTContext&) = delete;
   ASTContext(LangOptions &langOpts, SearchPathOptions &SearchPathOpts,
              SourceManager &SourceMgr, DiagnosticEngine &Diags);
};

} // polar::ast

#endif // POLARPHP_AST_ASTCONTEXT_H
