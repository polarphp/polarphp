# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
function(find_toolchain_tool result_var_name toolchain tool)
  execute_process(
     COMMAND "xcrun" "--toolchain" "${toolchain}" "--find" "${tool}"
     OUTPUT_VARIABLE tool_path
     OUTPUT_STRIP_TRAILING_WHITESPACE)
  set("${result_var_name}" "${tool_path}" PARENT_SCOPE)
endfunction()
