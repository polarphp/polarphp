# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/22.

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

   set_property(TARGET ${name} APPEND PROPERTY COMPILE_DEFINITIONS ${POLAR_COMPILE_DEFINITIONS})
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
   if(BUILD_SHARED_LIBS OR ARG_SHARED)
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
   cmake_parse_arguments(ARG "DISABLE_POLAR_LINK_POLAR_DYLIB;IGNORE_EXTERNALIZE_DEBUGINFO;NO_INSTALL_RPATH" "" "DEPENDS" ${ARGN})
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
      target_link_libraries(${name} PRIVATE ${POLAR_THREADS_LIBRARY})
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
      elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND NOT POLAR_LINKER_IS_GOLD)
         # $ORIGIN is not interpreted at link time by ld.bfd
         set_property(TARGET ${name} APPEND_STRING PROPERTY
            LINK_FLAGS " -Wl,-rpath-link,${POLAR_LIBRARY_OUTPUT_INTDIR} ")
      endif()
   else()
      return()
   endif()

   set_target_properties(${name} PROPERTIES
      BUILD_WITH_INSTALL_RPATH ON
      INSTALL_RPATH "${_install_rpath}"
      ${_install_name_dir})

endfunction(polar_setup_rpath)

