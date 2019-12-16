//===--- ReferenceCounting.h ------------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
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
// Created by polarboy on 2019/11/27.

#ifndef POLARPHP_AST_REFERENCE_COUNTING_H
#define POLARPHP_AST_REFERENCE_COUNTING_H

#include <cstdint>

namespace polar {

/// The kind of reference counting implementation a heap object uses.
enum class ReferenceCounting : std::uint8_t
{
   /// The object uses native Swift reference counting.
   Native,

   /// The object uses _Block_copy/_Block_release reference counting.
   ///
   /// This is a strict subset of ObjC; all blocks are also ObjC reference
   /// counting compatible. The block is assumed to have already been moved to
   /// the heap so that _Block_copy returns the same object back.
   Block,

   /// The object has an unknown reference counting implementation.
   ///
   /// This uses maximally-compatible reference counting entry points in the
   /// runtime.
   Unknown,

   /// Cases prior to this one are binary-compatible with Unknown reference
   /// counting.
   LastUnknownCompatible = Unknown,

   /// The object has an unknown reference counting implementation and
   /// the reference value may contain extra bits that need to be masked.
   ///
   /// This uses maximally-compatible reference counting entry points in the
   /// runtime, with a masking layer on top. A bit inside the pointer is used
   /// to signal native Swift refcounting.
   Bridge,

   /// The object uses ErrorType's reference counting entry points.
   Error,
};

}

#endif // POLARPHP_AST_REFERENCE_COUNTING_H
