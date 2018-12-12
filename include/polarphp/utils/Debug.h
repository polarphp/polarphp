// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/09.

#ifndef POLARPHP_UTILS_DEBUG_H
#define POLARPHP_UTILS_DEBUG_H

namespace polar {

namespace utils {
class RawOutStream;
} // utils

#ifndef NDEBUG

/// is_current_debug_type - Return true if the specified string is the debug type
/// specified on the command line, or if none was specified on the command line
/// with the -debug-only=X option.
///
bool is_current_debug_type(const char *Type);

/// set_current_debug_type - Set the current debug type, as if the -debug-only=X
/// option were specified.  Note that DebugFlag also needs to be set to true for
/// debug output to be produced.
///
void set_current_debug_type(const char *Type);

/// set_current_debug_types - Set the current debug type, as if the
/// -debug-only=X,Y,Z option were specified. Note that DebugFlag
/// also needs to be set to true for debug output to be produced.
///
void set_current_debug_types(const char **Types, unsigned Count);

/// DEBUG_WITH_TYPE macro - This macro should be used by passes to emit debug
/// information.  In the '-debug' option is specified on the commandline, and if
/// this is a debug build, then the code specified as the option to the macro
/// will be executed.  Otherwise it will not be.  Example:
///
/// DEBUG_WITH_TYPE("bitset", dbgs() << "Bitset contains: " << Bitset << "\n");
///
/// This will emit the debug information if -debug is present, and -debug-only
/// is not specified, or is specified as "bitset".
#define DEBUG_WITH_TYPE(TYPE, X)                                        \
   do { if (::polar::sg_debugFlag && ::polar::is_current_debug_type(TYPE)) { X; } \
} while (false)

#else
#define is_current_debug_type(X) (false)
#define set_current_debug_type(X)
#define set_current_debug_types(X, N)
#define DEBUG_WITH_TYPE(TYPE, X) do { } while (false)
#endif

/// This boolean is set to true if the '-debug' command line option
/// is specified.  This should probably not be referenced directly, instead, use
/// the DEBUG macro below.
///
extern bool sg_debugFlag;

/// \name Verification flags.
///
/// These flags turns on/off that are expensive and are turned off by default,
/// unless macro EXPENSIVE_CHECKS is defined. The flags allow selectively
/// turning the checks on without need to recompile.
/// \{

/// Enables verification of dominator trees.
///
extern bool g_verifyDomInfo;

/// Enables verification of loop info.
///
extern bool g_verifyLoopInfo;

/// Enables verification of MemorySSA.
///
extern bool g_verifyMemorySSA;

///\}

/// EnableDebugBuffering - This defaults to false.  If true, the debug
/// stream will install signal handlers to dump any buffered debug
/// output.  It allows clients to selectively allow the debug stream
/// to install signal handlers if they are certain there will be no
/// conflict.
///
extern bool g_enableDebugBuffering;

///\}

/// EnableDebugBuffering - This defaults to false.  If true, the debug
/// stream will install signal handlers to dump any buffered debug
/// output.  It allows clients to selectively allow the debug stream
/// to install signal handlers if they are certain there will be no
/// conflict.
///
extern bool sg_enableDebugBuffering;

utils::RawOutStream &debug_stream();

// DEBUG macro - This macro should be used by passes to emit debug information.
// In the '-debug' option is specified on the commandline, and if this is a
// debug build, then the code specified as the option to the macro will be
// executed.  Otherwise it will not be.  Example:
//
// POLAR_DEBUG(dbgs() << "Bitset contains: " << Bitset << "\n");
//
#define POLAR_DEBUG(X) DEBUG_WITH_TYPE(DEBUG_TYPE, X)

} // polar

#endif // POLARPHP_UTILS_DEBUG_H
