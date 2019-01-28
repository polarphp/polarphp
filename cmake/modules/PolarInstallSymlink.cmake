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

# We need to execute this script at installation time because the
# DESTDIR environment variable may be unset at configuration time.
# See PR8397.

function(polar_install_symlink name target outdir)
   if(UNIX)
      set(LINK_OR_COPY create_symlink)
      set(DESTDIR $ENV{DESTDIR})
   else()
      set(LINK_OR_COPY copy)
   endif()

   set(bindir "${DESTDIR}${CMAKE_INSTALL_PREFIX}/${outdir}/")

   message("Creating ${name}")

   execute_process(
      COMMAND "${CMAKE_COMMAND}" -E ${LINK_OR_COPY} "${target}" "${name}"
      WORKING_DIRECTORY "${bindir}")

endfunction()
