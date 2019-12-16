//===--- KnownInterfaces.h - Working with compiler protocols -----*- C++ -*-===//
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
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/29.

#ifndef POLARPHP_AST_KNOWNP_INTERFACES_H
#define POLARPHP_AST_KNOWNP_INTERFACES_H

#include "polarphp/basic/InlineBitfield.h"

namespace llvm {
class StringRef;
}

namespace polar {

using polar::count_bits_used;

/// The set of known protocols.
enum class KnownInterfaceKind : std::uint8_t
{
#define INTERFACE_WITH_NAME(Id, Name) Id,
#include "polarphp/ast/KnownInterfacesDef.h"
};

enum : std::uint8_t
{
   // This uses a preprocessor trick to count all the protocols. The enum value
   // expression below expands to "+1+1+1...". (Note that the first plus
   // is parsed as a unary operator.)
#define INTERFACE_WITH_NAME(Id, Name) +1
   /// The number of known protocols.
   NumKnownInterfaces =
#include "polarphp/ast/KnownInterfacesDef.h"
};

enum : unsigned
{
   NumKnownInterfaceKindBits =
   count_bits_used(static_cast<unsigned>(NumKnownInterfaces - 1))
};

/// Retrieve the name of the given known protocol.
llvm::StringRef get_interface_name(KnownInterfaceKind kind);

} // polar

#endif // POLARPHP_AST_KNOWNP_INTERFACES_H
