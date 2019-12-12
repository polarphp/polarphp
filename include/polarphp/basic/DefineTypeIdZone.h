//===--- DefineTypeIDZone.h - Define a TypeId Zone --------------*- C++ -*-===//
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
//  This file should be #included to define the TypeIDs for a given zone.
//  Two macros should be #define'd before inclusion, and will be #undef'd at
//  the end of this file:
//
//    POLAR_TYPEID_ZONE: The ID number of the Zone being defined, which must
//    be unique. 0 is reserved for basic C and LLVM types; 255 is reserved
//    for test cases.
//
//    POLAR_TYPEID_HEADER: A (quoted) name of the header to be
//    included to define the types in the zone.
//
//===----------------------------------------------------------------------===//
#ifndef POLAR_TYPEID_ZONE
#  error Must define the value of the TypeId zone with the given name.
#endif

#ifndef POLAR_TYPEID_HEADER
#  error Must define the TypeId header name with POLAR_TYPEID_HEADER
#endif

// Define a TypeId where the type name and internal name are the same.
#define POLAR_TYPEID(Type) POLAR_TYPEID_NAMED(Type, Type)
#define POLAR_REQUEST(Zone, Type, Sig, Caching, LocOptions) POLAR_TYPEID_NAMED(Type, Type)

// First pass: put all of the names into an enum so we get values for them.
template<> struct TypeIdZoneTypes<Zone::POLAR_TYPEID_ZONE> {
  enum Types : uint8_t {
#define POLAR_TYPEID_NAMED(Type, Name) Name,
#define POLAR_TYPEID_TEMPLATE1_NAMED(Template, Name, Param1, Arg1) Name,
#include POLAR_TYPEID_HEADER
#undef POLAR_TYPEID_NAMED
#undef POLAR_TYPEID_TEMPLATE1_NAMED
  };
};

// Second pass: create specializations of TypeId for these types.
#define POLAR_TYPEID_NAMED(Type, Name)                       \
template<> struct TypeId<Type> {                             \
  static constexpr uint8_t zoneID =                          \
    static_cast<uint8_t>(Zone::POLAR_TYPEID_ZONE);           \
  static constexpr uint8_t localID =                             \
    TypeIdZoneTypes<Zone::POLAR_TYPEID_ZONE>::Name;          \
                                                             \
  static constexpr uint64_t value = formTypeID(zoneID, localID); \
                                                             \
  static llvm::StringRef getName() { return #Name; }         \
};

#define POLAR_TYPEID_TEMPLATE1_NAMED(Template, Name, Param1, Arg1)    \
template<Param1> struct TypeId<Template<Arg1>> {                      \
private:                                                              \
  static constexpr uint64_t templateID =                                  \
    formTypeID(static_cast<uint8_t>(Zone::POLAR_TYPEID_ZONE),         \
               TypeIdZoneTypes<Zone::POLAR_TYPEID_ZONE>::Name);       \
                                                                      \
public:                                                               \
  static constexpr uint64_t value =                                       \
    (TypeId<Arg1>::value << 16) | templateID;                         \
                                                                      \
  static std::string getName() {                                      \
    return std::string(#Name) + "<" + TypeId<Arg1>::getName() + ">";  \
  }                                                                   \
};                                                                    \
                                                                      \
template<Param1> const uint64_t TypeId<Template<Arg1>>::value;

#include POLAR_TYPEID_HEADER

#undef POLAR_REQUEST

#undef POLAR_TYPEID_NAMED
#undef POLAR_TYPEID_TEMPLATE1_NAMED

#undef POLAR_TYPEID
#undef POLAR_TYPEID_ZONE
#undef POLAR_TYPEID_HEADER
