# Variable that tracks the set of configured SDKs.
#
# Each element in this list is an SDK for which the various
# POLAR_SDK_${name}_* variables are defined. Swift libraries will be
# built for each variant.
set(POLAR_CONFIGURED_SDKS)

include(PolarWindowsSupport)
include(PolarAndroidSupport)

# Report the given SDK to the user.
function(_report_sdk prefix)
  message(STATUS "${POLAR_SDK_${prefix}_NAME} SDK:")
  message(STATUS "  Object File Format: ${POLAR_SDK_${prefix}_OBJECT_FORMAT}")
  message(STATUS "  Swift Standard Library Path: ${POLAR_SDK_${prefix}_LIB_SUBDIR}")

  if("${prefix}" STREQUAL "WINDOWS")
    message(STATUS "  UCRT Version: ${UCRTVersion}")
    message(STATUS "  UCRT SDK Path: ${UniversalCRTSdkDir}")
    message(STATUS "  VC Path: ${VCToolsInstallDir}")
    if("${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
      message(STATUS "  ${CMAKE_BUILD_TYPE} VC++ CRT: MDd")
    else()
      message(STATUS "  ${CMAKE_BUILD_TYPE} VC++ CRT: MD")
    endif()
  endif()
  if(prefix IN_LIST POLAR_APPLE_PLATFORMS)
    message(STATUS "  Version: ${POLAR_SDK_${prefix}_VERSION}")
    message(STATUS "  Build number: ${POLAR_SDK_${prefix}_BUILD_NUMBER}")
    message(STATUS "  Deployment version: ${POLAR_SDK_${prefix}_DEPLOYMENT_VERSION}")
    message(STATUS "  Version min name: ${POLAR_SDK_${prefix}_VERSION_MIN_NAME}")
    message(STATUS "  Triple name: ${POLAR_SDK_${prefix}_TRIPLE_NAME}")
  endif()
  if(POLAR_SDK_${prefix}_MODULE_ARCHITECTURES)
    message(STATUS "  Module Architectures: ${POLAR_SDK_${prefix}_MODULE_ARCHITECTURES}")
  endif()

  message(STATUS "  Architectures: ${POLAR_SDK_${prefix}_ARCHITECTURES}")
  foreach(arch ${POLAR_SDK_${prefix}_ARCHITECTURES})
    message(STATUS "  ${arch} triple: ${POLAR_SDK_${prefix}_ARCH_${arch}_TRIPLE}")
  endforeach()
  if("${prefix}" STREQUAL "WINDOWS")
    foreach(arch ${POLAR_SDK_${prefix}_ARCHITECTURES})
      polar_windows_include_for_arch(${arch} ${arch}_INCLUDE)
      polar_windows_lib_for_arch(${arch} ${arch}_LIB)
      message(STATUS "  ${arch} INCLUDE: ${${arch}_INCLUDE}")
      message(STATUS "  ${arch} LIB: ${${arch}_LIB}")
    endforeach()
  elseif("${prefix}" STREQUAL "ANDROID")
    if(NOT "${POLAR_ANDROID_NDK_PATH}" STREQUAL "")
      message(STATUS " NDK: $ENV{POLAR_ANDROID_NDK_PATH}")
    endif()
    if(NOT "${POLAR_ANDROID_NATIVE_SYSROOT}" STREQUAL "")
      message(STATUS " Sysroot: ${POLAR_ANDROID_NATIVE_SYSROOT}")
    endif()
    foreach(arch ${POLAR_SDK_${prefix}_ARCHITECTURES})
      polar_android_include_for_arch(${arch} ${arch}_INCLUDE)
      polar_android_lib_for_arch(${arch} ${arch}_LIB)
      message(STATUS "  ${arch} INCLUDE: ${${arch}_INCLUDE}")
      message(STATUS "  ${arch} LIB: ${${arch}_LIB}")
    endforeach()
  else()
    foreach(arch ${POLAR_SDK_${prefix}_ARCHITECTURES})
      message(STATUS "  ${arch} Path: ${POLAR_SDK_${prefix}_ARCH_${arch}_PATH}")
    endforeach()
  endif()

  if(NOT prefix IN_LIST POLAR_APPLE_PLATFORMS)
    foreach(arch ${POLAR_SDK_${prefix}_ARCHITECTURES})
      message(STATUS "  ${arch} libc header path: ${POLAR_SDK_${prefix}_ARCH_${arch}_LIBC_INCLUDE_DIRECTORY}")
      message(STATUS "  ${arch} libc architecture specific header path: ${POLAR_SDK_${prefix}_ARCH_${arch}_LIBC_ARCHITECTURE_INCLUDE_DIRECTORY}")
    endforeach()
    if(POLAR_BUILD_STDLIB)
      foreach(arch ${POLAR_SDK_${prefix}_ARCHITECTURES})
        message(STATUS "  ${arch} ICU i18n INCLUDE: ${POLAR_${prefix}_${arch}_ICU_I18N_INCLUDE}")
        message(STATUS "  ${arch} ICU i18n LIB: ${POLAR_${prefix}_${arch}_ICU_I18N}")
        message(STATUS "  ${arch} ICU unicode INCLUDE: ${POLAR_${prefix}_${arch}_ICU_UC_INCLUDE}")
        message(STATUS "  ${arch} ICU unicode LIB: ${POLAR_${prefix}_${arch}_ICU_UC}")
      endforeach()
    endif()
  endif()

  message(STATUS "")
endfunction()

# Configure an SDK
#
# Usage:
#   configure_sdk_darwin(
#     prefix             # Prefix to use for SDK variables (e.g., OSX)
#     name               # Display name for this SDK
#     deployment_version # Deployment version
#     xcrun_name         # SDK name to use with xcrun
#     version_min_name   # The name used in the -mOS-version-min flag
#     triple_name        # The name used in Swift's -triple
#     architectures      # A list of architectures this SDK supports
#   )
#
# Sadly there are three OS naming conventions.
# xcrun SDK name:   macosx iphoneos iphonesimulator (+ version)
# -mOS-version-min: macosx ios      ios-simulator
# polar -triple:    macosx ios      ios
#
# This macro attempts to configure a given SDK. When successful, it
# defines a number of variables:
#
#   POLAR_SDK_${prefix}_NAME                Display name for the SDK
#   POLAR_SDK_${prefix}_VERSION             SDK version number (e.g., 10.9, 7.0)
#   POLAR_SDK_${prefix}_BUILD_NUMBER        SDK build number (e.g., 14A389a)
#   POLAR_SDK_${prefix}_DEPLOYMENT_VERSION  Deployment version (e.g., 10.9, 7.0)
#   POLAR_SDK_${prefix}_LIB_SUBDIR          Library subdir for this SDK
#   POLAR_SDK_${prefix}_VERSION_MIN_NAME    Version min name for this SDK
#   POLAR_SDK_${prefix}_TRIPLE_NAME         Triple name for this SDK
#   POLAR_SDK_${prefix}_ARCHITECTURES       Architectures (as a list)
#   POLAR_SDK_${prefix}_ARCH_${ARCH}_TRIPLE Triple name
macro(configure_sdk_darwin
   prefix name deployment_version xcrun_name
   version_min_name triple_name architectures)
  # Note: this has to be implemented as a macro because it sets global
  # variables.

  # Find the SDK
  set(POLAR_SDK_${prefix}_PATH "" CACHE PATH "Path to the ${name} SDK")

  if(NOT POLAR_SDK_${prefix}_PATH)
    execute_process(
       COMMAND "xcrun" "--sdk" "${xcrun_name}" "--show-sdk-path"
       OUTPUT_VARIABLE POLAR_SDK_${prefix}_PATH
       OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT EXISTS "${POLAR_SDK_${prefix}_PATH}/System/Library/Frameworks/module.map")
      message(FATAL_ERROR "${name} SDK not found at ${POLAR_SDK_${prefix}_PATH}.")
    endif()
  endif()

  if(NOT EXISTS "${POLAR_SDK_${prefix}_PATH}/System/Library/Frameworks/module.map")
    message(FATAL_ERROR "${name} SDK not found at ${POLAR_SDK_${prefix}_PATH}.")
  endif()

  # Determine the SDK version we found.
  execute_process(
     COMMAND "defaults" "read" "${POLAR_SDK_${prefix}_PATH}/SDKSettings.plist" "Version"
     OUTPUT_VARIABLE POLAR_SDK_${prefix}_VERSION
     OUTPUT_STRIP_TRAILING_WHITESPACE)
  # TODO
  #  execute_process(
  #    COMMAND "xcodebuild" "-sdk" "${POLAR_SDK_${prefix}_PATH}" "-version" "ProductBuildVersion"
  #      OUTPUT_VARIABLE POLAR_SDK_${prefix}_BUILD_NUMBER
  #      OUTPUT_STRIP_TRAILING_WHITESPACE)

  # Set other variables.
  set(POLAR_SDK_${prefix}_NAME "${name}")
  set(POLAR_SDK_${prefix}_DEPLOYMENT_VERSION "${deployment_version}")
  set(POLAR_SDK_${prefix}_LIB_SUBDIR "${xcrun_name}")
  set(POLAR_SDK_${prefix}_VERSION_MIN_NAME "${version_min_name}")
  set(POLAR_SDK_${prefix}_TRIPLE_NAME "${triple_name}")
  set(POLAR_SDK_${prefix}_OBJECT_FORMAT "MACHO")

  set(POLAR_SDK_${prefix}_ARCHITECTURES ${architectures})
  if(POLAR_DARWIN_SUPPORTED_ARCHS)
    list_intersect(
       "${architectures}"                  # lhs
       "${POLAR_DARWIN_SUPPORTED_ARCHS}"   # rhs
       POLAR_SDK_${prefix}_ARCHITECTURES)  # result
  endif()

  list_intersect(
     "${POLAR_DARWIN_MODULE_ARCHS}"            # lhs
     "${architectures}"                        # rhs
     POLAR_SDK_${prefix}_MODULE_ARCHITECTURES) # result

  # Ensure the architectures and module-only architectures lists are mutually
  # exclusive.
  list_subtract(
     "${POLAR_SDK_${prefix}_MODULE_ARCHITECTURES}" # lhs
     "${POLAR_SDK_${prefix}_ARCHITECTURES}"        # rhs
     POLAR_SDK_${prefix}_MODULE_ARCHITECTURES)     # result

  # Configure variables for _all_ architectures even if we aren't "building"
  # them because they aren't supported.
  foreach(arch ${architectures})
    # On Darwin, all archs share the same SDK path.
    set(POLAR_SDK_${prefix}_ARCH_${arch}_PATH "${POLAR_SDK_${prefix}_PATH}")

    set(POLAR_SDK_${prefix}_ARCH_${arch}_TRIPLE
       "${arch}-apple-${POLAR_SDK_${prefix}_TRIPLE_NAME}")
  endforeach()

  # Add this to the list of known SDKs.
  list(APPEND POLAR_CONFIGURED_SDKS "${prefix}")

  _report_sdk("${prefix}")
endmacro()

macro(configure_sdk_unix name architectures)
  # Note: this has to be implemented as a macro because it sets global
  # variables.

  string(TOUPPER ${name} prefix)
  string(TOLOWER ${name} platform)

  set(POLAR_SDK_${prefix}_NAME "${name}")
  set(POLAR_SDK_${prefix}_LIB_SUBDIR "${platform}")
  set(POLAR_SDK_${prefix}_ARCHITECTURES "${architectures}")
  if("${prefix}" STREQUAL "CYGWIN")
    set(POLAR_SDK_${prefix}_OBJECT_FORMAT "COFF")
  else()
    set(POLAR_SDK_${prefix}_OBJECT_FORMAT "ELF")
  endif()

  foreach(arch ${architectures})
    if("${prefix}" STREQUAL "ANDROID")
      if(NOT "${POLAR_ANDROID_NDK_PATH}" STREQUAL "")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_LIBC_INCLUDE_DIRECTORY "${POLAR_ANDROID_NDK_PATH}/sysroot/usr/include" CACHE STRING "Path to C library headers")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_LIBC_ARCHITECTURE_INCLUDE_DIRECTORY "${POLAR_ANDROID_NDK_PATH}/sysroot/usr/include" CACHE STRING "Path to C library architecture headers")
      elseif(NOT "${POLAR_ANDROID_NATIVE_SYSROOT}" STREQUAL "")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_LIBC_INCLUDE_DIRECTORY "${POLAR_ANDROID_NATIVE_SYSROOT}/usr/include" CACHE STRING "Path to C library headers")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_LIBC_ARCHITECTURE_INCLUDE_DIRECTORY "${POLAR_ANDROID_NATIVE_SYSROOT}/usr/include" CACHE STRING "Path to C library architecture headers")
      else()
        message(SEND_ERROR "Couldn't find LIBC_INCLUDE_DIRECTORY for Android")
      endif()

      if("${arch}" STREQUAL "armv7")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_NDK_TRIPLE "arm-linux-androideabi")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_ALT_SPELLING "arm")
        if(NOT "${POLAR_ANDROID_NDK_PATH}" STREQUAL "")
          set(POLAR_SDK_ANDROID_ARCH_${arch}_PATH "${POLAR_ANDROID_NDK_PATH}/platforms/android-${POLAR_ANDROID_API_LEVEL}/arch-arm")
        elseif(NOT "${POLAR_ANDROID_NATIVE_SYSROOT}" STREQUAL "")
          set(POLAR_SDK_ANDROID_ARCH_${arch}_PATH "${POLAR_ANDROID_NATIVE_SYSROOT}")
        else()
          message(SEND_ERROR "Couldn't find POLAR_SDK_ANDROID_ARCH_armv7_PATH")
        endif()
        set(POLAR_SDK_ANDROID_ARCH_${arch}_TRIPLE "armv7-none-linux-androideabi")
      elseif("${arch}" STREQUAL "aarch64")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_NDK_TRIPLE "aarch64-linux-android")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_ALT_SPELLING "aarch64")
        if(NOT "${POLAR_ANDROID_NDK_PATH}" STREQUAL "")
          set(POLAR_SDK_ANDROID_ARCH_${arch}_PATH "${POLAR_ANDROID_NDK_PATH}/platforms/android-${POLAR_ANDROID_API_LEVEL}/arch-arm64")
        elseif(NOT "${POLAR_ANDROID_NATIVE_SYSROOT}" STREQUAL "")
          set(POLAR_SDK_ANDROID_ARCH_${arch}_PATH "${POLAR_ANDROID_NATIVE_SYSROOT}")
        else()
          message(SEND_ERROR "Couldn't find POLAR_SDK_ANDROID_ARCH_aarch64_PATH")
        endif()
        set(POLAR_SDK_ANDROID_ARCH_${arch}_TRIPLE "aarch64-unknown-linux-android")
      elseif("${arch}" STREQUAL "i686")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_NDK_TRIPLE "i686-linux-android")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_ALT_SPELLING "i686")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_PATH "${POLAR_ANDROID_NDK_PATH}/platforms/android-${POLAR_ANDROID_API_LEVEL}/arch-x86")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_TRIPLE "i686-unknown-linux-android")
      elseif("${arch}" STREQUAL "x86_64")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_NDK_TRIPLE "x86_64-linux-android")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_ALT_SPELLING "x86_64")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_PATH "${POLAR_ANDROID_NDK_PATH}/platforms/android-${POLAR_ANDROID_API_LEVEL}/arch-x86_64")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_TRIPLE "x86_64-unknown-linux-android")
      else()
        message(FATAL_ERROR "unknown arch for android SDK: ${arch}")
      endif()

      # Get the prebuilt suffix to create the correct toolchain path when using the NDK
      if(CMAKE_HOST_SYSTEM_NAME STREQUAL Darwin)
        set(_polar_android_prebuilt_build darwin-x86_64)
      elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
        set(_polar_android_prebuilt_build linux-x86_64)
      elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
        set(_polar_android_prebuilt_build Windows-x86_64)
      else()
        message(SEND_ERROR "cannot cross-compile to android from ${CMAKE_HOST_SYSTEM_NAME}")
      endif()
      if("${arch}" STREQUAL "i686")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_NDK_PREBUILT_PATH
           "${POLAR_ANDROID_NDK_PATH}/toolchains/x86-${POLAR_ANDROID_NDK_GCC_VERSION}/prebuilt/${_polar_android_prebuilt_build}")
      elseif("${arch}" STREQUAL "x86_64")
        set(POLAR_SDK_ANDROID_ARCH_${arch}_NDK_PREBUILT_PATH
           "${POLAR_ANDROID_NDK_PATH}/toolchains/x86_64-${POLAR_ANDROID_NDK_GCC_VERSION}/prebuilt/${_polar_android_prebuilt_build}")
      else()
        set(POLAR_SDK_ANDROID_ARCH_${arch}_NDK_PREBUILT_PATH
           "${POLAR_ANDROID_NDK_PATH}/toolchains/${POLAR_SDK_ANDROID_ARCH_${arch}_NDK_TRIPLE}-${POLAR_ANDROID_NDK_GCC_VERSION}/prebuilt/${_polar_android_prebuilt_build}")
      endif()
    else()
      set(POLAR_SDK_${prefix}_ARCH_${arch}_PATH "/" CACHE STRING "CMAKE_SYSROOT for ${prefix} ${arch}")

      if("${prefix}" STREQUAL "HAIKU")
        set(POLAR_SDK_HAIKU_ARCH_${arch}_LIBC_INCLUDE_DIRECTORY "/system/develop/headers/posix" CACHE STRING "Path to C library headers")
        set(POLAR_SDK_HAIKU_ARCH_${arch}_LIBC_ARCHITECTURE_INCLUDE_DIRECTORY "/system/develop/headers" CACHE STRING "Path to C library architecture headers")
      else()
        set(POLAR_SDK_${prefix}_ARCH_${arch}_LIBC_INCLUDE_DIRECTORY "/usr/include" CACHE STRING "Path to C library headers")
        set(POLAR_SDK_${prefix}_ARCH_${arch}_LIBC_ARCHITECTURE_INCLUDE_DIRECTORY "${POLAR_SDK_${prefix}_ARCH_${arch}_LIBC_INCLUDE_DIRECTORY}/${CMAKE_LIBRARY_ARCHITECTURE}" CACHE STRING "Path to C library architecture headers")
      endif()

      if("${prefix}" STREQUAL "LINUX")
        if(arch MATCHES "(armv6|armv7)")
          set(POLAR_SDK_LINUX_ARCH_${arch}_TRIPLE "${arch}-unknown-linux-gnueabihf")
        elseif(arch MATCHES "(aarch64|i686|powerpc64|powerpc64le|s390x|x86_64)")
          set(POLAR_SDK_LINUX_ARCH_${arch}_TRIPLE "${arch}-unknown-linux-gnu")
        else()
          message(FATAL_ERROR "unknown arch for ${prefix}: ${arch}")
        endif()
      elseif("${prefix}" STREQUAL "FREEBSD")
        if(NOT arch STREQUAL x86_64)
          message(FATAL_ERROR "unsupported arch for FreeBSD: ${arch}")
        endif()

        if(CMAKE_HOST_SYSTEM_NAME NOT STREQUAL FreeBSD)
          message(WARNING "CMAKE_SYSTEM_VERSION will not match target")
        endif()

        string(REPLACE "[-].*" "" freebsd_system_version ${CMAKE_SYSTEM_VERSION})
        message(STATUS "FreeBSD Version: ${freebsd_system_version}")

        set(POLAR_SDK_FREEBSD_ARCH_x86_64_TRIPLE "x86_64-unknown-freebsd${freebsd_system_version}")
      elseif("${prefix}" STREQUAL "CYGWIN")
        if(NOT arch STREQUAL x86_64)
          message(FATAL_ERROR "unsupported arch for cygwin: ${arch}")
        endif()
        set(POLAR_SDK_CYGWIN_ARCH_x86_64_TRIPLE "x86_64-unknown-windows-cygnus")
      elseif("${prefix}" STREQUAL "HAIKU")
        if(NOT arch STREQUAL x86_64)
          message(FATAL_ERROR "unsupported arch for Haiku: ${arch}")
        endif()
        set(POLAR_SDK_HAIKU_ARCH_x86_64_TRIPLE "x86_64-unknown-haiku")
      else()
        message(FATAL_ERROR "unknown Unix OS: ${prefix}")
      endif()
    endif()
  endforeach()

  # Add this to the list of known SDKs.
  list(APPEND POLAR_CONFIGURED_SDKS "${prefix}")

  _report_sdk("${prefix}")
endmacro()

macro(configure_sdk_windows name environment architectures)
  # Note: this has to be implemented as a macro because it sets global
  # variables.

  polar_windows_cache_VCVARS()

  string(TOUPPER ${name} prefix)
  string(TOLOWER ${name} platform)

  set(POLAR_SDK_${prefix}_NAME "${name}")
  set(POLAR_SDK_${prefix}_LIB_SUBDIR "windows")
  set(POLAR_SDK_${prefix}_ARCHITECTURES "${architectures}")
  set(POLAR_SDK_${prefix}_OBJECT_FORMAT "COFF")

  foreach(arch ${architectures})
    if(arch STREQUAL armv7)
      set(POLAR_SDK_${prefix}_ARCH_${arch}_TRIPLE
         "thumbv7-unknown-windows-${environment}")
    else()
      set(POLAR_SDK_${prefix}_ARCH_${arch}_TRIPLE
         "${arch}-unknown-windows-${environment}")
    endif()
    # NOTE: set the path to / to avoid a spurious `--sysroot` from being passed
    # to the driver -- rely on the `INCLUDE` AND `LIB` environment variables
    # instead.
    set(POLAR_SDK_${prefix}_ARCH_${arch}_PATH "/")

    # NOTE(compnerd) workaround incorrectly extensioned import libraries from
    # the Windows SDK on case sensitive file systems.
    polar_windows_arch_spelling(${arch} WinSDKArchitecture)
    set(WinSDK${arch}UMDir "${UniversalCRTSdkDir}/Lib/${UCRTVersion}/um/${WinSDKArchitecture}")
    set(OverlayDirectory "${CMAKE_BINARY_DIR}/winsdk_lib_${arch}_symlinks")

    if(NOT EXISTS "${UniversalCRTSdkDir}/Include/${UCRTVersion}/um/WINDOWS.H")
      file(MAKE_DIRECTORY ${OverlayDirectory})

      file(GLOB libraries RELATIVE "${WinSDK${arch}UMDir}" "${WinSDK${arch}UMDir}/*")
      foreach(library ${libraries})
        get_filename_component(name_we "${library}" NAME_WE)
        get_filename_component(ext "${library}" EXT)
        string(TOLOWER "${ext}" lowercase_ext)
        set(lowercase_ext_symlink_name "${name_we}${lowercase_ext}")
        if(NOT library STREQUAL lowercase_ext_symlink_name)
          execute_process(COMMAND
             "${CMAKE_COMMAND}" -E create_symlink "${WinSDK${arch}UMDir}/${library}" "${OverlayDirectory}/${lowercase_ext_symlink_name}")
        endif()
      endforeach()
    endif()
  endforeach()

  # Add this to the list of known SDKs.
  list(APPEND POLAR_CONFIGURED_SDKS "${prefix}")

  _report_sdk("${prefix}")
endmacro()

# Configure a variant of a certain SDK
#
# In addition to the SDK and architecture, a variant determines build settings.
#
# FIXME: this is not wired up with anything yet.
function(configure_target_variant prefix name sdk build_config lib_subdir)
  set(POLAR_VARIANT_${prefix}_NAME               ${name})
  set(POLAR_VARIANT_${prefix}_SDK_PATH           ${POLAR_SDK_${sdk}_PATH})
  set(POLAR_VARIANT_${prefix}_VERSION            ${POLAR_SDK_${sdk}_VERSION})
  set(POLAR_VARIANT_${prefix}_BUILD_NUMBER       ${POLAR_SDK_${sdk}_BUILD_NUMBER})
  set(POLAR_VARIANT_${prefix}_DEPLOYMENT_VERSION ${POLAR_SDK_${sdk}_DEPLOYMENT_VERSION})
  set(POLAR_VARIANT_${prefix}_LIB_SUBDIR         "${lib_subdir}/${POLAR_SDK_${sdk}_LIB_SUBDIR}")
  set(POLAR_VARIANT_${prefix}_VERSION_MIN_NAME   ${POLAR_SDK_${sdk}_VERSION_MIN_NAME})
  set(POLAR_VARIANT_${prefix}_TRIPLE_NAME        ${POLAR_SDK_${sdk}_TRIPLE_NAME})
  set(POLAR_VARIANT_${prefix}_ARCHITECTURES      ${POLAR_SDK_${sdk}_ARCHITECTURES})
endfunction()

