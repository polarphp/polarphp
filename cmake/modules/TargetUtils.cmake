# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/22.

include(PolarList)
include(PolarXcodeSupport)
include(PolarWindowsSupport)
include(DetermineGCCCompatible)

function(polar_update_compile_flags name)
   get_property(sources TARGET ${name} PROPERTY SOURCES)
   if("${sources}" MATCHES "\\.c(;|$)")
      set(update_src_props ON)
   endif()

   # Assume that;
   #   - POLAR_COMPILE_FLAGS is list.
   #   - PROPERTY COMPILE_FLAGS is string.
   string(REPLACE ";" " " target_compile_flags " ${POLAR_COMPILE_FLAGS}")

   if(update_src_props)
      foreach(fn ${sources})
         get_filename_component(suf ${fn} EXT)
         if("${suf}" STREQUAL ".cpp")
            set_property(SOURCE ${fn} APPEND_STRING PROPERTY
               COMPILE_FLAGS "${target_compile_flags}")
         endif()
      endforeach()
   else()
      # Update target props, since all sources are C++.
      set_property(TARGET ${name} APPEND_STRING PROPERTY
         COMPILE_FLAGS "${target_compile_flags}")
   endif()
   #   set_property(TARGET ${name} APPEND PROPERTY COMPILE_DEFINITIONS "${POLAR_COMPILE_DEFINITIONS}")
endfunction()

function(polar_add_symbol_exports target_name export_file)
   if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(native_export_file "${target_name}.exports")
      add_custom_command(OUTPUT ${native_export_file}
         COMMAND sed -e "s/^/_/" < ${export_file} > ${native_export_file}
         DEPENDS ${export_file}
         VERBATIM
         COMMENT "Creating export file for ${target_name}")
      set_property(TARGET ${target_name} APPEND_STRING PROPERTY
         LINK_FLAGS " -Wl,-exported_symbols_list,${CMAKE_CURRENT_BINARY_DIR}/${native_export_file}")
   elseif(${CMAKE_SYSTEM_NAME} MATCHES "AIX")
      set_property(TARGET ${target_name} APPEND_STRING PROPERTY
         LINK_FLAGS " -Wl,-bE:${export_file}")
   elseif(POLAR_HAVE_LINK_VERSION_SCRIPT)
      # Gold and BFD ld require a version script rather than a plain list.
      set(native_export_file "${target_name}.exports")
      # FIXME: Don't write the "local:" line on OpenBSD.
      # in the export file, also add a linker script to version POLAR symbols (form: POLAR_N.M)
      add_custom_command(OUTPUT ${native_export_file}
         COMMAND echo "POLAR_${POLAR_VERSION_MAJOR}.${POLAR_VERSION_MINOR} {" > ${native_export_file}
         COMMAND grep -q "[[:alnum:]]" ${export_file} && echo "  global:" >> ${native_export_file} || :
         COMMAND sed -e "s/$/;/" -e "s/^/    /" < ${export_file} >> ${native_export_file}
         COMMAND echo "  local: *;" >> ${native_export_file}
         COMMAND echo "};" >> ${native_export_file}
         DEPENDS ${export_file}
         VERBATIM
         COMMENT "Creating export file for ${target_name}")
      if (${POLAR_LINKER_IS_SOLARISLD})
         set_property(TARGET ${target_name} APPEND_STRING PROPERTY
            LINK_FLAGS "  -Wl,-M,${CMAKE_CURRENT_BINARY_DIR}/${native_export_file}")
      else()
         set_property(TARGET ${target_name} APPEND_STRING PROPERTY
            LINK_FLAGS "  -Wl,--version-script,${CMAKE_CURRENT_BINARY_DIR}/${native_export_file}")
      endif()
   else()
      # TODO here need remove python dependency
      set(native_export_file "${target_name}.def")
      add_custom_command(OUTPUT ${native_export_file}
         COMMAND ${PYTHON_EXECUTABLE} -c "import sys;print(''.join(['EXPORTS\\n']+sys.stdin.readlines(),))"
         < ${export_file} > ${native_export_file}
         DEPENDS ${export_file}
         VERBATIM
         COMMENT "Creating export file for ${target_name}")
      set(export_file_linker_flag "${CMAKE_CURRENT_BINARY_DIR}/${native_export_file}")
      if(MSVC)
         set(export_file_linker_flag "/DEF:\"${export_file_linker_flag}\"")
      endif()
      set_property(TARGET ${target_name} APPEND_STRING PROPERTY
         LINK_FLAGS " ${export_file_linker_flag}")
   endif()

   add_custom_target(${target_name}_exports DEPENDS ${native_export_file})
   set_target_properties(${target_name}_exports PROPERTIES FOLDER "Misc")

   get_property(srcs TARGET ${target_name} PROPERTY SOURCES)
   foreach(src ${srcs})
      get_filename_component(extension ${src} EXT)
      if(extension STREQUAL ".cpp")
         set(first_source_file ${src})
         break()
      endif()
   endforeach()

   # Force re-linking when the exports file changes. Actually, it
   # forces recompilation of the source file. The LINK_DEPENDS target
   # property only works for makefile-based generators.
   # FIXME: This is not safe because this will create the same target
   # ${native_export_file} in several different file:
   # - One where we emitted ${target_name}_exports
   # - One where we emitted the build command for the following object.
   # set_property(SOURCE ${first_source_file} APPEND PROPERTY
   #   OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${native_export_file})

   set_property(DIRECTORY APPEND
      PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${native_export_file})

   add_dependencies(${target_name} ${target_name}_exports)

   # Add dependency to *_exports later -- CMake issue 14747
   list(APPEND POLAR_COMMON_DEPENDS ${target_name}_exports)
   set(POLAR_COMMON_DEPENDS ${POLAR_COMMON_DEPENDS} PARENT_SCOPE)
endfunction(polar_add_symbol_exports)

function(polar_add_link_opts)
   # Don't use linker optimizations in debug builds since it slows down the
   # linker in a context where the optimizations are not important.
   if (NOT POLAR_BUILD_TYPE STREQUAL "debug")

      # Pass -O3 to the linker. This enabled different optimizations on different
      # linkers.
      if(NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin|SunOS|AIX" OR WIN32))
         set_property(TARGET ${target_name} APPEND_STRING PROPERTY
            LINK_FLAGS " -Wl,-O3")
      endif()

      if(POLAR_LINKER_IS_GOLD)
         # With gold gc-sections is always safe.
         set_property(TARGET ${target_name} APPEND_STRING PROPERTY
            LINK_FLAGS " -Wl,--gc-sections")
         # Note that there is a bug with -Wl,--icf=safe so it is not safe
         # to enable. See https://sourceware.org/bugzilla/show_bug.cgi?id=17704.
      endif()

      if(NOT POLAR_NO_DEAD_STRIP)
         if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            # ld64's implementation of -dead_strip breaks tools that use plugins.
            set_property(TARGET ${target_name} APPEND_STRING PROPERTY
               LINK_FLAGS " -Wl,-dead_strip")
         elseif(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
            set_property(TARGET ${target_name} APPEND_STRING PROPERTY
               LINK_FLAGS " -Wl,-z -Wl,discard-unused=sections")
         elseif(NOT WIN32 AND NOT POLAR_LINKER_IS_GOLD)
            # Object files are compiled with -ffunction-data-sections.
            # Versions of bfd ld < 2.23.1 have a bug in --gc-sections that breaks
            # tools that use plugins. Always pass --gc-sections once we require
            # a newer linker.
            set_property(TARGET ${target_name} APPEND_STRING PROPERTY
               LINK_FLAGS " -Wl,--gc-sections")
         endif()
      endif()
   endif()
endfunction(polar_add_link_opts)

# Set each output directory according to ${CMAKE_CONFIGURATION_TYPES}.
# Note: Don't set variables CMAKE_*_OUTPUT_DIRECTORY any more,
# or a certain builder, for eaxample, msbuild.exe, would be confused.
function(polar_set_output_directory target)
   cmake_parse_arguments(ARG "" "BINARY_DIR;LIBRARY_DIR" "" ${ARGN})

   # module_dir -- corresponding to LIBRARY_OUTPUT_DIRECTORY.
   # It affects output of add_library(MODULE).
   if(WIN32 OR CYGWIN)
      # DLL platform
      set(module_dir ${ARG_BINARY_DIR})
   else()
      set(module_dir ${ARG_LIBRARY_DIR})
   endif()
   if(NOT "${CMAKE_CFG_INTDIR}" STREQUAL ".")
      foreach(build_mode ${CMAKE_CONFIGURATION_TYPES})
         string(TOUPPER "${build_mode}" CONFIG_SUFFIX)
         if(ARG_BINARY_DIR)
            string(REPLACE ${CMAKE_CFG_INTDIR} ${build_mode} bi ${ARG_BINARY_DIR})
            set_target_properties(${target} PROPERTIES "RUNTIME_OUTPUT_DIRECTORY_${CONFIG_SUFFIX}" ${bi})
         endif()
         if(ARG_LIBRARY_DIR)
            string(REPLACE ${CMAKE_CFG_INTDIR} ${build_mode} li ${ARG_LIBRARY_DIR})
            set_target_properties(${target} PROPERTIES "ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_SUFFIX}" ${li})
         endif()
         if(module_dir)
            string(REPLACE ${CMAKE_CFG_INTDIR} ${build_mode} mi ${module_dir})
            set_target_properties(${target} PROPERTIES "LIBRARY_OUTPUT_DIRECTORY_${CONFIG_SUFFIX}" ${mi})
         endif()
      endforeach()
   else()
      if(ARG_BINARY_DIR)
         set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ARG_BINARY_DIR})
      endif()
      if(ARG_LIBRARY_DIR)
         set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${ARG_LIBRARY_DIR})
      endif()
      if(module_dir)
         set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${module_dir})
      endif()
   endif()
endfunction(polar_set_output_directory)

# polar_add_library(name sources...
#   SHARED;STATIC
#     STATIC by default w/o BUILD_SHARED_LIBS.
#     SHARED by default w/  BUILD_SHARED_LIBS.
#   OBJECT
#     Also create an OBJECT library target. Default if STATIC && SHARED.
#   MODULE
#     Target ${name} might not be created on unsupported platforms.
#     Check with "if(TARGET ${name})".
#   DISABLE_POLAR_LINK_POLAR_DYLIB
#     Do not link this library to libPolarPHP, even if
#     POLAR_LINK_POLAR_DYLIB is enabled.
#   OUTPUT_NAME name
#     Corresponds to OUTPUT_NAME in target properties.
#   DEPENDS targets...
#     Same semantics as add_dependencies().
#   LINK_COMPONENTS components...
#     Same as the variable POLAR_LINK_COMPONENTS.
#   LINK_LIBS lib_targets...
#     Same semantics as target_link_libraries().
#   ADDITIONAL_HEADERS
#     May specify header files for IDE generators.
#   SONAME
#     Should set SONAME link flags and create symlinks
#   PLUGIN_TOOL
#     The tool (i.e. cmake target) that this plugin will link against
#   )
function(polar_add_library_internal name)
   cmake_parse_arguments(ARG
      "MODULE;SHARED;STATIC;OBJECT;DISABLE_POLAR_LINK_POLAR_DYLIB;SONAME"
      "OUTPUT_NAME;PLUGIN_TOOL"
      "ADDITIONAL_HEADERS;DEPENDS;LINK_COMPONENTS;LINK_LIBS;OBJLIBS"
      ${ARGN})
   list(APPEND POLAR_COMMON_DEPENDS ${ARG_DEPENDS})
   if(ARG_ADDITIONAL_HEADERS)
      # Pass through ADDITIONAL_HEADERS.
      set(ARG_ADDITIONAL_HEADERS ADDITIONAL_HEADERS ${ARG_ADDITIONAL_HEADERS})
   endif()

   if(ARG_OBJLIBS)
      set(ALL_FILES ${ARG_OBJLIBS})
   else()
      polar_process_sources(ALL_FILES ${ARG_UNPARSED_ARGUMENTS} ${ARG_ADDITIONAL_HEADERS})
   endif()
   handle_gyb_sources(gyb_dependency_targets ALL_FILES)

   if (gyb_dependency_targets)
      list(APPEND POLAR_COMMON_DEPENDS ${gyb_dependency_targets})
   endif()

   if(ARG_MODULE)
      if(ARG_SHARED OR ARG_STATIC)
         message(WARNING "MODULE with SHARED|STATIC doesn't make sense.")
      endif()
      # Plugins that link against a tool are allowed even when plugins in general are not
      if(NOT POLAR_ENABLE_PLUGINS AND NOT (ARG_PLUGIN_TOOL AND POLAR_EXPORT_SYMBOLS_FOR_PLUGINS))
         message(STATUS "${name} ignored -- Loadable modules not supported on this platform.")
         return()
      endif()
   else()
      if(ARG_PLUGIN_TOOL)
         message(WARNING "PLUGIN_TOOL without MODULE doesn't make sense.")
      endif()
      if(BUILD_SHARED_LIBS AND NOT ARG_STATIC)
         set(ARG_SHARED TRUE)
      endif()
      if(NOT ARG_SHARED)
         set(ARG_STATIC TRUE)
      endif()
   endif()

   # Generate objlib
   if((ARG_SHARED AND ARG_STATIC) OR ARG_OBJECT)
      # Generate an obj library for both targets.
      set(obj_name "obj.${name}")
      add_library(${obj_name} OBJECT EXCLUDE_FROM_ALL
         ${ALL_FILES}
         )
      polar_update_compile_flags(${obj_name})
      set(ALL_FILES "$<TARGET_OBJECTS:${obj_name}>")

      # Do add_dependencies(obj) later due to CMake issue 14747.
      list(APPEND objlibs ${obj_name})

      set_target_properties(${obj_name} PROPERTIES FOLDER "Object Libraries")
   endif()

   if(ARG_SHARED AND ARG_STATIC)
      # static
      set(name_static "${name}_static")
      if(ARG_OUTPUT_NAME)
         set(output_name OUTPUT_NAME "${ARG_OUTPUT_NAME}")
      endif()
      # DEPENDS has been appended to POLAR_COMMON_LIBS.
      polar_add_library_internal(${name_static} STATIC
         ${output_name}
         OBJLIBS ${ALL_FILES} # objlib
         LINK_LIBS ${ARG_LINK_LIBS}
         LINK_COMPONENTS ${ARG_LINK_COMPONENTS}
         )
      # FIXME: Add name_static to anywhere in TARGET ${name}'s PROPERTY.
      set(ARG_STATIC)
   endif()
   # is this really good ?
   if(ARG_MODULE)
      add_library(${name} MODULE ${ALL_FILES} ${POLAR_HEADER_SOURCES})
      polar_setup_rpath(${name})
   elseif(ARG_SHARED)
      add_library(${name} SHARED ${ALL_FILES} ${POLAR_HEADER_SOURCES})
      polar_setup_rpath(${name})
   else()
      add_library(${name} STATIC ${ALL_FILES} ${POLAR_HEADER_SOURCES})
   endif()

   polar_set_output_directory(${name} BINARY_DIR ${POLAR_RUNTIME_OUTPUT_INTDIR} LIBRARY_DIR ${POLAR_LIBRARY_OUTPUT_INTDIR})
   # $<TARGET_OBJECTS> doesn't require compile flags.
   if(NOT obj_name)
      polar_update_compile_flags(${name})
   endif()
   polar_add_link_opts(${name})
   if(ARG_OUTPUT_NAME)
      set_target_properties(${name}
         PROPERTIES
         OUTPUT_NAME ${ARG_OUTPUT_NAME}
         )
   endif()

   if(ARG_MODULE)
      set_target_properties(${name} PROPERTIES
         PREFIX ""
         SUFFIX ${POLAR_PLUGIN_EXT}
         )
   endif()

   if(ARG_SHARED)
      if(WIN32)
         set_target_properties(${name} PROPERTIES
            PREFIX ""
            )
      endif()

      # Set SOVERSION on shared libraries that lack explicit SONAME
      # specifier, on *nix systems that are not Darwin.
      if(UNIX AND NOT APPLE AND NOT ARG_SONAME)
         set_target_properties(${name}
            PROPERTIES
            # Since 1.0.0, the ABI version is indicated by the major version
            SOVERSION ${POLAR_VERSION_MAJOR}
            VERSION ${POLAR_VERSION_MAJOR}.${POLAR_VERSION_MINOR}.${POLAR_VERSION_PATCH}${POLAR_VERSION_SUFFIX})
      endif()
   endif()

   if(ARG_MODULE OR ARG_SHARED)
      # Do not add -Dname_EXPORTS to the command-line when building files in this
      # target. Doing so is actively harmful for the modules build because it
      # creates extra module variants, and not useful because we don't use these
      # macros.
      set_target_properties(${name} PROPERTIES DEFINE_SYMBOL "")

      if (POLAR_EXPORTED_SYMBOL_FILE)
         polar_add_symbol_exports(${name} ${POLAR_EXPORTED_SYMBOL_FILE})
      endif()
   endif()

   if(ARG_SHARED AND UNIX)
      if(NOT APPLE AND ARG_SONAME)
         get_target_property(output_name ${name} OUTPUT_NAME)
         if(${output_name} STREQUAL "output_name-NOTFOUND")
            set(output_name ${name})
         endif()
         set(library_name ${output_name}-${POLAR_VERSION_MAJOR}.${POLAR_VERSION_MINOR}${POLAR_VERSION_SUFFIX})
         set(api_name ${output_name}-${POLAR_VERSION_MAJOR}.${POLAR_VERSION_MINOR}.${POLAR_VERSION_PATCH}${POLAR_VERSION_SUFFIX})
         set_target_properties(${name} PROPERTIES OUTPUT_NAME ${library_name})
         polar_install_library_symlink(${api_name} ${library_name} SHARED
            COMPONENT ${name}
            ALWAYS_GENERATE)
         polar_install_library_symlink(${output_name} ${library_name} SHARED
            COMPONENT ${name}
            ALWAYS_GENERATE)
      endif()
   endif()

   # TODO here we need some work on plugin setup
   if(ARG_MODULE AND POLAR_EXPORT_SYMBOLS_FOR_PLUGINS AND ARG_PLUGIN_TOOL AND (WIN32 OR CYGWIN))
      # On DLL platforms symbols are imported from the tool by linking against it.
      set(polar_libs ${ARG_PLUGIN_TOOL})
   elseif (DEFINED POLAR_LINK_COMPONENTS OR DEFINED ARG_LINK_COMPONENTS)
      if (POLAR_LINK_POLAR_DYLIB AND NOT ARG_DISABLE_POLAR_LINK_POLAR_DYLIB)
         set(polar_libs polarVM)
      else()
         polar_map_components_to_libnames(polar_libs
            ${ARG_LINK_COMPONENTS}
            ${POLAR_LINK_COMPONENTS}
            )
      endif()
   else()
      # Components have not been defined explicitly in CMake, so add the
      # dependency information for this library as defined by POLARBuild.
      #
      # It would be nice to verify that we have the dependencies for this library
      # name, but using get_property(... SET) doesn't suffice to determine if a
      # property has been set to an empty value.
      get_property(lib_deps GLOBAL PROPERTY POLARBUILD_LIB_DEPS_${name})
   endif()

   if(ARG_STATIC)
      set(libtype INTERFACE)
   else()
      # We can use PRIVATE since SO knows its dependent libs.
      set(libtype PRIVATE)
   endif()
   target_link_libraries(${name} ${libtype}
      ${ARG_LINK_LIBS}
      ${lib_deps}
      ${polar_libs}
      ${POLAR_THREADS_LIBRARY}
      ${POLAR_RT_REQUIRE_LIBS}
      )

   if(POLAR_COMMON_DEPENDS)
      add_dependencies(${name} ${POLAR_COMMON_DEPENDS})
      # Add dependencies also to objlibs.
      # CMake issue 14747 --  add_dependencies() might be ignored to objlib's user.
      foreach(objlib ${objlibs})
         add_dependencies(${objlib} ${POLAR_COMMON_DEPENDS})
      endforeach()
   endif()

   if(ARG_SHARED OR ARG_MODULE)
      polar_externalize_debuginfo(${name})
   endif()
endfunction(polar_add_library_internal)

function(polar_add_install_targets target)
   cmake_parse_arguments(ARG "" "COMPONENT;PREFIX" "DEPENDS" ${ARGN})
   if(ARG_COMPONENT)
      set(component_option -DCMAKE_INSTALL_COMPONENT="${ARG_COMPONENT}")
   endif()
   if(ARG_PREFIX)
      set(prefix_option -DCMAKE_INSTALL_PREFIX="${ARG_PREFIX}")
   endif()

   add_custom_target(${target}
      DEPENDS ${ARG_DEPENDS}
      COMMAND "${CMAKE_COMMAND}"
      ${component_option}
      ${prefix_option}
      -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
      USES_TERMINAL)
   add_custom_target(${target}-stripped
      DEPENDS ${ARG_DEPENDS}
      COMMAND "${CMAKE_COMMAND}"
      ${component_option}
      ${prefix_option}
      -DCMAKE_INSTALL_DO_STRIP=1
      -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
      USES_TERMINAL)
endfunction(polar_add_install_targets)

macro(polar_add_library name)
   cmake_parse_arguments(ARG
      "SHARED;BUILDTREE_ONLY"
      ""
      ""
      ${ARGN})
   if(ARG_SHARED)
      polar_add_library_internal(${name} SHARED ${ARG_UNPARSED_ARGUMENTS})
   else()
      polar_add_library_internal(${name} STATIC ${ARG_UNPARSED_ARGUMENTS})
   endif()

   # Libraries that are meant to only be exposed via the build tree only are
   # never installed and are only exported as a target in the special build tree
   # config file.
   if (NOT ARG_BUILDTREE_ONLY)
      set_property(GLOBAL APPEND PROPERTY POLAR_LIBS ${name})
   endif()

   if(EXCLUDE_FROM_ALL)
      set_target_properties(${name} PROPERTIES EXCLUDE_FROM_ALL ON)
   elseif(ARG_BUILDTREE_ONLY)
      set_property(GLOBAL APPEND PROPERTY POLAR_EXPORTS_BUILDTREE_ONLY ${name})
   else()
      if (NOT POLAR_INSTALL_TOOLCHAIN_ONLY OR ${name} STREQUAL "LTO" OR
      (POLAR_LINK_POLAR_DYLIB AND ${name} STREQUAL "zendVM"))
         set(install_dir lib${POLAR_LIBDIR_SUFFIX})
         if(ARG_SHARED OR BUILD_SHARED_LIBS)
            if(WIN32 OR CYGWIN OR MINGW)
               set(install_type RUNTIME)
               set(install_dir bin)
            else()
               set(install_type LIBRARY)
            endif()
         else()
            set(install_type ARCHIVE)
         endif()

         if(${name} IN_LIST POLAR_DISTRIBUTION_COMPONENTS OR
            NOT POLAR_DISTRIBUTION_COMPONENTS)
            set(export_to_polarexports EXPORT PolarExports)
            set_property(GLOBAL PROPERTY POLAR_HAS_EXPORTS True)
         endif()
         if (ARG_SHARD)
            install(TARGETS ${name}
               ${export_to_polarexports}
               ${install_type} DESTINATION ${install_dir}
               COMPONENT ${name})
            if (NOT CMAKE_CONFIGURATION_TYPES)
               polar_add_install_targets(install-${name}
                  DEPENDS ${name}
                  COMPONENT ${name})
            endif()
         endif()
      endif()
      set_property(GLOBAL APPEND PROPERTY POLAR_EXPORTS ${name})
   endif()
   set_target_properties(${name} PROPERTIES FOLDER "Libraries")
endmacro(polar_add_library)

macro(polar_add_loadable_module name)
   polar_add_library_internal(${name} MODULE ${ARGN})
   if(NOT TARGET ${name})
      # Add empty "phony" target
      add_custom_target(${name})
   else()
      if(EXCLUDE_FROM_ALL)
         set_target_properties(${name} PROPERTIES EXCLUDE_FROM_ALL ON)
      else()
         if (NOT POLAR_INSTALL_TOOLCHAIN_ONLY)
            if(WIN32 OR CYGWIN)
               # DLL platform
               set(dlldir "bin")
            else()
               set(dlldir "lib${POLAR_LIBDIR_SUFFIX}")
            endif()

            if(${name} IN_LIST POLAR_DISTRIBUTION_COMPONENTS OR
               NOT POLAR_DISTRIBUTION_COMPONENTS)
               set(export_to_polarexports EXPORT PolarExports)
               set_property(GLOBAL PROPERTY POLAR_HAS_EXPORTS True)
            endif()

            install(TARGETS ${name}
               ${export_to_polarexports}
               LIBRARY DESTINATION ${dlldir}
               ARCHIVE DESTINATION lib${POLAR_LIBDIR_SUFFIX})
         endif()
         set_property(GLOBAL APPEND PROPERTY POLAR_EXPORTS ${name})
      endif()
   endif()

   set_target_properties(${name} PROPERTIES FOLDER "Loadable modules")
endmacro(polar_add_loadable_module)

macro(polar_add_executable name)
   cmake_parse_arguments(ARG "DISABLE_POLAR_LINK_POLAR_DYLIB;IGNORE_EXTERNALIZE_DEBUGINFO;NO_INSTALL_RPATH;" "" "LINK_LIBS;DEPENDS" ${ARGN})
   polar_process_sources(ALL_FILES ${ARG_UNPARSED_ARGUMENTS})
   list(APPEND POLAR_COMMON_DEPENDS ${ARG_DEPENDS})
   # Generate objlib
   if(POLAR_ENABLE_OBJLIB)
      # Generate an obj library for both targets.
      set(obj_name "obj.${name}")
      add_library(${obj_name} OBJECT EXCLUDE_FROM_ALL
         ${ALL_FILES}
         )
      polar_update_compile_flags(${obj_name})
      set(ALL_FILES "$<TARGET_OBJECTS:${obj_name}>")
      set_target_properties(${obj_name} PROPERTIES FOLDER "Object Libraries")
   endif()

   if(XCODE)
      # Note: the dummy.cpp source file provides no definitions. However,
      # it forces Xcode to properly link the static library.
      list(APPEND ALL_FILES "${POLAR_MAIN_SRC_DIR}/cmake/dummy.cpp")
   endif()

   if(EXCLUDE_FROM_ALL)
      add_executable(${name} EXCLUDE_FROM_ALL ${ALL_FILES})
   else()
      add_executable(${name} ${ALL_FILES})
   endif()

   if(NOT ARG_NO_INSTALL_RPATH)
      polar_setup_rpath(${name})
   endif()

   # $<TARGET_OBJECTS> doesn't require compile flags.
   if(NOT POLAR_ENABLE_OBJLIB)
      polar_update_compile_flags(${name})
   endif()
   polar_add_link_opts(${name})

   # Do not add -Dname_EXPORTS to the command-line when building files in this
   # target. Doing so is actively harmful for the modules build because it
   # creates extra module variants, and not useful because we don't use these
   # macros.
   set_target_properties(${name} PROPERTIES DEFINE_SYMBOL "")

   if (POLAR_EXPORTED_SYMBOL_FILE)
      polar_add_symbol_exports(${name} ${POLAR_EXPORTED_SYMBOL_FILE})
   endif(POLAR_EXPORTED_SYMBOL_FILE)

   if (POLAR_LINK_POLAR_DYLIB AND NOT ARG_DISABLE_POLAR_LINK_POLAR_DYLIB)
      set(USE_SHARED USE_SHARED)
   endif()

   set(EXCLUDE_FROM_ALL OFF)
   polar_set_output_directory(${name} BINARY_DIR ${POLAR_RUNTIME_OUTPUT_INTDIR} LIBRARY_DIR ${POLAR_LIBRARY_OUTPUT_INTDIR})
   if(POLAR_COMMON_DEPENDS)
      add_dependencies(${name} ${POLAR_COMMON_DEPENDS})
   endif(POLAR_COMMON_DEPENDS)

   if(NOT ARG_IGNORE_EXTERNALIZE_DEBUGINFO)
      polar_externalize_debuginfo(${name})
   endif()

   if (POLAR_THREADS_WORKING)
      # libpthreads overrides some standard library symbols, so main
      # executable must be linked with it in order to provide consistent
      # API for all shared libaries loaded by this executable.
      list(APPEND ARG_LINK_LIBS ${POLAR_THREADS_LIBRARY})
   endif()
   if (ARG_LINK_LIBS)
      target_link_libraries(${name} PRIVATE ${ARG_LINK_LIBS})
   endif()
endmacro(polar_add_executable)

function(polar_install_library_symlink name dest type)
   cmake_parse_arguments(ARG "ALWAYS_GENERATE" "COMPONENT" "" ${ARGN})
   foreach(path ${CMAKE_MODULE_PATH})
      if(EXISTS ${path}/PolarInstallSymlink.cmake)
         set(INSTALL_SYMLINK ${path}/PolarInstallSymlink.cmake)
         break()
      endif()
   endforeach()

   set(component ${ARG_COMPONENT})
   if(NOT component)
      set(component ${name})
   endif()

   set(full_name ${CMAKE_${type}_LIBRARY_PREFIX}${name}${CMAKE_${type}_LIBRARY_SUFFIX})
   set(full_dest ${CMAKE_${type}_LIBRARY_PREFIX}${dest}${CMAKE_${type}_LIBRARY_SUFFIX})

   set(output_dir lib${POLAR_LIBDIR_SUFFIX})
   if(WIN32 AND "${type}" STREQUAL "SHARED")
      set(output_dir bin)
   endif()

   install(SCRIPT ${INSTALL_SYMLINK}
      CODE "install_symlink(${full_name} ${full_dest} ${output_dir})"
      COMPONENT ${component})

   if (NOT CMAKE_CONFIGURATION_TYPES AND NOT ARG_ALWAYS_GENERATE)
      add_polar_install_targets(install-${name}
         DEPENDS ${name} ${dest} install-${dest}
         COMPONENT ${name})
   endif()
endfunction(polar_install_library_symlink)

function(polar_add_tool_symlink link_name target)
   cmake_parse_arguments(ARG "ALWAYS_GENERATE" "OUTPUT_DIR" "" ${ARGN})
   set(dest_binary "$<TARGET_FILE:${target}>")

   # This got a bit gross... For multi-configuration generators the target
   # properties return the resolved value of the string, not the build system
   # expression. To reconstruct the platform-agnostic path we have to do some
   # magic. First we grab one of the types, and a type-specific path. Then from
   # the type-specific path we find the last occurrence of the type in the path,
   # and replace it with CMAKE_CFG_INTDIR. This allows the build step to be type
   # agnostic again.
   if(NOT ARG_OUTPUT_DIR)
      # If you're not overriding the OUTPUT_DIR, we can make the link relative in
      # the same directory.
      if(UNIX)
         set(dest_binary "$<TARGET_FILE_NAME:${target}>")
      endif()
      if(CMAKE_CONFIGURATION_TYPES)
         list(GET CMAKE_CONFIGURATION_TYPES 0 first_type)
         string(TOUPPER ${first_type} first_type_upper)
         set(first_type_suffix _${first_type_upper})
      endif()
      get_target_property(target_type ${target} TYPE)
      if(${target_type} STREQUAL "STATIC_LIBRARY")
         get_target_property(ARG_OUTPUT_DIR ${target} ARCHIVE_OUTPUT_DIRECTORY${first_type_suffix})
      elseif(UNIX AND ${target_type} STREQUAL "SHARED_LIBRARY")
         get_target_property(ARG_OUTPUT_DIR ${target} LIBRARY_OUTPUT_DIRECTORY${first_type_suffix})
      else()
         get_target_property(ARG_OUTPUT_DIR ${target} RUNTIME_OUTPUT_DIRECTORY${first_type_suffix})
      endif()
      if(CMAKE_CONFIGURATION_TYPES)
         string(FIND "${ARG_OUTPUT_DIR}" "/${first_type}/" type_start REVERSE)
         string(SUBSTRING "${ARG_OUTPUT_DIR}" 0 ${type_start} path_prefix)
         string(SUBSTRING "${ARG_OUTPUT_DIR}" ${type_start} -1 path_suffix)
         string(REPLACE "/${first_type}/" "/${CMAKE_CFG_INTDIR}/"
            path_suffix ${path_suffix})
         set(ARG_OUTPUT_DIR ${path_prefix}${path_suffix})
      endif()
   endif()

   if(UNIX)
      set(POLAR_LINK_OR_COPY create_symlink)
   else()
      set(POLAR_LINK_OR_COPY copy)
   endif()

   set(output_path "${ARG_OUTPUT_DIR}/${link_name}${CMAKE_EXECUTABLE_SUFFIX}")

   set(target_name ${link_name})
   if(TARGET ${link_name})
      set(target_name ${link_name}-link)
   endif()

   if(ARG_ALWAYS_GENERATE)
      set_property(DIRECTORY APPEND PROPERTY
         ADDITIONAL_MAKE_CLEAN_FILES ${dest_binary})
      add_custom_command(TARGET ${target} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E ${POLAR_LINK_OR_COPY} "${dest_binary}" "${output_path}")
   else()
      add_custom_command(OUTPUT ${output_path}
         COMMAND ${CMAKE_COMMAND} -E ${POLAR_LINK_OR_COPY} "${dest_binary}" "${output_path}"
         DEPENDS ${target})
      add_custom_target(${target_name} ALL DEPENDS ${target} ${output_path})
      set_target_properties(${target_name} PROPERTIES FOLDER Tools)

      # Make sure both the link and target are toolchain tools
      if (${link_name} IN_LIST POLAR_TOOLCHAIN_TOOLS AND ${target} IN_LIST POLAR_TOOLCHAIN_TOOLS)
         set(TOOL_IS_TOOLCHAIN ON)
      endif()

      if ((TOOL_IS_TOOLCHAIN OR NOT POLAR_INSTALL_TOOLCHAIN_ONLY) AND POLAR_BUILD_TOOLS)
         polar_install_symlink(${link_name} ${target})
      endif()
   endif()
endfunction(polar_add_tool_symlink)

function(polar_externalize_debuginfo name)
   if(NOT POLAR_EXTERNALIZE_DEBUGINFO)
      return()
   endif()

   if(NOT POLAR_EXTERNALIZE_DEBUGINFO_SKIP_STRIP)
      if(APPLE)
         set(strip_command COMMAND xcrun strip -Sxl $<TARGET_FILE:${name}>)
      else()
         set(strip_command COMMAND strip -gx $<TARGET_FILE:${name}>)
      endif()
   endif()

   if(APPLE)
      if(CMAKE_CXX_FLAGS MATCHES "-flto"
         OR CMAKE_CXX_FLAGS_${uppercase_CMAKE_BUILD_TYPE} MATCHES "-flto")

         set(lto_object ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${name}-lto.o)
         set_property(TARGET ${name} APPEND_STRING PROPERTY
            LINK_FLAGS " -Wl,-object_path_lto,${lto_object}")
      endif()
      add_custom_command(TARGET ${name} POST_BUILD
         COMMAND xcrun dsymutil $<TARGET_FILE:${name}>
         ${strip_command}
         )
   else()
      add_custom_command(TARGET ${name} POST_BUILD
         COMMAND objcopy --only-keep-debug $<TARGET_FILE:${name}> $<TARGET_FILE:${name}>.debug
         ${strip_command} -R .gnu_debuglink
         COMMAND objcopy --add-gnu-debuglink=$<TARGET_FILE:${name}>.debug $<TARGET_FILE:${name}>
         )
   endif()
endfunction(polar_externalize_debuginfo)

function(polar_setup_rpath name)
   if(CMAKE_INSTALL_RPATH)
      return()
   endif()

   if(POLAR_INSTALL_PREFIX AND NOT (POLAR_INSTALL_PREFIX STREQUAL CMAKE_INSTALL_PREFIX))
      set(extra_libdir ${POLAR_LIBRARY_DIR})
   elseif(POLAR_BUILD_LIBRARY_DIR)
      set(extra_libdir ${POLAR_BUILD_LIBRARY_DIR})
   endif()

   if (APPLE)
      set(_install_name_dir INSTALL_NAME_DIR "@rpath")
      set(_install_rpath "@loader_path/../lib" ${extra_libdir})
   elseif(UNIX)
      set(_install_rpath "\$ORIGIN/../lib${POLAR_LIBDIR_SUFFIX}" ${extra_libdir})
      if(${CMAKE_SYSTEM_NAME} MATCHES "(FreeBSD|DragonFly)")
         set_property(TARGET ${name} APPEND_STRING PROPERTY
            LINK_FLAGS " -Wl,-z,origin ")
      elseif(LINUX AND NOT POLAR_LINKER_IS_GOLD)
         # $ORIGIN is not interpreted at link time by ld.bfd
         set_property(TARGET ${name} APPEND_STRING PROPERTY
            LINK_FLAGS " -Wl,-rpath-link,${POLAR_LIBRARY_OUTPUT_INTDIR}")
      endif()
   else()
      return()
   endif()
   if (LINUX)
      set(_install_rpath "${POLAR_COMPILER_ROOT_DIR}/lib64;${POLAR_COMPILER_ROOT_DIR}/lib;${_install_rpath}")
   endif()
   set_target_properties(${name} PROPERTIES
      BUILD_WITH_INSTALL_RPATH ON
      INSTALL_RPATH "${_install_rpath}"
      ${_install_name_dir})
endfunction(polar_setup_rpath)

# new target functions

# POLARLIB_DIR is the directory in the build tree where polar resource files
# should be placed.  Note that $CMAKE_CFG_INTDIR expands to "." for
# single-configuration builds.
set(POLARLIB_DIR
   "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib/polar")
set(POLARSTATICLIB_DIR
   "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib/polar_static")

function(add_dependencies_multiple_targets)
   cmake_parse_arguments(
      ADMT # prefix
      "" # options
      "" # single-value args
      "TARGETS;DEPENDS" # multi-value args
      ${ARGN})
   polar_precondition(ADMT_UNPARSED_ARGUMENTS NEGATE MESSAGE "unrecognized arguments: ${ADMT_UNPARSED_ARGUMENTS}")

   if(NOT "${ADMT_DEPENDS}" STREQUAL "")
      foreach(target ${ADMT_TARGETS})
         add_dependencies("${target}" ${ADMT_DEPENDS})
      endforeach()
   endif()
endfunction()

# Compute the library subdirectory to use for the given sdk and
# architecture, placing the result in 'result_var_name'.
function(compute_library_subdir result_var_name sdk arch)
   if(sdk IN_LIST POLAR_APPLE_PLATFORMS)
      set("${result_var_name}" "${POLAR_SDK_${sdk}_LIB_SUBDIR}" PARENT_SCOPE)
   else()
      set("${result_var_name}" "${POLAR_SDK_${sdk}_LIB_SUBDIR}/${arch}" PARENT_SCOPE)
   endif()
endfunction()

function(_compute_lto_flag option out_var)
   string(TOLOWER "${option}" lowercase_option)
   if (lowercase_option STREQUAL "full")
      set(${out_var} "-flto=full" PARENT_SCOPE)
   elseif (lowercase_option STREQUAL "thin")
      set(${out_var} "-flto=thin" PARENT_SCOPE)
   endif()
endfunction()

function(_set_target_prefix_and_suffix target kind sdk)
   precondition(target MESSAGE "target is required")
   precondition(kind MESSAGE "kind is required")
   precondition(sdk MESSAGE "sdk is required")

   if(${sdk} STREQUAL ANDROID)
      if(${kind} STREQUAL STATIC)
         set_target_properties(${target} PROPERTIES PREFIX "lib" SUFFIX ".a")
      elseif(${kind} STREQUAL SHARED)
         set_target_properties(${target} PROPERTIES PREFIX "lib" SUFFIX ".so")
      endif()
   elseif(${sdk} STREQUAL WINDOWS)
      if(${kind} STREQUAL STATIC)
         set_target_properties(${target} PROPERTIES PREFIX "" SUFFIX ".lib")
      elseif(${kind} STREQUAL SHARED)
         set_target_properties(${target} PROPERTIES PREFIX "" SUFFIX ".dll")
      endif()
   endif()
endfunction()

function(is_darwin_based_sdk sdk_name out_var)
   if ("${sdk_name}" STREQUAL "OSX" OR
      "${sdk_name}" STREQUAL "IOS" OR
      "${sdk_name}" STREQUAL "IOS_SIMULATOR" OR
      "${sdk_name}" STREQUAL "TVOS" OR
      "${sdk_name}" STREQUAL "TVOS_SIMULATOR" OR
      "${sdk_name}" STREQUAL "WATCHOS" OR
      "${sdk_name}" STREQUAL "WATCHOS_SIMULATOR")
      set(${out_var} TRUE PARENT_SCOPE)
   else()
      set(${out_var} FALSE PARENT_SCOPE)
   endif()
endfunction()

# Usage:
# _add_variant_c_compile_link_flags(
#   SDK sdk
#   ARCH arch
#   BUILD_TYPE build_type
#   ENABLE_LTO enable_lto
#   ANALYZE_CODE_COVERAGE analyze_code_coverage
#   RESULT_VAR_NAME result_var_name
#   DEPLOYMENT_VERSION_OSX version # If provided, overrides the default value of the OSX deployment target set by the Swift project for this compilation only.
#   DEPLOYMENT_VERSION_IOS version
#   DEPLOYMENT_VERSION_TVOS version
#   DEPLOYMENT_VERSION_WATCHOS version
#
# )
function(_add_variant_c_compile_link_flags)
   set(oneValueArgs SDK ARCH BUILD_TYPE RESULT_VAR_NAME ENABLE_LTO ANALYZE_CODE_COVERAGE
      DEPLOYMENT_VERSION_OSX DEPLOYMENT_VERSION_IOS DEPLOYMENT_VERSION_TVOS DEPLOYMENT_VERSION_WATCHOS)
   cmake_parse_arguments(CFLAGS
      ""
      "${oneValueArgs}"
      ""
      ${ARGN})

   set(result ${${CFLAGS_RESULT_VAR_NAME}})

   is_darwin_based_sdk("${CFLAGS_SDK}" IS_DARWIN)
   if(IS_DARWIN)
      # Check if there's a specific OS deployment version needed for this invocation
      if("${CFLAGS_SDK}" STREQUAL "OSX")
         set(DEPLOYMENT_VERSION ${CFLAGS_DEPLOYMENT_VERSION_OSX})
      elseif("${CFLAGS_SDK}" STREQUAL "IOS" OR "${CFLAGS_SDK}" STREQUAL "IOS_SIMULATOR")
         set(DEPLOYMENT_VERSION ${CFLAGS_DEPLOYMENT_VERSION_IOS})
      elseif("${CFLAGS_SDK}" STREQUAL "TVOS" OR "${CFLAGS_SDK}" STREQUAL "TVOS_SIMULATOR")
         set(DEPLOYMENT_VERSION ${CFLAGS_DEPLOYMENT_VERSION_TVOS})
      elseif("${CFLAGS_SDK}" STREQUAL "WATCHOS" OR "${CFLAGS_SDK}" STREQUAL "WATCHOS_SIMULATOR")
         set(DEPLOYMENT_VERSION ${CFLAGS_DEPLOYMENT_VERSION_WATCHOS})
      endif()

      if("${DEPLOYMENT_VERSION}" STREQUAL "")
         set(DEPLOYMENT_VERSION "${POLAR_SDK_${CFLAGS_SDK}_DEPLOYMENT_VERSION}")
      endif()
   endif()

   # MSVC, clang-cl, gcc don't understand -target.
   if(CMAKE_C_COMPILER_ID MATCHES "^Clang|AppleClang$" AND
      NOT POLAR_COMPILER_IS_MSVC_LIKE)
      list(APPEND result "-target" "${POLAR_SDK_${CFLAGS_SDK}_ARCH_${CFLAGS_ARCH}_TRIPLE}${DEPLOYMENT_VERSION}")
   endif()

   set(_sysroot "${POLAR_SDK_${CFLAGS_SDK}_ARCH_${CFLAGS_ARCH}_PATH}")
   if(IS_DARWIN)
      list(APPEND result "-isysroot" "${_sysroot}")
   elseif(NOT POLAR_COMPILER_IS_MSVC_LIKE AND NOT "${_sysroot}" STREQUAL "/")
      list(APPEND result "--sysroot=${_sysroot}")
   endif()

   if("${CFLAGS_SDK}" STREQUAL "ANDROID")
      # lld can handle targeting the android build.  However, if lld is not
      # enabled, then fallback to the linker included in the android NDK.
      if(NOT POLAR_ENABLE_LLD_LINKER)
         polar_android_tools_path(${CFLAGS_ARCH} tools_path)
         list(APPEND result "-B" "${tools_path}")
      endif()
   endif()
   #  TODO
   #   if(IS_DARWIN)
   #      list(APPEND result
   #         "-arch" "${CFLAGS_ARCH}"
   #         "-F" "${POLAR_SDK_${CFLAGS_SDK}_PATH}/../../../Developer/Library/Frameworks"
   #         "-m${POLAR_SDK_${CFLAGS_SDK}_VERSION_MIN_NAME}-version-min=${DEPLOYMENT_VERSION}")
   #   endif()

   if(CFLAGS_ANALYZE_CODE_COVERAGE)
      list(APPEND result "-fprofile-instr-generate"
         "-fcoverage-mapping")
   endif()

   _compute_lto_flag("${CFLAGS_ENABLE_LTO}" _lto_flag_out)
   if (_lto_flag_out)
      list(APPEND result "${_lto_flag_out}")
   endif()

   set("${CFLAGS_RESULT_VAR_NAME}" "${result}" PARENT_SCOPE)
endfunction()

function(_add_variant_c_compile_flags)
   set(oneValueArgs SDK ARCH BUILD_TYPE ENABLE_ASSERTIONS ANALYZE_CODE_COVERAGE
      DEPLOYMENT_VERSION_OSX DEPLOYMENT_VERSION_IOS DEPLOYMENT_VERSION_TVOS DEPLOYMENT_VERSION_WATCHOS
      RESULT_VAR_NAME ENABLE_LTO)
   cmake_parse_arguments(CFLAGS
      "FORCE_BUILD_OPTIMIZED"
      "${oneValueArgs}"
      ""
      ${ARGN})

   set(result ${${CFLAGS_RESULT_VAR_NAME}})

   _add_variant_c_compile_link_flags(
      SDK "${CFLAGS_SDK}"
      ARCH "${CFLAGS_ARCH}"
      BUILD_TYPE "${CFLAGS_BUILD_TYPE}"
      ENABLE_ASSERTIONS "${CFLAGS_ENABLE_ASSERTIONS}"
      ENABLE_LTO "${CFLAGS_ENABLE_LTO}"
      ANALYZE_CODE_COVERAGE FALSE
      DEPLOYMENT_VERSION_OSX "${CFLAGS_DEPLOYMENT_VERSION_OSX}"
      DEPLOYMENT_VERSION_IOS "${CFLAGS_DEPLOYMENT_VERSION_IOS}"
      DEPLOYMENT_VERSION_TVOS "${CFLAGS_DEPLOYMENT_VERSION_TVOS}"
      DEPLOYMENT_VERSION_WATCHOS "${CFLAGS_DEPLOYMENT_VERSION_WATCHOS}"
      RESULT_VAR_NAME result)

   polar_is_build_type_optimized("${CFLAGS_BUILD_TYPE}" optimized)
   if(optimized OR CFLAGS_FORCE_BUILD_OPTIMIZED)
      list(APPEND result "-O2")

      # Omit leaf frame pointers on x86 production builds (optimized, no debug
      # info, and no asserts).
      is_build_type_with_debuginfo("${CFLAGS_BUILD_TYPE}" debug)
      if(NOT debug AND NOT CFLAGS_ENABLE_ASSERTIONS)
         if("${CFLAGS_ARCH}" STREQUAL "i386" OR "${CFLAGS_ARCH}" STREQUAL "i686")
            if(NOT POLAR_COMPILER_IS_MSVC_LIKE)
               list(APPEND result "-momit-leaf-frame-pointer")
            else()
               list(APPEND result "/Oy")
            endif()
         endif()
      endif()
   else()
      if(NOT POLAR_COMPILER_IS_MSVC_LIKE)
         list(APPEND result "-O0")
      else()
         list(APPEND result "/Od")
      endif()
   endif()

   # CMake automatically adds the flags for debug info if we use MSVC/clang-cl.
   if(NOT POLAR_COMPILER_IS_MSVC_LIKE)
      polar_is_build_type_with_debuginfo("${CFLAGS_BUILD_TYPE}" debuginfo)
      if(debuginfo)
         _compute_lto_flag("${CFLAGS_ENABLE_LTO}" _lto_flag_out)
         if(_lto_flag_out)
            list(APPEND result "-gline-tables-only")
         else()
            list(APPEND result "-g")
         endif()
      else()
         list(APPEND result "-g0")
      endif()
   endif()

   if("${CFLAGS_SDK}" STREQUAL "WINDOWS")
      # MSVC doesn't support -Xclang. We don't need to manually specify
      # the dependent libraries as `cl` does so.
      if(NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
         list(APPEND result -Xclang;--dependent-lib=oldnames)
         # TODO(compnerd) handle /MT, /MTd
         if("${CFLAGS_BUILD_TYPE}" STREQUAL "Debug")
            list(APPEND result -Xclang;--dependent-lib=msvcrtd)
         else()
            list(APPEND result -Xclang;--dependent-lib=msvcrt)
         endif()
      endif()

      # MSVC/clang-cl don't support -fno-pic or -fms-compatibility-version.
      if(NOT POLAR_COMPILER_IS_MSVC_LIKE)
         list(APPEND result -fno-pic)
         list(APPEND result "-fms-compatibility-version=1900")
      endif()

      list(APPEND result "-DLLVM_ON_WIN32")
      list(APPEND result "-D_CRT_SECURE_NO_WARNINGS")
      list(APPEND result "-D_CRT_NONSTDC_NO_WARNINGS")
      if(NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
         list(APPEND result "-D_CRT_USE_BUILTIN_OFFSETOF")
      endif()
      # TODO(compnerd) permit building for different families
      list(APPEND result "-D_CRT_USE_WINAPI_FAMILY_DESKTOP_APP")
      if("${CFLAGS_ARCH}" MATCHES arm)
         list(APPEND result "-D_ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE")
      endif()
      list(APPEND result "-D_MT")
      # TODO(compnerd) handle /MT
      list(APPEND result "-D_DLL")
      # NOTE: We assume that we are using VS 2015 U2+
      list(APPEND result "-D_ENABLE_ATOMIC_ALIGNMENT_FIX")

      # msvcprt's std::function requires RTTI, but we do not want RTTI data.
      # Emulate /GR-.
      # TODO(compnerd) when moving up to VS 2017 15.3 and newer, we can disable
      # RTTI again
      if(POLAR_COMPILER_IS_MSVC_LIKE)
         list(APPEND result /GR-)
      else()
         list(APPEND result -frtti)
         list(APPEND result -Xclang;-fno-rtti-data)
      endif()

      # NOTE: VS 2017 15.3 introduced this to disable the static components of
      # RTTI as well.  This requires a newer SDK though and we do not have
      # guarantees on the SDK version currently.
      list(APPEND result "-D_HAS_STATIC_RTTI=0")

      # NOTE(compnerd) workaround LLVM invoking `add_definitions(-D_DEBUG)` which
      # causes failures for the runtime library when cross-compiling due to
      # undefined symbols from the standard library.
      if(NOT CMAKE_BUILD_TYPE STREQUAL Debug)
         list(APPEND result "-U_DEBUG")
      endif()
   endif()

   if(${CFLAGS_SDK} STREQUAL ANDROID)
      if(${CFLAGS_ARCH} STREQUAL x86_64)
         # NOTE(compnerd) Android NDK 21 or lower will generate library calls to
         # `__sync_val_compare_and_swap_16` rather than lowering to the CPU's
         # `cmpxchg16b` instruction as the `cx16` feature is disabled due to a bug
         # in Clang.  This is being fixed in the current master Clang and will
         # hopefully make it into Clang 9.0.  In the mean time, workaround this in
         # the build.
         if(CMAKE_C_COMPILER_ID MATCHES Clang AND CMAKE_C_COMPILER_VERSION
            VERSION_LESS 9.0.0)
            list(APPEND result -mcx16)
         endif()
      endif()
   endif()

   if(CFLAGS_ENABLE_ASSERTIONS)
      list(APPEND result "-UNDEBUG")
   else()
      list(APPEND result "-DNDEBUG")
   endif()

   if(POLAR_ENABLE_RUNTIME_FUNCTION_COUNTERS)
      list(APPEND result "-DPOLAR_ENABLE_RUNTIME_FUNCTION_COUNTERS")
   endif()

   if(CFLAGS_ANALYZE_CODE_COVERAGE)
      list(APPEND result "-fprofile-instr-generate"
         "-fcoverage-mapping")
   endif()

   if((CFLAGS_ARCH STREQUAL "armv7" OR CFLAGS_ARCH STREQUAL "aarch64") AND
   (CFLAGS_SDK STREQUAL "LINUX" OR CFLAGS_SDK STREQUAL "ANDROID"))
      list(APPEND result -funwind-tables)
   endif()

   if("${CFLAGS_SDK}" STREQUAL "ANDROID")
      list(APPEND result -nostdinc++)
      polar_android_libcxx_include_paths(CFLAGS_CXX_INCLUDES)
      polar_android_include_for_arch("${CFLAGS_ARCH}" "${CFLAGS_ARCH}_INCLUDE")
      foreach(path IN LISTS CFLAGS_CXX_INCLUDES ${CFLAGS_ARCH}_INCLUDE)
         list(APPEND result -isystem;${path})
      endforeach()
      list(APPEND result "-D__ANDROID_API__=${POLAR_ANDROID_API_LEVEL}")
   elseif(CFLAGS_SDK STREQUAL WINDOWS)
      polar_windows_include_for_arch(${CFLAGS_ARCH} ${CFLAGS_ARCH}_INCLUDE)
      foreach(path ${${CFLAGS_ARCH}_INCLUDE})
         list(APPEND result "\"${CMAKE_INCLUDE_FLAG_C}${path}\"")
      endforeach()
   endif()

   set(ICU_UC_INCLUDE_DIR ${POLAR_${CFLAGS_SDK}_${CFLAGS_ARCH}_ICU_UC_INCLUDE})
   if(NOT "${ICU_UC_INCLUDE_DIR}" STREQUAL "" AND
      NOT "${ICU_UC_INCLUDE_DIR}" STREQUAL "/usr/include" AND
      NOT "${ICU_UC_INCLUDE_DIR}" STREQUAL "/usr/${POLAR_SDK_${CFLAGS_SDK}_ARCH_${CFLAGS_ARCH}_TRIPLE}/include" AND
      NOT "${ICU_UC_INCLUDE_DIR}" STREQUAL "/usr/${POLAR_SDK_${POLAR_HOST_VARIANT_SDK}_ARCH_${POLAR_HOST_VARIANT_ARCH}_TRIPLE}/include")
      if(POLAR_COMPILER_IS_MSVC_LIKE)
         list(APPEND result -I;${ICU_UC_INCLUDE_DIR})
      else()
         list(APPEND result -isystem;${ICU_UC_INCLUDE_DIR})
      endif()
   endif()

   set(ICU_I18N_INCLUDE_DIR ${POLAR_${CFLAGS_SDK}_${CFLAGS_ARCH}_ICU_I18N_INCLUDE})
   if(NOT "${ICU_I18N_INCLUDE_DIR}" STREQUAL "" AND
      NOT "${ICU_I18N_INCLUDE_DIR}" STREQUAL "/usr/include" AND
      NOT "${ICU_I18N_INCLUDE_DIR}" STREQUAL "/usr/${POLAR_SDK_${CFLAGS_SDK}_ARCH_${CFLAGS_ARCH}_TRIPLE}/include" AND
      NOT "${ICU_I18N_INCLUDE_DIR}" STREQUAL "/usr/${POLAR_SDK_${POLAR_HOST_VARIANT_SDK}_ARCH_${POLAR_HOST_VARIANT_ARCH}_TRIPLE}/include")
      if(POLAR_COMPILER_IS_MSVC_LIKE)
         list(APPEND result -I;${ICU_I18N_INCLUDE_DIR})
      else()
         list(APPEND result -isystem;${ICU_I18N_INCLUDE_DIR})
      endif()
   endif()

   set("${CFLAGS_RESULT_VAR_NAME}" "${result}" PARENT_SCOPE)
endfunction()

function(_set_target_prefix_and_suffix target kind sdk)
   polar_precondition(target MESSAGE "target is required")
   polar_precondition(kind MESSAGE "kind is required")
   polar_precondition(sdk MESSAGE "sdk is required")

   if(${sdk} STREQUAL ANDROID)
      if(${kind} STREQUAL STATIC)
         set_target_properties(${target} PROPERTIES PREFIX "lib" SUFFIX ".a")
      elseif(${kind} STREQUAL SHARED)
         set_target_properties(${target} PROPERTIES PREFIX "lib" SUFFIX ".so")
      endif()
   elseif(${sdk} STREQUAL WINDOWS)
      if(${kind} STREQUAL STATIC)
         set_target_properties(${target} PROPERTIES PREFIX "" SUFFIX ".lib")
      elseif(${kind} STREQUAL SHARED)
         set_target_properties(${target} PROPERTIES PREFIX "" SUFFIX ".dll")
      endif()
   endif()
endfunction()

function(_add_variant_link_flags)
   set(oneValueArgs SDK ARCH BUILD_TYPE ENABLE_ASSERTIONS ANALYZE_CODE_COVERAGE
      DEPLOYMENT_VERSION_OSX DEPLOYMENT_VERSION_IOS DEPLOYMENT_VERSION_TVOS DEPLOYMENT_VERSION_WATCHOS
      RESULT_VAR_NAME ENABLE_LTO LTO_OBJECT_NAME LINK_LIBRARIES_VAR_NAME LIBRARY_SEARCH_DIRECTORIES_VAR_NAME)
   cmake_parse_arguments(LFLAGS
      ""
      "${oneValueArgs}"
      ""
      ${ARGN})

   polar_precondition(LFLAGS_SDK MESSAGE "Should specify an SDK")
   polar_precondition(LFLAGS_ARCH MESSAGE "Should specify an architecture")

   set(result ${${LFLAGS_RESULT_VAR_NAME}})
   set(link_libraries ${${LFLAGS_LINK_LIBRARIES_VAR_NAME}})
   set(library_search_directories ${${LFLAGS_LIBRARY_SEARCH_DIRECTORIES_VAR_NAME}})

   _add_variant_c_compile_link_flags(
      SDK "${LFLAGS_SDK}"
      ARCH "${LFLAGS_ARCH}"
      BUILD_TYPE "${LFLAGS_BUILD_TYPE}"
      ENABLE_ASSERTIONS "${LFLAGS_ENABLE_ASSERTIONS}"
      ENABLE_LTO "${LFLAGS_ENABLE_LTO}"
      ANALYZE_CODE_COVERAGE "${LFLAGS_ANALYZE_CODE_COVERAGE}"
      DEPLOYMENT_VERSION_OSX "${LFLAGS_DEPLOYMENT_VERSION_OSX}"
      DEPLOYMENT_VERSION_IOS "${LFLAGS_DEPLOYMENT_VERSION_IOS}"
      DEPLOYMENT_VERSION_TVOS "${LFLAGS_DEPLOYMENT_VERSION_TVOS}"
      DEPLOYMENT_VERSION_WATCHOS "${LFLAGS_DEPLOYMENT_VERSION_WATCHOS}"
      RESULT_VAR_NAME result)

   if("${LFLAGS_SDK}" STREQUAL "LINUX")
      list(APPEND link_libraries "pthread" "atomic" "dl")
   elseif("${LFLAGS_SDK}" STREQUAL "FREEBSD")
      list(APPEND link_libraries "pthread")
   elseif("${LFLAGS_SDK}" STREQUAL "CYGWIN")
      # No extra libraries required.
   elseif("${LFLAGS_SDK}" STREQUAL "WINDOWS")
      # We don't need to add -nostdlib using MSVC or clang-cl, as MSVC and clang-cl rely on auto-linking entirely.
      if(NOT POLAR_COMPILER_IS_MSVC_LIKE)
         # NOTE: we do not use "/MD" or "/MDd" and select the runtime via linker
         # options. This causes conflicts.
         list(APPEND result "-nostdlib")
      endif()
      polar_windows_lib_for_arch(${LFLAGS_ARCH} ${LFLAGS_ARCH}_LIB)
      list(APPEND library_search_directories ${${LFLAGS_ARCH}_LIB})

      # NOTE(compnerd) workaround incorrectly extensioned import libraries from
      # the Windows SDK on case sensitive file systems.
      list(APPEND library_search_directories
         ${CMAKE_BINARY_DIR}/winsdk_lib_${LFLAGS_ARCH}_symlinks)
   elseif("${LFLAGS_SDK}" STREQUAL "HAIKU")
      list(APPEND link_libraries "bsd" "atomic")
      list(APPEND result "-Wl,-Bsymbolic")
   elseif("${LFLAGS_SDK}" STREQUAL "ANDROID")
      list(APPEND link_libraries "dl" "log" "atomic")
      # We need to add the math library, which is linked implicitly by libc++
      list(APPEND result "-lm")

      # link against the custom C++ library
      polar_android_cxx_libraries_for_arch(${LFLAGS_ARCH} cxx_link_libraries)
      list(APPEND link_libraries ${cxx_link_libraries})

      # link against the ICU libraries
      list(APPEND link_libraries
         ${POLAR_ANDROID_${LFLAGS_ARCH}_ICU_I18N}
         ${POLAR_ANDROID_${LFLAGS_ARCH}_ICU_UC})

      polar_android_lib_for_arch(${LFLAGS_ARCH} ${LFLAGS_ARCH}_LIB)
      foreach(path IN LISTS ${LFLAGS_ARCH}_LIB)
         list(APPEND library_search_directories ${path})
      endforeach()
   else()
      # If lto is enabled, we need to add the object path flag so that the LTO code
      # generator leaves the intermediate object file in a place where it will not
      # be touched. The reason why this must be done is that on OS X, debug info is
      # left in object files. So if the object file is removed when we go to
      # generate a dsym, the debug info is gone.
      if (LFLAGS_ENABLE_LTO)
         precondition(LFLAGS_LTO_OBJECT_NAME
            MESSAGE "Should specify a unique name for the lto object")
         set(lto_object_dir ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
         set(lto_object ${lto_object_dir}/${LFLAGS_LTO_OBJECT_NAME}-lto.o)
         list(APPEND result "-Wl,-object_path_lto,${lto_object}")
      endif()
   endif()

   if(NOT "${POLAR_${LFLAGS_SDK}_${LFLAGS_ARCH}_ICU_UC}" STREQUAL "")
      get_filename_component(POLAR_${LFLAGS_SDK}_${LFLAGS_ARCH}_ICU_UC_LIBDIR
         "${POLAR_${LFLAGS_SDK}_${LFLAGS_ARCH}_ICU_UC}" DIRECTORY)
      list(APPEND library_search_directories "${POLAR_${LFLAGS_SDK}_${LFLAGS_ARCH}_ICU_UC_LIBDIR}")
   endif()
   if(NOT "${POLAR_${LFLAGS_SDK}_${LFLAGS_ARCH}_ICU_I18N}" STREQUAL "")
      get_filename_component(POLAR_${LFLAGS_SDK}_${LFLAGS_ARCH}_ICU_I18N_LIBDIR
         "${POLAR_${LFLAGS_SDK}_${LFLAGS_ARCH}_ICU_I18N}" DIRECTORY)
      list(APPEND library_search_directories "${POLAR_${LFLAGS_SDK}_${LFLAGS_ARCH}_ICU_I18N_LIBDIR}")
   endif()

   if(NOT POLAR_COMPILER_IS_MSVC_LIKE)
      # FIXME: On Apple platforms, find_program needs to look for "ld64.lld"
      find_program(LDLLD_PATH "ld.lld")
      if((POLAR_ENABLE_LLD_LINKER AND LDLLD_PATH AND NOT APPLE) OR
      ("${LFLAGS_SDK}" STREQUAL "WINDOWS" AND
         NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "WINDOWS"))
         list(APPEND result "-fuse-ld=lld")
      elseif(POLAR_ENABLE_GOLD_LINKER AND
         "${POLAR_SDK_${LFLAGS_SDK}_OBJECT_FORMAT}" STREQUAL "ELF")
         if(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
            list(APPEND result "-fuse-ld=gold.exe")
         else()
            list(APPEND result "-fuse-ld=gold")
         endif()
      endif()
   endif()

   # Enable dead stripping. Portions of this logic were copied from llvm's
   # `add_link_opts` function (which, perhaps, should have been used here in the
   # first place, but at this point it's hard to say whether that's feasible).
   #
   # TODO: Evaluate/enable -f{function,data}-sections --gc-sections for bfd,
   # gold, and lld.
   if(NOT CMAKE_BUILD_TYPE STREQUAL Debug)
      if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
         # See rdar://48283130: This gives 6MB+ size reductions for polar and
         # SourceKitService, and much larger size reductions for sil-opt etc.
         list(APPEND result "-Wl,-dead_strip")
      endif()
   endif()

   set("${LFLAGS_RESULT_VAR_NAME}" "${result}" PARENT_SCOPE)
   set("${LFLAGS_LINK_LIBRARIES_VAR_NAME}" "${link_libraries}" PARENT_SCOPE)
   set("${LFLAGS_LIBRARY_SEARCH_DIRECTORIES_VAR_NAME}" "${library_search_directories}" PARENT_SCOPE)
endfunction()

# function for typephp
function(_add_variant_typephp_compile_flags
   sdk arch build_type enable_assertions result_var_name)
   set(result ${${result_var_name}})

   # On Windows, we don't set POLAR_SDK_WINDOWS_PATH_ARCH_{ARCH}_PATH, so don't include it.
   # On Android the sdk is split to two different paths for includes and libs, so these
   # need to be set manually.
   if (NOT "${sdk}" STREQUAL "WINDOWS" AND NOT "${sdk}" STREQUAL "ANDROID")
      list(APPEND result "-sdk" "${POLAR_SDK_${sdk}_ARCH_${arch}_PATH}")
   endif()

   is_darwin_based_sdk("${sdk}" IS_DARWIN)
   if(IS_DARWIN)
      list(APPEND result
         "-target" "${POLAR_SDK_${sdk}_ARCH_${arch}_TRIPLE}${POLAR_SDK_${sdk}_DEPLOYMENT_VERSION}")
   else()
      list(APPEND result
         "-target" "${POLAR_SDK_${sdk}_ARCH_${arch}_TRIPLE}")
   endif()

   if("${sdk}" STREQUAL "ANDROID")
      polar_android_include_for_arch(${arch} ${arch}_polar_include)
      foreach(path IN LISTS ${arch}_polar_include)
         list(APPEND result "\"${CMAKE_INCLUDE_FLAG_C}${path}\"")
      endforeach()
   endif()

   if(NOT BUILD_STANDALONE)
      list(APPEND result "-resource-dir" "${POLARLIB_DIR}")
   endif()

   if(IS_DARWIN)
      list(APPEND result
         "-F" "${POLAR_SDK_${sdk}_ARCH_${arch}_PATH}/../../../Developer/Library/Frameworks")
   endif()

   is_build_type_optimized("${build_type}" optimized)
   if(optimized)
      list(APPEND result "-O")
   else()
      list(APPEND result "-Onone")
   endif()

   is_build_type_with_debuginfo("${build_type}" debuginfo)
   if(debuginfo)
      list(APPEND result "-g")
   endif()

   if(enable_assertions)
      list(APPEND result "-D" "INTERNAL_CHECKS_ENABLED")
   endif()

   if(POLAR_ENABLE_RUNTIME_FUNCTION_COUNTERS)
      list(APPEND result "-D" "POLAR_ENABLE_RUNTIME_FUNCTION_COUNTERS")
   endif()

   if(POLAR_ENABLE_EXPERIMENTAL_DIFFERENTIABLE_PROGRAMMING)
      list(APPEND result "-D" "POLAR_ENABLE_EXPERIMENTAL_DIFFERENTIABLE_PROGRAMMING")
   endif()

   set("${result_var_name}" "${result}" PARENT_SCOPE)
endfunction()

# function for typephp
# Look up extra flags for a module that matches a regexp.
function(_add_extra_typephp_flags_for_module module_name result_var_name)
   set(result_list)
   list(LENGTH POLAR_EXPERIMENTAL_EXTRA_REGEXP_FLAGS listlen)
   if (${listlen} GREATER 0)
      math(EXPR listlen "${listlen}-1")
      foreach(i RANGE 0 ${listlen} 2)
         list(GET POLAR_EXPERIMENTAL_EXTRA_REGEXP_FLAGS ${i} regex)
         if (module_name MATCHES "${regex}")
            math(EXPR ip1 "${i}+1")
            list(GET POLAR_EXPERIMENTAL_EXTRA_REGEXP_FLAGS ${ip1} flags)
            list(APPEND result_list ${flags})
            message(STATUS "Matched '${regex}' to module '${module_name}'. Compiling ${module_name} with special flags: ${flags}")
         endif()
      endforeach()
   endif()
   list(LENGTH POLAR_EXPERIMENTAL_EXTRA_NEGATIVE_REGEXP_FLAGS listlen)
   if (${listlen} GREATER 0)
      math(EXPR listlen "${listlen}-1")
      foreach(i RANGE 0 ${listlen} 2)
         list(GET POLAR_EXPERIMENTAL_EXTRA_NEGATIVE_REGEXP_FLAGS ${i} regex)
         if (NOT module_name MATCHES "${regex}")
            math(EXPR ip1 "${i}+1")
            list(GET POLAR_EXPERIMENTAL_EXTRA_NEGATIVE_REGEXP_FLAGS ${ip1} flags)
            list(APPEND result_list ${flags})
            message(STATUS "Matched NEGATIVE '${regex}' to module '${module_name}'. Compiling ${module_name} with special flags: ${flags}")
         endif()
      endforeach()
   endif()
   set("${result_var_name}" ${result_list} PARENT_SCOPE)
endfunction()

# function for typephp
# Add a universal binary target created from the output of the given
# set of targets by running 'lipo'.
#
# Usage:
#   _add_typephp_lipo_target(
#     sdk                 # The name of the SDK the target was created for.
#                         # Examples include "OSX", "IOS", "ANDROID", etc.
#     target              # The name of the target to create
#     output              # The file to be created by this target
#     source_targets...   # The source targets whose outputs will be
#                         # lipo'd into the output.
#   )
function(_add_typephp_lipo_target)
   cmake_parse_arguments(
      LIPO                # prefix
      "CODESIGN"          # options
      "SDK;TARGET;OUTPUT" # single-value args
      ""                  # multi-value args
      ${ARGN})

   precondition(LIPO_SDK MESSAGE "sdk is required")
   precondition(LIPO_TARGET MESSAGE "target is required")
   precondition(LIPO_OUTPUT MESSAGE "output is required")
   precondition(LIPO_UNPARSED_ARGUMENTS MESSAGE "one or more inputs are required")

   set(source_targets ${LIPO_UNPARSED_ARGUMENTS})

   # Gather the source binaries.
   set(source_binaries)
   foreach(source_target ${source_targets})
      list(APPEND source_binaries $<TARGET_FILE:${source_target}>)
   endforeach()

   if(${LIPO_SDK} IN_LIST POLAR_APPLE_PLATFORMS)
      if(LIPO_CODESIGN)
         set(codesign_command COMMAND "codesign" "-f" "-s" "-" "${LIPO_OUTPUT}")
      endif()
      # Use lipo to create the final binary.
      add_custom_command_target(unused_var
         COMMAND "${POLAR_LIPO}" "-create" "-output" "${LIPO_OUTPUT}" ${source_binaries}
         ${codesign_command}
         CUSTOM_TARGET_NAME "${LIPO_TARGET}"
         OUTPUT "${LIPO_OUTPUT}"
         DEPENDS ${source_targets})
   else()
      # We don't know how to create fat binaries for other platforms.
      add_custom_command_target(unused_var
         COMMAND "${CMAKE_COMMAND}" "-E" "copy" "${source_binaries}" "${LIPO_OUTPUT}"
         CUSTOM_TARGET_NAME "${LIPO_TARGET}"
         OUTPUT "${LIPO_OUTPUT}"
         DEPENDS ${source_targets})
   endif()
endfunction()

function(polar_target_link_search_directories target directories)
   set(STLD_FLAGS "")
   foreach(directory ${directories})
      set(STLD_FLAGS "${STLD_FLAGS} \"${CMAKE_LIBRARY_PATH_FLAG}${directory}\"")
   endforeach()
   set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS ${STLD_FLAGS})
endfunction()

# Add a single variant of a new polar host or typephp library.
#
# Usage:
#   _polar_add_library_single(
#     target
#     name
#     [MODULE_TARGET]
#     [SHARED]
#     [STATIC]
#     [SDK sdk]
#     [ARCHITECTURE architecture]
#     [DEPENDS dep1 ...]
#     [LINK_LIBRARIES dep1 ...]
#     [FRAMEWORK_DEPENDS dep1 ...]
#     [FRAMEWORK_DEPENDS_WEAK dep1 ...]
#     [LLVM_LINK_COMPONENTS comp1 ...]
#     [C_COMPILE_FLAGS flag1...]
#     [POLAR_COMPILE_FLAGS flag1...]
#     [LINK_FLAGS flag1...]
#     [FILE_DEPENDS target1 ...]
#     [DONT_EMBED_BITCODE]
#     [IS_STDLIB]
#     [FORCE_BUILD_OPTIMIZED]
#     [IS_STDLIB_CORE]
#     [IS_SDK_OVERLAY]
#     INSTALL_IN_COMPONENT comp
#     source1 [source2 source3 ...])
#
# target
#   Name of the target (e.g., PolarAST-IOS-armv7).
#
# name
#   Name of the library (e.g., PolarAST).
#
# MODULE_TARGET
#   Name of the module target (e.g., PolarAST-phpmodule-IOS-armv7).
#
# SHARED
#   Build a shared library.
#
# STATIC
#   Build a static library.
#
# SDK sdk
#   SDK to build for.
#
# ARCHITECTURE
#   Architecture to build for.
#
# DEPENDS
#   Targets that this library depends on.
#
# LINK_LIBRARIES
#   Libraries this library depends on.
#
# FRAMEWORK_DEPENDS
#   System frameworks this library depends on.
#
# FRAMEWORK_DEPENDS_WEAK
#   System frameworks this library depends on that should be weakly-linked.
#
# LLVM_LINK_COMPONENTS
#   LLVM components this library depends on.
#
# C_COMPILE_FLAGS
#   Extra compile flags (C, C++, ObjC).
#
# POLAR_COMPILE_FLAGS
#   Extra compile flags (Swift).
#
# LINK_FLAGS
#   Extra linker flags.
#
# FILE_DEPENDS
#   Additional files this library depends on.
#
# DONT_EMBED_BITCODE
#   Don't embed LLVM bitcode in this target, even if it is enabled globally.
#
# IS_STDLIB
#   Install library dylib and polar module files to lib/polarphp.
#
# IS_STDLIB_CORE
#   Compile as the standard library core.
#
# IS_SDK_OVERLAY
#   Treat the library as a part of the Swift SDK overlay.
#
# INSTALL_IN_COMPONENT comp
#   The polar installation component that this library belongs to.
#
# source1 ...
#   Sources to add into this library
function(_polar_add_library_single target name)
   set(POLARLIB_SINGLE_options
      DONT_EMBED_BITCODE
      FORCE_BUILD_OPTIMIZED
      IS_SDK_OVERLAY
      IS_STDLIB
      IS_STDLIB_CORE
      NOTYPEPHPRT
      OBJECT_LIBRARY
      SHARED
      STATIC
      TARGET_LIBRARY
      INSTALL_WITH_SHARED)
   set(POLARLIB_SINGLE_single_parameter_options
      ARCHITECTURE
      DEPLOYMENT_VERSION_IOS
      DEPLOYMENT_VERSION_OSX
      DEPLOYMENT_VERSION_TVOS
      DEPLOYMENT_VERSION_WATCHOS
      INSTALL_IN_COMPONENT
      DARWIN_INSTALL_NAME_DIR
      MODULE_TARGET
      SDK)
   set(POLARLIB_SINGLE_multiple_parameter_options
      C_COMPILE_FLAGS
      DEPENDS
      FILE_DEPENDS
      FRAMEWORK_DEPENDS
      FRAMEWORK_DEPENDS_WEAK
      GYB_SOURCES
      INCORPORATE_OBJECT_LIBRARIES
      INCORPORATE_OBJECT_LIBRARIES_SHARED_ONLY
      LINK_FLAGS
      LINK_LIBRARIES
      LLVM_LINK_COMPONENTS
      PRIVATE_LINK_LIBRARIES
      POLAR_COMPILE_FLAGS)

   cmake_parse_arguments(POLARLIB_SINGLE
      "${POLARLIB_SINGLE_options}"
      "${POLARLIB_SINGLE_single_parameter_options}"
      "${POLARLIB_SINGLE_multiple_parameter_options}"
      ${ARGN})
   set(POLARLIB_SINGLE_SOURCES ${POLARLIB_SINGLE_UNPARSED_ARGUMENTS})

   polar_translate_flags(POLARLIB_SINGLE "${POLARLIB_SINGLE_options}")

   # Check arguments.
   polar_precondition(POLARLIB_SINGLE_SDK MESSAGE "Should specify an SDK")
   polar_precondition(POLARLIB_SINGLE_ARCHITECTURE MESSAGE "Should specify an architecture")
   polar_precondition(POLARLIB_SINGLE_INSTALL_IN_COMPONENT MESSAGE "INSTALL_IN_COMPONENT is required")

   if(NOT POLARLIB_SINGLE_SHARED AND
      NOT POLARLIB_SINGLE_STATIC AND
      NOT POLARLIB_SINGLE_OBJECT_LIBRARY)
      message(FATAL_ERROR
         "Either SHARED, STATIC, or OBJECT_LIBRARY must be specified")
   endif()

   # Determine the subdirectory where this library will be installed.
   set(POLARLIB_SINGLE_SUBDIR
      "${POLAR_SDK_${POLARLIB_SINGLE_SDK}_LIB_SUBDIR}/${POLARLIB_SINGLE_ARCHITECTURE}")

   # Include LLVM Bitcode slices for iOS, Watch OS, and Apple TV OS device libraries.
   set(embed_bitcode_arg)
   if(POLAR_EMBED_BITCODE_SECTION AND NOT POLARLIB_SINGLE_DONT_EMBED_BITCODE)
      if("${POLARLIB_SINGLE_SDK}" STREQUAL "IOS" OR "${POLARLIB_SINGLE_SDK}" STREQUAL "TVOS" OR "${POLARLIB_SINGLE_SDK}" STREQUAL "WATCHOS")
         list(APPEND POLARLIB_SINGLE_C_COMPILE_FLAGS "-fembed-bitcode")
         list(APPEND POLARLIB_SINGLE_LINK_FLAGS "-Xlinker" "-bitcode_bundle" "-Xlinker" "-lto_library" "-Xlinker" "${LLVM_LIBRARY_DIR}/libLTO.dylib")
         # If we are asked to hide symbols, pass the obfuscation flag to libLTO.
         if (POLAR_EMBED_BITCODE_SECTION_HIDE_SYMBOLS)
            list(APPEND POLARLIB_SINGLE_LINK_FLAGS "-Xlinker" "-bitcode_hide_symbols")
         endif()
         set(embed_bitcode_arg EMBED_BITCODE)
      endif()
   endif()

   if(${POLARLIB_SINGLE_SDK} IN_LIST POLAR_APPLE_PLATFORMS)
      list(APPEND POLARLIB_SINGLE_LINK_FLAGS "-Xlinker" "-compatibility_version" "-Xlinker" "1")
      if (POLAR_COMPILER_VERSION)
         list(APPEND POLARLIB_SINGLE_LINK_FLAGS "-Xlinker" "-current_version" "-Xlinker" "${POLAR_COMPILER_VERSION}" )
      endif()
   endif()

   if(XCODE)
      string(REGEX MATCHALL "/[^/]+" split_path ${CMAKE_CURRENT_SOURCE_DIR})
      list(GET split_path -1 dir)
      file(GLOB_RECURSE POLARLIB_SINGLE_HEADERS
         ${POLAR_SOURCE_DIR}/include/polarphp${dir}/*.h
         ${POLAR_SOURCE_DIR}/include/polarphp${dir}/*.def
         ${CMAKE_CURRENT_SOURCE_DIR}/*.def)

      file(GLOB_RECURSE POLARLIB_SINGLE_TDS
         ${POLAR_SOURCE_DIR}/include/polarphp${dir}/*.td)

      set_source_files_properties(${POLARLIB_SINGLE_HEADERS} ${POLARLIB_SINGLE_TDS}
         PROPERTIES
         HEADER_FILE_ONLY true)
      source_group("TableGen descriptions" FILES ${POLARLIB_SINGLE_TDS})

      set(POLARLIB_SINGLE_SOURCES ${POLARLIB_SINGLE_SOURCES} ${POLARLIB_SINGLE_HEADERS} ${POLARLIB_SINGLE_TDS})
   endif()

   if(MODULE)
      set(libkind MODULE)
   elseif(POLARLIB_SINGLE_OBJECT_LIBRARY)
      set(libkind OBJECT)
      # If both SHARED and STATIC are specified, we add the SHARED library first.
      # The STATIC library is handled further below.
   elseif(POLARLIB_SINGLE_SHARED)
      set(libkind SHARED)
   elseif(POLARLIB_SINGLE_STATIC)
      set(libkind STATIC)
   else()
      message(FATAL_ERROR
         "Either SHARED, STATIC, or OBJECT_LIBRARY must be specified")
   endif()

   if(POLARLIB_SINGLE_GYB_SOURCES)
      handle_gyb_sources(
         gyb_dependency_targets
         POLARLIB_SINGLE_GYB_SOURCES
         "${POLARLIB_SINGLE_ARCHITECTURE}")
      set(POLARLIB_SINGLE_SOURCES ${POLARLIB_SINGLE_SOURCES}
         ${POLARLIB_SINGLE_GYB_SOURCES})
   endif()

   # Remove the "polar" prefix from the name to determine the module name.
   # TODO
   if(POLARLIB_IS_STDLIB_CORE)
      set(module_name "Polar")
   else()
      string(REPLACE Polar "" module_name "${name}")
   endif()

   if("${POLARLIB_SINGLE_SDK}" STREQUAL "WINDOWS")
      if(NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
         polar_windows_generate_sdk_vfs_overlay(POLARLIB_SINGLE_VFS_OVERLAY_FLAGS)
         foreach(flag ${POLARLIB_SINGLE_VFS_OVERLAY_FLAGS})
            list(APPEND POLARLIB_SINGLE_POLAR_COMPILE_FLAGS -Xcc;${flag})
            list(APPEND POLARLIB_SINGLE_C_COMPILE_FLAGS ${flag})
         endforeach()
      endif()
      polar_windows_include_for_arch(${POLARLIB_SINGLE_ARCHITECTURE} POLARLIB_INCLUDE)
      foreach(directory ${POLARLIB_INCLUDE})
         list(APPEND POLARLIB_SINGLE_POLAR_COMPILE_FLAGS -Xcc;-isystem;-Xcc;${directory})
      endforeach()
      if("${POLARLIB_SINGLE_ARCHITECTURE}" MATCHES arm)
         list(APPEND POLARLIB_SINGLE_POLAR_COMPILE_FLAGS -Xcc;-D_ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE)
      endif()
      list(APPEND POLARLIB_SINGLE_POLAR_COMPILE_FLAGS
         -libc;${POLAR_STDLIB_MSVC_RUNTIME_LIBRARY})
   endif()

   # FIXME: don't actually depend on the libraries in POLARLIB_SINGLE_LINK_LIBRARIES,
   # just any phpmodule files that are associated with them.
   polar_handle_sources(
      polar_object_dependency_target
      polar_module_dependency_target
      polar_pib_dependency_target
      polar_pibopt_dependency_target
      polar_pibgen_dependency_target
      POLARLIB_SINGLE_SOURCES
      POLARLIB_SINGLE_EXTERNAL_SOURCES ${name}
      DEPENDS
      ${gyb_dependency_targets}
      ${POLARLIB_SINGLE_DEPENDS}
      ${POLARLIB_SINGLE_FILE_DEPENDS}
      ${POLARLIB_SINGLE_LINK_LIBRARIES}
      SDK ${POLARLIB_SINGLE_SDK}
      ARCHITECTURE ${POLARLIB_SINGLE_ARCHITECTURE}
      MODULE_NAME ${module_name}
      COMPILE_FLAGS ${POLARLIB_SINGLE_POLAR_COMPILE_FLAGS}
      ${POLARLIB_SINGLE_IS_STDLIB_keyword}
      ${POLARLIB_SINGLE_IS_STDLIB_CORE_keyword}
      ${POLARLIB_SINGLE_IS_SDK_OVERLAY_keyword}
      ${embed_bitcode_arg}
      INSTALL_IN_COMPONENT "${POLARLIB_SINGLE_INSTALL_IN_COMPONENT}")
   add_typephp_source_group("${POLARLIB_SINGLE_EXTERNAL_SOURCES}")

   # If there were any typephp sources, then a .phpmodule may have been created.
   # If that is the case, then add a target which is an alias of the module files.
   set(VARIANT_SUFFIX "-${POLAR_SDK_${POLARLIB_SINGLE_SDK}_LIB_SUBDIR}-${POLARLIB_SINGLE_ARCHITECTURE}")
   if(NOT "${POLARLIB_SINGLE_MODULE_TARGET}" STREQUAL "" AND NOT "${polar_module_dependency_target}" STREQUAL "")
      add_custom_target("${POLARLIB_SINGLE_MODULE_TARGET}"
         DEPENDS ${polar_module_dependency_target})
      set_target_properties("${POLARLIB_SINGLE_MODULE_TARGET}" PROPERTIES
         FOLDER "Swift libraries/Modules")
   endif()

   # For standalone overlay builds to work
   if(NOT BUILD_STANDALONE)
      if (EXISTS polar_pib_dependency_target AND NOT "${polar_pib_dependency_target}" STREQUAL "")
         add_dependencies(polar-stdlib${VARIANT_SUFFIX}-sib ${polar_pib_dependency_target})
      endif()

      if (EXISTS polar_pibopt_dependency_target AND NOT "${polar_pibopt_dependency_target}" STREQUAL "")
         add_dependencies(polar-stdlib${VARIANT_SUFFIX}-sibopt ${polar_pibopt_dependency_target})
      endif()

      if (EXISTS polar_pibgen_dependency_target AND NOT "${polar_pibgen_dependency_target}" STREQUAL "")
         add_dependencies(polar-stdlib${VARIANT_SUFFIX}-sibgen ${polar_pibgen_dependency_target})
      endif()
   endif()

   # Only build the modules for any arch listed in the *_MODULE_ARCHITECTURES.
   if(POLARLIB_SINGLE_SDK IN_LIST POLAR_APPLE_PLATFORMS
      AND POLARLIB_SINGLE_ARCHITECTURE IN_LIST POLAR_SDK_${POLARLIB_SINGLE_SDK}_MODULE_ARCHITECTURES)
      # Create dummy target to hook up the module target dependency.
      add_custom_target("${target}"
         DEPENDS
         "${polar_module_dependency_target}")

      return()
   endif()

   set(POLARLIB_INCORPORATED_OBJECT_LIBRARIES_EXPRESSIONS)
   foreach(object_library ${POLARLIB_SINGLE_INCORPORATE_OBJECT_LIBRARIES})
      list(APPEND POLARLIB_INCORPORATED_OBJECT_LIBRARIES_EXPRESSIONS
         $<TARGET_OBJECTS:${object_library}${VARIANT_SUFFIX}>)
   endforeach()

   set(POLARLIB_INCORPORATED_OBJECT_LIBRARIES_EXPRESSIONS_SHARED_ONLY)
   foreach(object_library ${POLARLIB_SINGLE_INCORPORATE_OBJECT_LIBRARIES_SHARED_ONLY})
      list(APPEND POLARLIB_INCORPORATED_OBJECT_LIBRARIES_EXPRESSIONS_SHARED_ONLY
         $<TARGET_OBJECTS:${object_library}${VARIANT_SUFFIX}>)
   endforeach()

   set(POLARLIB_SINGLE_XCODE_WORKAROUND_SOURCES)
   if(XCODE AND POLARLIB_SINGLE_TARGET_LIBRARY)
      set(POLARLIB_SINGLE_XCODE_WORKAROUND_SOURCES
         # Note: the dummy.cpp source file provides no definitions. However,
         # it forces Xcode to properly link the static library.
         ${POLAR_SOURCE_DIR}/cmake/dummy.cpp)
   endif()

   set(INCORPORATED_OBJECT_LIBRARIES_EXPRESSIONS ${POLARLIB_INCORPORATED_OBJECT_LIBRARIES_EXPRESSIONS})
   if(${libkind} STREQUAL "SHARED")
      list(APPEND INCORPORATED_OBJECT_LIBRARIES_EXPRESSIONS
         ${POLARLIB_INCORPORATED_OBJECT_LIBRARIES_EXPRESSIONS_SHARED_ONLY})
   endif()

   add_library("${target}" ${libkind}
      ${POLARLIB_SINGLE_SOURCES}
      ${POLARLIB_SINGLE_EXTERNAL_SOURCES}
      ${INCORPORATED_OBJECT_LIBRARIES_EXPRESSIONS}
      ${POLARLIB_SINGLE_XCODE_WORKAROUND_SOURCES})
   if(("${POLAR_SDK_${POLARLIB_SINGLE_SDK}_OBJECT_FORMAT}" STREQUAL "ELF" OR
      "${POLAR_SDK_${POLARLIB_SINGLE_SDK}_OBJECT_FORMAT}" STREQUAL "COFF") AND
      POLARLIB_SINGLE_TARGET_LIBRARY)
      if("${libkind}" STREQUAL "SHARED" AND NOT POLARLIB_SINGLE_NOTYPEPHPRT)
         # TODO(compnerd) switch to the generator expression when cmake is upgraded
         # to a version which supports it.
         # target_sources(${target}
         #                PRIVATE
         #                  $<TARGET_OBJECTS:polarImageRegistrationObject${POLAR_SDK_${POLARLIB_SINGLE_SDK}_OBJECT_FORMAT}-${POLAR_SDK_${POLARLIB_SINGLE_SDK}_LIB_SUBDIR}-${POLARLIB_SINGLE_ARCHITECTURE}>)
         if(POLARLIB_SINGLE_SDK STREQUAL WINDOWS)
            set(extension .obj)
         else()
            set(extension .o)
         endif()
         target_sources(${target}
            PRIVATE
            "${POLARLIB_DIR}/${POLARLIB_SINGLE_SUBDIR}/typephprt${extension}")
         set_source_files_properties("${POLARLIB_DIR}/${POLARLIB_SINGLE_SUBDIR}/typephprt${extension}"
            PROPERTIES
            GENERATED 1)
      endif()
   endif()
   _set_target_prefix_and_suffix("${target}" "${libkind}" "${POLARLIB_SINGLE_SDK}")

   if("${POLARLIB_SINGLE_SDK}" STREQUAL "WINDOWS")
      polar_windows_include_for_arch(${POLARLIB_SINGLE_ARCHITECTURE} POLARLIB_INCLUDE)
      target_include_directories("${target}" SYSTEM PRIVATE ${POLARLIB_INCLUDE})
      set_target_properties(${target}
         PROPERTIES
         CXX_STANDARD 14)
   endif()

   if("${POLARLIB_SINGLE_SDK}" STREQUAL "WINDOWS" AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
      if("${libkind}" STREQUAL "SHARED")
         # Each dll has an associated .lib (import library); since we may be
         # building on a non-DLL platform (not windows), create an imported target
         # for the library which created implicitly by the dll.
         add_custom_command_target(${target}_IMPORT_LIBRARY
            OUTPUT "${POLARLIB_DIR}/${POLARLIB_SINGLE_SUBDIR}/${name}.lib"
            DEPENDS "${target}")
         add_library(${target}_IMPLIB SHARED IMPORTED GLOBAL)
         set_property(TARGET "${target}_IMPLIB" PROPERTY
            IMPORTED_LOCATION "${POLARLIB_DIR}/${POLARLIB_SINGLE_SUBDIR}/${name}.lib")
         add_dependencies(${target}_IMPLIB ${${target}_IMPORT_LIBRARY})
      endif()
      set_property(TARGET "${target}" PROPERTY NO_SONAME ON)
   endif()

   llvm_update_compile_flags(${target})

   set_output_directory(${target}
      BINARY_DIR ${POLAR_RUNTIME_OUTPUT_INTDIR}
      LIBRARY_DIR ${POLAR_LIBRARY_OUTPUT_INTDIR})

   if(MODULE)
      set_target_properties("${target}" PROPERTIES
         PREFIX ""
         SUFFIX ${LLVM_PLUGIN_EXT})
   endif()

   if(POLARLIB_SINGLE_TARGET_LIBRARY)
      # Install runtime libraries to lib/polarphp instead of lib. This works around
      # the fact that -isysroot prevents linking to libraries in the system
      # /usr/lib if Swift is installed in /usr.
      set_target_properties("${target}" PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY ${POLARLIB_DIR}/${POLARLIB_SINGLE_SUBDIR}
         ARCHIVE_OUTPUT_DIRECTORY ${POLARLIB_DIR}/${POLARLIB_SINGLE_SUBDIR})
      if(POLARLIB_SINGLE_SDK STREQUAL WINDOWS AND POLARLIB_SINGLE_IS_STDLIB_CORE
         AND libkind STREQUAL SHARED)
         add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${target}> ${POLARLIB_DIR}/${POLARLIB_SINGLE_SUBDIR})
      endif()

      foreach(config ${CMAKE_CONFIGURATION_TYPES})
         string(TOUPPER ${config} config_upper)
         escape_path_for_xcode("${config}" "${POLARLIB_DIR}" config_lib_dir)
         set_target_properties(${target} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY_${config_upper} ${config_lib_dir}/${POLARLIB_SINGLE_SUBDIR}
            ARCHIVE_OUTPUT_DIRECTORY_${config_upper} ${config_lib_dir}/${POLARLIB_SINGLE_SUBDIR})
      endforeach()
   endif()

   if(POLARLIB_SINGLE_SDK IN_LIST POLAR_APPLE_PLATFORMS)
      set(install_name_dir "@rpath")

      if(POLARLIB_SINGLE_IS_STDLIB)
         set(install_name_dir "${POLAR_DARWIN_STDLIB_INSTALL_NAME_DIR}")
      endif()

      # Always use @rpath for XCTest
      if(module_name STREQUAL "XCTest")
         set(install_name_dir "@rpath")
      endif()

      if(POLARLIB_SINGLE_DARWIN_INSTALL_NAME_DIR)
         set(install_name_dir "${POLARLIB_SINGLE_DARWIN_INSTALL_NAME_DIR}")
      endif()

      set_target_properties("${target}"
         PROPERTIES
         INSTALL_NAME_DIR "${install_name_dir}")
   elseif("${POLARLIB_SINGLE_SDK}" STREQUAL "LINUX")
      set_target_properties("${target}"
         PROPERTIES
         INSTALL_RPATH "$ORIGIN:/usr/lib/polarphp/linux")
   elseif("${POLARLIB_SINGLE_SDK}" STREQUAL "CYGWIN")
      set_target_properties("${target}"
         PROPERTIES
         INSTALL_RPATH "$ORIGIN:/usr/lib/polarphp/cygwin")
   elseif("${POLARLIB_SINGLE_SDK}" STREQUAL "ANDROID")
      # CMake generates incorrect rule `$SONAME_FLAG $INSTALLNAME_DIR$SONAME` for Android build on macOS cross-compile host.
      # Proper linker flags constructed manually. See below variable `polarlib_link_flags_all`.
      set_target_properties("${target}" PROPERTIES NO_SONAME TRUE)
      # Only set the install RPATH if cross-compiling the host tools, in which
      # case both the NDK and Sysroot paths must be set.
      if(NOT "${POLAR_ANDROID_NDK_PATH}" STREQUAL "" AND
         NOT "${POLAR_ANDROID_NATIVE_SYSROOT}" STREQUAL "")
         set_target_properties("${target}"
            PROPERTIES
            INSTALL_RPATH "$ORIGIN")
      endif()
   endif()

   set_target_properties("${target}" PROPERTIES BUILD_WITH_INSTALL_RPATH YES)
   set_target_properties("${target}" PROPERTIES FOLDER "Swift libraries")

   # Configure the static library target.
   # Set compile and link flags for the non-static target.
   # Do these LAST.
   set(target_static)
   if(POLARLIB_SINGLE_IS_STDLIB AND POLARLIB_SINGLE_STATIC)
      set(target_static "${target}-static")

      # We have already compiled Swift sources.  Link everything into a static
      # library.
      add_library(${target_static} STATIC
         ${POLARLIB_SINGLE_SOURCES}
         ${POLARLIB_INCORPORATED_OBJECT_LIBRARIES_EXPRESSIONS}
         ${POLARLIB_SINGLE_XCODE_WORKAROUND_SOURCES})

      set_output_directory(${target_static}
         BINARY_DIR ${POLAR_RUNTIME_OUTPUT_INTDIR}
         LIBRARY_DIR ${POLAR_LIBRARY_OUTPUT_INTDIR})

      if(POLARLIB_INSTALL_WITH_SHARED)
         set(polar_lib_dir ${POLARLIB_DIR})
      else()
         set(polar_lib_dir ${POLARSTATICLIB_DIR})
      endif()

      foreach(config ${CMAKE_CONFIGURATION_TYPES})
         string(TOUPPER ${config} config_upper)
         escape_path_for_xcode(
            "${config}" "${polar_lib_dir}" config_lib_dir)
         set_target_properties(${target_static} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY_${config_upper} ${config_lib_dir}/${POLARLIB_SINGLE_SUBDIR}
            ARCHIVE_OUTPUT_DIRECTORY_${config_upper} ${config_lib_dir}/${POLARLIB_SINGLE_SUBDIR})
      endforeach()

      set_target_properties(${target_static} PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY ${polar_lib_dir}/${POLARLIB_SINGLE_SUBDIR}
         ARCHIVE_OUTPUT_DIRECTORY ${polar_lib_dir}/${POLARLIB_SINGLE_SUBDIR})
   endif()

   set_target_properties(${target}
      PROPERTIES
      # Library name (without the variant information)
      OUTPUT_NAME ${name})
   if(target_static)
      set_target_properties(${target_static}
         PROPERTIES
         OUTPUT_NAME ${name})
   endif()

   # Don't build standard libraries by default.  We will enable building
   # standard libraries that the user requested; the rest can be built on-demand.
   if(POLARLIB_SINGLE_TARGET_LIBRARY)
      foreach(t "${target}" ${target_static})
         set_target_properties(${t} PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endforeach()
   endif()

   # Handle linking and dependencies.
   add_dependencies_multiple_targets(
      TARGETS "${target}" ${target_static}
      DEPENDS
      ${POLARLIB_SINGLE_DEPENDS}
      ${gyb_dependency_targets}
      "${polar_object_dependency_target}"
      "${polar_module_dependency_target}"
      ${LLVM_COMMON_DEPENDS})

   # HACK: On some systems or build directory setups, CMake will not find static
   # archives of Clang libraries in the Clang build directory, and it will pass
   # them as '-lclangFoo'.  Some other logic in CMake would reorder libraries
   # specified with this syntax, which breaks linking.
   set(prefixed_link_libraries)
   foreach(dep ${POLARLIB_SINGLE_LINK_LIBRARIES})
      if("${dep}" MATCHES "^clang")
         if("${POLAR_HOST_VARIANT_SDK}" STREQUAL "WINDOWS")
            set(dep "${LLVM_LIBRARY_OUTPUT_INTDIR}/${dep}.lib")
         else()
            set(dep "${LLVM_LIBRARY_OUTPUT_INTDIR}/lib${dep}.a")
         endif()
      endif()
      list(APPEND prefixed_link_libraries "${dep}")
   endforeach()
   set(POLARLIB_SINGLE_LINK_LIBRARIES "${prefixed_link_libraries}")

   if("${libkind}" STREQUAL "SHARED")
      target_link_libraries("${target}" PRIVATE ${POLARLIB_SINGLE_LINK_LIBRARIES})
   elseif("${libkind}" STREQUAL "OBJECT")
      precondition_list_empty(
         "${POLARLIB_SINGLE_LINK_LIBRARIES}"
         "OBJECT_LIBRARY may not link to anything")
   else()
      target_link_libraries("${target}" INTERFACE ${POLARLIB_SINGLE_LINK_LIBRARIES})
   endif()

   # Don't add the icucore target.
   set(POLARLIB_SINGLE_LINK_LIBRARIES_WITHOUT_ICU)
   foreach(item ${POLARLIB_SINGLE_LINK_LIBRARIES})
      if(NOT "${item}" STREQUAL "icucore")
         list(APPEND POLARLIB_SINGLE_LINK_LIBRARIES_WITHOUT_ICU "${item}")
      endif()
   endforeach()

   if(target_static)
      _list_add_string_suffix(
         "${POLARLIB_SINGLE_LINK_LIBRARIES_WITHOUT_ICU}"
         "-static"
         target_static_depends)
      # FIXME: should this be target_link_libraries?
      add_dependencies_multiple_targets(
         TARGETS "${target_static}"
         DEPENDS ${target_static_depends})
   endif()

   # Link against system frameworks.
   foreach(FRAMEWORK ${POLARLIB_SINGLE_FRAMEWORK_DEPENDS})
      foreach(t "${target}" ${target_static})
         target_link_libraries("${t}" PUBLIC "-framework ${FRAMEWORK}")
      endforeach()
   endforeach()
   foreach(FRAMEWORK ${POLARLIB_SINGLE_FRAMEWORK_DEPENDS_WEAK})
      foreach(t "${target}" ${target_static})
         target_link_libraries("${t}" PUBLIC "-weak_framework ${FRAMEWORK}")
      endforeach()
   endforeach()

   if(NOT POLARLIB_SINGLE_TARGET_LIBRARY)
      # Call llvm_config() only for libraries that are part of the compiler.
      polar_common_llvm_config("${target}" ${POLARLIB_SINGLE_LLVM_LINK_COMPONENTS})
   endif()

   # Collect compile and link flags for the static and non-static targets.
   # Don't set PROPERTY COMPILE_FLAGS or LINK_FLAGS directly.
   set(c_compile_flags ${POLARLIB_SINGLE_C_COMPILE_FLAGS})
   set(link_flags ${POLARLIB_SINGLE_LINK_FLAGS})

   set(library_search_subdir "${POLAR_SDK_${POLARLIB_SINGLE_SDK}_LIB_SUBDIR}")
   set(library_search_directories
      "${POLARLIB_DIR}/${POLARLIB_SINGLE_SUBDIR}"
      "${POLAR_NATIVE_POLARPHP_TOOLS_PATH}/../lib/polarphp/${POLARLIB_SINGLE_SUBDIR}"
      "${POLAR_NATIVE_POLARPHP_TOOLS_PATH}/../lib/polarphp/${POLAR_SDK_${POLARLIB_SINGLE_SDK}_LIB_SUBDIR}")

   # In certain cases when building, the environment variable SDKROOT is set to override
   # where the sdk root is located in the system. If that environment variable has been
   # set by the user, respect it and add the specified SDKROOT directory to the
   # library_search_directories so we are able to link against those libraries
   if(DEFINED ENV{SDKROOT} AND EXISTS "$ENV{SDKROOT}/usr/lib/polarphp")
      list(APPEND library_search_directories "$ENV{SDKROOT}/usr/lib/polarphp")
   endif()

   # Add variant-specific flags.
   if(POLARLIB_SINGLE_TARGET_LIBRARY)
      set(build_type "${POLAR_STDLIB_BUILD_TYPE}")
      set(enable_assertions "${POLAR_STDLIB_ASSERTIONS}")
   else()
      set(build_type "${CMAKE_BUILD_TYPE}")
      set(enable_assertions "${LLVM_ENABLE_ASSERTIONS}")
      set(analyze_code_coverage "${POLAR_ANALYZE_CODE_COVERAGE}")
   endif()

   if (NOT POLARLIB_SINGLE_TARGET_LIBRARY)
      set(lto_type "${POLAR_TOOLS_ENABLE_LTO}")
   endif()

   _add_variant_c_compile_flags(
      SDK "${POLARLIB_SINGLE_SDK}"
      ARCH "${POLARLIB_SINGLE_ARCHITECTURE}"
      BUILD_TYPE "${build_type}"
      ENABLE_ASSERTIONS "${enable_assertions}"
      ANALYZE_CODE_COVERAGE "${analyze_code_coverage}"
      ENABLE_LTO "${lto_type}"
      DEPLOYMENT_VERSION_OSX "${POLARLIB_DEPLOYMENT_VERSION_OSX}"
      DEPLOYMENT_VERSION_IOS "${POLARLIB_DEPLOYMENT_VERSION_IOS}"
      DEPLOYMENT_VERSION_TVOS "${POLARLIB_DEPLOYMENT_VERSION_TVOS}"
      DEPLOYMENT_VERSION_WATCHOS "${POLARLIB_DEPLOYMENT_VERSION_WATCHOS}"
      "${POLARLIB_SINGLE_FORCE_BUILD_OPTIMIZED_keyword}"
      RESULT_VAR_NAME c_compile_flags
   )

   if(POLARLIB_IS_STDLIB)
      # We don't ever want to link against the ABI-breakage checking symbols
      # in the standard library, runtime, or overlays because they only rely
      # on the header parts of LLVM's ADT.
      list(APPEND c_compile_flags
         "-DLLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING=1")
   endif()

   if(POLARLIB_SINGLE_SDK STREQUAL WINDOWS)
      if(libkind STREQUAL SHARED)
         list(APPEND c_compile_flags -D_WINDLL)
      endif()
   endif()
   _add_variant_link_flags(
      SDK "${POLARLIB_SINGLE_SDK}"
      ARCH "${POLARLIB_SINGLE_ARCHITECTURE}"
      BUILD_TYPE "${build_type}"
      ENABLE_ASSERTIONS "${enable_assertions}"
      ANALYZE_CODE_COVERAGE "${analyze_code_coverage}"
      ENABLE_LTO "${lto_type}"
      LTO_OBJECT_NAME "${target}-${POLARLIB_SINGLE_SDK}-${POLARLIB_SINGLE_ARCHITECTURE}"
      DEPLOYMENT_VERSION_OSX "${POLARLIB_DEPLOYMENT_VERSION_OSX}"
      DEPLOYMENT_VERSION_IOS "${POLARLIB_DEPLOYMENT_VERSION_IOS}"
      DEPLOYMENT_VERSION_TVOS "${POLARLIB_DEPLOYMENT_VERSION_TVOS}"
      DEPLOYMENT_VERSION_WATCHOS "${POLARLIB_DEPLOYMENT_VERSION_WATCHOS}"
      RESULT_VAR_NAME link_flags
      LINK_LIBRARIES_VAR_NAME link_libraries
      LIBRARY_SEARCH_DIRECTORIES_VAR_NAME library_search_directories
   )

   # Configure plist creation for OS X.
   set(PLIST_INFO_PLIST "Info.plist" CACHE STRING "Plist name")
   if("${POLARLIB_SINGLE_SDK}" IN_LIST POLAR_APPLE_PLATFORMS AND POLARLIB_SINGLE_IS_STDLIB)
      set(PLIST_INFO_NAME ${name})
      set(PLIST_INFO_UTI "com.apple.dt.runtime.${name}")
      set(PLIST_INFO_VERSION "${POLAR_VERSION}")
      if (POLAR_COMPILER_VERSION)
         set(PLIST_INFO_BUILD_VERSION
            "${POLAR_COMPILER_VERSION}")
      endif()

      set(PLIST_INFO_PLIST_OUT "${PLIST_INFO_PLIST}")
      list(APPEND link_flags
         "-Wl,-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_BINARY_DIR}/${PLIST_INFO_PLIST_OUT}")
      configure_file(
         "${POLAR_SOURCE_DIR}/stdlib/${PLIST_INFO_PLIST}.in"
         "${PLIST_INFO_PLIST_OUT}"
         @ONLY
         NEWLINE_STYLE UNIX)

      # If Application Extensions are enabled, pass the linker flag marking
      # the dylib as safe.
      if (CXX_SUPPORTS_FAPPLICATION_EXTENSION AND (NOT DISABLE_APPLICATION_EXTENSION))
         list(APPEND link_flags "-Wl,-application_extension")
      endif()

      set(PLIST_INFO_UTI)
      set(PLIST_INFO_NAME)
      set(PLIST_INFO_VERSION)
      set(PLIST_INFO_BUILD_VERSION)
   endif()

   # Convert variables to space-separated strings.
   _list_escape_for_shell("${c_compile_flags}" c_compile_flags)
   _list_escape_for_shell("${link_flags}" link_flags)

   # Set compilation and link flags.
   set_property(TARGET "${target}" APPEND_STRING PROPERTY
      COMPILE_FLAGS " ${c_compile_flags}")
   set_property(TARGET "${target}" APPEND_STRING PROPERTY
      LINK_FLAGS " ${link_flags}")
   set_property(TARGET "${target}" APPEND PROPERTY LINK_LIBRARIES ${link_libraries})
   polar_target_link_search_directories("${target}" "${library_search_directories}")

   # Adjust the linked libraries for windows targets.  On Windows, the link is
   # performed against the import library, and the runtime uses the dll.  Not
   # doing so will result in incorrect symbol resolution and linkage.  We created
   # import library targets when the library was added.  Use that to adjust the
   # link libraries.
   if(POLARLIB_SINGLE_SDK STREQUAL WINDOWS AND NOT CMAKE_SYSTEM_NAME STREQUAL Windows)
      foreach(library_list LINK_LIBRARIES PRIVATE_LINK_LIBRARIES)
         set(import_libraries)
         foreach(library ${POLARLIB_SINGLE_${library_list}})
            # Ensure that the library is a target.  If an absolute path was given,
            # then we do not have an import library associated with it.  This occurs
            # primarily with ICU (which will be an import library).  Import
            # libraries are only associated with shared libraries, so add an
            # additional check for that as well.
            set(import_library ${library})
            if(TARGET ${library})
               get_target_property(type ${library} TYPE)
               if(${type} STREQUAL "SHARED_LIBRARY")
                  set(import_library ${library}_IMPLIB)
               endif()
            endif()
            list(APPEND import_libraries ${import_library})
         endforeach()
         set(POLARLIB_SINGLE_${library_list} ${import_libraries})
      endforeach()
   endif()

   if("${libkind}" STREQUAL "OBJECT")
      precondition_list_empty(
         "${POLARLIB_SINGLE_PRIVATE_LINK_LIBRARIES}"
         "OBJECT_LIBRARY may not link to anything")
   else()
      target_link_libraries("${target}" PRIVATE
         ${POLARLIB_SINGLE_PRIVATE_LINK_LIBRARIES})
   endif()

   # NOTE(compnerd) use the C linker language to invoke `clang` rather than
   # `clang++` as we explicitly link against the C++ runtime.  We were previously
   # actually passing `-nostdlib++` to avoid the C++ runtime linkage.
   if("${POLARLIB_SINGLE_SDK}" STREQUAL "ANDROID")
      set_property(TARGET "${target}" PROPERTY
         LINKER_LANGUAGE "C")
   else()
      set_property(TARGET "${target}" PROPERTY
         LINKER_LANGUAGE "CXX")
   endif()

   if(target_static)
      set_property(TARGET "${target_static}" APPEND_STRING PROPERTY
         COMPILE_FLAGS " ${c_compile_flags}")
      # FIXME: The fallback paths here are going to be dynamic libraries.

      if(POLARLIB_INSTALL_WITH_SHARED)
         set(search_base_dir ${POLARLIB_DIR})
      else()
         set(search_base_dir ${POLARSTATICLIB_DIR})
      endif()
      set(library_search_directories
         "${search_base_dir}/${POLARLIB_SINGLE_SUBDIR}"
         "${POLAR_NATIVE_POLARPHP_TOOLS_PATH}/../lib/polarphp/${POLARLIB_SINGLE_SUBDIR}"
         "${POLAR_NATIVE_POLARPHP_TOOLS_PATH}/../lib/polarphp/${POLAR_SDK_${POLARLIB_SINGLE_SDK}_LIB_SUBDIR}")
      polar_target_link_search_directories("${target_static}" "${library_search_directories}")
      target_link_libraries("${target_static}" PRIVATE
         ${POLARLIB_SINGLE_PRIVATE_LINK_LIBRARIES})
   endif()
   add_dependencies(${target} llvm-project)
   add_dependencies(${target} clang-project)
   if (target_static)
      add_dependencies(${target_static} llvm-project)
      add_dependencies(${target_static} clang-project)
   endif()

   # Do not add code here.
endfunction()

# Add a new host library.
#
# Usage:
#   polar_add_host_library(name
#     [SHARED]
#     [STATIC]
#     [LLVM_LINK_COMPONENTS comp1 ...]
#     [FILE_DEPENDS target1 ...]
#     source1 [source2 source3 ...])
#
# name
#   Name of the library (e.g., PolarAST).
#
# SHARED
#   Build a shared library.
#
# STATIC
#   Build a static library.
#
# LLVM_LINK_COMPONENTS
#   LLVM components this library depends on.
#
# FILE_DEPENDS
#   Additional files this library depends on.
#
# source1 ...
#   Sources to add into this library.
function(polar_add_host_library name)
   set(options
      FORCE_BUILD_OPTIMIZED
      SHARED
      STATIC)
   set(single_parameter_options)
   set(multiple_parameter_options
      FILE_DEPENDS
      LINK_LIBRARIES
      LLVM_LINK_COMPONENTS)

   cmake_parse_arguments(ASHL
      "${options}"
      "${single_parameter_options}"
      "${multiple_parameter_options}"
      ${ARGN})
   set(ASHL_SOURCES ${ASHL_UNPARSED_ARGUMENTS})

   polar_translate_flags(ASHL "${options}")

   if(NOT ASHL_SHARED AND NOT ASHL_STATIC)
      message(FATAL_ERROR "Either SHARED or STATIC must be specified")
   endif()

   _polar_add_library_single(
      ${name}
      ${name}
      ${ASHL_SHARED_keyword}
      ${ASHL_STATIC_keyword}
      ${ASHL_SOURCES}
      ${ASHL_FORCE_BUILD_OPTIMIZED_keyword}
      SDK ${POLAR_HOST_VARIANT_SDK}
      ARCHITECTURE ${POLAR_HOST_VARIANT_ARCH}
      LLVM_LINK_COMPONENTS ${ASHL_LLVM_LINK_COMPONENTS}
      FILE_DEPENDS ${ASHL_FILE_DEPENDS}
      INSTALL_IN_COMPONENT "dev"
   )

   add_dependencies(dev ${name})
   if(NOT LLVM_INSTALL_TOOLCHAIN_ONLY)
      polar_install_in_component(TARGETS ${name}
         ARCHIVE DESTINATION lib${LLVM_LIBDIR_SUFFIX} COMPONENT dev
         LIBRARY DESTINATION lib${LLVM_LIBDIR_SUFFIX} COMPONENT dev
         RUNTIME DESTINATION bin COMPONENT dev)
   endif()

   polar_is_installing_component(dev is_installing)
   if(NOT is_installing)
      set_property(GLOBAL APPEND PROPERTY POLAR_BUILDTREE_EXPORTS ${name})
   else()
      set_property(GLOBAL APPEND PROPERTY POLAR_EXPORTS ${name})
   endif()
endfunction()

#
# function for typephp
#
# Add a new typephp target library.
#
# NOTE: This has not had the polarphp host code debrided from it yet. That will be
# in a forthcoming commit.
#
# Usage:
#   polar_add_typephp_target_library(name
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
#   Name of the library (e.g., polarParse).
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
#   Extra compiler flags (polarphp).
#
# LINK_FLAGS
#   Extra linker flags.
#
# DONT_EMBED_BITCODE
#   Don't embed LLVM bitcode in this target, even if it is enabled globally.
#
# IS_STDLIB
#   Treat the library as a part of the polarphp standard library.
#
# IS_STDLIB_CORE
#   Compile as the polarphp standard library core.
#
# IS_SDK_OVERLAY
#   Treat the library as a part of the polarphp SDK overlay.
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
function(polar_add_typephp_target_library name)
   set(POLARLIB_options
      DONT_EMBED_BITCODE
      FORCE_BUILD_OPTIMIZED
      HAS_POLAR_CONTENT
      IS_SDK_OVERLAY
      IS_STDLIB
      IS_STDLIB_CORE
      NOSWIFTRT
      OBJECT_LIBRARY
      SHARED
      STATIC
      INSTALL_WITH_SHARED)
   set(POLARLIB_single_parameter_options
      DEPLOYMENT_VERSION_IOS
      DEPLOYMENT_VERSION_OSX
      DEPLOYMENT_VERSION_TVOS
      DEPLOYMENT_VERSION_WATCHOS
      INSTALL_IN_COMPONENT
      DARWIN_INSTALL_NAME_DIR)
   set(POLARLIB_multiple_parameter_options
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

   cmake_parse_arguments(POLARLIB
      "${POLARLIB_options}"
      "${POLARLIB_single_parameter_options}"
      "${POLARLIB_multiple_parameter_options}"
      ${ARGN})
   set(POLARLIB_SOURCES ${POLARLIB_UNPARSED_ARGUMENTS})

   # Infer arguments.

   if(POLARLIB_IS_SDK_OVERLAY)
      set(POLARLIB_HAS_POLAR_CONTENT TRUE)
      set(POLARLIB_IS_STDLIB TRUE)
   endif()

   # Standard library is always a target library.
   if(POLARLIB_IS_STDLIB)
      set(POLARLIB_HAS_POLAR_CONTENT TRUE)
   endif()

   # If target SDKs are not specified, build for all known SDKs.
   if("${POLARLIB_TARGET_SDKS}" STREQUAL "")
      set(POLARLIB_TARGET_SDKS ${POLAR_SDKS})
   endif()
   list_replace(POLARLIB_TARGET_SDKS ALL_APPLE_PLATFORMS "${POLAR_APPLE_PLATFORMS}")

   # All Swift code depends on the standard library, except for the standard
   # library itself.
   if(POLARLIB_HAS_POLAR_CONTENT AND NOT POLARLIB_IS_STDLIB_CORE)
      list(APPEND POLARLIB_POLAR_MODULE_DEPENDS Core)

      # polarSwiftOnoneSupport does not depend on itself, obviously.
      if(NOT ${name} STREQUAL polarSwiftOnoneSupport)
         # All Swift code depends on the SwiftOnoneSupport in non-optimized mode,
         # except for the standard library itself.
         is_build_type_optimized("${POLAR_STDLIB_BUILD_TYPE}" optimized)
         if(NOT optimized)
            list(APPEND POLARLIB_POLAR_MODULE_DEPENDS SwiftOnoneSupport)
         endif()
      endif()
   endif()

   if((NOT "${POLAR_BUILD_STDLIB}") AND
   (NOT "${POLARLIB_POLAR_MODULE_DEPENDS}" STREQUAL ""))
      list(REMOVE_ITEM POLARLIB_POLAR_MODULE_DEPENDS Core SwiftOnoneSupport)
   endif()

   translate_flags(POLARLIB "${POLARLIB_options}")
   precondition(POLARLIB_INSTALL_IN_COMPONENT MESSAGE "INSTALL_IN_COMPONENT is required")

   if(NOT POLARLIB_SHARED AND
      NOT POLARLIB_STATIC AND
      NOT POLARLIB_OBJECT_LIBRARY)
      message(FATAL_ERROR
         "Either SHARED, STATIC, or OBJECT_LIBRARY must be specified")
   endif()

   # In the standard library and overlays, warn about implicit overrides
   # as a reminder to consider when inherited protocols need different
   # behavior for their requirements.
   if (POLARLIB_IS_STDLIB)
      list(APPEND POLARLIB_POLAR_COMPILE_FLAGS "-warn-implicit-overrides")
   endif()

   if(NOT POLAR_BUILD_RUNTIME_WITH_HOST_COMPILER AND NOT BUILD_STANDALONE)
      list(APPEND POLARLIB_DEPENDS clang)
   endif()

   # If we are building this library for targets, loop through the various
   # SDKs building the variants of this library.
   list_intersect(
      "${POLARLIB_TARGET_SDKS}" "${POLAR_SDKS}" POLARLIB_TARGET_SDKS)

   foreach(sdk ${POLARLIB_TARGET_SDKS})
      if(NOT POLAR_SDK_${sdk}_ARCHITECTURES)
         # POLAR_SDK_${sdk}_ARCHITECTURES is empty, so just continue
         continue()
      endif()

      set(THIN_INPUT_TARGETS)

      # Collect architecture agnostic SDK module dependencies
      set(polarlib_module_depends_flattened ${POLARLIB_POLAR_MODULE_DEPENDS})
      if(${sdk} STREQUAL OSX)
         list(APPEND polarlib_module_depends_flattened
            ${POLARLIB_POLAR_MODULE_DEPENDS_OSX})
      elseif(${sdk} STREQUAL IOS OR ${sdk} STREQUAL IOS_SIMULATOR)
         list(APPEND polarlib_module_depends_flattened
            ${POLARLIB_POLAR_MODULE_DEPENDS_IOS})
      elseif(${sdk} STREQUAL TVOS OR ${sdk} STREQUAL TVOS_SIMULATOR)
         list(APPEND polarlib_module_depends_flattened
            ${POLARLIB_POLAR_MODULE_DEPENDS_TVOS})
      elseif(${sdk} STREQUAL WATCHOS OR ${sdk} STREQUAL WATCHOS_SIMULATOR)
         list(APPEND polarlib_module_depends_flattened
            ${POLARLIB_POLAR_MODULE_DEPENDS_WATCHOS})
      elseif(${sdk} STREQUAL FREEBSD)
         list(APPEND polarlib_module_depends_flattened
            ${POLARLIB_POLAR_MODULE_DEPENDS_FREEBSD})
      elseif(${sdk} STREQUAL LINUX OR ${sdk} STREQUAL ANDROID)
         list(APPEND polarlib_module_depends_flattened
            ${POLARLIB_POLAR_MODULE_DEPENDS_LINUX})
      elseif(${sdk} STREQUAL CYGWIN)
         list(APPEND polarlib_module_depends_flattened
            ${POLARLIB_POLAR_MODULE_DEPENDS_CYGWIN})
      elseif(${sdk} STREQUAL HAIKU)
         list(APPEND polarlib_module_depends_flattened
            ${POLARLIB_POLAR_MODULE_DEPENDS_HAIKU})
      elseif(${sdk} STREQUAL WINDOWS)
         list(APPEND polarlib_module_depends_flattened
            ${POLARLIB_POLAR_MODULE_DEPENDS_WINDOWS})
      endif()

      # Collect architecture agnostic SDK framework dependencies
      set(polarlib_framework_depends_flattened ${POLARLIB_FRAMEWORK_DEPENDS})
      if(${sdk} STREQUAL OSX)
         list(APPEND polarlib_framework_depends_flattened
            ${POLARLIB_FRAMEWORK_DEPENDS_OSX})
      elseif(${sdk} STREQUAL IOS OR ${sdk} STREQUAL IOS_SIMULATOR OR
         ${sdk} STREQUAL TVOS OR ${sdk} STREQUAL TVOS_SIMULATOR)
         list(APPEND polarlib_framework_depends_flattened
            ${POLARLIB_FRAMEWORK_DEPENDS_IOS_TVOS})
      endif()

      # Collect architecutre agnostic compiler flags
      set(polarlib_polarphp_compile_flags_all ${POLARLIB_POLAR_COMPILE_FLAGS})
      if(${sdk} STREQUAL OSX)
         list(APPEND polarlib_polarphp_compile_flags_all
            ${POLARLIB_POLAR_COMPILE_FLAGS_OSX})
      elseif(${sdk} STREQUAL IOS OR ${sdk} STREQUAL IOS_SIMULATOR)
         list(APPEND polarlib_polarphp_compile_flags_all
            ${POLARLIB_POLAR_COMPILE_FLAGS_IOS})
      elseif(${sdk} STREQUAL TVOS OR ${sdk} STREQUAL TVOS_SIMULATOR)
         list(APPEND polarlib_polarphp_compile_flags_all
            ${POLARLIB_POLAR_COMPILE_FLAGS_TVOS})
      elseif(${sdk} STREQUAL WATCHOS OR ${sdk} STREQUAL WATCHOS_SIMULATOR)
         list(APPEND polarlib_polarphp_compile_flags_all
            ${POLARLIB_POLAR_COMPILE_FLAGS_WATCHOS})
      elseif(${sdk} STREQUAL LINUX)
         list(APPEND polarlib_polarphp_compile_flags_all
            ${POLARLIB_POLAR_COMPILE_FLAGS_LINUX})
      elseif(${sdk} STREQUAL WINDOWS)
         # FIXME(SR2005) static and shared are not mutually exclusive; however
         # since we do a single build of the sources, this doesn't work for
         # building both simultaneously.  Effectively, only shared builds are
         # supported on windows currently.
         if(POLARLIB_SHARED)
            list(APPEND polarlib_polarphp_compile_flags_all -D_WINDLL)
            if(POLARLIB_IS_STDLIB_CORE)
               list(APPEND polarlib_polarphp_compile_flags_all -DpolarCore_EXPORTS)
            endif()
         elseif(POLARLIB_STATIC)
            list(APPEND polarlib_polarphp_compile_flags_all -D_LIB)
         endif()
      endif()


      # Collect architecture agnostic SDK linker flags
      set(polarlib_link_flags_all ${POLARLIB_LINK_FLAGS})
      if(${sdk} STREQUAL IOS_SIMULATOR AND ${name} STREQUAL polarMediaPlayer)
         # message("DISABLING AUTOLINK FOR polarMediaPlayer")
         list(APPEND polarlib_link_flags_all "-Xlinker" "-ignore_auto_link")
      endif()

      # We unconditionally removed "-z,defs" from CMAKE_SHARED_LINKER_FLAGS in
      # polar_common_standalone_build_config_llvm within
      # SwiftSharedCMakeConfig.cmake, where it was added by a call to
      # HandleLLVMOptions.
      #
      # Rather than applying it to all targets and libraries, we here add it
      # back to supported targets and libraries only.  This is needed for ELF
      # targets only; however, RemoteMirror needs to build with undefined
      # symbols.
      if(${POLAR_SDK_${sdk}_OBJECT_FORMAT} STREQUAL ELF AND
         NOT ${name} STREQUAL polarRemoteMirror)
         list(APPEND polarlib_link_flags_all "-Wl,-z,defs")
      endif()
      # Setting back linker flags which are not supported when making Android build on macOS cross-compile host.
      if(POLARLIB_SHARED)
         if(sdk IN_LIST POLAR_APPLE_PLATFORMS)
            list(APPEND polarlib_link_flags_all "-dynamiclib -Wl,-headerpad_max_install_names")
         elseif(${sdk} STREQUAL ANDROID)
            list(APPEND polarlib_link_flags_all "-shared")
            # TODO: Instead of `lib${name}.so` find variable or target property which already have this value.
            list(APPEND polarlib_link_flags_all "-Wl,-soname,lib${name}.so")
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
         set(MODULE_VARIANT_SUFFIX "-polarmodule${VARIANT_SUFFIX}")
         set(MODULE_VARIANT_NAME "${name}${MODULE_VARIANT_SUFFIX}")

         # Map dependencies over to the appropriate variants.
         set(polarlib_link_libraries)
         foreach(lib ${POLARLIB_LINK_LIBRARIES})
            if(TARGET "${lib}${VARIANT_SUFFIX}")
               list(APPEND polarlib_link_libraries "${lib}${VARIANT_SUFFIX}")
            else()
               list(APPEND polarlib_link_libraries "${lib}")
            endif()
         endforeach()

         # Swift compiles depend on polar modules, while links depend on
         # linked libraries.  Find targets for both of these here.
         set(polarlib_module_dependency_targets)
         set(polarlib_private_link_libraries_targets)

         if(NOT BUILD_STANDALONE)
            foreach(mod ${polarlib_module_depends_flattened})
               list(APPEND polarlib_module_dependency_targets
                  "polar${mod}${MODULE_VARIANT_SUFFIX}")

               list(APPEND polarlib_private_link_libraries_targets
                  "polar${mod}${VARIANT_SUFFIX}")
            endforeach()
         endif()

         foreach(lib ${POLARLIB_PRIVATE_LINK_LIBRARIES})
            if(TARGET "${lib}${VARIANT_SUFFIX}")
               list(APPEND polarlib_private_link_libraries_targets
                  "${lib}${VARIANT_SUFFIX}")
            else()
               list(APPEND polarlib_private_link_libraries_targets "${lib}")
            endif()
         endforeach()

         # Add PrivateFrameworks, rdar://28466433
         set(polarlib_c_compile_flags_all ${POLARLIB_C_COMPILE_FLAGS})
         if(sdk IN_LIST POLAR_APPLE_PLATFORMS AND POLARLIB_IS_SDK_OVERLAY)
            set(polarlib_polarphp_compile_private_frameworks_flag "-Fsystem" "${POLAR_SDK_${sdk}_ARCH_${arch}_PATH}/System/Library/PrivateFrameworks/")
         endif()

         list(APPEND polarlib_c_compile_flags_all "-DPOLAR_TARGET_LIBRARY_NAME=${name}")

         # Add this library variant.
         _polar_add_library_single(
            ${VARIANT_NAME}
            ${name}
            ${POLARLIB_SHARED_keyword}
            ${POLARLIB_STATIC_keyword}
            ${POLARLIB_OBJECT_LIBRARY_keyword}
            ${POLARLIB_INSTALL_WITH_SHARED_keyword}
            ${POLARLIB_SOURCES}
            TARGET_LIBRARY
            MODULE_TARGET ${MODULE_VARIANT_NAME}
            SDK ${sdk}
            ARCHITECTURE ${arch}
            DEPENDS ${POLARLIB_DEPENDS}
            LINK_LIBRARIES ${polarlib_link_libraries}
            FRAMEWORK_DEPENDS ${polarlib_framework_depends_flattened}
            FRAMEWORK_DEPENDS_WEAK ${POLARLIB_FRAMEWORK_DEPENDS_WEAK}
            LLVM_LINK_COMPONENTS ${POLARLIB_LLVM_LINK_COMPONENTS}
            FILE_DEPENDS ${POLARLIB_FILE_DEPENDS} ${polarlib_module_dependency_targets}
            C_COMPILE_FLAGS ${polarlib_c_compile_flags_all}
            POLAR_COMPILE_FLAGS ${polarlib_polarphp_compile_flags_all} ${polarlib_polarphp_compile_flags_arch} ${polarlib_polarphp_compile_private_frameworks_flag}
            LINK_FLAGS ${polarlib_link_flags_all}
            PRIVATE_LINK_LIBRARIES ${polarlib_private_link_libraries_targets}
            INCORPORATE_OBJECT_LIBRARIES ${POLARLIB_INCORPORATE_OBJECT_LIBRARIES}
            INCORPORATE_OBJECT_LIBRARIES_SHARED_ONLY ${POLARLIB_INCORPORATE_OBJECT_LIBRARIES_SHARED_ONLY}
            ${POLARLIB_DONT_EMBED_BITCODE_keyword}
            ${POLARLIB_IS_STDLIB_keyword}
            ${POLARLIB_IS_STDLIB_CORE_keyword}
            ${POLARLIB_IS_SDK_OVERLAY_keyword}
            ${POLARLIB_FORCE_BUILD_OPTIMIZED_keyword}
            ${POLARLIB_NOTYPEPHPRT_keyword}
            DARWIN_INSTALL_NAME_DIR "${POLARLIB_DARWIN_INSTALL_NAME_DIR}"
            INSTALL_IN_COMPONENT "${POLARLIB_INSTALL_IN_COMPONENT}"
            DEPLOYMENT_VERSION_OSX "${POLARLIB_DEPLOYMENT_VERSION_OSX}"
            DEPLOYMENT_VERSION_IOS "${POLARLIB_DEPLOYMENT_VERSION_IOS}"
            DEPLOYMENT_VERSION_TVOS "${POLARLIB_DEPLOYMENT_VERSION_TVOS}"
            DEPLOYMENT_VERSION_WATCHOS "${POLARLIB_DEPLOYMENT_VERSION_WATCHOS}"
            GYB_SOURCES ${POLARLIB_GYB_SOURCES}
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

         if(NOT POLARLIB_OBJECT_LIBRARY)
            # Add dependencies on the (not-yet-created) custom lipo target.
            foreach(DEP ${POLARLIB_LINK_LIBRARIES})
               if (NOT "${DEP}" STREQUAL "icucore")
                  add_dependencies(${VARIANT_NAME}
                     "${DEP}-${POLAR_SDK_${sdk}_LIB_SUBDIR}")
               endif()
            endforeach()

            if (POLARLIB_IS_STDLIB AND POLARLIB_STATIC)
               # Add dependencies on the (not-yet-created) custom lipo target.
               foreach(DEP ${POLARLIB_LINK_LIBRARIES})
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
            set(_module_variant_name "${name}-polarmodule-${_variant_suffix}")

            add_dependencies("${_target}" ${_module_variant_name})

            # Add Swift standard library targets as dependencies to the top-level
            # convenience target.
            if(TARGET "polarphp-stdlib${_variant_suffix}")
               add_dependencies("polarphp-stdlib${_variant_suffix}"
                  "${_target}")
            endif()
         endforeach()

         return()
      endif()

      if(NOT POLARLIB_OBJECT_LIBRARY)
         # Determine the name of the universal library.
         if(POLARLIB_SHARED)
            if("${sdk}" STREQUAL "WINDOWS")
               set(UNIVERSAL_LIBRARY_NAME
                  "${POLARLIB_DIR}/${POLAR_SDK_${sdk}_LIB_SUBDIR}/${name}.dll")
            else()
               set(UNIVERSAL_LIBRARY_NAME
                  "${POLARLIB_DIR}/${POLAR_SDK_${sdk}_LIB_SUBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}${name}${CMAKE_SHARED_LIBRARY_SUFFIX}")
            endif()
         else()
            if("${sdk}" STREQUAL "WINDOWS")
               set(UNIVERSAL_LIBRARY_NAME
                  "${POLARLIB_DIR}/${POLAR_SDK_${sdk}_LIB_SUBDIR}/${name}.lib")
            else()
               set(UNIVERSAL_LIBRARY_NAME
                  "${POLARLIB_DIR}/${POLAR_SDK_${sdk}_LIB_SUBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${name}${CMAKE_STATIC_LIBRARY_SUFFIX}")
            endif()
         endif()

         set(lipo_target "${name}-${POLAR_SDK_${sdk}_LIB_SUBDIR}")
         if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin" AND POLARLIB_SHARED)
            set(codesign_arg CODESIGN)
         endif()
         precondition(THIN_INPUT_TARGETS)
         _polar_add_lipo_target(SDK
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

         if(POLARLIB_SHARED OR POLARLIB_INSTALL_WITH_SHARED)
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
            add_dependencies(${POLARLIB_INSTALL_IN_COMPONENT} ${name}-windows-${POLAR_PRIMARY_VARIANT_ARCH})
            polar_install_in_component(TARGETS ${name}-windows-${POLAR_PRIMARY_VARIANT_ARCH}
               RUNTIME
               DESTINATION "bin"
               COMPONENT "${POLARLIB_INSTALL_IN_COMPONENT}"
               LIBRARY
               DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/${resource_dir}/${resource_dir_sdk_subdir}/${POLAR_PRIMARY_VARIANT_ARCH}"
               COMPONENT "${POLARLIB_INSTALL_IN_COMPONENT}"
               ARCHIVE
               DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/${resource_dir}/${resource_dir_sdk_subdir}/${POLAR_PRIMARY_VARIANT_ARCH}"
               COMPONENT "${POLARLIB_INSTALL_IN_COMPONENT}"
               PERMISSIONS ${file_permissions})
         else()
            # NOTE: ${UNIVERSAL_LIBRARY_NAME} is the output associated with the target
            # ${lipo_target}
            add_dependencies(${POLARLIB_INSTALL_IN_COMPONENT} ${lipo_target})
            polar_install_in_component(FILES "${UNIVERSAL_LIBRARY_NAME}"
               DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/${resource_dir}/${resource_dir_sdk_subdir}"
               COMPONENT "${POLARLIB_INSTALL_IN_COMPONENT}"
               PERMISSIONS ${file_permissions}
               "${optional_arg}")
         endif()
         if(sdk STREQUAL WINDOWS)
            foreach(arch ${POLAR_SDK_WINDOWS_ARCHITECTURES})
               if(TARGET ${name}-windows-${arch}_IMPLIB)
                  get_target_property(import_library ${name}-windows-${arch}_IMPLIB IMPORTED_LOCATION)
                  add_dependencies(${POLARLIB_INSTALL_IN_COMPONENT} ${name}-windows-${arch}_IMPLIB)
                  polar_install_in_component(FILES ${import_library}
                     DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/${resource_dir}/${resource_dir_sdk_subdir}/${arch}"
                     COMPONENT ${POLARLIB_INSTALL_IN_COMPONENT}
                     PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
               endif()
            endforeach()
         endif()

         polar_is_installing_component(
            "${POLARLIB_INSTALL_IN_COMPONENT}"
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

         # Add the polarmodule-only targets to the lipo target depdencies.
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
         if (POLARLIB_IS_STDLIB AND POLARLIB_STATIC)
            set(THIN_INPUT_TARGETS_STATIC)
            foreach(TARGET ${THIN_INPUT_TARGETS})
               list(APPEND THIN_INPUT_TARGETS_STATIC "${TARGET}-static")
            endforeach()

            if(POLARLIB_INSTALL_WITH_SHARED)
               set(install_subdir "polarphp")
               set(universal_subdir ${POLARLIB_DIR})
            else()
               set(install_subdir "polarphp_static")
               set(universal_subdir ${POLARSTATICLIB_DIR})
            endif()

            set(lipo_target_static
               "${name}-${POLAR_SDK_${sdk}_LIB_SUBDIR}-static")
            set(UNIVERSAL_LIBRARY_NAME
               "${universal_subdir}/${POLAR_SDK_${sdk}_LIB_SUBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${name}${CMAKE_STATIC_LIBRARY_SUFFIX}")
            _polar_add_lipo_target(SDK
               ${sdk}
               TARGET
               ${lipo_target_static}
               OUTPUT
               "${UNIVERSAL_LIBRARY_NAME}"
               ${THIN_INPUT_TARGETS_STATIC})
            add_dependencies(${POLARLIB_INSTALL_IN_COMPONENT} ${lipo_target_static})
            polar_install_in_component(FILES "${UNIVERSAL_LIBRARY_NAME}"
               DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/${install_subdir}/${resource_dir_sdk_subdir}"
               PERMISSIONS
               OWNER_READ OWNER_WRITE
               GROUP_READ
               WORLD_READ
               COMPONENT "${POLARLIB_INSTALL_IN_COMPONENT}"
               "${optional_arg}")
         endif()

         # Add Swift standard library targets as dependencies to the top-level
         # convenience target.
         set(FILTERED_UNITTESTS
            polarphpStdlibCollectionUnittest
            polarphpStdlibUnicodeUnittest)

         foreach(arch ${POLAR_SDK_${sdk}_ARCHITECTURES})
            set(VARIANT_SUFFIX "-${POLAR_SDK_${sdk}_LIB_SUBDIR}-${arch}")
            if(TARGET "polarphp-stdlib${VARIANT_SUFFIX}" AND
               TARGET "polarphp-test-stdlib${VARIANT_SUFFIX}")
               add_dependencies("polarphp-stdlib${VARIANT_SUFFIX}"
                  ${lipo_target}
                  ${lipo_target_static})
               if(NOT "${name}" IN_LIST FILTERED_UNITTESTS)
                  add_dependencies("polar-test-stdlib${VARIANT_SUFFIX}"
                     ${lipo_target}
                     ${lipo_target_static})
               endif()
            endif()
         endforeach()
      endif()
   endforeach()
endfunction()

# function for typephp
# Add an executable compiled for a given variant.
#
# Don't use directly, use polar_add_typephp_executable and polar_add_typephp_target_executable
# instead.
#
# See polar_add_typephp_executable for detailed documentation.
#
# Additional parameters:
#   [SDK sdk]
#     SDK to build for.
#
#   [ARCHITECTURE architecture]
#     Architecture to build for.
function(_polar_add_executable_single name)
   # Parse the arguments we were given.
   cmake_parse_arguments(POLAREXE_SINGLE
      "EXCLUDE_FROM_ALL"
      "SDK;ARCHITECTURE"
      "DEPENDS;LLVM_LINK_COMPONENTS;LINK_LIBRARIES;COMPILE_FLAGS"
      ${ARGN})

   set(POLAREXE_SINGLE_SOURCES ${POLAREXE_SINGLE_UNPARSED_ARGUMENTS})

   polar_translate_flag(${POLAREXE_SINGLE_EXCLUDE_FROM_ALL}
      "EXCLUDE_FROM_ALL"
      POLAREXE_SINGLE_EXCLUDE_FROM_ALL_FLAG)

   # Check arguments.
   polar_precondition(POLAREXE_SINGLE_SDK MESSAGE "Should specify an SDK")
   polar_precondition(POLAREXE_SINGLE_ARCHITECTURE MESSAGE "Should specify an architecture")

   # Determine compiler flags.
   set(c_compile_flags)
   set(link_flags)

   # Prepare linker search directories.
   set(library_search_directories
      "${POLARLIB_DIR}/${POLAR_SDK_${POLAREXE_SINGLE_SDK}_LIB_SUBDIR}")

   # Add variant-specific flags.
   _add_variant_c_compile_flags(
      SDK "${POLAREXE_SINGLE_SDK}"
      ARCH "${POLAREXE_SINGLE_ARCHITECTURE}"
      BUILD_TYPE "${CMAKE_BUILD_TYPE}"
      ENABLE_ASSERTIONS "${LLVM_ENABLE_ASSERTIONS}"
      ENABLE_LTO "${POLAR_TOOLS_ENABLE_LTO}"
      ANALYZE_CODE_COVERAGE "${POLAR_ANALYZE_CODE_COVERAGE}"
      RESULT_VAR_NAME c_compile_flags)
   _add_variant_link_flags(
      SDK "${POLAREXE_SINGLE_SDK}"
      ARCH "${POLAREXE_SINGLE_ARCHITECTURE}"
      BUILD_TYPE "${CMAKE_BUILD_TYPE}"
      ENABLE_ASSERTIONS "${LLVM_ENABLE_ASSERTIONS}"
      ENABLE_LTO "${POLAR_TOOLS_ENABLE_LTO}"
      LTO_OBJECT_NAME "${name}-${POLAREXE_SINGLE_SDK}-${POLAREXE_SINGLE_ARCHITECTURE}"
      ANALYZE_CODE_COVERAGE "${POLAR_ANALYZE_CODE_COVERAGE}"
      RESULT_VAR_NAME link_flags
      LINK_LIBRARIES_VAR_NAME link_libraries
      LIBRARY_SEARCH_DIRECTORIES_VAR_NAME library_search_directories)

   if(${POLAREXE_SINGLE_SDK} IN_LIST POLAR_APPLE_PLATFORMS)
      list(APPEND link_flags
         "-Xlinker" "-rpath"
         "-Xlinker" "@executable_path/../lib/polarphp/${POLAR_SDK_${POLAREXE_SINGLE_SDK}_LIB_SUBDIR}")
   endif()

   _list_add_string_suffix(
      "${POLAREXE_SINGLE_LINK_LIBRARIES}"
      "-${POLAR_SDK_${POLAREXE_SINGLE_SDK}_LIB_SUBDIR}-${POLAREXE_SINGLE_ARCHITECTURE}"
      POLAREXE_SINGLE_LINK_LIBRARIES_TARGETS)
   set(POLAREXE_SINGLE_LINK_LIBRARIES ${POLAREXE_SINGLE_LINK_LIBRARIES_TARGETS})

   polar_handle_sources(
      dependency_target
      unused_module_dependency_target
      unused_sib_dependency_target
      unused_sibopt_dependency_target
      unused_sibgen_dependency_target
      POLAREXE_SINGLE_SOURCES POLAREXE_SINGLE_EXTERNAL_SOURCES ${name}
      DEPENDS
      ${POLAREXE_SINGLE_DEPENDS}
      MODULE_NAME ${name}
      SDK ${POLAREXE_SINGLE_SDK}
      ARCHITECTURE ${POLAREXE_SINGLE_ARCHITECTURE}
      COMPILE_FLAGS ${POLAREXE_SINGLE_COMPILE_FLAGS}
      IS_MAIN)
   add_typephp_source_group("${POLAREXE_SINGLE_EXTERNAL_SOURCES}")

   add_executable(${name}
      ${POLAREXE_SINGLE_EXCLUDE_FROM_ALL_FLAG}
      ${POLAREXE_SINGLE_SOURCES}
      ${POLAREXE_SINGLE_EXTERNAL_SOURCES})

   add_dependencies_multiple_targets(
      TARGETS "${name}"
      DEPENDS
      ${dependency_target}
      ${LLVM_COMMON_DEPENDS}
      ${POLAREXE_SINGLE_DEPENDS})
   llvm_update_compile_flags("${name}")

   # Convert variables to space-separated strings.
   _list_escape_for_shell("${c_compile_flags}" c_compile_flags)
   _list_escape_for_shell("${link_flags}" link_flags)

   set_property(TARGET ${name} APPEND_STRING PROPERTY
      COMPILE_FLAGS " ${c_compile_flags}")
   #  TODO
   #   polar_target_link_search_directories("${name}" "${library_search_directories}")
   set_property(TARGET ${name} APPEND_STRING PROPERTY
      LINK_FLAGS " ${link_flags}")
   set_property(TARGET ${name} APPEND PROPERTY LINK_LIBRARIES ${link_libraries})
   if (POLAR_PARALLEL_LINK_JOBS)
      set_property(TARGET ${name} PROPERTY JOB_POOL_LINK polar_link_job_pool)
   endif()
   set_output_directory(${name}
      BINARY_DIR ${POLAR_RUNTIME_OUTPUT_INTDIR}
      LIBRARY_DIR ${POLAR_LIBRARY_OUTPUT_INTDIR})

   target_link_libraries("${name}" PRIVATE ${POLAREXE_SINGLE_LINK_LIBRARIES})
   polar_common_llvm_config("${name}" ${POLAREXE_SINGLE_LLVM_LINK_COMPONENTS})

   # NOTE(compnerd) use the C linker language to invoke `clang` rather than
   # `clang++` as we explicitly link against the C++ runtime.  We were previously
   # actually passing `-nostdlib++` to avoid the C++ runtime linkage.
   if(${POLAREXE_SINGLE_SDK} STREQUAL ANDROID)
      set_property(TARGET "${name}" PROPERTY
         LINKER_LANGUAGE "C")
   else()
      set_property(TARGET "${name}" PROPERTY
         LINKER_LANGUAGE "CXX")
   endif()

   set_target_properties(${name} PROPERTIES FOLDER "polarphp executables")
endfunction()

macro(polar_add_tool_subdirectory name)
   add_llvm_subdirectory(POLAR TOOL ${name})
endmacro()

macro(polar_add_lib_subdirectory name)
   add_llvm_subdirectory(POLAR LIB ${name})
endmacro()

function(polar_add_host_tool executable)
   set(options)
   set(single_parameter_options POLAR_COMPONENT)
   set(multiple_parameter_options LINK_LIBRARIES)

   cmake_parse_arguments(ASHT
      "${options}"
      "${single_parameter_options}"
      "${multiple_parameter_options}"
      ${ARGN})

   polar_precondition(ASHT_POLAR_COMPONENT
      MESSAGE "polarphp Component is required to add a host tool")

   # Create the executable rule.
   _polar_add_executable_single(${executable}
      SDK ${POLAR_HOST_VARIANT_SDK}
      ARCHITECTURE ${POLAR_HOST_VARIANT_ARCH}
      ${ASHT_UNPARSED_ARGUMENTS})

   add_dependencies(${ASHT_POLAR_COMPONENT} ${executable})
   polar_install_in_component(TARGETS ${executable}
      RUNTIME
      DESTINATION bin
      COMPONENT ${ASHT_POLAR_COMPONENT})

   polar_is_installing_component(${ASHT_POLAR_COMPONENT} is_installing)

   if(NOT is_installing)
      set_property(GLOBAL APPEND PROPERTY POLAR_BUILDTREE_EXPORTS ${executable})
   else()
      set_property(GLOBAL APPEND PROPERTY POLAR_EXPORTS ${executable})
   endif()
endfunction()

# This declares a polar host tool that links with libfuzzer.
function(polar_add_fuzzer_host_tool executable)
   # First create our target. We do not actually parse the argument since we do
   # not care about the arguments, we just pass them all through to
   # polar_add_host_tool.
   polar_add_host_tool(${executable} ${ARGN})

   # Then make sure that we pass the -fsanitize=fuzzer flag both on the cflags
   # and cxx flags line.
   target_compile_options(${executable} PRIVATE "-fsanitize=fuzzer")
   target_link_libraries(${executable} PRIVATE "-fsanitize=fuzzer")
endfunction()

macro(polar_add_tool_symlink name dest component)
   add_llvm_tool_symlink(${name} ${dest} ALWAYS_GENERATE)
   llvm_install_symlink(${name} ${dest} ALWAYS_GENERATE COMPONENT ${component})
endmacro()
