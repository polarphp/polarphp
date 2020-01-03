include(PolarUtils)

# Process the sources within the given variable, pulling out any Swift
# sources to be compiled with 'swift' directly. This updates
# ${sourcesvar} in place with the resulting list and ${externalvar} with the
# list of externally-build sources.
#
# Usage:
#   polar_handle_sources(sourcesvar externalvar)
function(polar_handle_sources
   dependency_target_out_var_name
   dependency_module_target_out_var_name
   dependency_pib_target_out_var_name
   dependency_pibopt_target_out_var_name
   dependency_pibgen_target_out_var_name
   sourcesvar externalvar name)
   cmake_parse_arguments(POLARSOURCES
      "IS_MAIN;IS_STDLIB;IS_STDLIB_CORE;IS_SDK_OVERLAY;EMBED_BITCODE"
      "SDK;ARCHITECTURE;INSTALL_IN_COMPONENT"
      "DEPENDS;COMPILE_FLAGS;MODULE_NAME"
      ${ARGN})
   polar_translate_flag(${POLARSOURCES_IS_MAIN} "IS_MAIN" IS_MAIN_arg)
   polar_translate_flag(${POLARSOURCES_IS_STDLIB} "IS_STDLIB" IS_STDLIB_arg)
   polar_translate_flag(${POLARSOURCES_IS_STDLIB_CORE} "IS_STDLIB_CORE"
      IS_STDLIB_CORE_arg)
   polar_translate_flag(${POLARSOURCES_IS_SDK_OVERLAY} "IS_SDK_OVERLAY"
      IS_SDK_OVERLAY_arg)
   polar_translate_flag(${POLARSOURCES_EMBED_BITCODE} "EMBED_BITCODE"
      EMBED_BITCODE_arg)

   if(POLARSOURCES_IS_MAIN)
      set(POLARSOURCES_INSTALL_IN_COMPONENT never_install)
   endif()

   # Check arguments.
   polar_precondition(POLARSOURCES_SDK "Should specify an SDK")
   polar_precondition(POLARSOURCES_ARCHITECTURE "Should specify an architecture")
   polar_precondition(POLARSOURCES_INSTALL_IN_COMPONENT "INSTALL_IN_COMPONENT is required")

   # Clear the result variable.
   set("${dependency_target_out_var_name}" "" PARENT_SCOPE)
   set("${dependency_module_target_out_var_name}" "" PARENT_SCOPE)
   set("${dependency_pib_target_out_var_name}" "" PARENT_SCOPE)
   set("${dependency_pibopt_target_out_var_name}" "" PARENT_SCOPE)
   set("${dependency_pibgen_target_out_var_name}" "" PARENT_SCOPE)

   set(result)
   set(polar_sources)
   foreach(src ${${sourcesvar}})
      get_filename_component(extension ${src} EXT)
      if(extension STREQUAL ".php")
         list(APPEND polar_sources ${src})
      else()
         list(APPEND result ${src})
      endif()
   endforeach()

   set(polar_compile_flags ${POLARSOURCES_COMPILE_FLAGS})
   if (NOT POLARSOURCES_IS_MAIN)
      list(APPEND polar_compile_flags "-module-link-name" "${name}")
   endif()

   if(polar_sources)
      set(objsubdir "/${POLARSOURCES_SDK}/${POLARSOURCES_ARCHITECTURE}")

      file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}${objsubdir}")

      set(polar_obj
         "${CMAKE_CURRENT_BINARY_DIR}${objsubdir}/${POLARSOURCES_MODULE_NAME}${CMAKE_C_OUTPUT_EXTENSION}")

      # FIXME: We shouldn't /have/ to build things in a single process.
      # <rdar://problem/15972329>
      list(APPEND polar_compile_flags "-force-single-frontend-invocation")

      _compile_typephp_files(
         dependency_target
         module_dependency_target
         pib_dependency_target
         pibopt_dependency_target
         pibgen_dependency_target
         OUTPUT ${polar_obj}
         SOURCES ${polar_sources}
         DEPENDS ${POLARSOURCES_DEPENDS}
         FLAGS ${polar_compile_flags}
         SDK ${POLARSOURCES_SDK}
         ARCHITECTURE ${POLARSOURCES_ARCHITECTURE}
         MODULE_NAME ${POLARSOURCES_MODULE_NAME}
         ${IS_MAIN_arg}
         ${IS_STDLIB_arg}
         ${IS_STDLIB_CORE_arg}
         ${IS_SDK_OVERLAY_arg}
         ${EMBED_BITCODE_arg}
         ${STATIC_arg}
         INSTALL_IN_COMPONENT "${POLARSOURCES_INSTALL_IN_COMPONENT}")
      set("${dependency_target_out_var_name}" "${dependency_target}" PARENT_SCOPE)
      set("${dependency_module_target_out_var_name}" "${module_dependency_target}" PARENT_SCOPE)
      set("${dependency_pib_target_out_var_name}" "${pib_dependency_target}" PARENT_SCOPE)
      set("${dependency_pibopt_target_out_var_name}" "${pibopt_dependency_target}" PARENT_SCOPE)
      set("${dependency_pibgen_target_out_var_name}" "${pibgen_dependency_target}" PARENT_SCOPE)

      list(APPEND result ${polar_obj})
   endif()

   llvm_process_sources(result ${result})
   set(${sourcesvar} "${result}" PARENT_SCOPE)
   set(${externalvar} ${polar_sources} PARENT_SCOPE)
endfunction()

# Add Typephp source files to the (Xcode) project.
#
# Usage:
#   add_typephp_source_group(sources)
function(add_typephp_source_group sources)
   source_group("Typephp Sources" FILES ${sources})
   # Mark the source files as HEADER_FILE_ONLY, so that Xcode doesn't try
   # to build them itself.
   set_source_files_properties(${sources}
      PROPERTIES
      HEADER_FILE_ONLY true)
endfunction()

# Compile a typephp file into an object file (as a library).
#
# Usage:
#   _compile_typephp_files(
#     dependency_target_out_var_name
#     dependency_module_target_out_var_name
#     dependency_pib_target_out_var_name    # -Onone pib target
#     dependency_pibopt_target_out_var_name # -O pib target
#     dependency_pibgen_target_out_var_name # -pibgen target
#     OUTPUT objfile                    # Name of the resulting object file
#     SOURCES typephp_src [typephp_src...]  # typephp source files to compile
#     FLAGS -module-name foo            # Flags to add to the compilation
#     [SDK sdk]                         # SDK to build for
#     [ARCHITECTURE architecture]       # Architecture to build for
#     [DEPENDS cmake_target...]         # CMake targets on which the object
#                                       # file depends.
#     [IS_MAIN]                         # This is an executable, not a library
#     [IS_STDLIB]
#     [IS_STDLIB_CORE]                  # This is the core standard library
#     [OPT_FLAGS]                       # Optimization flags (overrides POLAR_OPTIMIZE)
#     [MODULE_DIR]                      # Put .phpmodule, .phpdoc., and .o
#                                       # into this directory.
#     [MODULE_NAME]                     # The module name.
#     [INSTALL_IN_COMPONENT]            # Install produced files.
#     [EMBED_BITCODE]                   # Embed LLVM bitcode into the .o files
#     )
function(_compile_typephp_files
   dependency_target_out_var_name dependency_module_target_out_var_name
   dependency_pib_target_out_var_name dependency_pibopt_target_out_var_name
   dependency_pibgen_target_out_var_name)
   cmake_parse_arguments(POLARFILE
      "IS_MAIN;IS_STDLIB;IS_STDLIB_CORE;IS_SDK_OVERLAY;EMBED_BITCODE"
      "OUTPUT;MODULE_NAME;INSTALL_IN_COMPONENT"
      "SOURCES;FLAGS;DEPENDS;SDK;ARCHITECTURE;OPT_FLAGS;MODULE_DIR"
      ${ARGN})

   # Check arguments.
   list(LENGTH POLARFILE_OUTPUT num_outputs)
   list(GET POLARFILE_OUTPUT 0 first_output)

   precondition(num_outputs MESSAGE "OUTPUT must not be empty")

   foreach(output ${POLARFILE_OUTPUT})
      if (NOT IS_ABSOLUTE "${output}")
         message(FATAL_ERROR "OUTPUT should be an absolute path")
      endif()
   endforeach()

   if(POLARFILE_IS_MAIN AND POLARFILE_IS_STDLIB)
      message(FATAL_ERROR "Cannot set both IS_MAIN and IS_STDLIB")
   endif()

   precondition(POLARFILE_SDK MESSAGE "Should specify an SDK")
   precondition(POLARFILE_ARCHITECTURE MESSAGE "Should specify an architecture")
   precondition(POLARFILE_INSTALL_IN_COMPONENT MESSAGE "INSTALL_IN_COMPONENT is required")

   if ("${POLARFILE_MODULE_NAME}" STREQUAL "")
      get_filename_component(POLARFILE_MODULE_NAME "${first_output}" NAME_WE)
      message(SEND_ERROR
         "No module name specified; did you mean to use MODULE_NAME "
         "${POLARFILE_MODULE_NAME}?")
   endif()

   set(source_files)
   foreach(file ${POLARFILE_SOURCES})
      # Determine where this file is.
      get_filename_component(file_path ${file} PATH)
      if(IS_ABSOLUTE "${file_path}")
         list(APPEND source_files "${file}")
      else()
         list(APPEND source_files "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
      endif()
   endforeach()

   # Compute flags for the Swift compiler.
   set(polar_flags)
   set(polar_module_flags)

   _add_variant_polar_compile_flags(
      "${POLARFILE_SDK}"
      "${POLARFILE_ARCHITECTURE}"
      "${POLAR_STDLIB_BUILD_TYPE}"
      "${POLAR_STDLIB_ASSERTIONS}"
      polar_flags)

   # Determine the subdirectory where the binary should be placed.
   compute_library_subdir(library_subdir
      "${POLARFILE_SDK}" "${POLARFILE_ARCHITECTURE}")

   # Allow import of other Swift modules we just built.
   list(APPEND polar_flags
      "-I" "${POLARLIB_DIR}/${library_subdir}")
   # FIXME: should we use '-resource-dir' here?  Seems like it has no advantage
   # over '-I' in this case.

   # If we have a custom module cache path, use it.
   if (POLAR_MODULE_CACHE_PATH)
      list(APPEND polar_flags "-module-cache-path" "${POLAR_MODULE_CACHE_PATH}")
   endif()

   # Don't include libarclite in any build products by default.
   list(APPEND polar_flags "-no-link-objc-runtime")

   if(POLAR_PIL_VERIFY_ALL)
      list(APPEND polar_flags "-Xfrontend" "-pil-verify-all")
   endif()

   # The standard library and overlays are always built resiliently.
   if(POLARFILE_IS_STDLIB)
      list(APPEND polar_flags "-enable-library-evolution")
      list(APPEND polar_flags "-Xfrontend" "-enable-ownership-stripping-after-serialization")
   endif()

   if(POLAR_STDLIB_USE_NONATOMIC_RC)
      list(APPEND polar_flags "-Xfrontend" "-assume-single-threaded")
   endif()

   if(NOT POLAR_ENABLE_STDLIBCORE_EXCLUSIVITY_CHECKING AND POLARFILE_IS_STDLIB)
      list(APPEND polar_flags "-Xfrontend" "-enforce-exclusivity=unchecked")
   endif()

   if(POLAR_EMIT_SORTED_PIL_OUTPUT)
      list(APPEND polar_flags "-Xfrontend" "-emit-sorted-pil")
   endif()

   # FIXME: Cleaner way to do this?
   if(POLARFILE_IS_STDLIB_CORE)
      list(APPEND polar_flags
         "-nostdimport" "-parse-stdlib" "-module-name" "Polar")
      list(APPEND polar_flags "-Xfrontend" "-group-info-path"
         "-Xfrontend" "${GROUP_INFO_JSON_FILE}")
   else()
      list(APPEND polar_flags "-module-name" "${POLARFILE_MODULE_NAME}")
   endif()

   # Force swift 5 mode for Standard Library and overlays.
   if (POLARFILE_IS_STDLIB)
      list(APPEND polar_flags "-polarphp-version" "5")
   endif()
   if (POLARFILE_IS_SDK_OVERLAY)
      list(APPEND polar_flags "-polarphp-version" "5")
   endif()

   if(POLARFILE_IS_SDK_OVERLAY)
      list(APPEND polar_flags "-autolink-force-load")
   endif()

   # Don't need to link runtime compatibility libraries for older runtimes
   # into the new runtime.
   if (POLARFILE_IS_STDLIB OR POLARFILE_IS_SDK_OVERLAY)
      list(APPEND polar_flags "-runtime-compatibility-version" "none")
      list(APPEND polar_flags "-disable-autolinking-runtime-compatibility-dynamic-replacements")
   endif()

   if (POLARFILE_IS_STDLIB_CORE OR POLARFILE_IS_SDK_OVERLAY)
      list(APPEND polar_flags "-warn-polarphp3-objc-inference-complete")
   endif()

   list(APPEND polar_flags ${POLAR_EXPERIMENTAL_EXTRA_FLAGS})

   if(POLARFILE_OPT_FLAGS)
      list(APPEND polar_flags ${POLARFILE_OPT_FLAGS})
   endif()

   list(APPEND polar_flags ${POLARFILE_FLAGS})

   set(dirs_to_create)
   foreach(output ${POLARFILE_OUTPUT})
      get_filename_component(objdir "${output}" PATH)
      list(APPEND dirs_to_create "${objdir}")
   endforeach()

   set(module_file)
   set(module_doc_file)
   set(interface_file)

   if(NOT POLARFILE_IS_MAIN)
      # Determine the directory where the module file should be placed.
      if(POLARFILE_MODULE_DIR)
         set(module_dir "${POLARFILE_MODULE_DIR}")
      elseif(POLARFILE_IS_STDLIB)
         set(module_dir "${POLARLIB_DIR}/${library_subdir}")
      else()
         message(FATAL_ERROR "Don't know where to put the module files")
      endif()

      list(APPEND polar_flags "-parse-as-library")

      set(module_base "${module_dir}/${POLARFILE_MODULE_NAME}")
      if(POLARFILE_SDK IN_LIST POLAR_APPLE_PLATFORMS)
         set(specific_module_dir "${module_base}.swiftmodule")
         set(specific_module_project_dir "${specific_module_dir}/Project")
         set(source_info_file "${specific_module_project_dir}/${POLARFILE_ARCHITECTURE}.swiftsourceinfo")
         set(module_base "${module_base}.phpmodule/${POLARFILE_ARCHITECTURE}")
      else()
         set(specific_module_dir)
         set(specific_module_project_dir)
         set(source_info_file "${module_base}.swiftsourceinfo")
      endif()
      set(module_file "${module_base}.phpmodule")
      set(module_doc_file "${module_base}.phpdoc")

      # FIXME: These don't really belong inside the swiftmodule, but there's not
      # an obvious alternate place to put them.
      set(pib_file "${module_base}.Onone.pib")
      set(pibopt_file "${module_base}.O.pib")
      set(pibgen_file "${module_base}.pibgen")

      if(POLAR_ENABLE_MODULE_INTERFACES)
         set(interface_file "${module_base}.phpinterface")
         list(APPEND polar_module_flags
            "-emit-module-interface-path" "${interface_file}")
      endif()

      if (NOT POLARFILE_IS_STDLIB_CORE)
         list(APPEND polar_module_flags
            "-Xfrontend" "-experimental-skip-non-inlinable-function-bodies")
      endif()

      # If we have extra regexp flags, check if we match any of the regexps. If so
      # add the relevant flags to our polar_flags.
      if (POLAR_EXPERIMENTAL_EXTRA_REGEXP_FLAGS OR POLAR_EXPERIMENTAL_EXTRA_NEGATIVE_REGEXP_FLAGS)
         set(extra_polar_flags_for_module)
         _add_extra_polar_flags_for_module("${POLARFILE_MODULE_NAME}" extra_polar_flags_for_module)
         if (extra_polar_flags_for_module)
            list(APPEND polar_flags ${extra_polar_flags_for_module})
         endif()
      endif()
   endif()

   set(module_outputs "${module_file}" "${module_doc_file}")
   if(interface_file)
      list(APPEND module_outputs "${interface_file}")
   endif()

   if(POLARFILE_SDK IN_LIST POLAR_APPLE_PLATFORMS)
      polar_install_in_component(DIRECTORY "${specific_module_dir}"
         DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/polarphp/${library_subdir}"
         COMPONENT "${POLARFILE_INSTALL_IN_COMPONENT}"
         OPTIONAL
         PATTERN "Project" EXCLUDE)
   else()
      polar_install_in_component(FILES ${module_outputs}
         DESTINATION "lib${LLVM_LIBDIR_SUFFIX}/polarphp/${library_subdir}"
         COMPONENT "${POLARFILE_INSTALL_IN_COMPONENT}")
   endif()

   set(line_directive_tool "${POLAR_SOURCE_DIR}/utils/line-directive")
   set(polar_compiler_tool "${POLAR_NATIVE_POLAR_TOOLS_PATH}/polarphpc")
   set(polar_compiler_tool_dep)
   if(POLAR_INCLUDE_TOOLS)
      # Depend on the binary itself, in addition to the symlink.
      set(polar_compiler_tool_dep "polarphp")
   endif()

   # If there are more than one output files, we assume that they are specified
   # otherwise e.g. with an output file map.
   set(output_option)
   if (${num_outputs} EQUAL 1)
      set(output_option "-o" ${first_output})
   endif()

   set(embed_bitcode_option)
   if (POLARFILE_EMBED_BITCODE)
      set(embed_bitcode_option "-embed-bitcode")
   endif()

   set(main_command "-c")
   if (POLAR_CHECK_INCREMENTAL_COMPILATION)
      set(swift_compiler_tool "${POLAR_SOURCE_DIR}/utils/check-incremental" "${polar_compiler_tool}")
   endif()

   if (POLAR_REPORT_STATISTICS)
      list(GET dirs_to_create 0 first_obj_dir)
      list(APPEND polar_flags "-stats-output-dir" ${first_obj_dir})
   endif()

   set(standard_outputs ${POLARFILE_OUTPUT})
   set(pib_outputs "${pib_file}")
   set(pibopt_outputs "${pibopt_file}")
   set(pibgen_outputs "${pibgen_file}")

   if(XCODE)
      # HACK: work around an issue with CMake Xcode generator and the Swift
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
   if(POLARFILE_IS_STDLIB_CORE)
      set(POLARFILE_PLATFORM "${POLAR_SDK_${POLARFILE_SDK}_LIB_SUBDIR}")
      set(copy_legacy_layouts_dep
         "copy-legacy-layouts-${POLARFILE_PLATFORM}-${POLARFILE_ARCHITECTURE}")
   else()
      set(copy_legacy_layouts_dep)
   endif()

   add_custom_command_target(
      dependency_target
      COMMAND
      "${PYTHON_EXECUTABLE}" "${line_directive_tool}" "@${file_path}" --
      "${polar_compiler_tool}" "${main_command}" ${polar_flags}
      ${output_option} ${embed_bitcode_option} "@${file_path}"
      ${command_touch_standard_outputs}
      OUTPUT ${standard_outputs}
      DEPENDS
      ${polar_compiler_tool_dep}
      ${file_path} ${source_files} ${POLARFILE_DEPENDS}
      ${polar_ide_test_dependency}
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
   if (NOT POLARFILE_IS_MAIN)
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
         "${polar_compiler_tool}" "-emit-module" "-o" "${module_file}"
         "-emit-module-source-info-path" "${source_info_file}"
         ${polar_flags} ${polar_module_flags} "@${file_path}"
         ${command_touch_module_outputs}
         OUTPUT ${module_outputs}
         DEPENDS
         ${polar_compiler_tool_dep}
         ${source_files} ${POLARFILE_DEPENDS}
         ${polar_ide_test_dependency}
         ${create_dirs_dependency_target}
         COMMENT "Generating ${module_file}")
      set("${dependency_module_target_out_var_name}" "${module_dependency_target}" PARENT_SCOPE)

      # This is the target to generate the .pib files. It is not built by default.
      add_custom_command_target(
         pib_dependency_target
         COMMAND
         "${PYTHON_EXECUTABLE}" "${line_directive_tool}" "@${file_path}" --
         "${polar_compiler_tool}" "-emit-pib" "-o" "${pib_file}" ${polar_flags} -Onone
         "@${file_path}"
         ${command_touch_pib_outputs}
         OUTPUT ${pib_outputs}
         DEPENDS
         ${polar_compiler_tool_dep}
         ${source_files} ${POLARFILE_DEPENDS}
         ${create_dirs_dependency_target}
         COMMENT "Generating ${pib_file}"
         EXCLUDE_FROM_ALL)
      set("${dependency_pib_target_out_var_name}" "${pib_dependency_target}" PARENT_SCOPE)

      add_custom_command_target(
         pibopt_dependency_target
         COMMAND
         "${PYTHON_EXECUTABLE}" "${line_directive_tool}" "@${file_path}" --
         "${polar_compiler_tool}" "-emit-pib" "-o" "${pibopt_file}" ${polar_flags} -O
         "@${file_path}"
         ${command_touch_pibopt_outputs}
         OUTPUT ${pibopt_outputs}
         DEPENDS
         ${polar_compiler_tool_dep}
         ${source_files} ${POLARFILE_DEPENDS}
         ${create_dirs_dependency_target}
         COMMENT "Generating ${pibopt_file}"
         EXCLUDE_FROM_ALL)
      set("${dependency_pibopt_target_out_var_name}" "${pibopt_dependency_target}" PARENT_SCOPE)

      # This is the target to generate the .pibgen files. It is not built by default.
      add_custom_command_target(
         pibgen_dependency_target
         COMMAND
         "${PYTHON_EXECUTABLE}" "${line_directive_tool}" "@${file_path}" --
         "${polar_compiler_tool}" "-emit-pibgen" "-o" "${pibgen_file}" ${polar_flags}
         "@${file_path}"
         ${command_touch_pibgen_outputs}
         OUTPUT ${pibgen_outputs}
         DEPENDS
         ${polar_compiler_tool_dep}
         ${source_files} ${POLARFILE_DEPENDS}
         ${create_dirs_dependency_target}
         COMMENT "Generating ${pibgen_file}"
         EXCLUDE_FROM_ALL)
      set("${dependency_pibgen_target_out_var_name}" "${pibgen_dependency_target}" PARENT_SCOPE)
   endif()


   # Make sure the build system knows the file is a generated object file.
   set_source_files_properties(${POLARFILE_OUTPUT}
      PROPERTIES
      GENERATED true
      EXTERNAL_OBJECT true
      LANGUAGE C
      OBJECT_DEPENDS "${source_files}")
endfunction()
