# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarPHP software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors

macro(polar_setup_php_version)
   find_file(configureAcFilePath configure.ac PATHS ${POLAR_SOURCE_DIR})
   if (configureAcFilePath)
      file(READ ${configureAcFilePath} configureAcContents)
      string(REGEX MATCH "PHP_MAJOR_VERSION=([0-9]*)" matchMajorVersion "${configureAcContents}")
      string(REGEX MATCH "PHP_MINOR_VERSION=([0-9]*)" matchMinorVersion "${configureAcContents}")
      string(REGEX MATCH "PHP_RELEASE_VERSION=([0-9]*)" matchReleaseVersion "${configureAcContents}")
      string(REGEX MATCH "PHP_EXTRA_VERSION=\"([^\"]*)\"" matchExtraVersion "${configureAcContents}")

      string(REPLACE "PHP_MAJOR_VERSION=" "" matchMajorVersion ${matchMajorVersion})
      string(REPLACE "PHP_MINOR_VERSION=" "" matchMinorVersion ${matchMinorVersion})
      string(REPLACE "PHP_RELEASE_VERSION=" "" matchReleaseVersion ${matchReleaseVersion})
      string(REPLACE "PHP_EXTRA_VERSION=" "" matchExtraVersion ${matchExtraVersion})
      string(REPLACE "\"" "" matchExtraVersion ${matchExtraVersion})

      set(POLAR_VERSION_MAJOR ${matchMajorVersion})
      set(POLAR_VERSION_MINOR ${matchMinorVersion})
      set(POLAR_VERSION_RELEASE ${matchReleaseVersion})
      set(POLAR_VERSION_EXTRA ${matchExtraVersion})

      set(POLAR_PACKAGE_VERSION "${POLAR_VERSION_MAJOR}.${POLAR_VERSION_MINOR}.${POLAR_VERSION_RELEASE}${POLAR_VERSION_EXTRA}")
      math(EXPR POLAR_VERSION_ID ${POLAR_VERSION_MAJOR}*10000+${POLAR_VERSION_MINOR}*100+${POLAR_VERSION_RELEASE})

      set(PHP_VERSION_MAJOR ${POLAR_VERSION_MAJOR})
      set(PHP_VERSION_MINOR ${POLAR_VERSION_MINOR})
      set(PHP_VERSION_RELEASE ${POLAR_VERSION_RELEASE})
      set(PHP_VERSION_EXTRA ${POLAR_VERSION_EXTRA})

      set(PHP_PACKAGE_VERSION ${POLAR_PACKAGE_VERSION})
      set(PHP_VERSION_ID ${POLAR_VERSION_ID})

   endif()
endmacro()
