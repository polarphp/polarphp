//===--- EditorPlaceholder.cpp - Handling for editor placeholders ---------===//
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
// Created by polarboy on 2019/11/28.

//===----------------------------------------------------------------------===//
///
/// \file
/// Provides info about editor placeholders, <#such as this#>.
///
//===----------------------------------------------------------------------===//

#include "polarphp/basic/EditorPlaceholder.h"
#include "llvm/ADT/Optional.h"

namespace polar {

using namespace llvm;

// Placeholder text must start with '<#' and end with
// '#>'.
//
// Placeholder kinds:
//
// Typed:
//   'T##' display-string '##' type-string ('##' type-for-expansion-string)?
//   'T##' display-and-type-string
//
// Basic:
//   display-string
//
// NOTE: It is required that '##' is not a valid substring of display-string
// or type-string. If this ends up not the case for some reason, we can consider
// adding escaping for '##'.

Optional<EditorPlaceholderData> parse_editor_placeholder(StringRef placeholderText)
{
   if (!placeholderText.startswith("<#") ||
       !placeholderText.endswith("#>")) {
      return None;
   }
   placeholderText = placeholderText.drop_front(2).drop_back(2);
   EditorPlaceholderData phDataBasic;
   phDataBasic.kind = EditorPlaceholderKind::Basic;
   phDataBasic.display = placeholderText;
   if (!placeholderText.startswith("T##")) {
      // Basic.
      return phDataBasic;
   }

   // Typed.
   EditorPlaceholderData phDataTyped;
   phDataTyped.kind = EditorPlaceholderKind::Typed;

   assert(placeholderText.startswith("T##"));
   placeholderText = placeholderText.drop_front(3);
   size_t Pos = placeholderText.find("##");
   if (Pos == StringRef::npos) {
      phDataTyped.display = phDataTyped.type = phDataTyped.typeForExpansion =
            placeholderText;
      return phDataTyped;
   }
   phDataTyped.display = placeholderText.substr(0, Pos);

   placeholderText = placeholderText.substr(Pos+2);
   Pos = placeholderText.find("##");
   if (Pos == StringRef::npos) {
      phDataTyped.type = phDataTyped.typeForExpansion = placeholderText;
   } else {
      phDataTyped.type = placeholderText.substr(0, Pos);
      phDataTyped.typeForExpansion = placeholderText.substr(Pos+2);
   }

   return phDataTyped;
}

bool is_editor_placeholder(StringRef identifierText)
{
   return identifierText.startswith("<#");
}

} // polar
