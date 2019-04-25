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
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/25.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Provides info about editor placeholders, <#such as this#>.
///
//===----------------------------------------------------------------------===//

#include "polarphp/utils/EditorPlaceholder.h"
#include <optional>

namespace polar::utils {

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

std::optional<EditorPlaceholderData> parse_editor_placeholder(StringRef placeholderText)
{
   if (!placeholderText.startsWith("<#") ||
       !placeholderText.endsWith("#>")) {
      return std::nullopt;
   }
   placeholderText = placeholderText.dropFront(2).dropBack(2);
   EditorPlaceholderData PHDataBasic;
   PHDataBasic.kind = EditorPlaceholderKind::Basic;
   PHDataBasic.display = placeholderText;

   if (!placeholderText.startsWith("T##")) {
      // Basic.
      return PHDataBasic;
   }

   // Typed.
   EditorPlaceholderData PHDataTyped;
   PHDataTyped.kind = EditorPlaceholderKind::Typed;

   assert(placeholderText.startsWith("T##"));
   placeholderText = placeholderText.dropFront(3);
   size_t pos = placeholderText.find("##");
   if (pos == StringRef::npos) {
      PHDataTyped.display = PHDataTyped.type = PHDataTyped.typeForExpansion =
            placeholderText;
      return PHDataTyped;
   }
   PHDataTyped.display = placeholderText.substr(0, pos);

   placeholderText = placeholderText.substr(pos+2);
   pos = placeholderText.find("##");
   if (pos == StringRef::npos) {
      PHDataTyped.type = PHDataTyped.typeForExpansion = placeholderText;
   } else {
      PHDataTyped.type = placeholderText.substr(0, pos);
      PHDataTyped.typeForExpansion = placeholderText.substr(pos+2);
   }

   return PHDataTyped;
}

bool is_editor_placeholder(StringRef identifierText)
{
   return identifierText.startsWith("<#");
}

} // polar::utils
