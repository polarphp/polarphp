//===--- Callee.h -----------------------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_GEN_CALLEE_H
#define POLARPHP_PIL_GEN_CALLEE_H

#include "polarphp/ast/ForeignErrorConvention.h"
#include "polarphp/ast/Types.h"
#include "polarphp/pil/lang/AbstractionPattern.h"

namespace polar::lowering {

class CalleeTypeInfo {
public:
   CanPILFunctionType substFnType;
   Optional<AbstractionPattern> origResultType;
   CanType substResultType;
   Optional<ForeignErrorConvention> foreignError;
   ImportAsMemberStatus foreignSelf;

private:
   Optional<PILFunctionTypeRepresentation> overrideRep;

public:
   CalleeTypeInfo() = default;

   CalleeTypeInfo(CanPILFunctionType substFnType,
                  AbstractionPattern origResultType, CanType substResultType,
                  const Optional<ForeignErrorConvention> &foreignError,
                  ImportAsMemberStatus foreignSelf,
                  Optional<PILFunctionTypeRepresentation> overrideRep = None)
      : substFnType(substFnType), origResultType(origResultType),
        substResultType(substResultType), foreignError(foreignError),
        foreignSelf(foreignSelf), overrideRep(overrideRep) {}

   CalleeTypeInfo(CanPILFunctionType substFnType,
                  AbstractionPattern origResultType, CanType substResultType,
                  Optional<PILFunctionTypeRepresentation> overrideRep = None)
      : substFnType(substFnType), origResultType(origResultType),
        substResultType(substResultType), foreignError(), foreignSelf(),
        overrideRep(overrideRep) {}

   PILFunctionTypeRepresentation getOverrideRep() const {
      return overrideRep.getValueOr(substFnType->getRepresentation());
   }
};

} // namespace polar::lowering

#endif // POLARPHP_PIL_GEN_CALLEE_H
