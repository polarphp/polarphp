// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/12.

#include "ExecEnv.h"
#include "polarphp/global/CompilerFeature.h"
#include "polarphp/global/Config.h"
#include "Defs.h"

#include <cstdio>

#ifdef POLAR_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef POLAR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef POLAR_HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef POLAR_HAVE_SETLOCALE
#include <locale.h>
#endif

#ifdef PHP_SIGCHILD
#include <sys/types.h>
#include <sys/wait.h>
#endif

#if HAVE_MMAP || defined(PHP_WIN32)
# if HAVE_UNISTD_H
#  include <unistd.h>
#  if defined(_SC_PAGESIZE)
#    define REAL_PAGE_SIZE sysconf(_SC_PAGESIZE);
#  elif defined(_SC_PAGE_SIZE)
#    define REAL_PAGE_SIZE sysconf(_SC_PAGE_SIZE);
#  endif
# endif
# if HAVE_SYS_MMAN_H
#  include <sys/mman.h>
# endif
# ifndef REAL_PAGE_SIZE
#  ifdef PAGE_SIZE
#   define REAL_PAGE_SIZE PAGE_SIZE
#  else
#   define REAL_PAGE_SIZE 4096
#  endif
# endif
#endif

#define SAFE_FILENAME(f) ((f)?(f):"-")

ZEND_TSRMLS_CACHE_DEFINE()

namespace polar
{
thread_local PhpCoreGlobals sg_coreGlobals;

void php_module_startup();
void php_module_shutdown();

//      PHP_INI_BEGIN()
//         PHP_INI_ENTRY_EX("highlight.comment",		HL_COMMENT_COLOR,	PHP_INI_ALL,	NULL,			php_ini_color_displayer_cb)
//         PHP_INI_ENTRY_EX("highlight.default",		HL_DEFAULT_COLOR,	PHP_INI_ALL,	NULL,			php_ini_color_displayer_cb)
//         PHP_INI_ENTRY_EX("highlight.html",			HL_HTML_COLOR,		PHP_INI_ALL,	NULL,			php_ini_color_displayer_cb)
//         PHP_INI_ENTRY_EX("highlight.keyword",		HL_KEYWORD_COLOR,	PHP_INI_ALL,	NULL,			php_ini_color_displayer_cb)
//         PHP_INI_ENTRY_EX("highlight.string",		HL_STRING_COLOR,	PHP_INI_ALL,	NULL,			php_ini_color_displayer_cb)

//         STD_PHP_INI_ENTRY_EX("display_errors",		"1",		PHP_INI_ALL,		OnUpdateDisplayErrors,	display_errors,			php_core_globals,	core_globals, display_errors_mode)
//         STD_PHP_INI_BOOLEAN("display_startup_errors",	"0",	PHP_INI_ALL,		OnUpdateBool,			display_startup_errors,	php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("enable_dl",			"1",		PHP_INI_SYSTEM,		OnUpdateBool,			enable_dl,				php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("expose_php",			"1",		PHP_INI_SYSTEM,		OnUpdateBool,			expose_php,				php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("docref_root", 			"", 		PHP_INI_ALL,		OnUpdateString,			docref_root,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("docref_ext",				"",			PHP_INI_ALL,		OnUpdateString,			docref_ext,				php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("html_errors",			"1",		PHP_INI_ALL,		OnUpdateBool,			html_errors,			php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("xmlrpc_errors",		"0",		PHP_INI_SYSTEM,		OnUpdateBool,			xmlrpc_errors,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("xmlrpc_error_number",	"0",		PHP_INI_ALL,		OnUpdateLong,			xmlrpc_error_number,	php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("max_input_time",			"-1",	PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateLong,			max_input_time,	php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("ignore_user_abort",	"0",		PHP_INI_ALL,		OnUpdateBool,			ignore_user_abort,		php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("implicit_flush",		"0",		PHP_INI_ALL,		OnUpdateBool,			implicit_flush,			php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("log_errors",			"0",		PHP_INI_ALL,		OnUpdateBool,			log_errors,				php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("log_errors_max_len",	 "1024",		PHP_INI_ALL,		OnUpdateLong,			log_errors_max_len,		php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("ignore_repeated_errors",	"0",	PHP_INI_ALL,		OnUpdateBool,			ignore_repeated_errors,	php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("ignore_repeated_source",	"0",	PHP_INI_ALL,		OnUpdateBool,			ignore_repeated_source,	php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("report_memleaks",		"1",		PHP_INI_ALL,		OnUpdateBool,			report_memleaks,		php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("report_zend_debug",	"1",		PHP_INI_ALL,		OnUpdateBool,			report_zend_debug,		php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("output_buffering",		"0",		PHP_INI_PERDIR|PHP_INI_SYSTEM,	OnUpdateLong,	output_buffering,		php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("output_handler",			NULL,		PHP_INI_PERDIR|PHP_INI_SYSTEM,	OnUpdateString,	output_handler,		php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("register_argc_argv",	"1",		PHP_INI_PERDIR|PHP_INI_SYSTEM,	OnUpdateBool,	register_argc_argv,		php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("auto_globals_jit",		"1",		PHP_INI_PERDIR|PHP_INI_SYSTEM,	OnUpdateBool,	auto_globals_jit,	php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("short_open_tag",	DEFAULT_SHORT_OPEN_TAG,	PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateBool,			short_tags,				zend_compiler_globals,	compiler_globals)
//         STD_PHP_INI_BOOLEAN("track_errors",			"0",		PHP_INI_ALL,		OnUpdateBool,			track_errors,			php_core_globals,	core_globals)

//         STD_PHP_INI_ENTRY("unserialize_callback_func",	NULL,	PHP_INI_ALL,		OnUpdateString,			unserialize_callback_func,	php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("serialize_precision",	"-1",	PHP_INI_ALL,		OnSetSerializePrecision,			serialize_precision,	php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("arg_separator.output",	"&",		PHP_INI_ALL,		OnUpdateStringUnempty,	arg_separator.output,	php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("arg_separator.input",	"&",		PHP_INI_SYSTEM|PHP_INI_PERDIR,	OnUpdateStringUnempty,	arg_separator.input,	php_core_globals,	core_globals)

//         STD_PHP_INI_ENTRY("auto_append_file",		NULL,		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateString,			auto_append_file,		php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("auto_prepend_file",		NULL,		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateString,			auto_prepend_file,		php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("doc_root",				NULL,		PHP_INI_SYSTEM,		OnUpdateStringUnempty,	doc_root,				php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("default_charset",		PHP_DEFAULT_CHARSET,	PHP_INI_ALL,	OnUpdateDefaultCharset,			default_charset,		sapi_globals_struct, sapi_globals)
//         STD_PHP_INI_ENTRY("default_mimetype",		SAPI_DEFAULT_MIMETYPE,	PHP_INI_ALL,	OnUpdateString,			default_mimetype,		sapi_globals_struct, sapi_globals)
//         STD_PHP_INI_ENTRY("internal_encoding",		NULL,			PHP_INI_ALL,	OnUpdateInternalEncoding,	internal_encoding,	php_core_globals, core_globals)
//         STD_PHP_INI_ENTRY("input_encoding",			NULL,			PHP_INI_ALL,	OnUpdateInputEncoding,				input_encoding,		php_core_globals, core_globals)
//         STD_PHP_INI_ENTRY("output_encoding",		NULL,			PHP_INI_ALL,	OnUpdateOutputEncoding,				output_encoding,	php_core_globals, core_globals)
//         STD_PHP_INI_ENTRY("error_log",				NULL,		PHP_INI_ALL,		OnUpdateErrorLog,			error_log,				php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("extension_dir",			PHP_EXTENSION_DIR,		PHP_INI_SYSTEM,		OnUpdateStringUnempty,	extension_dir,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("sys_temp_dir",			NULL,		PHP_INI_SYSTEM,		OnUpdateStringUnempty,	sys_temp_dir,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("include_path",			PHP_INCLUDE_PATH,		PHP_INI_ALL,		OnUpdateStringUnempty,	include_path,			php_core_globals,	core_globals)
//         PHP_INI_ENTRY("max_execution_time",			"30",		PHP_INI_ALL,			OnUpdateTimeout)
//         STD_PHP_INI_ENTRY("open_basedir",			NULL,		PHP_INI_ALL,		OnUpdateBaseDir,			open_basedir,			php_core_globals,	core_globals)

//         STD_PHP_INI_BOOLEAN("file_uploads",			"1",		PHP_INI_SYSTEM,		OnUpdateBool,			file_uploads,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("upload_max_filesize",	"2M",		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateLong,			upload_max_filesize,	php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("post_max_size",			"8M",		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateLong,			post_max_size,			sapi_globals_struct,sapi_globals)
//         STD_PHP_INI_ENTRY("upload_tmp_dir",			NULL,		PHP_INI_SYSTEM,		OnUpdateStringUnempty,	upload_tmp_dir,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("max_input_nesting_level", "64",		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateLongGEZero,	max_input_nesting_level,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("max_input_vars",			"1000",		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateLongGEZero,	max_input_vars,						php_core_globals,	core_globals)

//         STD_PHP_INI_ENTRY("user_dir",				NULL,		PHP_INI_SYSTEM,		OnUpdateString,			user_dir,				php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("variables_order",		"EGPCS",	PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateStringUnempty,	variables_order,		php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("request_order",			NULL,		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateString,	request_order,		php_core_globals,	core_globals)

//         STD_PHP_INI_ENTRY("error_append_string",	NULL,		PHP_INI_ALL,		OnUpdateString,			error_append_string,	php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("error_prepend_string",	NULL,		PHP_INI_ALL,		OnUpdateString,			error_prepend_string,	php_core_globals,	core_globals)

//         PHP_INI_ENTRY("SMTP",						"localhost",PHP_INI_ALL,		NULL)
//         PHP_INI_ENTRY("smtp_port",					"25",		PHP_INI_ALL,		NULL)
//         STD_PHP_INI_BOOLEAN("mail.add_x_header",			"0",		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateBool,			mail_x_header,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("mail.log",					NULL,		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateMailLog,			mail_log,			php_core_globals,	core_globals)
//         PHP_INI_ENTRY("browscap",					NULL,		PHP_INI_SYSTEM,		OnChangeBrowscap)
//         PHP_INI_ENTRY("memory_limit",				"128M",		PHP_INI_ALL,		OnChangeMemoryLimit)
//         PHP_INI_ENTRY("precision",					"14",		PHP_INI_ALL,		OnSetPrecision)
//         PHP_INI_ENTRY("sendmail_from",				NULL,		PHP_INI_ALL,		NULL)
//         PHP_INI_ENTRY("sendmail_path",	DEFAULT_SENDMAIL_PATH,	PHP_INI_SYSTEM,		NULL)
//         PHP_INI_ENTRY("mail.force_extra_parameters",NULL,		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnChangeMailForceExtra)
//         PHP_INI_ENTRY("disable_functions",			"",			PHP_INI_SYSTEM,		NULL)
//         PHP_INI_ENTRY("disable_classes",			"",			PHP_INI_SYSTEM,		NULL)
//         PHP_INI_ENTRY("max_file_uploads",			"20",			PHP_INI_SYSTEM|PHP_INI_PERDIR,		NULL)

//         STD_PHP_INI_BOOLEAN("allow_url_fopen",		"1",		PHP_INI_SYSTEM,		OnUpdateBool,		allow_url_fopen,		php_core_globals,		core_globals)
//         STD_PHP_INI_BOOLEAN("allow_url_include",	"0",		PHP_INI_SYSTEM,		OnUpdateBool,		allow_url_include,		php_core_globals,		core_globals)
//         STD_PHP_INI_BOOLEAN("enable_post_data_reading",	"1",	PHP_INI_SYSTEM|PHP_INI_PERDIR,	OnUpdateBool,	enable_post_data_reading,	php_core_globals,	core_globals)

//         STD_PHP_INI_ENTRY("realpath_cache_size",	"4096K",	PHP_INI_SYSTEM,		OnUpdateLong,	realpath_cache_size_limit,	virtual_cwd_globals,	cwd_globals)
//         STD_PHP_INI_ENTRY("realpath_cache_ttl",		"120",		PHP_INI_SYSTEM,		OnUpdateLong,	realpath_cache_ttl,			virtual_cwd_globals,	cwd_globals)

//         STD_PHP_INI_ENTRY("user_ini.filename",		".user.ini",	PHP_INI_SYSTEM,		OnUpdateString,		user_ini_filename,	php_core_globals,		core_globals)
//         STD_PHP_INI_ENTRY("user_ini.cache_ttl",		"300",			PHP_INI_SYSTEM,		OnUpdateLong,		user_ini_cache_ttl,	php_core_globals,		core_globals)
//         STD_PHP_INI_ENTRY("hard_timeout",			"2",			PHP_INI_SYSTEM,		OnUpdateLong,		hard_timeout,		zend_executor_globals,	executor_globals)
//      #ifdef PHP_WIN32
//         STD_PHP_INI_BOOLEAN("windows.show_crt_warning",		"0",		PHP_INI_ALL,		OnUpdateBool,			windows_show_crt_warning,			php_core_globals,	core_globals)
//      #endif
//         STD_PHP_INI_ENTRY("syslog.facility",		"LOG_USER",		PHP_INI_SYSTEM,		OnSetFacility,		syslog_facility,	php_core_globals,		core_globals)
//         STD_PHP_INI_ENTRY("syslog.ident",		"php",			PHP_INI_SYSTEM,		OnUpdateString,		syslog_ident,		php_core_globals,		core_globals)
//         STD_PHP_INI_ENTRY("syslog.filter",		"no-ctrl",		PHP_INI_ALL,		OnSetLogFilter,		syslog_filter,		php_core_globals, 		core_globals)
//         PHP_INI_END()

ZEND_COLD void php_error_docref0(const char *docref, int type, const char *format, ...)
{
   va_list args;

   va_start(args, format);
   php_verror(docref, "", type, format, args);
   va_end(args);
}

ZEND_COLD void php_error_docref1(const char *docref, const char *param1, int type, const char *format, ...)
{
   va_list args;

   va_start(args, format);
   php_verror(docref, param1, type, format, args);
   va_end(args);
}

ZEND_COLD void php_error_docref2(const char *docref, const char *param1, const char *param2, int type, const char *format, ...)
{
   char *params;
   va_list args;
   zend_spprintf(&params, 0, "%s,%s", param1, param2);
   va_start(args, format);
   php_verror(docref, params ? params : "...", type, format, args);
   va_end(args);
   if (params) {
      efree(params);
   }
}

ZEND_COLD void php_verror(const char *docref, const char *params, int type, const char *format, va_list args)
{
//   zend_string *replace_buffer = NULL, *replace_origin = NULL;
//   char *buffer = NULL, *docref_buf = NULL, *target = NULL;
//   char *docref_target = "", *docref_root = "";
//   char *p;
//   int buffer_len = 0;
//   const char *space = "";
//   const char *class_name = "";
//   const char *function;
//   int origin_len;
//   char *origin;
//   char *message;
//   int is_function = 0;

//   /* get error text into buffer and escape for html if necessary */
//   buffer_len = (int)vspprintf(&buffer, 0, format, args);

//   if (PG(html_errors)) {
//      replace_buffer = php_escape_html_entities((unsigned char*)buffer, buffer_len, 0, ENT_COMPAT, get_safe_charset_hint());
//      /* Retry with substituting invalid chars on fail. */
//      if (!replace_buffer || ZSTR_LEN(replace_buffer) < 1) {
//         replace_buffer = php_escape_html_entities((unsigned char*)buffer, buffer_len, 0, ENT_COMPAT | ENT_HTML_SUBSTITUTE_ERRORS, get_safe_charset_hint());
//      }

//      efree(buffer);

//      if (replace_buffer) {
//         buffer = ZSTR_VAL(replace_buffer);
//         buffer_len = (int)ZSTR_LEN(replace_buffer);
//      } else {
//         buffer = "";
//         buffer_len = 0;
//      }
//   }

//   /* which function caused the problem if any at all */
//   if (php_during_module_startup()) {
//      function = "PHP Startup";
//   } else if (php_during_module_shutdown()) {
//      function = "PHP Shutdown";
//   } else if (EG(current_execute_data) &&
//            EG(current_execute_data)->func &&
//            ZEND_USER_CODE(EG(current_execute_data)->func->common.type) &&
//            EG(current_execute_data)->opline &&
//            EG(current_execute_data)->opline->opcode == ZEND_INCLUDE_OR_EVAL
//   ) {
//      switch (EG(current_execute_data)->opline->extended_value) {
//         case ZEND_EVAL:
//            function = "eval";
//            is_function = 1;
//            break;
//         case ZEND_INCLUDE:
//            function = "include";
//            is_function = 1;
//            break;
//         case ZEND_INCLUDE_ONCE:
//            function = "include_once";
//            is_function = 1;
//            break;
//         case ZEND_REQUIRE:
//            function = "require";
//            is_function = 1;
//            break;
//         case ZEND_REQUIRE_ONCE:
//            function = "require_once";
//            is_function = 1;
//            break;
//         default:
//            function = "Unknown";
//      }
//   } else {
//      function = get_active_function_name();
//      if (!function || !strlen(function)) {
//         function = "Unknown";
//      } else {
//         is_function = 1;
//         class_name = get_active_class_name(&space);
//      }
//   }

//   /* if we still have memory then format the origin */
//   if (is_function) {
//      origin_len = (int)spprintf(&origin, 0, "%s%s%s(%s)", class_name, space, function, params);
//   } else {
//      origin_len = (int)spprintf(&origin, 0, "%s", function);
//   }

//   if (PG(html_errors)) {
//      replace_origin = php_escape_html_entities((unsigned char*)origin, origin_len, 0, ENT_COMPAT, get_safe_charset_hint());
//      efree(origin);
//      origin = ZSTR_VAL(replace_origin);
//   }

//   /* origin and buffer available, so lets come up with the error message */
//   if (docref && docref[0] == '#') {
//      docref_target = strchr(docref, '#');
//      docref = NULL;
//   }

//   /* no docref given but function is known (the default) */
//   if (!docref && is_function) {
//      int doclen;
//      while (*function == '_') {
//         function++;
//      }
//      if (space[0] == '\0') {
//         doclen = (int)spprintf(&docref_buf, 0, "function.%s", function);
//      } else {
//         doclen = (int)spprintf(&docref_buf, 0, "%s.%s", class_name, function);
//      }
//      while((p = strchr(docref_buf, '_')) != NULL) {
//         *p = '-';
//      }
//      docref = php_strtolower(docref_buf, doclen);
//   }

//   /* we have a docref for a function AND
//    * - we show errors in html mode AND
//    * - the user wants to see the links
//    */
//   if (docref && is_function && PG(html_errors) && strlen(PG(docref_root))) {
//      if (strncmp(docref, "http://", 7)) {
//         /* We don't have 'http://' so we use docref_root */

//         char *ref;  /* temp copy for duplicated docref */

//         docref_root = PG(docref_root);

//         ref = estrdup(docref);
//         if (docref_buf) {
//            efree(docref_buf);
//         }
//         docref_buf = ref;
//         /* strip of the target if any */
//         p = strrchr(ref, '#');
//         if (p) {
//            target = estrdup(p);
//            if (target) {
//               docref_target = target;
//               *p = '\0';
//            }
//         }
//         /* add the extension if it is set in ini */
//         if (PG(docref_ext) && strlen(PG(docref_ext))) {
//            spprintf(&docref_buf, 0, "%s%s", ref, PG(docref_ext));
//            efree(ref);
//         }
//         docref = docref_buf;
//      }
//      /* display html formatted or only show the additional links */
//      if (PG(html_errors)) {
//         spprintf(&message, 0, "%s [<a href='%s%s%s'>%s</a>]: %s", origin, docref_root, docref, docref_target, docref, buffer);
//      } else {
//         spprintf(&message, 0, "%s [%s%s%s]: %s", origin, docref_root, docref, docref_target, buffer);
//      }
//      if (target) {
//         efree(target);
//      }
//   } else {
//      spprintf(&message, 0, "%s: %s", origin, buffer);
//   }
//   if (replace_origin) {
//      zend_string_free(replace_origin);
//   } else {
//      efree(origin);
//   }
//   if (docref_buf) {
//      efree(docref_buf);
//   }

//   if (PG(track_errors) && module_initialized && EG(active) &&
//         (Z_TYPE(EG(user_error_handler)) == IS_UNDEF || !(EG(user_error_handler_error_reporting) & type))) {
//      zval tmp;
//      ZVAL_STRINGL(&tmp, buffer, buffer_len);
//      if (EG(current_execute_data)) {
//         if (zend_set_local_var_str("php_errormsg", sizeof("php_errormsg")-1, &tmp, 0) == FAILURE) {
//            zval_ptr_dtor(&tmp);
//         }
//      } else {
//         zend_hash_str_update_ind(&EG(symbol_table), "php_errormsg", sizeof("php_errormsg")-1, &tmp);
//      }
//   }
//   if (replace_buffer) {
//      zend_string_free(replace_buffer);
//   } else {
//      if (buffer_len > 0) {
//         efree(buffer);
//      }
//   }

//   php_error(type, "%s", message);
//   efree(message);
}


} // polar
