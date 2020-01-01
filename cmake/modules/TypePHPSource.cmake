# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

include(PolarUtils)

# Process the sources within the given variable, pulling out any typephp
# sources to be compiled with 'polarphp' directly. This updates
# ${sourcesvar} in place with the resulting list and ${externalvar} with the
# list of externally-build sources.
#
# Usage:
#   typephp_handle_sources(sourcesvar externalvar)
function(typephp_handle_sources
   dependency_target_out_var_name
   dependency_module_target_out_var_name
   dependency_pib_target_out_var_name
   dependency_pibopt_target_out_var_name
   dependency_pibgen_target_out_var_name
   sourcesvar externalvar name)
   cmake_parse_arguments(SWIFTSOURCES
      "IS_MAIN;IS_STDLIB;IS_STDLIB_CORE;IS_SDK_OVERLAY;EMBED_BITCODE"
      "SDK;ARCHITECTURE;INSTALL_IN_COMPONENT"
      "DEPENDS;COMPILE_FLAGS;MODULE_NAME"
      ${ARGN})
   polar_translate_flag(${POLARPHP_SOURCES_IS_MAIN} "IS_MAIN" IS_MAIN_arg)
   polar_translate_flag(${POLARPHP_SOURCES_IS_STDLIB} "IS_STDLIB" IS_STDLIB_arg)
   polar_translate_flag(${POLARPHP_SOURCES_IS_STDLIB_CORE} "IS_STDLIB_CORE"
      IS_STDLIB_CORE_arg)
   polar_translate_flag(${POLARPHP_SOURCES_IS_SDK_OVERLAY} "IS_SDK_OVERLAY"
      IS_SDK_OVERLAY_arg)
   polar_translate_flag(${POLARPHP_SOURCES_EMBED_BITCODE} "EMBED_BITCODE"
      EMBED_BITCODE_arg)

   if(POLARPHP_SOURCES_IS_MAIN)
      set(POLARPHP_SOURCES_INSTALL_IN_COMPONENT never_install)
   endif()

   # Check arguments.
   precondition(POLARPHP_SOURCES_SDK "Should specify an SDK")
   precondition(POLARPHP_SOURCES_ARCHITECTURE "Should specify an architecture")
   precondition(POLARPHP_SOURCES_INSTALL_IN_COMPONENT "INSTALL_IN_COMPONENT is required")

   # Clear the result variable.
   set("${dependency_target_out_var_name}" "" PARENT_SCOPE)
   set("${dependency_module_target_out_var_name}" "" PARENT_SCOPE)
   set("${dependency_pib_target_out_var_name}" "" PARENT_SCOPE)
   set("${dependency_pibopt_target_out_var_name}" "" PARENT_SCOPE)
   set("${dependency_pibgen_target_out_var_name}" "" PARENT_SCOPE)

   set(result)
   set(typephp_sources)
   foreach(src ${${sourcesvar}})
      get_filename_component(extension ${src} EXT)
      if(extension STREQUAL ".php")
         list(APPEND typephp_sources ${src})
      else()
         list(APPEND result ${src})
      endif()
   endforeach()

   set(polarphp_compile_flags ${POLARPHP_SOURCES_COMPILE_FLAGS})
   if (NOT POLARPHP_SOURCES_IS_MAIN)
      list(APPEND polarphp_compile_flags "-module-link-name" "${name}")
   endif()

   if(typephp_sources)
      set(objsubdir "/${POLARPHP_SOURCES_SDK}/${POLARPHP_SOURCES_ARCHITECTURE}")

      file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}${objsubdir}")

      set(typephp_obj
         "${CMAKE_CURRENT_BINARY_DIR}${objsubdir}/${POLARPHP_SOURCES_MODULE_NAME}${CMAKE_C_OUTPUT_EXTENSION}")

      # FIXME: We shouldn't /have/ to build things in a single process.
      # <rdar://problem/15972329>
      list(APPEND polarphp_compile_flags "-force-single-frontend-invocation")

      _polar_compile_typephp_files(
         dependency_target
         module_dependency_target
         pib_dependency_target
         pibopt_dependency_target
         pibgen_dependency_target
         OUTPUT ${typephp_obj}
         SOURCES ${typephp_sources}
         DEPENDS ${POLARPHP_SOURCES_DEPENDS}
         FLAGS ${polarphp_compile_flags}
         SDK ${POLARPHP_SOURCES_SDK}
         ARCHITECTURE ${POLARPHP_SOURCES_ARCHITECTURE}
         MODULE_NAME ${POLARPHP_SOURCES_MODULE_NAME}
         ${IS_MAIN_arg}
         ${IS_STDLIB_arg}
         ${IS_STDLIB_CORE_arg}
         ${IS_SDK_OVERLAY_arg}
         ${EMBED_BITCODE_arg}
         ${STATIC_arg}
         INSTALL_IN_COMPONENT "${POLARPHP_SOURCES_INSTALL_IN_COMPONENT}")

      set("${dependency_target_out_var_name}" "${dependency_target}" PARENT_SCOPE)
      set("${dependency_module_target_out_var_name}" "${module_dependency_target}" PARENT_SCOPE)
      set("${dependency_pib_target_out_var_name}" "${pib_dependency_target}" PARENT_SCOPE)
      set("${dependency_pibopt_target_out_var_name}" "${pibopt_dependency_target}" PARENT_SCOPE)
      set("${dependency_pibgen_target_out_var_name}" "${pibgen_dependency_target}" PARENT_SCOPE)

      list(APPEND result ${typephp_obj})
   endif()

   llvm_process_sources(result ${result})
   set(${sourcesvar} "${result}" PARENT_SCOPE)
   set(${externalvar} ${typephp_sources} PARENT_SCOPE)
endfunction()

# Add TypePHP source files to the (Xcode) project.
#
# Usage:
#   polar_add_typephp_source_group(sources)
function(polar_add_typephp_source_group sources)
   source_group("TypePHP Sources" FILES ${sources})
   # Mark the source files as HEADER_FILE_ONLY, so that Xcode doesn't try
   # to build them itself.
   set_source_files_properties(${sources}
      PROPERTIES
      HEADER_FILE_ONLY true)
endfunction()


# Compile a typephp file into an object file (as a library).
#
# Usage:
#   _polar_compile_typephp_files(
#     dependency_target_out_var_name
#     dependency_module_target_out_var_name
#     dependency_pib_target_out_var_name    # -Onone pib target
#     dependency_pibopt_target_out_var_name # -O pib target
#     dependency_pibgen_target_out_var_name # -pibgen target
#     OUTPUT objfile                    # Name of the resulting object file
#     SOURCES typephp_src [typephp_src...]  # TypePHP source files to compile
#     FLAGS -module-name foo            # Flags to add to the compilation
#     [SDK sdk]                         # SDK to build for
#     [ARCHITECTURE architecture]       # Architecture to build for
#     [DEPENDS cmake_target...]         # CMake targets on which the object
#                                       # file depends.
#     [IS_MAIN]                         # This is an executable, not a library
#     [IS_STDLIB]
#     [IS_STDLIB_CORE]                  # This is the core standard library
#     [OPT_FLAGS]                       # Optimization flags (overrides POLAR_OPTIMIZE)
#     [MODULE_DIR]                      # Put .phpmodule, .typedoc., and .o
#                                       # into this directory.
#     [MODULE_NAME]                     # The module name.
#     [INSTALL_IN_COMPONENT]            # Install produced files.
#     [EMBED_BITCODE]                   # Embed LLVM bitcode into the .o files
#     )

function(_polar_compile_typephp_files
   dependency_target_out_var_name dependency_module_target_out_var_name
   dependency_pib_target_out_var_name dependency_pibopt_target_out_var_name
   dependency_pibgen_target_out_var_name)
   cmake_parse_arguments(POLARPHP_FILE
      "IS_MAIN;IS_STDLIB;IS_STDLIB_CORE;IS_SDK_OVERLAY;EMBED_BITCODE"
      "OUTPUT;MODULE_NAME;INSTALL_IN_COMPONENT"
      "SOURCES;FLAGS;DEPENDS;SDK;ARCHITECTURE;OPT_FLAGS;MODULE_DIR"
      ${ARGN})

   # Check arguments.
   list(LENGTH POLARPHP_FILE_OUTPUT num_outputs)
   list(GET POLARPHP_FILE_OUTPUT 0 first_output)

   precondition(num_outputs MESSAGE "OUTPUT must not be empty")

   foreach(output ${POLARPHP_FILE_OUTPUT})
      if (NOT IS_ABSOLUTE "${output}")
         message(FATAL_ERROR "OUTPUT should be an absolute path")
      endif()
   endforeach()

   if(POLARPHP_FILE_IS_MAIN AND POLARPHP_FILE_IS_STDLIB)
      message(FATAL_ERROR "Cannot set both IS_MAIN and IS_STDLIB")
   endif()

   precondition(POLARPHP_FILE_SDK MESSAGE "Should specify an SDK")
   precondition(POLARPHP_FILE_ARCHITECTURE MESSAGE "Should specify an architecture")
   precondition(POLARPHP_FILE_INSTALL_IN_COMPONENT MESSAGE "INSTALL_IN_COMPONENT is required")

   if ("${POLARPHP_FILE_MODULE_NAME}" STREQUAL "")
      get_filename_component(POLARPHP_FILE_MODULE_NAME "${first_output}" NAME_WE)
      message(SEND_ERROR
         "No module name specified; did you mean to use MODULE_NAME "
         "${POLARPHP_FILE_MODULE_NAME}?")
   endif()

   set(source_files)
   foreach(file ${POLARPHP_FILE_SOURCES})
      # Determine where this file is.
      get_filename_component(file_path ${file} PATH)
      if(IS_ABSOLUTE "${file_path}")
         list(APPEND source_files "${file}")
      else()
         list(APPEND source_files "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
      endif()
   endforeach()

   # Compute flags for the polarphp compiler.
   set(polarphp_flags)
   set(polarphp_module_flags)

   _polar_add_variant_typephp_compile_flags(
      "${POLARPHP_FILE_SDK}"
      "${POLARPHP_FILE_ARCHITECTURE}"
      "${SWIFT_STDLIB_BUILD_TYPE}"
      "${SWIFT_STDLIB_ASSERTIONS}"
      polarphp_flags)

   # Determine the subdirectory where the binary should be placed.
   compute_library_subdir(library_subdir
      "${POLARPHP_FILE_SDK}" "${POLARPHP_FILE_ARCHITECTURE}")

   # Allow import of other TypePHP modules we just built.
   list(APPEND polarphp_flags
      "-I" "${POLARPHP_LIB_DIR}/${library_subdir}")
   # FIXME: should we use '-resource-dir' here?  Seems like it has no advantage
   # over '-I' in this case.

   # If we have a custom module cache path, use it.
   if (SWIFT_MODULE_CACHE_PATH)
      list(APPEND polarphp_flags "-module-cache-path" "${SWIFT_MODULE_CACHE_PATH}")
   endif()

   # Don't include libarclite in any build products by default.
   list(APPEND polarphp_flags "-no-link-objc-runtime")

   if(POLARPHP_PIL_VERIFY_ALL)
      list(APPEND polarphp_flags "-Xfrontend" "-sil-verify-all")
   endif()

   # The standard library and overlays are always built resiliently.
   if(POLARPHP_FILE_IS_STDLIB)
      list(APPEND polarphp_flags "-enable-library-evolution")
      list(APPEND polarphp_flags "-Xfrontend" "-enable-ownership-stripping-after-serialization")
   endif()

   if(POLARPHP_STDLIB_USE_NONATOMIC_RC)
      list(APPEND polarphp_flags "-Xfrontend" "-assume-single-threaded")
   endif()

   if(NOT POLARPHP_ENABLE_STDLIBCORE_EXCLUSIVITY_CHECKING AND POLARPHP_FILE_IS_STDLIB)
      list(APPEND polarphp_flags "-Xfrontend" "-enforce-exclusivity=unchecked")
   endif()

   if(POLARPHP_EMIT_SORTED_PIL_OUTPUT)
      list(APPEND polarphp_flags "-Xfrontend" "-emit-sorted-sil")
   endif()

   # FIXME: Cleaner way to do this?
   if(POLARPHP_FILE_IS_STDLIB_CORE)
      list(APPEND polarphp_flags
         "-nostdimport" "-parse-stdlib" "-module-name" "Swift")
      list(APPEND polarphp_flags "-Xfrontend" "-group-info-path"
         "-Xfrontend" "${GROUP_INFO_JSON_FILE}")
   else()
      list(APPEND polarphp_flags "-module-name" "${POLARPHP_FILE_MODULE_NAME}")
   endif()

   # Force swift 5 mode for Standard Library and overlays.
   if (POLARPHP_FILE_IS_STDLIB)
      list(APPEND polarphp_flags "-polarphp-version" "5")
   endif()
   if (POLARPHP_FILE_IS_SDK_OVERLAY)
      list(APPEND polarphp_flags "-polarphp-version" "5")
   endif()

   if(POLARPHP_FILE_IS_SDK_OVERLAY)
      list(APPEND polarphp_flags "-autolink-force-load")
   endif()

   # Don't need to link runtime compatibility libraries for older runtimes
   # into the new runtime.
   if (POLARPHP_FILE_IS_STDLIB OR POLARPHP_FILE_IS_SDK_OVERLAY)
      list(APPEND polarphp_flags "-runtime-compatibility-version" "none")
      list(APPEND polarphp_flags "-disable-autolinking-runtime-compatibility-dynamic-replacements")
   endif()

   list(APPEND polarphp_flags ${SWIFT_EXPERIMENTAL_EXTRA_FLAGS})

   if(POLARPHP_FILE_OPT_FLAGS)
      list(APPEND polarphp_flags ${POLARPHP_FILE_OPT_FLAGS})
   endif()

   list(APPEND polarphp_flags ${POLARPHP_FILE_FLAGS})

   set(dirs_to_create)
   foreach(output ${POLARPHP_FILE_OUTPUT})
      get_filename_component(objdir "${output}" PATH)
      list(APPEND dirs_to_create "${objdir}")
   endforeach()

   set(module_file)
   set(module_doc_file)
   set(interface_file)

   if(NOT POLARPHP_FILE_IS_MAIN)
      # Determine the directory where the module file should be placed.
      if(POLARPHP_FILE_MODULE_DIR)
         set(module_dir "${POLARPHP_FILE_MODULE_DIR}")
      elseif(POLARPHP_FILE_IS_STDLIB)
         set(module_dir "${POLARPHP_LIB_DIR}/${library_subdir}")
      else()
         message(FATAL_ERROR "Don't know where to put the module files")
      endif()

      list(APPEND polarphp_flags "-parse-as-library")

      set(module_base "${module_dir}/${POLARPHP_FILE_MODULE_NAME}")
      if(POLARPHP_FILE_SDK IN_LIST SWIFT_APPLE_PLATFORMS)
         set(specific_module_dir "${module_base}.phpmodule")
         set(specific_module_project_dir "${specific_module_dir}/Project")
         set(source_info_file "${specific_module_project_dir}/${POLARPHP_FILE_ARCHITECTURE}.phpsourceinfo")
         set(module_base "${module_base}.phpmodule/${POLARPHP_FILE_ARCHITECTURE}")
      else()
         set(specific_module_dir)
         set(specific_module_project_dir)
         set(source_info_file "${module_base}.phpsourceinfo")
      endif()
      set(module_file "${module_base}.phpmodule")
      set(module_doc_file "${module_base}.phpdoc")

      # FIXME: These don't really belong inside the swiftmodule, but there's not
      # an obvious alternate place to put them.
      set(pib_file "${module_base}.Onone.pib")
      set(pibopt_file "${module_base}.O.pib")
      set(pibgen_file "${module_base}.pibgen")

      if(SWIFT_ENABLE_MODULE_INTERFACES)
         set(interface_file "${module_base}.phpinterface")
         list(APPEND polarphp_module_flags
            "-emit-module-interface-path" "${interface_file}")
      endif()

      if (NOT POLARPHP_FILE_IS_STDLIB_CORE)
         list(APPEND polarphp_module_flags
            "-Xfrontend" "-experimental-skip-non-inlinable-function-bodies")
      endif()

      # If we have extra regexp flags, check if we match any of the regexps. If so
      # add the relevant flags to our polarphp_flags.
      if (POLARPHP_EXPERIMENTAL_EXTRA_REGEXP_FLAGS OR POLARPHP_EXPERIMENTAL_EXTRA_NEGATIVE_REGEXP_FLAGS)
         set(extra_polarphp_flags_for_module)
         _add_extra_polarphp_flags_for_module("${POLARPHP_FILE_MODULE_NAME}" extra_polarphp_flags_for_module)
         if (extra_polarphp_flags_for_module)
            list(APPEND polarphp_flags ${extra_polarphp_flags_for_module})
         endif()
      endif()
   endif()

   set(module_outputs "${module_file}" "${module_doc_file}")
   if(interface_file)
      list(APPEND module_outputs "${interface_file}")
   endif()

   if(POLARPHP_FILE_SDK IN_LIST POLARPHP_APPLE_PLATFORMS)
      swift_install_in_component(DIRECTORY "${specific_module_dir}"
         DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/swift/${library_subdir}"
         COMPONENT "${POLARPHP_FILE_INSTALL_IN_COMPONENT}"
         OPTIONAL
         PATTERN "Project" EXCLUDE)
   else()
      swift_install_in_component(FILES ${module_outputs}
         DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/swift/${library_subdir}"
         COMPONENT "${POLARPHP_FILE_INSTALL_IN_COMPONENT}")
   endif()

   set(line_directive_tool "${POLARPHP_SOURCE_DIR}/utils/line-directive")
   set(polarphp_compiler_tool "${POLARPHP_NATIVE_POLARPHP_TOOLS_PATH}/polarphp")
   set(polarphp_compiler_tool_dep)
   if(POLARPHP_INCLUDE_TOOLS)
      # Depend on the binary itself, in addition to the symlink.
      set(polarphp_compiler_tool_dep "polarphp")
   endif()

   # If there are more than one output files, we assume that they are specified
   # otherwise e.g. with an output file map.
   set(output_option)
   if (${num_outputs} EQUAL 1)
      set(output_option "-o" ${first_output})
   endif()

   set(embed_bitcode_option)
   if (POLARPHP_FILE_EMBED_BITCODE)
      set(embed_bitcode_option "-embed-bitcode")
   endif()

   set(main_command "-c")
   if (POLARPHP_CHECK_INCREMENTAL_COMPILATION)
      set(polarphp_compiler_tool "${SWIFT_SOURCE_DIR}/utils/check-incremental" "${polarphp_compiler_tool}")
   endif()

   if (POLARPHP_REPORT_STATISTICS)
      list(GET dirs_to_create 0 first_obj_dir)
      list(APPEND polarphp_flags "-stats-output-dir" ${first_obj_dir})
   endif()

   set(standard_outputs ${POLARPHP_FILE_OUTPUT})
   set(pib_outputs "${pib_file}")
   set(pibopt_outputs "${pibopt_file}")
   set(pibgen_outputs "${pibgen_file}")

   if(XCODE)
      # HACK: work around an issue with CMake Xcode generator and the polarphp
      # driver.
      #
      # The Swift driver does not update the mtime of the output files if the
      # existing output files on disk are identical to the ones that are about
      # to be written.  This behavior confuses the makefiles used in CMake Xcode
      # projects: the makefiles will not consider everything up to date after
      # invoking the compiler.  As a result, the standard library gets rebuilt
      # multiple times during a single build.
      #
      # To work around this issue we touch the output files so that their mtime
      # always gets updated.
      set(command_touch_standard_outputs
         COMMAND "${CMAKE_COMMAND}" -E touch ${standard_outputs})
      set(command_touch_module_outputs
         COMMAND "${CMAKE_COMMAND}" -E touch ${module_outputs})
      set(command_touch_pib_outputs
         COMMAND "${CMAKE_COMMAND}" -E touch ${pib_outputs})
      set(command_touch_pibopt_outputs
         COMMAND "${CMAKE_COMMAND}" -E touch ${pibopt_outputs})
      set(command_touch_pibgen_outputs
         COMMAND "${CMAKE_COMMAND}" -E touch ${pibgen_outputs})
   endif()

   # First generate the obj dirs
   list(REMOVE_DUPLICATES dirs_to_create)
   add_custom_command_target(
      create_dirs_dependency_target
      COMMAND "${CMAKE_COMMAND}" -E make_directory ${dirs_to_create}
      OUTPUT ${dirs_to_create}
      COMMENT "Generating dirs for ${first_output}")

   # Then we can compile both the object files and the swiftmodule files
   # in parallel in this target for the object file, and ...

   # Windows doesn't support long command line paths, of 8191 chars or over. We
   # need to work around this by avoiding long command line arguments. This can
   # be achieved by writing the list of file paths to a file, then reading that
   # list in the Python script.
   string(RANDOM file_name)
   set(file_path "${CMAKE_CURRENT_BINARY_DIR}/${file_name}.txt")
   string(REPLACE ";" "'\n'" source_files_quoted "${source_files}")
   file(WRITE "${file_path}" "'${source_files_quoted}'")

   # If this platform/architecture combo supports backward deployment to old
   # Objective-C runtimes, we need to copy a YAML file with legacy type layout
   # information to the build directory so that the compiler can find it.
   #
   # See stdlib/CMakeLists.txt and TypeConverter::TypeConverter() in
   # lib/IRGen/GenType.cpp.
   if(POLARPHP_FILE_IS_STDLIB_CORE)
      set(POLARPHP_FILE_PLATFORM "${SWIFT_SDK_${POLARPHP_FILE_SDK}_LIB_SUBDIR}")
      set(copy_legacy_layouts_dep
         "copy-legacy-layouts-${POLARPHP_FILE_PLATFORM}-${POLARPHP_FILE_ARCHITECTURE}")
   else()
      set(copy_legacy_layouts_dep)
   endif()

   add_custom_command_target(
      dependency_target
      COMMAND
      "${PYTHON_EXECUTABLE}" "${line_directive_tool}" "@${file_path}" --
      "${polarphp_compiler_tool}" "${main_command}" ${polarphp_flags}
      ${output_option} ${embed_bitcode_option} "@${file_path}"
      ${command_touch_standard_outputs}
      OUTPUT ${standard_outputs}
      DEPENDS
      ${polarphp_compiler_tool_dep}
      ${file_path} ${source_files} ${POLARPHP_FILE_DEPENDS}
      ${polarphp_ide_test_dependency}
      ${create_dirs_dependency_target}
      ${copy_legacy_layouts_dep}
      COMMENT "Compiling ${first_output}")
   set("${dependency_target_out_var_name}" "${dependency_target}" PARENT_SCOPE)

   # This is the target to generate:
   #
   # 1. *.phpmodule
   # 2. *.phpdoc
   # 3. *.phpinterface
   # 4. *.Onone.pib
   # 5. *.O.pib
   # 6. *.pibgen
   #
   # Only 1,2,3 are built by default. 4,5,6 are utility targets for use by
   # engineers and thus even though the targets are generated, the targets are
   # not built by default.
   #
   # We only build these when we are not producing a main file. We could do this
   # with pib/pibgen, but it is useful for looking at the stdlib.
   if (NOT POLARPHP_FILE_IS_MAIN)
      add_custom_command_target(
         module_dependency_target
         COMMAND
         "${CMAKE_COMMAND}" "-E" "remove" "-f" ${module_outputs}
         COMMAND
         "${CMAKE_COMMAND}" "-E" "make_directory" ${module_dir}
         ${specific_module_dir}
         ${specific_module_project_dir}
         COMMAND
         "${PYTHON_EXECUTABLE}" "${line_directive_tool}" "@${file_path}" --
         "${polarphp_compiler_tool}" "-emit-module" "-o" "${module_file}"
         "-emit-module-source-info-path" "${source_info_file}"
         ${polarphp_flags} ${polarphp_module_flags} "@${file_path}"
         ${command_touch_module_outputs}
         OUTPUT ${module_outputs}
         DEPENDS
         ${polarphp_compiler_tool_dep}
         ${source_files} ${POLARPHP_FILE_DEPENDS}
         ${polarphp_ide_test_dependency}
         ${create_dirs_dependency_target}
         COMMENT "Generating ${module_file}")
      set("${dependency_module_target_out_var_name}" "${module_dependency_target}" PARENT_SCOPE)

      # This is the target to generate the .pib files. It is not built by default.
      add_custom_command_target(
         pib_dependency_target
         COMMAND
         "${PYTHON_EXECUTABLE}" "${line_directive_tool}" "@${file_path}" --
         "${polarphp_compiler_tool}" "-emit-pib" "-o" "${pib_file}" ${polarphp_flags} -Onone
         "@${file_path}"
         ${command_touch_pib_outputs}
         OUTPUT ${pib_outputs}
         DEPENDS
         ${polarphp_compiler_tool_dep}
         ${source_files} ${POLARPHP_FILE_DEPENDS}
         ${create_dirs_dependency_target}
         COMMENT "Generating ${pib_file}"
         EXCLUDE_FROM_ALL)
      set("${dependency_pib_target_out_var_name}" "${pib_dependency_target}" PARENT_SCOPE)

      add_custom_command_target(
         pibopt_dependency_target
         COMMAND
         "${PYTHON_EXECUTABLE}" "${line_directive_tool}" "@${file_path}" --
         "${polarphp_compiler_tool}" "-emit-pib" "-o" "${pibopt_file}" ${polarphp_flags} -O
         "@${file_path}"
         ${command_touch_pibopt_outputs}
         OUTPUT ${pibopt_outputs}
         DEPENDS
         ${polarphp_compiler_tool_dep}
         ${source_files} ${POLARPHP_FILE_DEPENDS}
         ${create_dirs_dependency_target}
         COMMENT "Generating ${pibopt_file}"
         EXCLUDE_FROM_ALL)
      set("${dependency_pibopt_target_out_var_name}" "${pibopt_dependency_target}" PARENT_SCOPE)

      # This is the target to generate the .pibgen files. It is not built by default.
      add_custom_command_target(
         pibgen_dependency_target
         COMMAND
         "${PYTHON_EXECUTABLE}" "${line_directive_tool}" "@${file_path}" --
         "${polarphp_compiler_tool}" "-emit-pibgen" "-o" "${pibgen_file}" ${polarphp_flags}
         "@${file_path}"
         ${command_touch_pibgen_outputs}
         OUTPUT ${pibgen_outputs}
         DEPENDS
         ${polarphp_compiler_tool_dep}
         ${source_files} ${POLARPHP_FILE_DEPENDS}
         ${create_dirs_dependency_target}
         COMMENT "Generating ${pibgen_file}"
         EXCLUDE_FROM_ALL)
      set("${dependency_pibgen_target_out_var_name}" "${pibgen_dependency_target}" PARENT_SCOPE)
   endif()
   # Make sure the build system knows the file is a generated object file.
   set_source_files_properties(${POLARPHP_FILE_OUTPUT}
      PROPERTIES
      GENERATED true
      EXTERNAL_OBJECT true
      LANGUAGE C
      OBJECT_DEPENDS "${source_files}")
endfunction()



