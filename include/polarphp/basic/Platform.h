//===--- Platform.h - Helpers related to target platforms -------*- C++ -*-===//
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
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/02.

#ifndef POLARPHP_BASIC_PLATFORM_H
#define POLARPHP_BASIC_PLATFORM_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/global/Config.h"
#include "llvm/ADT/StringRef.h"

namespace llvm {
class Triple;
class VersionTuple;
} // llvm

namespace polar {

enum class DarwinPlatformKind : unsigned
{
   MacOS,
   IPhoneOS,
   IPhoneOSSimulator,
   TvOS,
   TvOSSimulator,
   WatchOS,
   WatchOSSimulator
};

/// Returns true if the given triple represents iOS running in a simulator.
bool triple_is_ios_simulator(const llvm::Triple &triple);

/// Returns true if the given triple represents AppleTV running in a simulator.
bool triple_is_apple_tv_simulator(const llvm::Triple &triple);

/// Returns true if the given triple represents watchOS running in a simulator.
bool triple_is_watch_simulator(const llvm::Triple &triple);

/// Return true if the given triple represents any simulator.
bool triple_is_any_simulator(const llvm::Triple &triple);

/// Returns the platform Kind for Darwin triples.
DarwinPlatformKind get_darwin_platform_kind(const llvm::Triple &triple);

/// Maps an arbitrary platform to its non-simulator equivalent.
///
/// If \p platform is not a simulator platform, it will be returned as is.
DarwinPlatformKind get_non_simulator_platform(DarwinPlatformKind platform);

/// Returns the platform name for a given target triple.
///
/// For example, the iOS simulator has the name "iphonesimulator", while real
/// iOS uses "iphoneos". OS X is "macosx". (These names are intended to be
/// compatible with Xcode's SDKs.)
///
/// If the triple does not correspond to a known platform, the empty string is
/// returned.
StringRef get_platform_name_for_triple(const llvm::Triple &triple);

/// Returns true if the given triple represents an OS that ships with
/// ABI-stable php libraries (eg. in /usr/lib/swift).
bool triple_requires_rpath_for_php_in_os(const llvm::Triple &triple);


/// Returns the architecture component of the path for a given target triple.
///
/// Typically this is used for mapping the architecture component of the
/// path.
///
/// For example, on Linux "armv6l" and "armv7l" are mapped to "armv6" and
/// "armv7", respectively, within LLVM. Therefore the component path for the
/// architecture specific objects will be found in their "mapped" paths.
///
/// This is a stop-gap until full Triple support (ala Clang) exists within swiftc.
StringRef get_major_architecture_name(const llvm::Triple &triple);

/// Computes the normalized target triple used as the most preferred name for
/// module loading.
///
/// For platforms with fat binaries, this canonicalizes architecture,
/// vendor, and OS names, strips OS versions, and makes inferred environments
/// explicit. For other platforms, it returns the unmodified triple.
///
/// The input triple should already be "normalized" in the sense that
/// llvm::Triple::normalize() would not affect it.
llvm::Triple get_target_specific_module_triple(const llvm::Triple &triple);


/// Get the polarphp runtime version to deploy back to, given a deployment target expressed as an
/// LLVM target triple.
Optional<llvm::VersionTuple>
get_runtime_compatibility_version_for_target(const llvm::Triple &Triple);

} // polar

#endif // POLARPHP_BASIC_PLATFORM_H

