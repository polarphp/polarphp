//===--- TypeID.h - Simple Type Identification ------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/29.
//===----------------------------------------------------------------------===//
//
//  This file defines the TypeID template, which provides a numeric
//  encoding of (static) type information for use as a simple replacement
//  for run-time type information.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_TYPEID_H
#define POLARPHP_BASIC_TYPEID_H

#include "llvm/ADTStringRef.h"
#include "llvm/ADTTinyPtrVector.h"
#include <cstdint>
#include <string>
#include <vector>

namespace polar::basic {

/// Form a unique 64-bit integer value describing the type `T`.
///
/// This template needs to be specialized for every type that can
/// participate in this kind of run-time type information, e.g., so
/// that it can be stored in \c AnyValue.
template<typename T>
struct TypeId;

/// Template whose specializations provide the set of type IDs within a
/// given zone.
template<uint8_t Zone>
struct TypeIdZoneTypes;

/// Form a type ID given a zone and type value.
constexpr uint64_t form_type_id(uint8_t zone, uint8_t type)
{
   return (uint64_t(zone) << 8) | uint64_t(type);
}

// Define the C type zone (zone 0).
#define POLAR_TYPEID_ZONE 0
#define POLAR_TYPEID_HEADER "polarphp/basic/CTypeIdZoneDefs.h"
#include "polarphp/basic/DefineTypeIdZone.h"

} // polar::basic

#endif // POLARPHP_BASIC_TYPEID_H
