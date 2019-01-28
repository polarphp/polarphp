# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/12/10.

list(APPEND POLAR_ZENDVM_INCLUDE_DIRS
   ${POLAR_MAIN_INCLUDE_DIR}/polarphp/global
   ${POLAR_MAIN_INCLUDE_DIR}/polarphp/vm/zend
   ${POLAR_MAIN_INCLUDE_DIR}/polarphp/vm/tsrm
   ${POLAR_SOURCE_DIR}/src/vm/TSRM
   ${POLAR_SOURCE_DIR}/src/vm/Zend
   ${POLAR_INCLUDE_DIR}/polarphp/global
   ${POLAR_INCLUDE_DIR}/polarphp/vm/zend
   ${POLAR_INCLUDE_DIR}/polarphp/vm/tsrm)

add_subdirectory(TSRM)
add_subdirectory(Zend)

set(ZENDAPI_SRCS
   AbstractClass.cpp
   AbstractMember.cpp
   BooleanMember.cpp
   Callable.cpp
   Closure.cpp
   FloatMember.cpp
   IteratorBridge.cpp
   NullMember.cpp
   NumericMember.cpp
   ObjectBinder.cpp
   Property.cpp
   StringMember.cpp
   ZendApi.cpp
   ZvalMember.cpp
   StdClass.cpp
   )

polar_collect_files(
   TYPE_SOURCE
   DIR ds
   OUTPUT_VAR ZENDAPI_DS_SRCS)

polar_collect_files(
   TYPE_SOURCE
   DIR lang
   OUTPUT_VAR ZENDAPI_LANG_SRCS)

polar_collect_files(
   TYPE_SOURCE
   DIR protocol
   OUTPUT_VAR ZENDAPI_PROTOCOL_SRCS)

polar_collect_files(
   TYPE_SOURCE
   DIR utils
   OUTPUT_VAR ZENDAPI_UTILS_SRCS)

polar_merge_list(ZENDAPI_SRCS ZENDAPI_DS_SRCS)
polar_merge_list(ZENDAPI_SRCS ZENDAPI_LANG_SRCS)
polar_merge_list(ZENDAPI_SRCS ZENDAPI_PROTOCOL_SRCS)
polar_merge_list(ZENDAPI_SRCS ZENDAPI_UTILS_SRCS)
polar_merge_list(ZENDAPI_SRCS POLAR_ZENDVM_ZEND_HEADERS)
polar_merge_list(ZENDAPI_SRCS POLAR_HEADERS)

polar_add_library_internal(ZendApi SHARED ${ZENDAPI_SRCS})
target_link_libraries(ZendApi PUBLIC PolarRuntime PolarGlobal)
set_target_properties(ZendApi
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${POLAR_ZENDVM_INCLUDE_DIRS}")
set(lib${POLAR_LIBDIR_SUFFIX})

install(TARGETS ZendApi LIBRARY
   DESTINATION lib${POLAR_LIBDIR_SUFFIX}
   COMPONENT corelibs)
