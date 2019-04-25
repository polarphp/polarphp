//===--- EditorPlaceholder.h - Handling for editor placeholders -*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
//===----------------------------------------------------------------------===//
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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

#ifndef POLARPHP_UTILS_EDITOR_PLACEHOLDER_H
#define POLARPHP_UTILS_EDITOR_PLACEHOLDER_H

#include "polarphp/basic/adt/StringRef.h"

namespace polar::utils {

using polar::basic::StringRef;

enum class EditorPlaceholderKind
{
   Basic,
   Typed,
};

struct EditorPlaceholderData
{
   /// Placeholder kind.
   EditorPlaceholderKind kind;
   /// The part that is displayed in the editor.
   StringRef display;
   /// If kind is \c Typed, this is the type string for the placeholder.
   StringRef type;
   /// If kind is \c Typed, this is the type string to be considered for
   /// placeholder expansion.
   /// It can be same as \c Type or different if \c Type is a typealias.
   StringRef typeForExpansion;
};

/// Deconstructs a placeholder string and returns info about it.
/// \returns None if the \c PlaceholderText is not a valid placeholder string.
std::optional<EditorPlaceholderData> parse_editor_placeholder(StringRef placeholderText);

/// Checks if an identifier with the given text is an editor placeholder
bool is_editor_placeholder(StringRef identifierText);

} // polar::utils

#endif // POLARPHP_UTILS_EDITOR_PLACEHOLDER_H
