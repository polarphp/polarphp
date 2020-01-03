#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "cmark" for configuration "Release"
set_property(TARGET cmark APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(cmark PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/cmark"
  )

list(APPEND _IMPORT_CHECK_TARGETS cmark )
list(APPEND _IMPORT_CHECK_FILES_FOR_cmark "${_IMPORT_PREFIX}/bin/cmark" )

# Import target "libcmark" for configuration "Release"
set_property(TARGET libcmark APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libcmark PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libcmark.0.29.0.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libcmark.0.29.0.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS libcmark )
list(APPEND _IMPORT_CHECK_FILES_FOR_libcmark "${_IMPORT_PREFIX}/lib/libcmark.0.29.0.dylib" )

# Import target "libcmark_static" for configuration "Release"
set_property(TARGET libcmark_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libcmark_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libcmark.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS libcmark_static )
list(APPEND _IMPORT_CHECK_FILES_FOR_libcmark_static "${_IMPORT_PREFIX}/lib/libcmark.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
