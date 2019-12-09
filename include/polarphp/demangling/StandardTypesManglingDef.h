//===--- StandardTypesMangling.def - Mangling Metaprogramming ---*- C++ -*-===//
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

/// STANDARD_TYPE(KIND, MANGLING, TYPENAME)
///   The 1-character MANGLING for a known TYPENAME of KIND.

/// OBJC_INTEROP_STANDARD_TYPE(KIND, MANGLING, TYPENAME)
///   The 1-character MANGLING for a known TYPENAME of KIND, for a type that's
///   only available with ObjC interop enabled.

#ifndef OBJC_INTEROP_STANDARD_TYPE
#define OBJC_INTEROP_STANDARD_TYPE(KIND, MANGLING, TYPENAME) \
  STANDARD_TYPE(KIND, MANGLING, TYPENAME)
#endif

OBJC_INTEROP_STANDARD_TYPE(Structure, A, AutoreleasingUnsafeMutablePointer)
STANDARD_TYPE(Structure, a, Array)
STANDARD_TYPE(Structure, b, Bool)
STANDARD_TYPE(Structure, D, Dictionary)
STANDARD_TYPE(Structure, d, Double)
STANDARD_TYPE(Structure, f, Float)
STANDARD_TYPE(Structure, h, Set)
STANDARD_TYPE(Structure, I, DefaultIndices)
STANDARD_TYPE(Structure, i, Int)
STANDARD_TYPE(Structure, J, Character)
STANDARD_TYPE(Structure, N, ClosedRange)
STANDARD_TYPE(Structure, n, Range)
STANDARD_TYPE(Structure, O, ObjectIdentifier)
STANDARD_TYPE(Structure, P, UnsafePointer)
STANDARD_TYPE(Structure, p, UnsafeMutablePointer)
STANDARD_TYPE(Structure, R, UnsafeBufferPointer)
STANDARD_TYPE(Structure, r, UnsafeMutableBufferPointer)
STANDARD_TYPE(Structure, S, String)
STANDARD_TYPE(Structure, s, Substring)
STANDARD_TYPE(Structure, u, UInt)
STANDARD_TYPE(Structure, V, UnsafeRawPointer)
STANDARD_TYPE(Structure, v, UnsafeMutableRawPointer)
STANDARD_TYPE(Structure, W, UnsafeRawBufferPointer)
STANDARD_TYPE(Structure, w, UnsafeMutableRawBufferPointer)

STANDARD_TYPE(Enum, q, Optional)

STANDARD_TYPE(Interface, B, BinaryFloatingPoint)
STANDARD_TYPE(Interface, E, Encodable)
STANDARD_TYPE(Interface, e, Decodable)
STANDARD_TYPE(Interface, F, FloatingPoint)
STANDARD_TYPE(Interface, G, RandomNumberGenerator)
STANDARD_TYPE(Interface, H, Hashable)
STANDARD_TYPE(Interface, j, Numeric)
STANDARD_TYPE(Interface, K, BidirectionalCollection)
STANDARD_TYPE(Interface, k, RandomAccessCollection)
STANDARD_TYPE(Interface, L, Comparable)
STANDARD_TYPE(Interface, l, Collection)
STANDARD_TYPE(Interface, M, MutableCollection)
STANDARD_TYPE(Interface, m, RangeReplaceableCollection)
STANDARD_TYPE(Interface, Q, Equatable)
STANDARD_TYPE(Interface, T, Sequence)
STANDARD_TYPE(Interface, t, IteratorInterface)
STANDARD_TYPE(Interface, U, UnsignedInteger)
STANDARD_TYPE(Interface, X, RangeExpression)
STANDARD_TYPE(Interface, x, Strideable)
STANDARD_TYPE(Interface, Y, RawRepresentable)
STANDARD_TYPE(Interface, y, StringInterface)
STANDARD_TYPE(Interface, Z, SignedInteger)
STANDARD_TYPE(Interface, z, BinaryInteger)

#undef STANDARD_TYPE
#undef OBJC_INTEROP_STANDARD_TYPE
