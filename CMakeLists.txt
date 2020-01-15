# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2020/01/01.

cmake_minimum_required(VERSION 3.10.2)

if(POLICY CMP0068)
   cmake_policy(SET CMP0068 NEW)
   set(CMAKE_BUILD_WITH_INSTALL_NAME_DIR ON)
endif()
if(POLICY CMP0068)
   cmake_policy(SET CMP0077 OLD)
endif()

set(CMAKE_DISABLE_IN_SOURCE_BUILD YES)

if(DEFINED CMAKE_JOB_POOLS)
   # CMake < 3.11 doesn't support CMAKE_JOB_POOLS. Manually set the property.
   set_property(GLOBAL PROPERTY JOB_POOLS "${CMAKE_JOB_POOLS}")
else()
   # Make a job pool for things that can't yet be distributed
   cmake_host_system_information(
      RESULT localhost_logical_cores QUERY NUMBER_OF_LOGICAL_CORES)
   set_property(GLOBAL APPEND PROPERTY JOB_POOLS local_jobs=${localhost_logical_cores})
   # Put linking in that category
   set(CMAKE_JOB_POOL_LINK local_jobs)
endif()

if(NOT DEFINED POLAR_VERSION_MAJOR)
   set(POLAR_VERSION_MAJOR 0)
endif()
if(NOT DEFINED POLAR_VERSION_MINOR)
   set(POLAR_VERSION_MINOR 0)
endif()
if(NOT DEFINED POLAR_VERSION_PATCH)
   set(POLAR_VERSION_PATCH 1)
endif()
if(NOT DEFINED POLAR_VERSION_SUFFIX)
   set(POLAR_VERSION_SUFFIX git)
endif()

if (NOT POLAR_PACKAGE_VERSION)
   set(POLAR_PACKAGE_VERSION
      "${POLAR_VERSION_MAJOR}.${POLAR_VERSION_MINOR}.${POLAR_VERSION_PATCH}-${POLAR_VERSION_SUFFIX}")
endif()

if (NOT POLAR_VERSION)
   set(POLAR_VERSION
      "${POLAR_VERSION_MAJOR}.${POLAR_VERSION_MINOR}.${POLAR_VERSION_PATCH}")
endif()

math(EXPR POLAR_VERSION_ID ${POLAR_VERSION_MAJOR}*10000+${POLAR_VERSION_MINOR}*100+${POLAR_VERSION_PATCH})

set(POLAR_PACKAGE_NAME polarphp)
set(POLAR_PACKAGE_STRING "${POLAR_PACKAGE_NAME} ${POLAR_PACKAGE_VERSION}")
set(POLAR_PACKAGE_BUGREPORT "https://polarphp.org/bugs/")

project(${POLAR_PACKAGE_NAME} LANGUAGES C CXX
   VERSION ${POLAR_VERSION_MAJOR}.${POLAR_VERSION_MINOR}.${POLAR_VERSION_PATCH})

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
   message(STATUS "No build type selected, default to Debug")
   set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (default Debug)" FORCE)
endif()

# setup some important path

set(POLAR_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}") # --src-root
set(POLAR_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}") # --prefix

set(POLAR_THIRDPARTY_DIR "${POLAR_SOURCE_DIR}/thirdparty")
set(POLAR_DEPS_INSTALL_DIR "${POLAR_BINARY_DIR}/depslibs")
set(POLAR_CMAKE_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
set(POLAR_CMAKE_SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/scripts")
set(POLAR_MAIN_INCLUDE_DIR "${POLAR_SOURCE_DIR}/include")
set(POLAR_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include") # --includedir
set(POLAR_MAIN_INCLUDE_DIR "${POLAR_SOURCE_DIR}/include")
set(POLAR_TEST_DIR "${POLAR_SOURCE_DIR}/tests")
set(POLAR_ASSET_DIR "${POLAR_SOURCE_DIR}/assets")
set(POLAR_CMAKE_TEST_CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake/codes)
set(POLAR_DEVTOOLS_DIR ${CMAKE_SOURCE_DIR}/devtools)
set(POLAR_LLVM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/llvm-project/llvm)
set(POLAR_CLANG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/llvm-project/clang)

set(POLAR_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_NAME})
string(TOLOWER ${POLAR_SYSTEM_NAME} POLAR_SYSTEM_NORMAL_NAME)

# Add path for custom CMake modules.
list(APPEND CMAKE_MODULE_PATH
   "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
   "${POLAR_CMAKE_MODULES_DIR}"
   "${POLAR_CMAKE_MODULES_DIR}/clang"
   "${POLAR_CMAKE_MODULES_DIR}/llvm"
   )

list(APPEND CMAKE_INSTALL_RPATH "${POLAR_DEPS_INSTALL_DIR}/lib")

if (NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Debug")
endif()

string(TOLOWER ${CMAKE_BUILD_TYPE} POLAR_BUILD_TYPE)
string(TOUPPER ${CMAKE_BUILD_TYPE} POLAR_BUILD_CONFIG)

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL Linux)
   set(LINUX ON)
endif()

if (CMAKE_BUILD_TYPE AND
   NOT POLAR_BUILD_TYPE MATCHES "^(debug|release|relwithdebinfo|minsizerel)$")
   message(FATAL_ERROR "Invalid value for CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

include(ProcessorCount)
ProcessorCount(POLAR_PROCESSOR_COUNT)
if(NOT POLAR_PROCESSOR_COUNT EQUAL 0)
   set(POLAR_BUILD_FLAGS -j${POLAR_PROCESSOR_COUNT})
endif()

# First include general CMake utilities.
include(PolarUtils)
include(CheckSymbolExists)

#
# User-configurable options that control the inclusion and default build
# behavior for components which may not strictly be necessary (tools, examples,
# and tests).
#
# This is primarily to support building smaller or faster project files.
#
option(POLAR_INCLUDE_TOOLS
   "Generate build targets for polarphp tools"
   TRUE)

option(POLAR_BUILD_REMOTE_MIRROR
   "Build the polarphp Remote Mirror Library"
   TRUE)

option(POLAR_BUILD_DYNAMIC_STDLIB
   "Build dynamic variants of the polarphp standard library"
   TRUE)

option(POLAR_BUILD_STATIC_STDLIB
   "Build static variants of the polarphp standard library"
   FALSE)

option(POLAR_BUILD_DYNAMIC_SDK_OVERLAY
   "Build dynamic variants of the polarphp SDK overlay"
   TRUE)

option(POLAR_BUILD_STATIC_SDK_OVERLAY
   "Build static variants of the polarphp SDK overlay"
   FALSE)

option(POLAR_BUILD_STDLIB_EXTRA_TOOLCHAIN_CONTENT
   "If not building stdlib, controls whether to build 'stdlib/toolchain' content"
   TRUE)

# In many cases, the CMake build system needs to determine whether to include
# a directory, or perform other actions, based on whether the stdlib or SDK is
# being built at all -- statically or dynamically. Please note that these
# flags are not related to the deprecated build-script-impl arguments
# 'build-php-stdlib' and 'build-php-sdk-overlay'. These are not flags that
# the build script should be able to set.
if(POLAR_BUILD_DYNAMIC_STDLIB OR POLAR_BUILD_STATIC_STDLIB)
   set(POLAR_BUILD_STDLIB TRUE)
else()
   set(POLAR_BUILD_STDLIB FALSE)
endif()

if(POLAR_BUILD_DYNAMIC_SDK_OVERLAY OR POLAR_BUILD_STATIC_SDK_OVERLAY)
   set(POLAR_BUILD_SDK_OVERLAY TRUE)
else()
   set(POLAR_BUILD_SDK_OVERLAY FALSE)
endif()

option(POLAR_BUILD_PERF_TESTSUITE
   "Create in-tree targets for building polarphp performance benchmarks."
   FALSE)

option(POLAR_BUILD_EXTERNAL_PERF_TESTSUITE
   "Create out-of-tree targets for building polarphp performance benchmarks."
   FALSE)

option(POLAR_INCLUDE_TESTS "Create targets for building/running tests." TRUE)

option(POLAR_INCLUDE_DOCS
   "Create targets for building docs."
   TRUE)

set(_polar_include_apinotes_default FALSE)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
   set(_polar_include_apinotes_default TRUE)
endif()

set(POLAR_ANALYZE_CODE_COVERAGE FALSE CACHE STRING
   "Build polarphp with code coverage instrumenting enabled [FALSE, NOT-MERGED, MERGED]")

set(POLAR_VENDOR "" CACHE STRING
   "The vendor name of the polarphp compiler")
set(POLAR_COMPILER_VERSION "" CACHE STRING
   "The internal version of the polarphp compiler")
set(CLANG_COMPILER_VERSION "" CACHE STRING
   "The internal version of the Clang compiler")

# Indicate whether polarphp should attempt to use the lld linker.
set(POLAR_ENABLE_LLD_LINKER TRUE CACHE BOOL
   "Enable using the lld linker when available")

# Indicate whether polarphp should attempt to use the gold linker.
# This is not used on Darwin.
set(POLAR_ENABLE_GOLD_LINKER TRUE CACHE BOOL
   "Enable using the gold linker when available")

set(POLAR_TOOLS_ENABLE_LTO OFF CACHE STRING "Build polarphp tools with LTO. One
    must specify the form of LTO by setting this to one of: 'full', 'thin'. This
    option only affects the tools that run on the host (the compiler), and has
    no effect on the target libraries (the standard library and the runtime).")

# The following only works with the Ninja generator in CMake >= 3.0.
set(POLAR_PARALLEL_LINK_JOBS "" CACHE STRING
   "Define the maximum number of linker jobs for polarphp.")

option(POLAR_FORCE_OPTIMIZED_TYPECHECKER "Override the optimization setting of
  the type checker so that it always compiles with optimization. This eases
  debugging after type checking occurs by speeding up type checking" FALSE)

# Allow building polarphp with Clang's Profile Guided Optimization
if(POLAR_PROFDATA_FILE AND EXISTS ${POLAR_PROFDATA_FILE})
   if(NOT CMAKE_C_COMPILER_ID MATCHES Clang)
      message(FATAL_ERROR "POLAR_PROFDATA_FILE can only be specified when compiling with clang")
   endif()
   add_definitions("-fprofile-instr-use=${POLAR_PROFDATA_FILE}")
endif()

#
# User-configurable Swift Standard Library specific options.
#
# TODO: Once the stdlib/compiler builds are split, this should be sunk into the
# stdlib cmake.
#

set(POLAR_STDLIB_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
   "Build type for the polarphp standard library and SDK overlays [Debug, RelWithDebInfo, Release, MinSizeRel]")
# Allow the user to specify the standard library CMAKE_MSVC_RUNTIME_LIBRARY
# value.  The following values are valid:
#   - MultiThreaded (/MT)
#   - MultiThreadedDebug (/MTd)
#   - MultiThreadedDLL (/MD)
#   - MultiThreadedDebugDLL (/MDd)
if(CMAKE_BUILD_TYPE STREQUAL Debug)
   set(POLAR_STDLIB_MSVC_RUNTIME_LIBRARY_default MultiThreadedDebugDLL)
else()
   set(POLAR_STDLIB_MSVC_RUNTIME_LIBRARY_default MultiThreadedDLL)
endif()
set(POLAR_STDLIB_MSVC_RUNTIME_LIBRARY
   ${POLAR_STDLIB_MSVC_RUNTIME_LIBRARY_default}
   CACHE STRING "MSVC Runtime Library for the standard library")

polar_is_build_type_optimized("${POLAR_STDLIB_BUILD_TYPE}" polarphp_optimized)

if(polarphp_optimized)
   set(POLAR_STDLIB_ASSERTIONS_DEFAULT FALSE)
else()
   set(POLAR_STDLIB_ASSERTIONS_DEFAULT TRUE)
endif()

option(POLAR_STDLIB_ASSERTIONS
   "Enable internal checks for the polarphp standard library (useful for debugging the library itself, does not affect checks required for safety)"
   "${POLAR_STDLIB_ASSERTIONS_DEFAULT}")

option(POLAR_BUILD_RUNTIME_WITH_HOST_COMPILER
   "Use the host compiler and not the internal clang to build the polarphp runtime"
   FALSE)

set(POLAR_SDKS "" CACHE STRING
   "If non-empty, limits building target binaries only to specified SDKs (despite other SDKs being available)")

set(POLAR_PRIMARY_VARIANT_SDK "" CACHE STRING
   "Primary SDK for target binaries")

set(POLAR_PRIMARY_VARIANT_ARCH "" CACHE STRING
   "Primary arch for target binaries")

set(POLAR_NATIVE_LLVM_TOOLS_PATH "" CACHE STRING
   "Path to the directory that contains LLVM tools that are executable on the build machine")

set(POLAR_NATIVE_CLANG_TOOLS_PATH "" CACHE STRING
   "Path to the directory that contains Clang tools that are executable on the build machine")

set(POLAR_NATIVE_POLARPHP_TOOLS_PATH "" CACHE STRING
   "Path to the directory that contains polarphp tools that are executable on the build machine")

option(POLAR_ENABLE_MODULE_INTERFACES
   "Generate .phpinterface files alongside .phpmodule files"
   TRUE)

option(POLAR_STDLIB_ENABLE_PIB_TARGETS
   "Should we generate pib targets for the stdlib or not?"
   FALSE)

set(POLAR_DARWIN_SUPPORTED_ARCHS "" CACHE STRING
   "Semicolon-separated list of architectures to configure on Darwin platforms. \
If left empty all default architectures are configured.")

set(POLAR_DARWIN_MODULE_ARCHS "" CACHE STRING
   "Semicolon-separated list of architectures to configure polarphp module-only \
targets on Darwin platforms. These targets are in addition to the full \
library targets.")

set(POLAR_ANDROID_API_LEVEL "" CACHE STRING
   "Version number for the Android API")

set(POLAR_ANDROID_NDK_PATH "" CACHE STRING
   "Path to the directory that contains the Android NDK tools that are executable on the build machine")
set(POLAR_ANDROID_NDK_GCC_VERSION "" CACHE STRING
   "The GCC version to use when building for Android. Currently only 4.9 is supported.")
set(POLAR_ANDROID_DEPLOY_DEVICE_PATH "" CACHE STRING
   "Path on an Android device where build products will be pushed. These are used when running the test suite against the device")

#
# User-configurable ICU specific options for Android, FreeBSD, Linux and Haiku.
#
foreach(sdk ANDROID;FREEBSD;LINUX;WINDOWS;HAIKU)
   foreach(arch aarch64;armv6;armv7;i686;powerpc64;powerpc64le;s390x;x86_64)
      set(POLAR_${sdk}_${arch}_ICU_UC "" CACHE STRING
         "Path to a directory containing the icuuc library for ${sdk}")
      set(POLAR_${sdk}_${arch}_ICU_UC_INCLUDE "" CACHE STRING
         "Path to a directory containing headers for icuuc for ${sdk}")
      set(POLAR_${sdk}_${arch}_ICU_I18N "" CACHE STRING
         "Path to a directory containing the icui18n library for ${sdk}")
      set(POLAR_${sdk}_${arch}_ICU_I18N_INCLUDE "" CACHE STRING
         "Path to a directory containing headers icui18n for ${sdk}")
   endforeach()
endforeach()

#
# User-configurable Darwin-specific options.
#
option(POLAR_EMBED_BITCODE_SECTION
   "If non-empty, embeds LLVM bitcode binary sections in the standard library and overlay binaries for supported platforms"
   FALSE)

option(POLAR_EMBED_BITCODE_SECTION_HIDE_SYMBOLS
   "If non-empty, when embedding the LLVM bitcode binary sections into the relevant binaries, pass in -bitcode_hide_symbols. Does nothing if POLAR_EMBED_BITCODE_SECTION is set to false."
   FALSE)

option(POLAR_RUNTIME_CRASH_REPORTER_CLIENT
   "Whether to enable CrashReporter integration"
   FALSE)

set(POLAR_DARWIN_XCRUN_TOOLCHAIN "XcodeDefault" CACHE STRING
   "The name of the toolchain to pass to 'xcrun'")

set(POLAR_DARWIN_STDLIB_INSTALL_NAME_DIR "/usr/lib/polarphp" CACHE STRING
   "The directory of the install_name for standard library dylibs")

# We don't want to use the same install_name_dir as the standard library which
# will be installed in /usr/lib/polarphp. These private libraries should continue
# to use @rpath for now.
set(POLAR_DARWIN_STDLIB_PRIVATE_INSTALL_NAME_DIR "@rpath" CACHE STRING
   "The directory of the install_name for the private standard library dylibs")

set(POLAR_DARWIN_DEPLOYMENT_VERSION_OSX "10.9" CACHE STRING
   "Minimum deployment target version for OS X")

set(POLAR_DARWIN_DEPLOYMENT_VERSION_IOS "7.0" CACHE STRING
   "Minimum deployment target version for iOS")

set(POLAR_DARWIN_DEPLOYMENT_VERSION_TVOS "9.0" CACHE STRING
   "Minimum deployment target version for tvOS")

set(POLAR_DARWIN_DEPLOYMENT_VERSION_WATCHOS "2.0" CACHE STRING
   "Minimum deployment target version for watchOS")

#
# User-configurable debugging options.
#
option(POLAR_AST_VERIFIER
   "Enable the AST verifier in the built compiler, and run it on every compilation"
   TRUE)

option(POLAR_PIL_VERIFY_ALL
   "Run PIL verification after each transform when building typephp files in the build process"
   FALSE)

option(POLAR_EMIT_SORTED_SIL_OUTPUT
   "Sort PIL output by name to enable diffing of output"
   FALSE)

if(POLAR_STDLIB_ASSERTIONS)
   set(POLAR_RUNTIME_CLOBBER_FREED_OBJECTS_DEFAULT TRUE)
else()
   set(POLAR_RUNTIME_CLOBBER_FREED_OBJECTS_DEFAULT FALSE)
endif()

option(POLAR_RUNTIME_CLOBBER_FREED_OBJECTS
   "Overwrite memory for deallocated typephp objects"
   "${POLAR_RUNTIME_CLOBBER_FREED_OBJECTS_DEFAULT}")

option(POLAR_STDLIB_PIL_DEBUGGING
   "Compile the typephp standard library with -gpil to enable debugging and profiling on PIL level"
   FALSE)

option(POLAR_CHECK_INCREMENTAL_COMPILATION
   "Check if incremental compilation works when compiling the typephp libraries"
   FALSE)

option(POLAR_REPORT_STATISTICS
   "Create json files which contain internal compilation statistics"
   FALSE)

#
# User-configurable experimental options.  Do not use in production builds.
#

set(POLAR_EXPERIMENTAL_EXTRA_FLAGS "" CACHE STRING
   "Extra flags to pass when compiling typephp files.  Use this option *only* for one-off experiments")

set(POLAR_EXPERIMENTAL_EXTRA_REGEXP_FLAGS "" CACHE STRING
   "A list of [module_regexp1;flags1;module_regexp2;flags2,...] which can be used to apply specific flags to modules that match a cmake regexp. It always applies the first regexp that matches.")

set(POLAR_EXPERIMENTAL_EXTRA_NEGATIVE_REGEXP_FLAGS "" CACHE STRING
   "A list of [module_regexp1;flags1;module_regexp2;flags2,...] which can be used to apply specific flags to modules that do not match a cmake regexp. It always applies the first regexp that does not match. The reason this is necessary is that cmake does not provide negative matches in the regex. Instead you have to use NOT in the if statement requiring a separate variable.")

option(POLAR_RUNTIME_ENABLE_LEAK_CHECKER
   "Should the runtime be built with support for non-thread-safe leak detecting entrypoints"
   FALSE)

option(POLAR_STDLIB_USE_NONATOMIC_RC
   "Build the standard libraries and overlays with nonatomic reference count operations enabled"
   FALSE)

option(POLAR_ENABLE_RUNTIME_FUNCTION_COUNTERS
   "Enable runtime function counters and expose the API."
   FALSE)

option(POLAR_ENABLE_STDLIBCORE_EXCLUSIVITY_CHECKING
   "Build stdlibCore with exclusivity checking enabled"
   FALSE)

option(POLAR_ENABLE_EXPERIMENTAL_DIFFERENTIABLE_PROGRAMMING
   "Enable experimental Swift differentiable programming features"
   FALSE)

#
# End of user-configurable options.
#

set(POLAR_BUILT_STANDALONE FALSE)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
   set(POLAR_BUILT_STANDALONE TRUE)
endif()

if(POLAR_BUILT_STANDALONE)
   project(polarphp C CXX ASM)
endif()

if(MSVC OR "${CMAKE_SIMULATE_ID}" STREQUAL MSVC)
   include(PolarClangClCompileRules)
endif()

if(CMAKE_C_COMPILER_ID MATCHES Clang)
   add_compile_options($<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-Werror=gnu>)
endif()

#
# Assume a new enough ar to generate the index at construction time. This avoids
# having to invoke ranlib as a secondary command.
#

set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> qs <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_FINISH "")

set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> qs <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_FINISH "")

#
# Include CMake modules
#
include(CheckCXXSourceRuns)
include(CMakeParseArguments)
include(CMakePushCheckState)

polar_print_versions()

include(PolarSharedCMakeConfig)

# NOTE: We include this before SwiftComponents as it relies on some LLVM CMake
# functionality.
# Support building Swift as a standalone project, using LLVM as an
# external library.
if(POLAR_BUILT_STANDALONE)
   include(ImportLLVM)
   include(ImportClang)
   polar_common_standalone_build_config(POLAR)
else()
   polar_common_unified_build_config(POLAR)
endif()

include(ImportThirdparty)
include(PolarComponents)
include(TargetUtils)
include(PolarList)
include(PolarConfigureSDK)
include(PolarSource)
include(PolarSetIfArchBitness)
include(HandleGybSources)
include(ProcessSources)

# Configure polarphp include, install, build components.
polar_configure_components()

# lipo is used to create universal binaries.
include(PolarToolchainUtils)
if(NOT POLAR_LIPO)
   find_toolchain_tool(POLAR_LIPO "${POLAR_DARWIN_XCRUN_TOOLCHAIN}" lipo)
endif()

# Reset CMAKE_SYSTEM_PROCESSOR if not cross-compiling.
# CMake refuses to use `uname -m` on OS X
# http://public.kitware.com/Bug/view.php?id=10326
if(NOT CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_PROCESSOR STREQUAL "i386")
   execute_process(
      COMMAND "uname" "-m"
      OUTPUT_VARIABLE CMAKE_SYSTEM_PROCESSOR
      OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

set(POLAR_RUNTIME_OUTPUT_INTDIR ${POLAR_BINARY_DIR}/${CMAKE_CFG_INTDIR}/bin)
set(POLAR_LIBRARY_OUTPUT_INTDIR ${POLAR_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib${POLAR_LIBDIR_SUFFIX})
get_filename_component(POLAR_RUNTIME_OUTPUT_INTDIR ${POLAR_RUNTIME_OUTPUT_INTDIR} ABSOLUTE)
get_filename_component(POLAR_LIBRARY_OUTPUT_INTDIR ${POLAR_LIBRARY_OUTPUT_INTDIR} ABSOLUTE)
set(POLAR_TOOLS_BINARY_DIR ${POLAR_RUNTIME_OUTPUT_INTDIR}) # --bindir
set(POLAR_LIBRARY_DIR      ${POLAR_LIBRARY_OUTPUT_INTDIR}) # --libdir

if("${POLAR_NATIVE_POLAR_TOOLS_PATH}" STREQUAL "")
   set(POLAR_NATIVE_POLAR_TOOLS_PATH "${POLAR_RUNTIME_OUTPUT_INTDIR}")
endif()

# This setting causes all CMakeLists.txt to automatically have
# ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CURRENT_SOURCE_DIR} as an
# include_directories path. This is done for developer
# convenience. Additionally, LLVM/Clang build with this option enabled, so we
# should match them unless it is removed from LLVM/Clang as well.
#
# *NOTE* Even though these directories are added to the include path for a
# specific CMakeLists.txt, these include paths are not propagated down to
# subdirectories.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${POLAR_SOURCE_DIR}
   ${POLAR_MAIN_INCLUDE_DIR}
   ${POLAR_MAIN_INCLUDE_DIR}/polarphp
   ${POLAR_INCLUDE_DIR}
   ${POLAR_INCLUDE_DIR}/polarphp
   ${POLAR_DEPS_INSTALL_DIR}/include)

# A convenience pattern to match Darwin platforms. Example:
#  if(POLAR_HOST_VARIANT MATCHES "${POLAR_DARWIN_VARIANTS}")
#     ...
#  endif()
set(POLAR_DARWIN_VARIANTS "^(macosx|iphoneos|iphonesimulator|appletvos|appletvsimulator|watchos|watchsimulator)")
set(POLAR_DARWIN_EMBEDDED_VARIANTS "^(iphoneos|iphonesimulator|appletvos|appletvsimulator|watchos|watchsimulator)")

# A convenient list to match Darwin SDKs. Example:
#  if("${POLAR_HOST_VARIANT_SDK}" IN_LIST POLAR_APPLE_PLATFORMS)
#    ...
#  endif()
set(POLAR_APPLE_PLATFORMS "IOS" "IOS_SIMULATOR" "TVOS" "TVOS_SIMULATOR" "WATCHOS" "WATCHOS_SIMULATOR" "OSX")

# Configuration flags passed to all of our invocations of gyb.  Try to
# avoid making up new variable names here if you can find a CMake
# variable that will do the job.
set(POLAR_GYB_FLAGS
   "-DunicodeGraphemeBreakPropertyFile=${POLAR_SOURCE_DIR}/utils/UnicodeData/GraphemeBreakProperty.txt"
   "-DunicodeGraphemeBreakTestFile=${POLAR_SOURCE_DIR}/utils/UnicodeData/GraphemeBreakTest.txt")

# Directory to use as the Clang module cache when building Swift source files.
set(POLAR_MODULE_CACHE_PATH
   "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/module-cache")

# Xcode: use libc++ and c++11 using proper build settings.
if(XCODE)
   polar_common_xcode_cxx_config()
endif()

include(PolarCheckCXXNativeRegex)
check_cxx_native_regex(POLAR_HAVE_WORKING_STD_REGEX)

# If POLAR_HOST_VARIANT_SDK not given, try to detect from the CMAKE_SYSTEM_NAME.
if(POLAR_HOST_VARIANT_SDK)
   set(POLAR_HOST_VARIANT_SDK_DEFAULT "${POLAR_HOST_VARIANT_SDK}")
else()
   if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
      # CMake on an Android host sets this to Linux, so check for the ANDROID_DATA
      # environment variable to see if we're building on Android.
      if(NOT "$ENV{ANDROID_DATA}" STREQUAL "")
         set(POLAR_HOST_VARIANT_SDK_DEFAULT "ANDROID")
      else()
         set(POLAR_HOST_VARIANT_SDK_DEFAULT "LINUX")
      endif()
   elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")
      set(POLAR_HOST_VARIANT_SDK_DEFAULT "FREEBSD")
   elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "CYGWIN")
      set(POLAR_HOST_VARIANT_SDK_DEFAULT "CYGWIN")
   elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
      set(POLAR_HOST_VARIANT_SDK_DEFAULT "WINDOWS")
   elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Haiku")
      set(POLAR_HOST_VARIANT_SDK_DEFAULT "HAIKU")
   elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
      # CMAKE_SYSTEM_NAME might be set this way when cross-compiling to Android.
      set(POLAR_HOST_VARIANT_SDK_DEFAULT "ANDROID")
   elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
      set(POLAR_HOST_VARIANT_SDK_DEFAULT "OSX")
   else()
      message(FATAL_ERROR "Unable to detect SDK for host system: ${CMAKE_SYSTEM_NAME}")
   endif()
endif()

# If POLAR_HOST_VARIANT_ARCH not given, try to detect from the CMAKE_SYSTEM_PROCESSOR.
if(POLAR_HOST_VARIANT_ARCH)
   set(POLAR_HOST_VARIANT_ARCH_DEFAULT "${POLAR_HOST_VARIANT_ARCH}")
else()
   if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
      set(POLAR_HOST_VARIANT_ARCH_DEFAULT "x86_64")
   elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
      set(POLAR_HOST_VARIANT_ARCH_DEFAULT "aarch64")
   elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "ppc64")
      set(POLAR_HOST_VARIANT_ARCH_DEFAULT "powerpc64")
   elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "ppc64le")
      set(POLAR_HOST_VARIANT_ARCH_DEFAULT "powerpc64le")
   elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "s390x")
      set(POLAR_HOST_VARIANT_ARCH_DEFAULT "s390x")
      # FIXME: Only matches v6l/v7l - by far the most common variants
   elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv6l")
      set(POLAR_HOST_VARIANT_ARCH_DEFAULT "armv6")
   elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "armv7l|armv7-a")
      set(POLAR_HOST_VARIANT_ARCH_DEFAULT "armv7")
   elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "IA64")
      set(POLAR_HOST_VARIANT_ARCH_DEFAULT "itanium")
   elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "(x86|i686)")
      set(POLAR_HOST_VARIANT_ARCH_DEFAULT "i686")
   else()
      message(FATAL_ERROR "Unrecognized architecture on host system: ${CMAKE_SYSTEM_PROCESSOR}")
   endif()
endif()

set(POLAR_HOST_VARIANT_SDK "${POLAR_HOST_VARIANT_SDK_DEFAULT}" CACHE STRING
   "Deployment sdk for Swift host tools (the compiler).")
set(POLAR_HOST_VARIANT_ARCH "${POLAR_HOST_VARIANT_ARCH_DEFAULT}" CACHE STRING
   "Deployment arch for Swift host tools (the compiler).")

#
# Enable additional warnings.
#
polar_common_cxx_warnings()

# Check if we're build with MSVC or Clang-cl, as these compilers have similar command line arguments.
if("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC" OR "${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
   set(POLAR_COMPILER_IS_MSVC_LIKE TRUE)
endif()

#
# Configure SDKs.
#

if(XCODE)
   # FIXME: Cannot cross-compile the standard library using Xcode.  Xcode
   # insists on passing -mmacosx-version-min to the compiler, and we need
   # to pass -mios-version-min.  Clang sees both options and complains.
   set(POLAR_SDKS "OSX")
endif()

# FIXME: the parameters we specify in POLAR_SDKS are lacking architecture specifics,
# so we need to hard-code it. For example, the SDK for Android is just 'ANDROID',
# and we have to specify POLAR_SDK_ANDROID_ARCHITECTURES separately.
# The iOS SDKs all have their architectures hardcoded because they are just specified by name (e.g. 'IOS' or 'WATCHOS').
# We can't cross-compile the standard library for another linux architecture,
# because the SDK list would just be 'LINUX' and we couldn't disambiguate it from the host.
#
# To fix it, we would need to append the architecture to the SDKs,
# for example: 'OSX-x86_64;IOS-armv7;...etc'.
# We could easily do that - we have all of that information in build-script-impl.
# Darwin targets cheat and use `xcrun`.

if("${POLAR_HOST_VARIANT_SDK}" STREQUAL "LINUX")
   set(POLAR_HOST_VARIANT "linux" CACHE STRING
      "Deployment OS for polarphp host tools (the compiler) [linux].")
   # Should we build the standard library for the host?
   is_sdk_requested(LINUX polar_build_linux)
   if(polar_build_linux)
      configure_sdk_unix("Linux" "${POLAR_HOST_VARIANT_ARCH}")
      set(POLAR_PRIMARY_VARIANT_SDK_default  "${POLAR_HOST_VARIANT_SDK}")
      set(POLAR_PRIMARY_VARIANT_ARCH_default "${POLAR_HOST_VARIANT_ARCH}")
   endif()

elseif("${POLAR_HOST_VARIANT_SDK}" STREQUAL "FREEBSD")

   set(POLAR_HOST_VARIANT "freebsd" CACHE STRING
      "Deployment OS for polarphp host tools (the compiler) [freebsd].")

   configure_sdk_unix("FreeBSD" "${POLAR_HOST_VARIANT_ARCH}")
   set(POLAR_PRIMARY_VARIANT_SDK_default  "${POLAR_HOST_VARIANT_SDK}")
   set(POLAR_PRIMARY_VARIANT_ARCH_default "${POLAR_HOST_VARIANT_ARCH}")

elseif("${POLAR_HOST_VARIANT_SDK}" STREQUAL "CYGWIN")

   set(POLAR_HOST_VARIANT "cygwin" CACHE STRING
      "Deployment OS for polarphp host tools (the compiler) [cygwin].")

   configure_sdk_unix("Cygwin" "${POLAR_HOST_VARIANT_ARCH}")
   set(POLAR_PRIMARY_VARIANT_SDK_default "${POLAR_HOST_VARIANT_SDK}")
   set(POLAR_PRIMARY_VARIANT_ARCH_default "${POLAR_HOST_VARIANT_ARCH}")

elseif("${POLAR_HOST_VARIANT_SDK}" STREQUAL "WINDOWS")

   set(POLAR_HOST_VARIANT "windows" CACHE STRING
      "Deployment OS for polarphp host tools (the compiler) [windows].")

   configure_sdk_windows("Windows" "msvc" "${POLAR_HOST_VARIANT_ARCH}")
   set(POLAR_PRIMARY_VARIANT_SDK_default  "${POLAR_HOST_VARIANT_SDK}")
   set(POLAR_PRIMARY_VARIANT_ARCH_default "${POLAR_HOST_VARIANT_ARCH}")

elseif("${POLAR_HOST_VARIANT_SDK}" STREQUAL "HAIKU")

   set(POLAR_HOST_VARIANT "haiku" CACHE STRING
      "Deployment OS for polarphp host tools (the compiler) [haiku].")

   configure_sdk_unix("Haiku" "${POLAR_HOST_VARIANT_ARCH}")
   set(POLAR_PRIMARY_VARIANT_SDK_default  "${POLAR_HOST_VARIANT_SDK}")
   set(POLAR_PRIMARY_VARIANT_ARCH_default "${POLAR_HOST_VARIANT_ARCH}")

elseif("${POLAR_HOST_VARIANT_SDK}" STREQUAL "ANDROID")

   set(POLAR_HOST_VARIANT "android" CACHE STRING
      "Deployment OS for polarphp host tools (the compiler) [android]")

   set(POLAR_ANDROID_NATIVE_SYSROOT "/data/data/com.termux/files" CACHE STRING
      "Path to Android sysroot, default initialized to the Termux app's layout")

   if("${POLAR_SDK_ANDROID_ARCHITECTURES}" STREQUAL "")
      set(POLAR_SDK_ANDROID_ARCHITECTURES ${POLAR_HOST_VARIANT_ARCH})
   endif()

   configure_sdk_unix("Android" "${POLAR_SDK_ANDROID_ARCHITECTURES}")
   set(POLAR_PRIMARY_VARIANT_SDK_default  "${POLAR_HOST_VARIANT_SDK}")
   set(POLAR_PRIMARY_VARIANT_ARCH_default "${POLAR_HOST_VARIANT_ARCH}")

elseif("${POLAR_HOST_VARIANT_SDK}" MATCHES "(OSX|IOS*|TVOS*|WATCHOS*)")

   set(POLAR_HOST_VARIANT "macosx" CACHE STRING
      "Deployment OS for polarphp host tools (the compiler) [macosx, iphoneos].")

   # Display Xcode toolchain version.
   # The SDK configuration below prints each SDK version.
   # TODO
   #   execute_process(
   #      COMMAND "xcodebuild" "-version"
   #      OUTPUT_VARIABLE xcode_version
   #      OUTPUT_STRIP_TRAILING_WHITESPACE)
   #   string(REPLACE "\n" ", " xcode_version "${xcode_version}")
   #   message(STATUS "${xcode_version}")
   #   message(STATUS "")

   include(PolarDarwinSDKs)

   # FIXME: guess target variant based on the host.
   # if(POLAR_HOST_VARIANT MATCHES "^macosx")
   #   set(POLAR_PRIMARY_VARIANT_GUESS "OSX-R")
   # elseif(POLAR_HOST_VARIANT MATCHES "^iphoneos")
   #   set(POLAR_PRIMARY_VARIANT_GUESS "IOS-R")
   # else()
   #   message(FATAL_ERROR "Unknown POLAR_HOST_VARIANT '${POLAR_HOST_VARIANT}'")
   # endif()
   #
   # set(POLAR_PRIMARY_VARIANT ${POLAR_PRIMARY_VARIANT_GUESS} CACHE STRING
   #    "[OSX-DA, OSX-RA, OSX-R, IOS-DA, IOS-RA, IOS-R, IOS_SIMULATOR-DA, IOS_SIMULATOR-RA, IOS_SIMULATOR-R]")
   #
   # Primary variant is always OSX; even on iOS hosts.
   set(POLAR_PRIMARY_VARIANT_SDK_default "OSX")
   set(POLAR_PRIMARY_VARIANT_ARCH_default "x86_64")

endif()

if("${POLAR_PRIMARY_VARIANT_SDK}" STREQUAL "")
   set(POLAR_PRIMARY_VARIANT_SDK "${POLAR_PRIMARY_VARIANT_SDK_default}")
endif()
if("${POLAR_PRIMARY_VARIANT_ARCH}" STREQUAL "")
   set(POLAR_PRIMARY_VARIANT_ARCH "${POLAR_PRIMARY_VARIANT_ARCH_default}")
endif()

# Should we cross-compile the standard library for Android?
polar_is_sdk_requested(ANDROID polar_build_android)
if(polar_build_android AND NOT "${POLAR_HOST_VARIANT_SDK}" STREQUAL "ANDROID")
   if ("${POLAR_ANDROID_NDK_PATH}" STREQUAL "")
      message(FATAL_ERROR "You must set POLAR_ANDROID_NDK_PATH to cross-compile the Swift runtime for Android")
   endif()
   if (NOT ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Darwin" OR "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux"))
      message(FATAL_ERROR "A Darwin or Linux host is required to build the Swift runtime for Android")
   endif()

   if("${POLAR_SDK_ANDROID_ARCHITECTURES}" STREQUAL "")
      set(POLAR_SDK_ANDROID_ARCHITECTURES armv7;aarch64)
   endif()
   configure_sdk_unix("Android" "${POLAR_SDK_ANDROID_ARCHITECTURES}")
endif()

# Should we cross-compile the standard library for Windows?
polar_is_sdk_requested(WINDOWS polar_build_windows)
if(polar_build_windows AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
   if("${POLAR_SDK_WINDOWS_ARCHITECTURES}" STREQUAL "")
      set(POLAR_SDK_WINDOWS_ARCHITECTURES aarch64;armv7;i686;x86_64)
   endif()
   configure_sdk_windows("Windows" "msvc" "${POLAR_SDK_WINDOWS_ARCHITECTURES}")
endif()

if("${POLAR_SDKS}" STREQUAL "")
   set(POLAR_SDKS "${POLAR_CONFIGURED_SDKS}")
endif()

list_subtract("${POLAR_SDKS}" "${POLAR_CONFIGURED_SDKS}" unknown_sdks)

polar_precondition(unknown_sdks NEGATE MESSAGE "Unknown SDKs: ${unknown_sdks}")
polar_precondition(POLAR_CONFIGURED_SDKS MESSAGE "No SDKs selected.")
polar_precondition(POLAR_HOST_VARIANT_SDK MESSAGE "No SDK for host tools.")
polar_precondition(POLAR_HOST_VARIANT_ARCH MESSAGE "No arch for host tools")

set(POLAR_PRIMARY_VARIANT_SUFFIX
   "-${POLAR_SDK_${POLAR_PRIMARY_VARIANT_SDK}_LIB_SUBDIR}-${POLAR_PRIMARY_VARIANT_ARCH}")

# Clear universal library names to prevent adding duplicates
foreach(sdk ${POLAR_SDKS})
   unset(UNIVERSAL_LIBRARY_NAMES_${POLAR_SDK_${sdk}_LIB_SUBDIR} CACHE)
endforeach()

if(POLAR_PARALLEL_LINK_JOBS)
   if(NOT CMAKE_MAKE_PROGRAM MATCHES "ninja")
      message(WARNING "Job pooling is only available with Ninja generators.")
   else()
      set_property(GLOBAL APPEND PROPERTY JOB_POOLS polar_link_job_pool=${POLAR_PARALLEL_LINK_JOBS})
      set(CMAKE_JOB_POOL_LINK polar_link_job_pool)
   endif()
endif()

# Set the CMAKE_OSX_* variables in a way that minimizes conflicts.
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin" AND NOT CMAKE_CROSSCOMPILING)
   set(CMAKE_OSX_SYSROOT "${POLAR_SDK_${POLAR_HOST_VARIANT_SDK}_PATH}")
   set(CMAKE_OSX_ARCHITECTURES "")
   set(CMAKE_OSX_DEPLOYMENT_TARGET "")
endif()

include(ProcesseRequirePackages)
include(CheckCompilerVersion)
include(DetermineGCCCompatible)
include(DetectHostEnvironment)

# You need libedit linked in order to check if you have el_wgets.
cmake_push_check_state()
list(APPEND CMAKE_REQUIRED_LIBRARIES "edit")
check_symbol_exists(el_wgets "histedit.h" HAVE_EL_WGETS)
if(HAVE_EL_WGETS)
   set(HAVE_UNICODE_LIBEDIT 1)
endif()
cmake_pop_check_state()

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# Add all of the subdirectories, where we actually do work.

###############
# PLEASE READ #
###############
#
# We have to include stdlib/ before tools/.
# Do not move add_subdirectory(stdlib) after add_subdirectory(tools)!
#
# We must include stdlib/ before tools/ because stdlib/CMakeLists.txt
# declares the polarphp-stdlib-* set of targets. These targets will then
# implicitly depend on any targets declared with IS_STDLIB.

if(POLAR_BUILD_STDLIB)
   add_subdirectory(stdlib)
else()
   if(POLAR_BUILD_STDLIB_EXTRA_TOOLCHAIN_CONTENT)
      add_subdirectory(stdlib/toolchain)
   endif()

   # Some tools (e.g. polar-reflection-dump) rely on a host polarReflection, so
   # ensure we build that when building tools.
   if(POLAR_INCLUDE_TOOLS)
      # add_subdirectory(stdlib/public/PolarShims)
   endif()
endif()

add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(tools)

include(SummaryOutput)
