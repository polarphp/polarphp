
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was googletestConfig.cmake.in                            ########

set(PACKAGE_PREFIX_DIR ${POLAR_DEPS_INSTALL_DIR})

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

set(googletest_BUILD_SHARED_LIBS ON)

set(googletest_NAMESPACE_TARGETS)
set(googletest_ALL_INCLUDE_DIRS)

foreach(target gtest;gtest_main;gmock;gmock_main)
  include(${CMAKE_CURRENT_LIST_DIR}/${target}ConfigInternal.cmake)

  add_library(googletest::${target} INTERFACE IMPORTED)
  set_target_properties(googletest::${target}
    PROPERTIES
      INTERFACE_LINK_LIBRARIES googletest_${target}
      IMPORTED_GLOBAL ON)
  if(googletest_BUILD_SHARED_LIBS)
    set_target_properties(googletest::${target}
      PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
  endif()
  add_library(${target} ALIAS googletest::${target})

  get_target_property(${target}_INCLUDE_DIRS googletest_${target} INTERFACE_INCLUDE_DIRECTORIES)

  list(APPEND googletest_ALL_INCLUDE_DIRS ${${target}_INCLUDE_DIRS})
  list(APPEND googletest_NAMESPACE_TARGETS googletest::${target})
endforeach()

list(REMOVE_DUPLICATES googletest_ALL_INCLUDE_DIRS)
set(GOOGLETEST_INCLUDE_DIRS ${googletest_ALL_INCLUDE_DIRS})

list(REMOVE_DUPLICATES googletest_NAMESPACE_TARGETS)
set(GOOGLETEST_LIBRARIES ${googletest_NAMESPACE_TARGETS})

set(GOOGLETEST_VERSION "1.9.0")
