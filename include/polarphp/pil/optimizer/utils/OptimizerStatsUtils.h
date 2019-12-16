//===--- OptimizerStatsUtils.h - Utils for collecting stats  --*- C++ ---*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_UTILS_STATS_UTILS_H
#define POLARPHP_PIL_OPTIMIZER_UTILS_STATS_UTILS_H

namespace polar {

class PILModule;
class PILTransform;
class PILPassManager;

/// Updates PILModule stats before executing the transform \p Transform.
///
/// \param M PILModule to be processed
/// \param Transform the PIL transformation that was just executed
/// \param PM the PassManager being used
void updatePILModuleStatsBeforeTransform(PILModule &M, PILTransform *Transform,
                                         PILPassManager &PM, int PassNumber);

/// Updates PILModule stats after finishing executing the
/// transform \p Transform.
///
/// \param M PILModule to be processed
/// \param Transform the PIL transformation that was just executed
/// \param PM the PassManager being used
void updatePILModuleStatsAfterTransform(PILModule &M, PILTransform *Transform,
                                        PILPassManager &PM, int PassNumber,
                                        int Duration);

} // end namespace polar

#endif
