# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/10/12.

# just for expendiently edit

set(FILE_TYPES "*[.php|.yaml|.json]")

set(POLAR_GYB_SOURCES "")

file(GLOB_RECURSE tempFiles
   LIST_DIRECTORIES false
   src/${FILE_TYPES})

list(APPEND POLAR_GYB_SOURCES ${tempFiles})

file(GLOB_RECURSE tempFiles
   LIST_DIRECTORIES false
   config/${FILE_TYPES})

list(APPEND POLAR_GYB_SOURCES ${tempFiles})

add_custom_target(
   gyb
   SOURCES ${POLAR_GYB_SOURCES})

