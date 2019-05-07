//===--- References.h - Swift Syntax Reference-Counting Misc. ---*- C++ -*-===//
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
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/07.

#ifndef POLARPHP_SYNTAX_REFERENCES_H
#define POLARPHP_SYNTAX_REFERENCES_H

#include "polarphp/basic/adt/IntrusiveRefCountPtr.h"

namespace polar::syntax {

/// A shorthand to clearly indicate that a value is a reference counted and
/// heap-allocated.
///
template <typename InnerClsType>
using RefCountPtr = polar::utils::IntrusiveRefCountPtr<InnerClsType>;

} // polar::syntax

#endif // POLARPHP_SYNTAX_REFERENCES_H
