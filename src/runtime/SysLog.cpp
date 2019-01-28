// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/18.

#include <cstdio>
#include <cstring>
#include <cassert>
#include <stdlib.h>

#include "polarphp/runtime/SysLog.h"
#include "polarphp/runtime/ExecEnv.h"
#include "polarphp/runtime/RtDefs.h"
#include "polarphp/runtime/internal/DepsZendVmHeaders.h"

/*
 * The SCO OpenServer 5 Development System (not the UDK)
 * defines syslog to std_syslog.
 */
#if HAVE_STD_SYSLOG
#define syslog std_syslog
#endif

namespace polar {
namespace runtime {

void php_openlog(const char *ident, int option, int facility)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   ::openlog(ident, option, facility);
   execEnvInfo.haveCalledOpenlog = true;
}

#ifdef POLAR_OS_WIN32
void php_syslog(int priority, const char *format, ...) /* {{{ */
{
   va_list args;

   /*
    * don't rely on openlog() being called by syslog() if it's
    * not already been done; call it ourselves and pass the
    * correct parameters!
    */
   if (!PG(have_called_openlog)) {
      php_openlog(execEnv.getSyslogIdent(), 0, execEnv.getSyslogFacility());
   }

   va_start(args, format);
   vsyslog(priority, format, args);
   va_end(args);
}
/* }}} */
#else
void php_syslog(int priority, const char *format, ...)
{
   /// TODO maybe memory leak here
   const char *ptr;
   unsigned char c;
   smart_string fbuf = {0};
   smart_string sbuf = {0};
   va_list args;
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   /*
    * don't rely on openlog() being called by syslog() if it's
    * not already been done; call it ourselves and pass the
    * correct parameters!
    */
   if (!execEnvInfo.haveCalledOpenlog) {
      php_openlog(execEnvInfo.syslogIdent.c_str(), 0, execEnvInfo.syslogFacility);
   }
   va_start(args, format);
   zend_printf_to_smart_string(&fbuf, format, args);
   smart_string_0(&fbuf);
   va_end(args);

   for (ptr = fbuf.c; ; ++ptr) {
      c = *ptr;
      if (c == '\0') {
         syslog(priority, "%.*s", (int)sbuf.len, sbuf.c);
         break;
      }
      /* check for NVT ASCII only unless test disabled */
      if (((0x20 <= c) && (c <= 0x7e)))
         smart_string_appendc(&sbuf, c);
      else if ((c >= 0x80) && (execEnvInfo.syslogFilter != PHP_SYSLOG_FILTER_ASCII))
         smart_string_appendc(&sbuf, c);
      else if (c == '\n') {
         syslog(priority, "%.*s", (int)sbuf.len, sbuf.c);
         smart_string_reset(&sbuf);
      } else if ((c < 0x20) && (execEnvInfo.syslogFilter == PHP_SYSLOG_FILTER_ALL))
         smart_string_appendc(&sbuf, c);
      else {
         const char xdigits[] = "0123456789abcdef";
         smart_string_appendl(&sbuf, "\\x", 2);
         smart_string_appendc(&sbuf, xdigits[(c / 0x10)]);
         c &= 0x0f;
         smart_string_appendc(&sbuf, xdigits[c]);
      }
   }

   smart_string_free(&fbuf);
   smart_string_free(&sbuf);
}
#endif

} // runtime
} // polar
