/*                                                                -*- C -*-
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2018 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Stig Sæther Bakken <ssb@php.net>                             |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#cmakedefine CONFIGURE_COMMAND "@CONFIGURE_COMMAND@"
#cmakedefine PHP_CFLAGS		"@CFLAGS@"
#cmakedefine PHP_BUILD_DEBUG		"@DEBUG_CFLAGS@"
#cmakedefine PHP_INSTALL_IT		"@INSTALL_IT@"
#cmakedefine PHP_PROG_SENDMAIL	"@PROG_SENDMAIL@"
#cmakedefine PEAR_INSTALLDIR         "@EXPANDED_PEAR_INSTALLDIR@"
#cmakedefine PHP_INCLUDE_PATH	"@INCLUDE_PATH@"
#cmakedefine PHP_EXTENSION_DIR       "@EXPANDED_EXTENSION_DIR@"
#cmakedefine PHP_PREFIX              "@prefix@"
#cmakedefine PHP_BINDIR              "@EXPANDED_BINDIR@"
#cmakedefine PHP_SBINDIR             "@EXPANDED_SBINDIR@"
#cmakedefine PHP_MANDIR              "@EXPANDED_MANDIR@"
#cmakedefine PHP_LIBDIR              "@EXPANDED_LIBDIR@"
#cmakedefine PHP_DATADIR             "@EXPANDED_DATADIR@"
#cmakedefine PHP_SYSCONFDIR          "@EXPANDED_SYSCONFDIR@"
#cmakedefine PHP_LOCALSTATEDIR       "@EXPANDED_LOCALSTATEDIR@"
#cmakedefine PHP_CONFIG_FILE_PATH    "@EXPANDED_PHP_CONFIG_FILE_PATH@"
#cmakedefine PHP_CONFIG_FILE_SCAN_DIR    "@EXPANDED_PHP_CONFIG_FILE_SCAN_DIR@"
#cmakedefine PHP_SHLIB_SUFFIX        "@SHLIB_DL_SUFFIX_NAME@"
#cmakedefine PHP_SHLIB_EXT_PREFIX    ""
