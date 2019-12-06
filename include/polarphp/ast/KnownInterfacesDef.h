//===--- KnownInterfaces.def - Compiler protocol metaprogramming -*- C++ -*-===//
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
//
// This file defines macros used for macro-metaprogramming with compiler-known
// protocols. Note that this mechanism does not look through an overlay into its
// underlying module, so it typically cannot find Objective-C protocols.
//
//===----------------------------------------------------------------------===//


/// \def INTERFACE_WITH_NAME(Id, Name)
///
/// The enumerator value is \c KnownInterfaceKind::Id. The protocol represented
/// is simply named \p Name.
#ifndef INTERFACE_WITH_NAME
#define INTERFACE_WITH_NAME(Id, Name)
#endif

/// \def EXPRESSIBLE_BY_LITERAL_INTERFACE_WITH_NAME(Id, Name, typeName, performLocalLookup)
/// \param typeName supplies the name used for type lookup,
/// \param performLocalLookup specifies whether to first look in the local context.
#ifndef EXPRESSIBLE_BY_LITERAL_INTERFACE_WITH_NAME
#define EXPRESSIBLE_BY_LITERAL_INTERFACE_WITH_NAME(Id, Name, typeName, performLocalLookup) \
    INTERFACE_WITH_NAME(Id, Name)
#endif

/// \def BUILTIN_EXPRESSIBLE_BY_LITERAL_INTERFACE_WITH_NAME(Id, Name)
///
/// Note that this is not a special form of EXPRESSIBLE_BY_LITERAL_INTERFACE.
#ifndef BUILTIN_EXPRESSIBLE_BY_LITERAL_INTERFACE_WITH_NAME
#define BUILTIN_EXPRESSIBLE_BY_LITERAL_INTERFACE_WITH_NAME(Id, Name) \
    INTERFACE_WITH_NAME(Id, Name)
#endif


#define INTERFACE(name) INTERFACE_WITH_NAME(name, #name)
#define INTERFACE_(name) INTERFACE_WITH_NAME(name, "_" #name)

/// \param typeName supplies the name used for type lookup,
/// \param performLocalLookup specifies whether to first look in the local context.
#define EXPRESSIBLE_BY_LITERAL_INTERFACE(name, typeName, performLocalLookup) \
    EXPRESSIBLE_BY_LITERAL_INTERFACE_WITH_NAME(name, #name, typeName, performLocalLookup)

/// \param typeName supplies the name used for type lookup,
/// \param performLocalLookup specifies whether to first look in the local context.
#define EXPRESSIBLE_BY_LITERAL_INTERFACE_(name, typeName, performLocalLookup) \
    EXPRESSIBLE_BY_LITERAL_INTERFACE_WITH_NAME(name, "_" #name, typeName, performLocalLookup)

#define BUILTIN_EXPRESSIBLE_BY_LITERAL_INTERFACE_(name) \
    BUILTIN_EXPRESSIBLE_BY_LITERAL_INTERFACE_WITH_NAME(name, "_" #name)

INTERFACE(Sequence)
INTERFACE(IteratorInterface)
INTERFACE(RawRepresentable)
INTERFACE(Equatable)
INTERFACE(Hashable)
INTERFACE(Comparable)
INTERFACE(Error)
INTERFACE_(ErrorCodeInterface)
INTERFACE(OptionSet)
INTERFACE(CaseIterable)
INTERFACE(SIMDScalar)

INTERFACE_(BridgedNSError)
INTERFACE_(BridgedStoredNSError)
INTERFACE_(CFObject)
INTERFACE_(SwiftNewtypeWrapper)
INTERFACE(CodingKey)
INTERFACE(Encodable)
INTERFACE(Decodable)

INTERFACE_(ObjectiveCBridgeable)
INTERFACE_(DestructorSafeContainer)

INTERFACE(StringInterpolationInterface)

INTERFACE(Differentiable)

EXPRESSIBLE_BY_LITERAL_INTERFACE(ExpressibleByArrayLiteral, "Array", false)
EXPRESSIBLE_BY_LITERAL_INTERFACE(ExpressibleByBooleanLiteral, "BooleanLiteralType", true)
EXPRESSIBLE_BY_LITERAL_INTERFACE(ExpressibleByDictionaryLiteral, "Dictionary", false)
EXPRESSIBLE_BY_LITERAL_INTERFACE(ExpressibleByExtendedGraphemeClusterLiteral, "ExtendedGraphemeClusterType", true)
EXPRESSIBLE_BY_LITERAL_INTERFACE(ExpressibleByFloatLiteral, "FloatLiteralType", true)
EXPRESSIBLE_BY_LITERAL_INTERFACE(ExpressibleByIntegerLiteral, "IntegerLiteralType", true)
EXPRESSIBLE_BY_LITERAL_INTERFACE(ExpressibleByStringInterpolation, "StringLiteralType", true)
EXPRESSIBLE_BY_LITERAL_INTERFACE(ExpressibleByStringLiteral, "StringLiteralType", true)
EXPRESSIBLE_BY_LITERAL_INTERFACE(ExpressibleByNilLiteral, nullptr, false)
EXPRESSIBLE_BY_LITERAL_INTERFACE(ExpressibleByUnicodeScalarLiteral, "UnicodeScalarType", true)

EXPRESSIBLE_BY_LITERAL_INTERFACE_(ExpressibleByColorLiteral, "_ColorLiteralType", true)
EXPRESSIBLE_BY_LITERAL_INTERFACE_(ExpressibleByImageLiteral, "_ImageLiteralType", true)
EXPRESSIBLE_BY_LITERAL_INTERFACE_(ExpressibleByFileReferenceLiteral, "_FileReferenceLiteralType", true)

BUILTIN_EXPRESSIBLE_BY_LITERAL_INTERFACE_(ExpressibleByBuiltinBooleanLiteral)
BUILTIN_EXPRESSIBLE_BY_LITERAL_INTERFACE_(ExpressibleByBuiltinExtendedGraphemeClusterLiteral)
BUILTIN_EXPRESSIBLE_BY_LITERAL_INTERFACE_(ExpressibleByBuiltinFloatLiteral)
BUILTIN_EXPRESSIBLE_BY_LITERAL_INTERFACE_(ExpressibleByBuiltinIntegerLiteral)
BUILTIN_EXPRESSIBLE_BY_LITERAL_INTERFACE_(ExpressibleByBuiltinStringLiteral)
BUILTIN_EXPRESSIBLE_BY_LITERAL_INTERFACE_(ExpressibleByBuiltinUnicodeScalarLiteral)

#undef EXPRESSIBLE_BY_LITERAL_INTERFACE
#undef EXPRESSIBLE_BY_LITERAL_INTERFACE_
#undef EXPRESSIBLE_BY_LITERAL_INTERFACE_WITH_NAME
#undef BUILTIN_EXPRESSIBLE_BY_LITERAL_INTERFACE_
#undef BUILTIN_EXPRESSIBLE_BY_LITERAL_INTERFACE_WITH_NAME
#undef INTERFACE
#undef INTERFACE_
#undef INTERFACE_WITH_NAME
