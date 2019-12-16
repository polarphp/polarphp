//===--- EditorPlaceholder.h - Handling for editor placeholders -*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//===----------------------------------------------------------------------===//
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

#ifndef POLARPHP_BASIC_EDITORPLACEHOLDER_H
#define POLARPHP_BASIC_EDITORPLACEHOLDER_H

#include "llvm/ADT/StringRef.h"
#include "polarphp/basic/LLVM.h"

namespace polar {

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
Optional<EditorPlaceholderData>
parse_editor_placeholder(StringRef placeholderText);

/// Checks if an identifier with the given text is an editor placeholder
bool is_editor_placeholder(StringRef identifierText);


} // polar

#endif // POLARPHP_BASIC_EDITORPLACEHOLDER_H
