//// Automatically Generated From UnicodeExtendedGraphemeClusters.cpp.gyb.
//// Do Not Edit Directly!
//===----------------------------------------------------------------------===//
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

#include "polarphp/basic/Unicode.h"

namespace polar::unicode {

/// @todo use gyb to generate unicode data

GraphemeClusterBreakProperty getGraphemeClusterBreakProperty(uint32_t C) {
   // FIXME: replace linear search with a trie lookup.
   return GraphemeClusterBreakProperty::Other;
}

const uint16_t ExtendedGraphemeClusterNoBoundaryRulesMatrix[] = {
   1
};

} // polar::unicode


