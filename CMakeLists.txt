# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarPHP software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
#
# Created by polarboy on 2018/08/17.

cmake_minimum_required(VERSION 3.10.2)

project(polarphp C CXX)

# Don't allow in-source builds
if(PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
   message(FATAL_ERROR "Please set your build dir outside your source dir and try again.")
endif(PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)

# setup some important path

set(POLAR_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(POLAR_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(POLAR_THIRDPARTY_DIR "${POLAR_SOURCE_DIR}/thirdparty")
set(POLAR_DEPS_INSTALL_DIR "${POLAR_BINARY_DIR}/depslibs")
set(POLAR_DEPS_CMAKE_FIND_PATH "${POLAR_DEPS_INSTALL_DIR}/lib/cmake")
set(POLAR_CMAKE_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
set(POLAR_MAIN_INCLUDE_DIR "${POLAR_SOURCE_DIR}/include")
set(POLAR_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
set(POLAR_TEST_DIR "${POLAR_SOURCE_DIR}/tests")
set(POLAR_CMAKE_TEST_CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake/codes)
set(POLAR_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_NAME})
string(TOLOWER ${POLAR_SYSTEM_NAME} POLAR_SYSTEM_NORMAL_NAME)
set(POLAR_BUILD_TOOL_DIR ${POLAR_SOURCE_DIR}/build)
set(POLAR_CONFIGURE_TEMP_DIR ${POLAR_BINARY_DIR}/conf_temp)

set(POLAR_RUNTIME_OUTPUT_INTDIR "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/bin")
set(POLAR_LIBRARY_OUTPUT_INTDIR "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib")

# add third party library install dir into find paths
set(CMAKE_FIND_ROOT_PATH "${POLAR_DEPS_INSTALL_DIR} ${CMAKE_FIND_ROOT_PATH}")

# Add path for custom CMake modules.
list(APPEND CMAKE_MODULE_PATH
   "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
   "${POLAR_CMAKE_MODULES_DIR}")

# Make a job pool for things that can't yet be distributed
cmake_host_system_information(
   RESULT localhost_logical_cores QUERY NUMBER_OF_LOGICAL_CORES)
set_property(GLOBAL PROPERTY JOB_POOLS local_jobs=${localhost_logical_cores})
# Put linking in that category
set_property(GLOBAL PROPERTY JOB_POOL_LINK local_jobs)

# First include general CMake utilities.
include(PolarUtils)
include(SummaryOutput)
include(ProcessVersion)
include(ThreadUtils)
include(CheckUtils)
include(AddDefinitions)
include(AddUnittests)
include(GetHostTriple)
include(TargetUtils)

set(CMAKE_C_STANDARD 11)

# setup options
option(POLAR_ENABLE_RE2C_CGOTO
   "re2c-cgoto, whether to enable computed goto gcc extension with re2c"
   OFF)

option(POLAR_ENABLE_LIBGCC
   "Enable explicitly linking against libgcc"
   OFF)

option(POLAR_WITH_VALGRIND
   "whether to enable valgrind support"
   OFF)
set(POLAR_VALGRIND_DIR "" CACHE string "the valgrind directory")

option(POLAR_ENABLE_GCOV
   "Enable GCOV code coverage (requires LTP) - FOR DEVELOPERS ONLY!!"
   OFF)

option(POLAR_DISABLE_INLINE_OPTIMIZATION
   "If building zend_execute.lo fails, try this switch"
   OFF)

set(POLAR_CONFIG_FILE_PATH "Default" CACHE string "Set the path in which to look for php.ini")
set(POLAR_CONFIG_FILE_SCAN_DIR "Default" CACHE string "Set the path where to scan for configuration files")

option(POLAR_ENABLE_SIGCHILD
   "whether to enable PHP's own SIGCHLD handler"
   OFF)

option(POLAR_ENABLE_DMALLOC
   "whether to enable dmalloc"
   OFF)

option(POLAR_ENABLE_IPV6
   "whether to enable IPv6 support"
   OFF)

##
## DTRACE CHECKS
##
option(POLAR_ENABLE_DTRACE
   "whether to enable DTrace support"
   OFF)

option(POLAR_ENABLE_FD_SETSIZE
   "how big to make fd sets"
   OFF)

option(POLAR_DISABLE_ZEND_SIGNALS
   "whether to enable zend signal handling"
   OFF)

set(POLAR_FD_SETSIZE 7168 CACHE STRING "how big to make fd sets")

option(POLAR_TSRM_USE_PTH
   "Use GNU Pth"
   OFF)

option(POLAR_TSRM_USE_PTHREADS
   "Use pthread implementation"
   ON)

option(POLAR_DISABLE_GCC_GLOABL_REGS
   "whether to enable GCC global register variables"
   OFF)

set(POLAR_WITH_TSRM_PTH_CONFIG "pth-config" CACHE FILEPATH "Use GNU Pth")

set(POLAR_COMPILE_DEFINITIONS "")
set(POLAR_COMPILE_FLAGS "")
set(POLAR_COMMON_DEPENDS "")

# setup php version info
polar_setup_php_version()

message("build polarphp version: ${POLAR_PACKAGE_VERSION}")
message("build polarphp version id: ${POLAR_VERSION_ID}")

set(POLAR_PACKAGE_NAME polarphp)
set(POLAR_PACKAGE_STRING "${POLAR_PACKAGE_NAME} ${POLAR_PACKAGE_VERSION}")
set(POLAR_BUG_REPORT_URL "http://polarphp.org/bugs/")
set(PACKAGE_NAME ${POLAR_PACKAGE_NAME})
set(PACKAGE_STRING ${POLAR_PACKAGE_STRING})
set(PACKAGE_BUGREPORT ${POLAR_BUG_REPORT_URL})
set(PACKAGE_VERSION ${POLAR_PACKAGE_VERSION})

# setup php build sh tool path
set(POLAR_SHTOOL ${POLAR_BUILD_TOOL_DIR}/shtool)

if (NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Debug")
endif()

string(TOLOWER ${CMAKE_BUILD_TYPE} POLAR_BUILD_TYPE)
string(TOUPPER ${CMAKE_BUILD_TYPE} POLAR_BUILD_CONFIG)

# add /usr/local/bin to PATH Env var
set(ENV{PATH} "/usr/local/bin:$ENV{PATH}")

if (NOT EXISTS POLAR_CONFIGURE_TEMP_DIR)
   file(MAKE_DIRECTORY ${POLAR_CONFIGURE_TEMP_DIR})
endif()

include(CheckCompilerVersion)
include(DetermineGCCCompatible)
include(HandlePolarOptions)
include(ProcesseRequirePackages)
include(DetectHostEnvironment)

# Configuring Zend and TSRM.
include(CheckTsrm)
include(CheckZend)

message("${POLAR_COMPILE_DEFINITIONS}")
#add_compile_definitions(${POLAR_COMPILE_DEFINITIONS})

string(TIMESTAMP PHP_BUILD_DATE "%Y-%m-%d %H:%M")
set(PHP_OS ${POLAR_SYSTEM_NAME})

include_directories(${POLAR_SOURCE_DIR}
   ${POLAR_SOURCE_DIR}/global
   ${POLAR_SOURCE_DIR}/Zend
   ${POLAR_SOURCE_DIR}/TSRM
   ${POLAR_BINARY_DIR}
   ${POLAR_BINARY_DIR}/global
   ${POLAR_BINARY_DIR}/Zend
   ${POLAR_BINARY_DIR}/TSRM)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(ImportThirdparty)

add_subdirectory(Zend)
add_subdirectory(polarmain)
add_subdirectory(TSRM)
add_subdirectory(polarapi)

# configure header
configure_file(${POLAR_SOURCE_DIR}/global/php_config.h.cmake ${POLAR_BINARY_DIR}/global/php_config.h
   @ONLY)
configure_file(${POLAR_SOURCE_DIR}/TSRM/tsrm_config.h.cmake ${POLAR_BINARY_DIR}/TSRM/tsrm_config.h
   @ONLY)
configure_file(${POLAR_SOURCE_DIR}/Zend/zend_config.h.cmake ${POLAR_BINARY_DIR}/Zend/zend_config.h
   @ONLY)

polar_compile_env_summary_output()
