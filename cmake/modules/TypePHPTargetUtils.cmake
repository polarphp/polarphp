# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

include(TargetUtils)

# Add a new PHP target library.
#
# NOTE: This has not had the polarphp host code debrided from it yet. That will be
# in a forthcoming commit.
#
# Usage:
#   polar_add_php_target_library(name
#     [SHARED]
#     [STATIC]
#     [DEPENDS dep1 ...]
#     [LINK_LIBRARIES dep1 ...]
#     [POLAR_MODULE_DEPENDS dep1 ...]
#     [FRAMEWORK_DEPENDS dep1 ...]
#     [FRAMEWORK_DEPENDS_WEAK dep1 ...]
#     [LLVM_LINK_COMPONENTS comp1 ...]
#     [FILE_DEPENDS target1 ...]
#     [TARGET_SDKS sdk1...]
#     [C_COMPILE_FLAGS flag1...]
#     [POLAR_COMPILE_FLAGS flag1...]
#     [LINK_FLAGS flag1...]
#     [DONT_EMBED_BITCODE]
#     [INSTALL]
#     [IS_STDLIB]
#     [IS_STDLIB_CORE]
#     [INSTALL_WITH_SHARED]
#     INSTALL_IN_COMPONENT comp
#     DEPLOYMENT_VERSION_OSX version
#     DEPLOYMENT_VERSION_IOS version
#     DEPLOYMENT_VERSION_TVOS version
#     DEPLOYMENT_VERSION_WATCHOS version
#     source1 [source2 source3 ...])
#
# name
#   Name of the library (e.g., PolarLLParser).
#
# SHARED
#   Build a shared library.
#
# STATIC
#   Build a static library.
#
# DEPENDS
#   Targets that this library depends on.
#
# LINK_LIBRARIES
#   Libraries this library depends on.
#
# POLAR_MODULE_DEPENDS
#   Swift modules this library depends on.
#
# POLAR_MODULE_DEPENDS_OSX
#   Swift modules this library depends on when built for OS X.
#
# POLAR_MODULE_DEPENDS_IOS
#   Swift modules this library depends on when built for iOS.
#
# POLAR_MODULE_DEPENDS_TVOS
#   Swift modules this library depends on when built for tvOS.
#
# POLAR_MODULE_DEPENDS_WATCHOS
#   Swift modules this library depends on when built for watchOS.
#
# POLAR_MODULE_DEPENDS_FREEBSD
#   Swift modules this library depends on when built for FreeBSD.
#
# POLAR_MODULE_DEPENDS_LINUX
#   Swift modules this library depends on when built for Linux.
#
# POLAR_MODULE_DEPENDS_CYGWIN
#   Swift modules this library depends on when built for Cygwin.
#
# POLAR_MODULE_DEPENDS_HAIKU
#   Swift modules this library depends on when built for Haiku.
#
# FRAMEWORK_DEPENDS
#   System frameworks this library depends on.
#
# FRAMEWORK_DEPENDS_WEAK
#   System frameworks this library depends on that should be weak-linked
#
# LLVM_LINK_COMPONENTS
#   LLVM components this library depends on.
#
# FILE_DEPENDS
#   Additional files this library depends on.
#
# TARGET_SDKS
#   The set of SDKs in which this library is included. If empty, the library
#   is included in all SDKs.
#
# C_COMPILE_FLAGS
#   Extra compiler flags (C, C++, ObjC).
#
# POLAR_COMPILE_FLAGS
#   Extra compiler flags (Swift).
#
# LINK_FLAGS
#   Extra linker flags.
#
# DONT_EMBED_BITCODE
#   Don't embed LLVM bitcode in this target, even if it is enabled globally.
#
# IS_STDLIB
#   Treat the library as a part of the Swift standard library.
#
# IS_STDLIB_CORE
#   Compile as the Swift standard library core.
#
# IS_SDK_OVERLAY
#   Treat the library as a part of the Swift SDK overlay.
#   IS_SDK_OVERLAY implies IS_STDLIB.
#
# INSTALL_IN_COMPONENT comp
#   The Swift installation component that this library belongs to.
#
# DEPLOYMENT_VERSION_OSX
#   The minimum deployment version to build for if this is an OSX library.
#
# DEPLOYMENT_VERSION_IOS
#   The minimum deployment version to build for if this is an iOS library.
#
# DEPLOYMENT_VERSION_TVOS
#   The minimum deployment version to build for if this is an TVOS library.
#
# DEPLOYMENT_VERSION_WATCHOS
#   The minimum deployment version to build for if this is an WATCHOS library.
#
# INSTALL_WITH_SHARED
#   Install a static library target alongside shared libraries
#
# source1 ...
#   Sources to add into this library.
function(polar_add_php_target_library name)
   set(POLARPHP_LIB_options
      DONT_EMBED_BITCODE
      FORCE_BUILD_OPTIMIZED
      HAS_PHP_CONTENT
      IS_SDK_OVERLAY
      IS_STDLIB
      IS_STDLIB_CORE
      NOPHP_RT
      OBJECT_LIBRARY
      SHARED
      STATIC
      INSTALL_WITH_SHARED)
   set(POLARPHP_LIB_single_parameter_options
      DEPLOYMENT_VERSION_IOS
      DEPLOYMENT_VERSION_OSX
      DEPLOYMENT_VERSION_TVOS
      DEPLOYMENT_VERSION_WATCHOS
      INSTALL_IN_COMPONENT
      DARWIN_INSTALL_NAME_DIR)
   set(POLARPHP_LIB_multiple_parameter_options
      C_COMPILE_FLAGS
      DEPENDS
      FILE_DEPENDS
      FRAMEWORK_DEPENDS
      FRAMEWORK_DEPENDS_IOS_TVOS
      FRAMEWORK_DEPENDS_OSX
      FRAMEWORK_DEPENDS_WEAK
      GYB_SOURCES
      INCORPORATE_OBJECT_LIBRARIES
      INCORPORATE_OBJECT_LIBRARIES_SHARED_ONLY
      LINK_FLAGS
      LINK_LIBRARIES
      LLVM_LINK_COMPONENTS
      PRIVATE_LINK_LIBRARIES
      POLAR_COMPILE_FLAGS
      POLAR_COMPILE_FLAGS_IOS
      POLAR_COMPILE_FLAGS_OSX
      POLAR_COMPILE_FLAGS_TVOS
      POLAR_COMPILE_FLAGS_WATCHOS
      POLAR_COMPILE_FLAGS_LINUX
      POLAR_MODULE_DEPENDS
      POLAR_MODULE_DEPENDS_CYGWIN
      POLAR_MODULE_DEPENDS_FREEBSD
      POLAR_MODULE_DEPENDS_HAIKU
      POLAR_MODULE_DEPENDS_IOS
      POLAR_MODULE_DEPENDS_LINUX
      POLAR_MODULE_DEPENDS_OSX
      POLAR_MODULE_DEPENDS_TVOS
      POLAR_MODULE_DEPENDS_WATCHOS
      POLAR_MODULE_DEPENDS_WINDOWS
      TARGET_SDKS)

   cmake_parse_arguments(PHP_LIB
      "${POLARPHP_LIB_options}"
      "${POLARPHP_LIB_single_parameter_options}"
      "${POLARPHP_LIB_multiple_parameter_options}"
      ${ARGN})
   set(POLARPHP_LIB_SOURCES ${POLARPHP_LIB_UNPARSED_ARGUMENTS})

   # Infer arguments.

   if(POLARPHP_LIB_IS_SDK_OVERLAY)
      set(POLARPHP_LIB_HAS_PHP_CONTENT TRUE)
      set(POLARPHP_LIB_IS_STDLIB TRUE)
   endif()

   # Standard library is always a target library.
   if(POLARPHP_LIB_IS_STDLIB)
      set(POLARPHP_LIB_HAS_PHP_CONTENT TRUE)
   endif()

   # If target SDKs are not specified, build for all known SDKs.
   if("${POLARPHP_LIB_TARGET_SDKS}" STREQUAL "")
      set(POLARPHP_LIB_TARGET_SDKS ${POLAR_SDKS})
   endif()
   list_replace(POLARPHP_LIB_TARGET_SDKS ALL_APPLE_PLATFORMS "${POLAR_APPLE_PLATFORMS}")

   # All Swift code depends on the standard library, except for the standard
   # library itself.
   if(POLARPHP_LIB_HAS_PHP_CONTENT AND NOT POLARPHP_LIB_IS_STDLIB_CORE)
      list(APPEND POLARPHP_LIB_POLAR_MODULE_DEPENDS Core)

      # polarphpOnoneSupport does not depend on itself, obviously.
      if(NOT ${name} STREQUAL polarphpOnoneSupport)
         # All Swift code depends on the SwiftOnoneSupport in non-optimized mode,
         # except for the standard library itself.
         is_build_type_optimized("${POLAR_STDLIB_BUILD_TYPE}" optimized)
         if(NOT optimized)
            list(APPEND POLARPHP_LIB_POLAR_MODULE_DEPENDS SwiftOnoneSupport)
         endif()
      endif()
   endif()

   if((NOT "${POLAR_BUILD_STDLIB}") AND
   (NOT "${POLARPHP_LIB_POLAR_MODULE_DEPENDS}" STREQUAL ""))
      list(REMOVE_ITEM POLARPHP_LIB_POLAR_MODULE_DEPENDS Core SwiftOnoneSupport)
   endif()

   translate_flags(PHP_LIB "${POLARPHP_LIB_options}")
   precondition(POLARPHP_LIB_INSTALL_IN_COMPONENT MESSAGE "INSTALL_IN_COMPONENT is required")

   if(NOT POLARPHP_LIB_SHARED AND
      NOT POLARPHP_LIB_STATIC AND
      NOT POLARPHP_LIB_OBJECT_LIBRARY)
      message(FATAL_ERROR
         "Either SHARED, STATIC, or OBJECT_LIBRARY must be specified")
   endif()

   # In the standard library and overlays, warn about implicit overrides
   # as a reminder to consider when inherited protocols need different
   # behavior for their requirements.
   if (POLARPHP_LIB_IS_STDLIB)
      list(APPEND POLARPHP_LIB_POLAR_COMPILE_FLAGS "-warn-implicit-overrides")
   endif()

   if(NOT POLAR_BUILD_RUNTIME_WITH_HOST_COMPILER AND NOT BUILD_STANDALONE)
      list(APPEND POLARPHP_LIB_DEPENDS clang)
   endif()

   # If we are building this library for targets, loop through the various
   # SDKs building the variants of this library.
   list_intersect(
      "${POLARPHP_LIB_TARGET_SDKS}" "${POLAR_SDKS}" POLARPHP_LIB_TARGET_SDKS)

   foreach(sdk ${POLARPHP_LIB_TARGET_SDKS})
      if(NOT POLAR_SDK_${sdk}_ARCHITECTURES)
         # POLAR_SDK_${sdk}_ARCHITECTURES is empty, so just continue
         continue()
      endif()

      set(THIN_INPUT_TARGETS)

      # Collect architecture agnostic SDK module dependencies
      set(phplib_module_depends_flattened ${POLARPHP_LIB_POLAR_MODULE_DEPENDS})
      if(${sdk} STREQUAL OSX)
         list(APPEND phplib_module_depends_flattened
            ${POLARPHP_LIB_POLAR_MODULE_DEPENDS_OSX})
      elseif(${sdk} STREQUAL IOS OR ${sdk} STREQUAL IOS_SIMULATOR)
         list(APPEND phplib_module_depends_flattened
            ${POLARPHP_LIB_POLAR_MODULE_DEPENDS_IOS})
      elseif(${sdk} STREQUAL TVOS OR ${sdk} STREQUAL TVOS_SIMULATOR)
         list(APPEND phplib_module_depends_flattened
            ${POLARPHP_LIB_POLAR_MODULE_DEPENDS_TVOS})
      elseif(${sdk} STREQUAL WATCHOS OR ${sdk} STREQUAL WATCHOS_SIMULATOR)
         list(APPEND phplib_module_depends_flattened
            ${POLARPHP_LIB_POLAR_MODULE_DEPENDS_WATCHOS})
      elseif(${sdk} STREQUAL FREEBSD)
         list(APPEND phplib_module_depends_flattened
            ${POLARPHP_LIB_POLAR_MODULE_DEPENDS_FREEBSD})
      elseif(${sdk} STREQUAL LINUX OR ${sdk} STREQUAL ANDROID)
         list(APPEND phplib_module_depends_flattened
            ${POLARPHP_LIB_POLAR_MODULE_DEPENDS_LINUX})
      elseif(${sdk} STREQUAL CYGWIN)
         list(APPEND phplib_module_depends_flattened
            ${POLARPHP_LIB_POLAR_MODULE_DEPENDS_CYGWIN})
      elseif(${sdk} STREQUAL HAIKU)
         list(APPEND phplib_module_depends_flattened
            ${POLARPHP_LIB_POLAR_MODULE_DEPENDS_HAIKU})
      elseif(${sdk} STREQUAL WINDOWS)
         list(APPEND phplib_module_depends_flattened
            ${POLARPHP_LIB_POLAR_MODULE_DEPENDS_WINDOWS})
      endif()

      # Collect architecture agnostic SDK framework dependencies
      set(phplib_framework_depends_flattened ${POLARPHP_LIB_FRAMEWORK_DEPENDS})
      if(${sdk} STREQUAL OSX)
         list(APPEND phplib_framework_depends_flattened
            ${POLARPHP_LIB_FRAMEWORK_DEPENDS_OSX})
      elseif(${sdk} STREQUAL IOS OR ${sdk} STREQUAL IOS_SIMULATOR OR
         ${sdk} STREQUAL TVOS OR ${sdk} STREQUAL TVOS_SIMULATOR)
         list(APPEND phplib_framework_depends_flattened
            ${POLARPHP_LIB_FRAMEWORK_DEPENDS_IOS_TVOS})
      endif()

      # Collect architecutre agnostic compiler flags
      set(phplib_polarphp_compile_flags_all ${POLARPHP_LIB_POLAR_COMPILE_FLAGS})
      if(${sdk} STREQUAL OSX)
         list(APPEND phplib_polarphp_compile_flags_all
            ${POLARPHP_LIB_POLAR_COMPILE_FLAGS_OSX})
      elseif(${sdk} STREQUAL IOS OR ${sdk} STREQUAL IOS_SIMULATOR)
         list(APPEND phplib_polarphp_compile_flags_all
            ${POLARPHP_LIB_POLAR_COMPILE_FLAGS_IOS})
      elseif(${sdk} STREQUAL TVOS OR ${sdk} STREQUAL TVOS_SIMULATOR)
         list(APPEND phplib_polarphp_compile_flags_all
            ${POLARPHP_LIB_POLAR_COMPILE_FLAGS_TVOS})
      elseif(${sdk} STREQUAL WATCHOS OR ${sdk} STREQUAL WATCHOS_SIMULATOR)
         list(APPEND phplib_polarphp_compile_flags_all
            ${POLARPHP_LIB_POLAR_COMPILE_FLAGS_WATCHOS})
      elseif(${sdk} STREQUAL LINUX)
         list(APPEND phplib_polarphp_compile_flags_all
            ${POLARPHP_LIB_POLAR_COMPILE_FLAGS_LINUX})
      elseif(${sdk} STREQUAL WINDOWS)
         # FIXME(SR2005) static and shared are not mutually exclusive; however
         # since we do a single build of the sources, this doesn't work for
         # building both simultaneously.  Effectively, only shared builds are
         # supported on windows currently.
         if(POLARPHP_LIB_SHARED)
            list(APPEND phplib_polarphp_compile_flags_all -D_WINDLL)
            if(POLARPHP_LIB_IS_STDLIB_CORE)
               list(APPEND phplib_polarphp_compile_flags_all -DpolarphpCore_EXPORTS)
            endif()
         elseif(POLARPHP_LIB_STATIC)
            list(APPEND phplib_polarphp_compile_flags_all -D_LIB)
         endif()
      endif()


      # Collect architecture agnostic SDK linker flags
      set(phplib_link_flags_all ${POLARPHP_LIB_LINK_FLAGS})
      if(${sdk} STREQUAL IOS_SIMULATOR AND ${name} STREQUAL polarphpMediaPlayer)
         # message("DISABLING AUTOLINK FOR polarphpMediaPlayer")
         list(APPEND phplib_link_flags_all "-Xlinker" "-ignore_auto_link")
      endif()

      # We unconditionally removed "-z,defs" from CMAKE_SHARED_LINKER_FLAGS in
      # polarphp_common_standalone_build_config_llvm within
      # TypePHPSharedCMakeConfig.cmake, where it was added by a call to
      # HandleLLVMOptions.
      #
      # Rather than applying it to all targets and libraries, we here add it
      # back to supported targets and libraries only.  This is needed for ELF
      # targets only; however, RemoteMirror needs to build with undefined
      # symbols.
      if(${POLAR_SDK_${sdk}_OBJECT_FORMAT} STREQUAL ELF AND
         NOT ${name} STREQUAL PolarRemoteMirror)
         list(APPEND phplib_link_flags_all "-Wl,-z,defs")
      endif()
      # Setting back linker flags which are not supported when making Android build on macOS cross-compile host.
      if(POLARPHP_LIB_SHARED)
         if(sdk IN_LIST POLAR_APPLE_PLATFORMS)
            list(APPEND phplib_link_flags_all "-dynamiclib -Wl,-headerpad_max_install_names")
         elseif(${sdk} STREQUAL ANDROID)
            list(APPEND phplib_link_flags_all "-shared")
            # TODO: Instead of `lib${name}.so` find variable or target property which already have this value.
            list(APPEND phplib_link_flags_all "-Wl,-soname,lib${name}.so")
         endif()
      endif()

      set(sdk_supported_archs
         ${POLAR_SDK_${sdk}_ARCHITECTURES}
         ${POLAR_SDK_${sdk}_MODULE_ARCHITECTURES})
      list(REMOVE_DUPLICATES sdk_supported_archs)

      # For each architecture supported by this SDK
      foreach(arch ${sdk_supported_archs})
         # Configure variables for this subdirectory.
         set(VARIANT_SUFFIX "-${POLAR_SDK_${sdk}_LIB_SUBDIR}-${arch}")
         set(VARIANT_NAME "${name}${VARIANT_SUFFIX}")
         set(MODULE_VARIANT_SUFFIX "-phpmodule${VARIANT_SUFFIX}")
         set(MODULE_VARIANT_NAME "${name}${MODULE_VARIANT_SUFFIX}")

         # Map dependencies over to the appropriate variants.
         set(phplib_link_libraries)
         foreach(lib ${POLARPHP_LIB_LINK_LIBRARIES})
            if(TARGET "${lib}${VARIANT_SUFFIX}")
               list(APPEND phplib_link_libraries "${lib}${VARIANT_SUFFIX}")
            else()
               list(APPEND phplib_link_libraries "${lib}")
            endif()
         endforeach()

         # polarphp compiles depend on php modules, while links depend on
         # linked libraries.  Find targets for both of these here.
         set(phplib_module_dependency_targets)
         set(phplib_private_link_libraries_targets)

         if(NOT BUILD_STANDALONE)
            foreach(mod ${phplib_module_depends_flattened})
               list(APPEND phplib_module_dependency_targets
                  "polar${mod}${MODULE_VARIANT_SUFFIX}")

               list(APPEND phplib_private_link_libraries_targets
                  "polar${mod}${VARIANT_SUFFIX}")
            endforeach()
         endif()

         foreach(lib ${POLARPHP_LIB_PRIVATE_LINK_LIBRARIES})
            if(TARGET "${lib}${VARIANT_SUFFIX}")
               list(APPEND phplib_private_link_libraries_targets
                  "${lib}${VARIANT_SUFFIX}")
            else()
               list(APPEND phplib_private_link_libraries_targets "${lib}")
            endif()
         endforeach()

         # Add PrivateFrameworks, rdar://28466433
         set(phplib_c_compile_flags_all ${POLARPHP_LIB_C_COMPILE_FLAGS})
         if(sdk IN_LIST POLAR_APPLE_PLATFORMS AND POLARPHP_LIB_IS_SDK_OVERLAY)
            set(phplib_polarphp_compile_private_frameworks_flag "-Fsystem" "${POLAR_SDK_${sdk}_ARCH_${arch}_PATH}/System/Library/PrivateFrameworks/")
         endif()

         list(APPEND phplib_c_compile_flags_all "-DPOLAR_TARGET_LIBRARY_NAME=${name}")

         # Add this library variant.
         _polar_add_library_single(
            ${VARIANT_NAME}
            ${name}
            ${POLARPHP_LIB_SHARED_keyword}
            ${POLARPHP_LIB_STATIC_keyword}
            ${POLARPHP_LIB_OBJECT_LIBRARY_keyword}
            ${POLARPHP_LIB_INSTALL_WITH_SHARED_keyword}
            ${POLARPHP_LIB_SOURCES}
            TARGET_LIBRARY
            MODULE_TARGET ${MODULE_VARIANT_NAME}
            SDK ${sdk}
            ARCHITECTURE ${arch}
            DEPENDS ${POLARPHP_LIB_DEPENDS}
            LINK_LIBRARIES ${phplib_link_libraries}
            FRAMEWORK_DEPENDS ${phplib_framework_depends_flattened}
            FRAMEWORK_DEPENDS_WEAK ${POLARPHP_LIB_FRAMEWORK_DEPENDS_WEAK}
            LLVM_LINK_COMPONENTS ${POLARPHP_LIB_LLVM_LINK_COMPONENTS}
            FILE_DEPENDS ${POLARPHP_LIB_FILE_DEPENDS} ${phplib_module_dependency_targets}
            C_COMPILE_FLAGS ${phplib_c_compile_flags_all}
            POLAR_COMPILE_FLAGS ${phplib_polarphp_compile_flags_all} ${phplib_polarphp_compile_flags_arch} ${phplib_polarphp_compile_private_frameworks_flag}
            LINK_FLAGS ${phplib_link_flags_all}
            PRIVATE_LINK_LIBRARIES ${phplib_private_link_libraries_targets}
            INCORPORATE_OBJECT_LIBRARIES ${POLARPHP_LIB_INCORPORATE_OBJECT_LIBRARIES}
            INCORPORATE_OBJECT_LIBRARIES_SHARED_ONLY ${POLARPHP_LIB_INCORPORATE_OBJECT_LIBRARIES_SHARED_ONLY}
            ${POLARPHP_LIB_DONT_EMBED_BITCODE_keyword}
            ${POLARPHP_LIB_IS_STDLIB_keyword}
            ${POLARPHP_LIB_IS_STDLIB_CORE_keyword}
            ${POLARPHP_LIB_IS_SDK_OVERLAY_keyword}
            ${POLARPHP_LIB_FORCE_BUILD_OPTIMIZED_keyword}
            ${POLARPHP_LIB_NOPHP_RT_keyword}
            DARWIN_INSTALL_NAME_DIR "${POLARPHP_LIB_DARWIN_INSTALL_NAME_DIR}"
            INSTALL_IN_COMPONENT "${POLARPHP_LIB_INSTALL_IN_COMPONENT}"
            DEPLOYMENT_VERSION_OSX "${POLARPHP_LIB_DEPLOYMENT_VERSION_OSX}"
            DEPLOYMENT_VERSION_IOS "${POLARPHP_LIB_DEPLOYMENT_VERSION_IOS}"
            DEPLOYMENT_VERSION_TVOS "${POLARPHP_LIB_DEPLOYMENT_VERSION_TVOS}"
            DEPLOYMENT_VERSION_WATCHOS "${POLARPHP_LIB_DEPLOYMENT_VERSION_WATCHOS}"
            GYB_SOURCES ${POLARPHP_LIB_GYB_SOURCES}
         )
         if(NOT POLAR_BUILT_STANDALONE AND NOT "${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
            add_dependencies(${VARIANT_NAME} clang)
         endif()

         if(sdk STREQUAL WINDOWS)
            if(POLAR_COMPILER_IS_MSVC_LIKE)
               if (POLAR_STDLIB_MSVC_RUNTIME_LIBRARY MATCHES MultiThreadedDebugDLL)
                  target_compile_options(${VARIANT_NAME} PRIVATE /MDd /D_DLL /D_DEBUG)
               elseif (POLAR_STDLIB_MSVC_RUNTIME_LIBRARY MATCHES MultiThreadedDebug)
                  target_compile_options(${VARIANT_NAME} PRIVATE /MTd /U_DLL /D_DEBUG)
               elseif (POLAR_STDLIB_MSVC_RUNTIME_LIBRARY MATCHES MultiThreadedDLL)
                  target_compile_options(${VARIANT_NAME} PRIVATE /MD /D_DLL /U_DEBUG)
               elseif (POLAR_STDLIB_MSVC_RUNTIME_LIBRARY MATCHES MultiThreaded)
                  target_compile_options(${VARIANT_NAME} PRIVATE /MT /U_DLL /U_DEBUG)
               endif()
            endif()
         endif()

         if(NOT POLARPHP_LIB_OBJECT_LIBRARY)
            # Add dependencies on the (not-yet-created) custom lipo target.
            foreach(DEP ${POLARPHP_LIB_LINK_LIBRARIES})
               if (NOT "${DEP}" STREQUAL "icucore")
                  add_dependencies(${VARIANT_NAME}
                     "${DEP}-${POLAR_SDK_${sdk}_LIB_SUBDIR}")
               endif()
            endforeach()

            if (POLARPHP_LIB_IS_STDLIB AND POLARPHP_LIB_STATIC)
               # Add dependencies on the (not-yet-created) custom lipo target.
               foreach(DEP ${POLARPHP_LIB_LINK_LIBRARIES})
                  if (NOT "${DEP}" STREQUAL "icucore")
                     add_dependencies("${VARIANT_NAME}-static"
                        "${DEP}-${POLAR_SDK_${sdk}_LIB_SUBDIR}-static")
                  endif()
               endforeach()
            endif()

            if(arch IN_LIST POLAR_SDK_${sdk}_ARCHITECTURES)
               # Note this thin library.
               list(APPEND THIN_INPUT_TARGETS ${VARIANT_NAME})
            endif()
         endif()
      endforeach()

      # Configure module-only targets
      if(NOT POLAR_SDK_${sdk}_ARCHITECTURES
         AND POLAR_SDK_${sdk}_MODULE_ARCHITECTURES)
         set(_target "${name}-${POLAR_SDK_${sdk}_LIB_SUBDIR}")

         # Create unified sdk target
         add_custom_target("${_target}")

         foreach(_arch ${POLAR_SDK_${sdk}_MODULE_ARCHITECTURES})
            set(_variant_suffix "-${POLAR_SDK_${sdk}_LIB_SUBDIR}-${_arch}")
            set(_module_variant_name "${name}-phpmodule-${_variant_suffix}")

            add_dependencies("${_target}" ${_module_variant_name})

            # Add Swift standard library targets as dependencies to the top-level
            # convenience target.
            if(TARGET "php-stdlib${_variant_suffix}")
               add_dependencies("php-stdlib${_variant_suffix}"
                  "${_target}")
            endif()
         endforeach()

         return()
      endif()

      if(NOT POLARPHP_LIB_OBJECT_LIBRARY)
         # Determine the name of the universal library.
         if(POLARPHP_LIB_SHARED)
            if("${sdk}" STREQUAL "WINDOWS")
               set(UNIVERSAL_LIBRARY_NAME
                  "${POLARPHP_LIB_DIR}/${POLAR_SDK_${sdk}_LIB_SUBDIR}/${name}.dll")
            else()
               set(UNIVERSAL_LIBRARY_NAME
                  "${POLARPHP_LIB_DIR}/${POLAR_SDK_${sdk}_LIB_SUBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}${name}${CMAKE_SHARED_LIBRARY_SUFFIX}")
            endif()
         else()
            if("${sdk}" STREQUAL "WINDOWS")
               set(UNIVERSAL_LIBRARY_NAME
                  "${POLARPHP_LIB_DIR}/${POLAR_SDK_${sdk}_LIB_SUBDIR}/${name}.lib")
            else()
               set(UNIVERSAL_LIBRARY_NAME
                  "${POLARPHP_LIB_DIR}/${POLAR_SDK_${sdk}_LIB_SUBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${name}${CMAKE_STATIC_LIBRARY_SUFFIX}")
            endif()
         endif()

         set(lipo_target "${name}-${POLAR_SDK_${sdk}_LIB_SUBDIR}")
         if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin" AND POLARPHP_LIB_SHARED)
            set(codesign_arg CODESIGN)
         endif()
         precondition(THIN_INPUT_TARGETS)
         _add_php_lipo_target(SDK
            ${sdk}
            TARGET
            ${lipo_target}
            OUTPUT
            ${UNIVERSAL_LIBRARY_NAME}
            ${codesign_arg}
            ${THIN_INPUT_TARGETS})

         # Cache universal libraries for dependency purposes
         set(UNIVERSAL_LIBRARY_NAMES_${POLAR_SDK_${sdk}_LIB_SUBDIR}
            ${UNIVERSAL_LIBRARY_NAMES_${POLAR_SDK_${sdk}_LIB_SUBDIR}}
            ${lipo_target}
            CACHE INTERNAL "UNIVERSAL_LIBRARY_NAMES_${POLAR_SDK_${sdk}_LIB_SUBDIR}")

         # Determine the subdirectory where this library will be installed.
         set(resource_dir_sdk_subdir "${POLAR_SDK_${sdk}_LIB_SUBDIR}")
         precondition(resource_dir_sdk_subdir)

         if(POLARPHP_LIB_SHARED OR POLARPHP_LIB_INSTALL_WITH_SHARED)
            set(resource_dir "polarphp")
            set(file_permissions
               OWNER_READ OWNER_WRITE OWNER_EXECUTE
               GROUP_READ GROUP_EXECUTE
               WORLD_READ WORLD_EXECUTE)
         else()
            set(resource_dir "polarphp_static")
            set(file_permissions
               OWNER_READ OWNER_WRITE
               GROUP_READ
               WORLD_READ)
         endif()

         set(optional_arg)
         if(sdk IN_LIST POLAR_APPLE_PLATFORMS)
            # Allow installation of stdlib without building all variants on Darwin.
            set(optional_arg "OPTIONAL")
         endif()

         if(sdk STREQUAL WINDOWS AND CMAKE_SYSTEM_NAME STREQUAL Windows)
            add_dependencies(${POLARPHP_LIB_INSTALL_IN_COMPONENT} ${name}-windows-${POLAR_PRIMARY_VARIANT_ARCH})
            polar_install_in_component(TARGETS ${name}-windows-${POLAR_PRIMARY_VARIANT_ARCH}
               RUNTIME
               DESTINATION "bin"
               COMPONENT "${POLARPHP_LIB_INSTALL_IN_COMPONENT}"
               LIBRARY
               DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/${resource_dir}/${resource_dir_sdk_subdir}/${POLAR_PRIMARY_VARIANT_ARCH}"
               COMPONENT "${POLARPHP_LIB_INSTALL_IN_COMPONENT}"
               ARCHIVE
               DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/${resource_dir}/${resource_dir_sdk_subdir}/${POLAR_PRIMARY_VARIANT_ARCH}"
               COMPONENT "${POLARPHP_LIB_INSTALL_IN_COMPONENT}"
               PERMISSIONS ${file_permissions})
         else()
            # NOTE: ${UNIVERSAL_LIBRARY_NAME} is the output associated with the target
            # ${lipo_target}
            add_dependencies(${POLARPHP_LIB_INSTALL_IN_COMPONENT} ${lipo_target})
            polar_install_in_component(FILES "${UNIVERSAL_LIBRARY_NAME}"
               DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/${resource_dir}/${resource_dir_sdk_subdir}"
               COMPONENT "${POLARPHP_LIB_INSTALL_IN_COMPONENT}"
               PERMISSIONS ${file_permissions}
               "${optional_arg}")
         endif()
         if(sdk STREQUAL WINDOWS)
            foreach(arch ${POLAR_SDK_WINDOWS_ARCHITECTURES})
               if(TARGET ${name}-windows-${arch}_IMPLIB)
                  get_target_property(import_library ${name}-windows-${arch}_IMPLIB IMPORTED_LOCATION)
                  add_dependencies(${POLARPHP_LIB_INSTALL_IN_COMPONENT} ${name}-windows-${arch}_IMPLIB)
                  polar_install_in_component(FILES ${import_library}
                     DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/${resource_dir}/${resource_dir_sdk_subdir}/${arch}"
                     COMPONENT ${POLARPHP_LIB_INSTALL_IN_COMPONENT}
                     PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
               endif()
            endforeach()
         endif()

         polar_is_installing_component(
            "${POLARPHP_LIB_INSTALL_IN_COMPONENT}"
            is_installing)

         # Add the arch-specific library targets to the global exports.
         foreach(arch ${POLAR_SDK_${sdk}_ARCHITECTURES})
            set(_variant_name "${name}-${POLAR_SDK_${sdk}_LIB_SUBDIR}-${arch}")
            if(NOT TARGET "${_variant_name}")
               continue()
            endif()

            if(is_installing)
               set_property(GLOBAL APPEND
                  PROPERTY POLAR_EXPORTS ${_variant_name})
            else()
               set_property(GLOBAL APPEND
                  PROPERTY POLAR_BUILDTREE_EXPORTS ${_variant_name})
            endif()
         endforeach()

         # Add the phpmodule-only targets to the lipo target depdencies.
         foreach(arch ${POLAR_SDK_${sdk}_MODULE_ARCHITECTURES})
            set(_variant_name "${name}-${POLAR_SDK_${sdk}_LIB_SUBDIR}-${arch}")
            if(NOT TARGET "${_variant_name}")
               continue()
            endif()

            add_dependencies("${lipo_target}" "${_variant_name}")
         endforeach()

         # If we built static variants of the library, create a lipo target for
         # them.
         set(lipo_target_static)
         if (POLARPHP_LIB_IS_STDLIB AND POLARPHP_LIB_STATIC)
            set(THIN_INPUT_TARGETS_STATIC)
            foreach(TARGET ${THIN_INPUT_TARGETS})
               list(APPEND THIN_INPUT_TARGETS_STATIC "${TARGET}-static")
            endforeach()

            if(POLARPHP_LIB_INSTALL_WITH_SHARED)
               set(install_subdir "polarphp")
               set(universal_subdir ${POLARPHP_LIB_DIR})
            else()
               set(install_subdir "polarphp_static")
               set(universal_subdir ${POLARPHP_STATICLIB_DIR})
            endif()

            set(lipo_target_static
               "${name}-${POLAR_SDK_${sdk}_LIB_SUBDIR}-static")
            set(UNIVERSAL_LIBRARY_NAME
               "${universal_subdir}/${POLAR_SDK_${sdk}_LIB_SUBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${name}${CMAKE_STATIC_LIBRARY_SUFFIX}")
            _add_php_lipo_target(SDK
               ${sdk}
               TARGET
               ${lipo_target_static}
               OUTPUT
               "${UNIVERSAL_LIBRARY_NAME}"
               ${THIN_INPUT_TARGETS_STATIC})
            add_dependencies(${POLARPHP_LIB_INSTALL_IN_COMPONENT} ${lipo_target_static})
            polar_install_in_component(FILES "${UNIVERSAL_LIBRARY_NAME}"
               DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/${install_subdir}/${resource_dir_sdk_subdir}"
               PERMISSIONS
               OWNER_READ OWNER_WRITE
               GROUP_READ
               WORLD_READ
               COMPONENT "${POLARPHP_LIB_INSTALL_IN_COMPONENT}"
               "${optional_arg}")
         endif()

         # Add Swift standard library targets as dependencies to the top-level
         # convenience target.
         set(FILTERED_UNITTESTS
            phpStdlibCollectionUnittest
            phpStdlibUnicodeUnittest)

         foreach(arch ${POLAR_SDK_${sdk}_ARCHITECTURES})
            set(VARIANT_SUFFIX "-${POLAR_SDK_${sdk}_LIB_SUBDIR}-${arch}")
            if(TARGET "php-stdlib${VARIANT_SUFFIX}" AND
               TARGET "php-test-stdlib${VARIANT_SUFFIX}")
               add_dependencies("php-stdlib${VARIANT_SUFFIX}"
                  ${lipo_target}
                  ${lipo_target_static})
               if(NOT "${name}" IN_LIST FILTERED_UNITTESTS)
                  add_dependencies("php-test-stdlib${VARIANT_SUFFIX}"
                     ${lipo_target}
                     ${lipo_target_static})
               endif()
            endif()
         endforeach()
      endif()
   endforeach()
endfunction()