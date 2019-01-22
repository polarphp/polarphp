// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/25.

#include "polarphp/runtime/BuildinIniModifyHandler.h"
#include "polarphp/runtime/RtDefs.h"
#include "polarphp/runtime/Ini.h"
#include "polarphp/runtime/Output.h"
#include "polarphp/runtime/ExecEnv.h"
#include "polarphp/runtime/Utils.h"

#include <string>

namespace polar {
namespace runtime {

ZEND_INI_MH(update_bool_handler)
{
   zend_bool *p;
   char *base = (char *) mh_arg2;
   p = (zend_bool *) (base+(size_t) mh_arg1);
   *p = zend_ini_parse_bool(new_value);
   return SUCCESS;
}

ZEND_INI_MH(update_long_handler)
{
   zend_long *p;
   char *base = (char *) mh_arg2;
   p = (zend_long *) (base+(size_t) mh_arg1);
   *p = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
   return SUCCESS;
}

ZEND_INI_MH(update_long_ge_zero_handler)
{
   zend_long *p, tmp;
   char *base = (char *) mh_arg2;
   tmp = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
   if (tmp < 0) {
      return FAILURE;
   }
   p = (zend_long *) (base+(size_t) mh_arg1);
   *p = tmp;
   return SUCCESS;
}

ZEND_INI_MH(update_real_handler)
{
   double *p;
   char *base;
   base = (char *) ts_resource(*((int *) mh_arg2));
   p = (double *) (base+(size_t) mh_arg1);
   *p = zend_strtod(ZSTR_VAL(new_value), nullptr);
   return SUCCESS;
}

ZEND_INI_MH(update_string_handler)
{
   std::string *p;
   char *base = (char *) mh_arg2;
   p = (std::string *) (base+(size_t) mh_arg1);
   *p = new_value ? ZSTR_VAL(new_value) : nullptr;
   return SUCCESS;
}

ZEND_INI_MH(update_string_unempty_handler)
{
   std::string *p;
   char *base = (char *) mh_arg2;
   if (new_value && !ZSTR_VAL(new_value)[0]) {
      return FAILURE;
   }
   p = (std::string *) (base+(size_t) mh_arg1);
   *p = new_value ? ZSTR_VAL(new_value) : nullptr;
   return SUCCESS;
}

namespace {

int php_get_display_errors_mode(char *value, size_t value_length)
{
   int mode;

   if (!value) {
      return PHP_DISPLAY_ERRORS_STDOUT;
   }
   if (value_length == 2 && !strcasecmp("on", value)) {
      mode = PHP_DISPLAY_ERRORS_STDOUT;
   } else if (value_length == 3 && !strcasecmp("yes", value)) {
      mode = PHP_DISPLAY_ERRORS_STDOUT;
   } else if (value_length == 4 && !strcasecmp("true", value)) {
      mode = PHP_DISPLAY_ERRORS_STDOUT;
   } else if (value_length == 6 && !strcasecmp(value, "stderr")) {
      mode = PHP_DISPLAY_ERRORS_STDERR;
   } else if (value_length == 6 && !strcasecmp(value, "stdout")) {
      mode = PHP_DISPLAY_ERRORS_STDOUT;
   } else {
      ZEND_ATOL(mode, value);
      if (mode && mode != PHP_DISPLAY_ERRORS_STDOUT && mode != PHP_DISPLAY_ERRORS_STDERR) {
         mode = PHP_DISPLAY_ERRORS_STDOUT;
      }
   }
   return mode;
}
} // anonymous namespace

POLAR_INI_DISP(display_errors_mode)
{
   int mode;
   size_t tmp_value_length;
   char *tmp_value;
   if (type == ZEND_INI_DISPLAY_ORIG && ini_entry->modified) {
      tmp_value = (ini_entry->orig_value ? ZSTR_VAL(ini_entry->orig_value) : nullptr );
      tmp_value_length = (ini_entry->orig_value? ZSTR_LEN(ini_entry->orig_value) : 0);
   } else if (ini_entry->value) {
      tmp_value = ZSTR_VAL(ini_entry->value);
      tmp_value_length = ZSTR_LEN(ini_entry->value);
   } else {
      tmp_value = NULL;
      tmp_value_length = 0;
   }
   mode = php_get_display_errors_mode(tmp_value, tmp_value_length);
   switch (mode) {
   case PHP_DISPLAY_ERRORS_STDERR:
      PUTS("STDERR");
      break;
   case PHP_DISPLAY_ERRORS_STDOUT:
      PUTS("STDOUT");
      break;
   default:
      PUTS("Off");
      break;
   }
}

POLAR_INI_MH(update_display_errors_handler)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   execEnvInfo.displayErrors = (zend_bool) php_get_display_errors_mode(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
   return SUCCESS;
}

POLAR_INI_MH(set_serialize_precision_handler)
{
   zend_long i;
   ZEND_ATOL(i, ZSTR_VAL(new_value));
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   if (i >= -1) {
      execEnvInfo.serializePrecision = i;
      return SUCCESS;
   } else {
      return FAILURE;
   }
}

POLAR_INI_MH(update_internal_encoding_handler)
{
   if (new_value) {
      update_string_handler(entry, new_value, mh_arg1, mh_arg2, mh_arg3, stage);
#ifdef POLAR_OS_WIN32
      php_win32_cp_do_update(ZSTR_VAL(new_value));
#endif
   }
   return SUCCESS;
}

POLAR_INI_MH(update_error_log_handler)
{
   /* Only do the safemode/open_basedir check at runtime */
   ///
   /// need review here
   ///
   if ((stage == POLAR_INI_STAGE_RUNTIME || stage == POLAR_INI_STAGE_HTACCESS) && new_value && strcmp(ZSTR_VAL(new_value), "syslog")) {
      ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
      if (!execEnvInfo.openBaseDir.empty() && php_check_open_basedir(ZSTR_VAL(new_value))) {
         return FAILURE;
      }
   }
   update_string_handler(entry, new_value, mh_arg1, mh_arg2, mh_arg3, stage);
   return SUCCESS;
}

POLAR_INI_MH(update_timeout_handler)
{
   if (stage == POLAR_INI_STAGE_STARTUP) {
      /* Don't set a timeout on startup, only per-request */
      ZEND_ATOL(EG(timeout_seconds), ZSTR_VAL(new_value));
      return SUCCESS;
   }
   zend_unset_timeout();
   ZEND_ATOL(EG(timeout_seconds), ZSTR_VAL(new_value));
   zend_set_timeout(EG(timeout_seconds), 0);
   return SUCCESS;
}

///
/// review for memory leak
///
ZEND_INI_MH(update_base_dir_handler)
{
   std::string *p;
   char *pathbuf, *ptr, *end;
   char *base = (char *) mh_arg2;
   p = (std::string *) (base + (size_t) mh_arg1);
   if (stage == POLAR_INI_STAGE_STARTUP || stage == POLAR_INI_STAGE_SHUTDOWN || stage == POLAR_INI_STAGE_ACTIVATE || stage == POLAR_INI_STAGE_DEACTIVATE) {
      /* We're in a PHP_INI_SYSTEM context, no restrictions */
      *p = new_value ? ZSTR_VAL(new_value) : NULL;
      return SUCCESS;
   }

   /* Otherwise we're in runtime */
   if (!p->empty()) {
      /* open_basedir not set yet, go ahead and give it a value */
      *p = ZSTR_VAL(new_value);
      return SUCCESS;
   }

   /* Shortcut: When we have a open_basedir and someone tries to unset, we know it'll fail */
   if (!new_value || !*ZSTR_VAL(new_value)) {
      return FAILURE;
   }

   /* Is the proposed open_basedir at least as restrictive as the current setting? */
   ptr = pathbuf = estrdup(ZSTR_VAL(new_value));
   while (ptr && *ptr) {
      end = strchr(ptr, DEFAULT_DIR_SEPARATOR);
      if (end != NULL) {
         *end = '\0';
         end++;
      }
      if (php_check_open_basedir(ptr, 0) != 0) {
         /* At least one portion of this open_basedir is less restrictive than the prior one, FAIL */
         efree(pathbuf);
         return FAILURE;
      }
      ptr = end;
   }
   efree(pathbuf);
   /* Everything checks out, set it */
   *p = ZSTR_VAL(new_value);
   return SUCCESS;
}

POLAR_INI_MH(change_memory_limit_handler)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   if (new_value) {
      execEnvInfo.memoryLimit = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
   } else {
      execEnvInfo.memoryLimit = Z_L(1)<<30;		/* effectively, no limit */
   }
   return zend_set_memory_limit(execEnvInfo.memoryLimit);
}

POLAR_INI_MH(set_precision_handler)
{
   zend_long i;
   ZEND_ATOL(i, ZSTR_VAL(new_value));
   if (i >= -1) {
      EG(precision) = i;
      return SUCCESS;
   } else {
      return FAILURE;
   }
}

POLAR_INI_MH(set_facility_handler)
{
   const char *facility = ZSTR_VAL(new_value);
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
#ifdef LOG_AUTH
   if (!strcmp(facility, "LOG_AUTH") || !strcmp(facility, "auth") || !strcmp(facility, "security")) {
      execEnvInfo.syslogFacility = LOG_AUTH;
      return SUCCESS;
   }
#endif
#ifdef LOG_AUTHPRIV
   if (!strcmp(facility, "LOG_AUTHPRIV") || !strcmp(facility, "authpriv")) {
      execEnvInfo.syslogFacility = LOG_AUTHPRIV;
      return SUCCESS;
   }
#endif
#ifdef LOG_CRON
   if (!strcmp(facility, "LOG_CRON") || !strcmp(facility, "cron")) {
      execEnvInfo.syslogFacility = LOG_CRON;
      return SUCCESS;
   }
#endif
#ifdef LOG_DAEMON
   if (!strcmp(facility, "LOG_DAEMON") || !strcmp(facility, "daemon")) {
      execEnvInfo.syslogFacility = LOG_DAEMON;
      return SUCCESS;
   }
#endif
#ifdef LOG_FTP
   if (!strcmp(facility, "LOG_FTP") || !strcmp(facility, "ftp")) {
      execEnvInfo.syslogFacility = LOG_FTP;
      return SUCCESS;
   }
#endif
#ifdef LOG_KERN
   if (!strcmp(facility, "LOG_KERN") || !strcmp(facility, "kern")) {
      execEnvInfo.syslogFacility = LOG_KERN;
      return SUCCESS;
   }
#endif
#ifdef LOG_LPR
   if (!strcmp(facility, "LOG_LPR") || !strcmp(facility, "lpr")) {
      execEnvInfo.syslogFacility = LOG_LPR;
      return SUCCESS;
   }
#endif
#ifdef LOG_MAIL
   if (!strcmp(facility, "LOG_MAIL") || !strcmp(facility, "mail")) {
      execEnvInfo.syslogFacility = LOG_MAIL;
      return SUCCESS;
   }
#endif
#ifdef LOG_INTERNAL_MARK
   if (!strcmp(facility, "LOG_INTERNAL_MARK") || !strcmp(facility, "mark")) {
      execEnvInfo.syslogFacility = LOG_INTERNAL_MARK;
      return SUCCESS;
   }
#endif
#ifdef LOG_NEWS
   if (!strcmp(facility, "LOG_NEWS") || !strcmp(facility, "news")) {
      execEnvInfo.syslogFacility = LOG_NEWS;
      return SUCCESS;
   }
#endif
#ifdef LOG_SYSLOG
   if (!strcmp(facility, "LOG_SYSLOG") || !strcmp(facility, "syslog")) {
      execEnvInfo.syslogFacility = LOG_SYSLOG;
      return SUCCESS;
   }
#endif
#ifdef LOG_USER
   if (!strcmp(facility, "LOG_USER") || !strcmp(facility, "user")) {
      execEnvInfo.syslogFacility = LOG_USER;
      return SUCCESS;
   }
#endif
#ifdef LOG_UUCP
   if (!strcmp(facility, "LOG_UUCP") || !strcmp(facility, "uucp")) {
      execEnvInfo.syslogFacility = LOG_UUCP;
      return SUCCESS;
   }
#endif
#ifdef LOG_LOCAL0
   if (!strcmp(facility, "LOG_LOCAL0") || !strcmp(facility, "local0")) {
      execEnvInfo.syslogFacility = LOG_LOCAL0;
      return SUCCESS;
   }
#endif
#ifdef LOG_LOCAL1
   if (!strcmp(facility, "LOG_LOCAL1") || !strcmp(facility, "local1")) {
      execEnvInfo.syslogFacility = LOG_LOCAL1;
      return SUCCESS;
   }
#endif
#ifdef LOG_LOCAL2
   if (!strcmp(facility, "LOG_LOCAL2") || !strcmp(facility, "local2")) {
      execEnvInfo.syslogFacility = LOG_LOCAL2;
      return SUCCESS;
   }
#endif
#ifdef LOG_LOCAL3
   if (!strcmp(facility, "LOG_LOCAL3") || !strcmp(facility, "local3")) {
      execEnvInfo.syslogFacility = LOG_LOCAL3;
      return SUCCESS;
   }
#endif
#ifdef LOG_LOCAL4
   if (!strcmp(facility, "LOG_LOCAL4") || !strcmp(facility, "local4")) {
      execEnvInfo.syslogFacility = LOG_LOCAL4;
      return SUCCESS;
   }
#endif
#ifdef LOG_LOCAL5
   if (!strcmp(facility, "LOG_LOCAL5") || !strcmp(facility, "local5")) {
      execEnvInfo.syslogFacility = LOG_LOCAL5;
      return SUCCESS;
   }
#endif
#ifdef LOG_LOCAL6
   if (!strcmp(facility, "LOG_LOCAL6") || !strcmp(facility, "local6")) {
      execEnvInfo.syslogFacility = LOG_LOCAL6;
      return SUCCESS;
   }
#endif
#ifdef LOG_LOCAL7
   if (!strcmp(facility, "LOG_LOCAL7") || !strcmp(facility, "local7")) {
      execEnvInfo.syslogFacility = LOG_LOCAL7;
      return SUCCESS;
   }
#endif

   return FAILURE;
}

POLAR_INI_MH(set_log_filter_handler)
{
   const char *filter = ZSTR_VAL(new_value);
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   if (!strcmp(filter, "all")) {
      execEnvInfo.syslogFilter = PHP_SYSLOG_FILTER_ALL;
      return SUCCESS;
   }
   if (!strcmp(filter, "no-ctrl")) {
      execEnvInfo.syslogFilter = PHP_SYSLOG_FILTER_NO_CTRL;
      return SUCCESS;
   }
   if (!strcmp(filter, "ascii")) {
      execEnvInfo.syslogFilter = PHP_SYSLOG_FILTER_ASCII;
      return SUCCESS;
   }
   return FAILURE;
}

} // runtime
} // polar
