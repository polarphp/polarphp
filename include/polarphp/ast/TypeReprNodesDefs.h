//===--- TypeReprNodes.def - Swift TypeRepr AST Metaprogramming -*- C++ -*-===//
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
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/26.
//===----------------------------------------------------------------------===//
//
// This file defines macros used for macro-metaprogramming with TypeReprs.
//
//===----------------------------------------------------------------------===//

/// TYPEREPR(Id, Parent)
///   The TypeRepr's enumerator value is TypeReprKind::Id.  The TypeRepr's
///   class name is Id##TypeRepr, and the name of its base class is Parent.
#ifndef TYPEREPR
# error Included TypeReprNodes.def without defining TYPEREPR!
#endif

/// An abstract node is an abstract base class in the hierarchy;
/// it is never a most-derived type, and it does not have an enumerator in
/// TypeReprKind.
///
/// Most metaprograms do not care about abstract classes, so the default
/// is to ignore them.
#ifndef ABSTRACT_TYPEREPR
#define ABSTRACT_TYPEREPR(Id, Parent)
#endif

#ifndef TYPEREPR
#define TYPEREPR(CLASS, PARENT)
#endif

#ifndef LAST_TYPEREPR
#define LAST_TYPEREPR(Id)
#endif

TYPEREPR(Error, TypeRepr)
TYPEREPR(Attributed, TypeRepr)

ABSTRACT_TYPEREPR(Ident, TypeRepr)
  ABSTRACT_TYPEREPR(ComponentIdent, IdentTypeRepr)
    TYPEREPR(SimpleIdent, ComponentIdentTypeRepr)
    TYPEREPR(GenericIdent, ComponentIdentTypeRepr)
  TYPEREPR(CompoundIdent, IdentTypeRepr)
TYPEREPR(Function, TypeRepr)
TYPEREPR(Array, TypeRepr)
TYPEREPR(Dictionary, TypeRepr)
TYPEREPR(Optional, TypeRepr)
TYPEREPR(ImplicitlyUnwrappedOptional, TypeRepr)
TYPEREPR(Tuple, TypeRepr)
TYPEREPR(Composition, TypeRepr)
TYPEREPR(Metatype, TypeRepr)
TYPEREPR(Protocol, TypeRepr)
ABSTRACT_TYPEREPR(Specifier, TypeRepr)
  TYPEREPR(InOut, SpecifierTypeRepr)
  TYPEREPR(Shared, SpecifierTypeRepr)
  TYPEREPR(Owned, SpecifierTypeRepr)
TYPEREPR(Fixed, TypeRepr)
LAST_TYPEREPR(Fixed)

#undef ABSTRACT_TYPEREPR
#undef TYPEREPR
#undef LAST_TYPEREPR
