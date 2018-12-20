# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/12/20.

# when in debug mode, we copy php.ini at configure cycle
#
if (POLAR_DEBUG_BUILD)
   file(COPY ${POLAR_ASSET_DIR}/php.ini-development
      DESTINATION ${POLARPHP_CONFIG_FILE_PATH}/php.ini)
endif()

install(FILES ${POLAR_ASSET_DIR}/php.ini-production
   DESTINATION ${POLARPHP_CONFIG_FILE_PATH}
   CONFIGURATIONS Release
   COMPONENT data
   RENAME php.ini)
