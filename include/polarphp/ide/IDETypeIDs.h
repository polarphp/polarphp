//===--- IDETypeIDs.h - IDE Type Ids ----------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
//  This file defines TypeID support for IDE types.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IDE_IDETYPEIDS_H
#define POLARPHP_IDE_IDETYPEIDS_H

#include "polarphp/basic/TypeId.h"

namespace polar {

#define POLAR_TYPEID_ZONE IDETypes
#define POLAR_TYPEID_HEADER "polarphp/ide/IDETypeIDZoneDef.h"
#include "polarphp/basic/DefineTypeIDZone.h"

} // end namespace polar

#endif // POLARPHP_IDE_IDETYPEIDS_H
