//===--- PlatformKind.h - Swift Language platform Kinds ---------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
//===----------------------------------------------------------------------===//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/04/26.
//===----------------------------------------------------------------------===//
//
// This file defines the platform kinds for API availability.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_PLATFORM_KIND_H
#define POLARPHP_AST_PLATFORM_KIND_H

#include "polarphp/basic/adt/StringRef.h"

/// forward declare class with namespace
namespace polar::basic {
class LangOptions;
}

namespace polar::ast {

using polar::basic::StringRef;
using polar::basic::LangOptions;

/// Available platforms for the availability attribute.
enum class PlatformKind
{
  none,
#define AVAILABILITY_PLATFORM(X, PrettyName) X,
#include "polarphp/ast/PlatformKindsDefs.h"
};

/// Returns the short string representing the platform, suitable for
/// use in availability specifications (e.g., "OSX").
StringRef platform_string(PlatformKind platform);

/// Returns the platform kind corresponding to the passed-in short platform name
/// or None if such a platform kind does not exist.
std::optional<PlatformKind> platform_from_string(StringRef Name);

/// Returns a human-readable version of the platform name as a string, suitable
/// for emission in diagnostics (e.g., "OS X").
StringRef pretty_platform_string(PlatformKind platform);

/// Returns whether the passed-in platform is active, given the language
/// options. A platform is active if either it is the target platform or its
/// AppExtension variant is the target platform. For example, OS X is
/// considered active when the target operating system is OS X and app extension
/// restrictions are enabled, but OSXApplicationExtension is not considered
/// active when the target platform is OS X and app extension restrictions are
/// disabled. PlatformKind::none is always considered active.
bool is_platform_active(PlatformKind platform, LangOptions &langOpts);

/// Returns the target platform for the given language options.
PlatformKind target_platform(LangOptions &langOpts);

} // polar::ast

#endif // POLARPHP_AST_PLATFORM_KIND_H
