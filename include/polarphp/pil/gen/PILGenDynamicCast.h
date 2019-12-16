//===--- PILGenDynamicCast.h - PILGen for dynamic casts ---------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_GEN_PILGEN_DYNAMIC_CAST_H
#define POLARPHP_PIL_GEN_PILGEN_DYNAMIC_CAST_H

#include "polarphp/pil/gen/PILGenFunction.h"

namespace polar::lowering {

RValue emitUnconditionalCheckedCast(PILGenFunction &SGF,
                                    PILLocation loc,
                                    Expr *operand,
                                    Type targetType,
                                    CheckedCastKind castKind,
                                    SGFContext C);

RValue emitConditionalCheckedCast(PILGenFunction &SGF, PILLocation loc,
                                  ManagedValue operand, Type operandType,
                                  Type targetType, CheckedCastKind castKind,
                                  SGFContext C, ProfileCounter TrueCount,
                                  ProfileCounter FalseCount);

PILValue emitIsa(PILGenFunction &SGF, PILLocation loc,
                 Expr *operand, Type targetType,
                 CheckedCastKind castKind);

} // polar::lowering

#endif // POLARPHP_PIL_GEN_PILGEN_DYNAMIC_CAST_H
