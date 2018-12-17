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

#ifndef POLAR_OS_WIN32
# define php_select(m, r, w, e, t)	::select(m, r, w, e, t)
#else
# include "win32/select.h"
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

namespace polar
{

CliShellCallbacksType sg_cliShellCallbacks = {nullptr, nullptr, nullptr};

POLAR_DECL_EXPORT CliShellCallbacksType *php_cli_get_shell_callbacks()
{
   return &sg_cliShellCallbacks;
}

int php_execute_script(zend_file_handle *primaryFile)
{
   return 0;
}

POLAR_DECL_EXPORT int php_execute_simple_script(zend_file_handle *primaryFile, zval *ret)
{
   return 0;
}

ExecEnv &retrieve_global_execenv()
{
   thread_local ExecEnv execEnv;
   return execEnv;
}

ExecEnv::ExecEnv()
   : m_started(false),
     /// TODO read from cfg file
     m_defaultSocketTimeout(60)
{
}

ExecEnv::~ExecEnv()
{
}

void ExecEnv::activate()
{
   m_started = false;
}

void ExecEnv::deactivate()
{
   m_started = false;
}

//      PHP_INI_BEGIN()
//         PHP_INI_ENTRY_EX("highlight.comment",		HL_COMMENT_COLOR,	PHP_INI_ALL,	nullptr,			php_ini_color_displayer_cb)
//         PHP_INI_ENTRY_EX("highlight.default",		HL_DEFAULT_COLOR,	PHP_INI_ALL,	nullptr,			php_ini_color_displayer_cb)
//         PHP_INI_ENTRY_EX("highlight.html",			HL_HTML_COLOR,		PHP_INI_ALL,	nullptr,			php_ini_color_displayer_cb)
//         PHP_INI_ENTRY_EX("highlight.keyword",		HL_KEYWORD_COLOR,	PHP_INI_ALL,	nullptr,			php_ini_color_displayer_cb)
//         PHP_INI_ENTRY_EX("highlight.string",		HL_STRING_COLOR,	PHP_INI_ALL,	nullptr,			php_ini_color_displayer_cb)

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
//         STD_PHP_INI_ENTRY("output_handler",			nullptr,		PHP_INI_PERDIR|PHP_INI_SYSTEM,	OnUpdateString,	output_handler,		php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("register_argc_argv",	"1",		PHP_INI_PERDIR|PHP_INI_SYSTEM,	OnUpdateBool,	register_argc_argv,		php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("auto_globals_jit",		"1",		PHP_INI_PERDIR|PHP_INI_SYSTEM,	OnUpdateBool,	auto_globals_jit,	php_core_globals,	core_globals)
//         STD_PHP_INI_BOOLEAN("short_open_tag",	DEFAULT_SHORT_OPEN_TAG,	PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateBool,			short_tags,				zend_compiler_globals,	compiler_globals)
//         STD_PHP_INI_BOOLEAN("track_errors",			"0",		PHP_INI_ALL,		OnUpdateBool,			track_errors,			php_core_globals,	core_globals)

//         STD_PHP_INI_ENTRY("unserialize_callback_func",	nullptr,	PHP_INI_ALL,		OnUpdateString,			unserialize_callback_func,	php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("serialize_precision",	"-1",	PHP_INI_ALL,		OnSetSerializePrecision,			serialize_precision,	php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("arg_separator.output",	"&",		PHP_INI_ALL,		OnUpdateStringUnempty,	arg_separator.output,	php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("arg_separator.input",	"&",		PHP_INI_SYSTEM|PHP_INI_PERDIR,	OnUpdateStringUnempty,	arg_separator.input,	php_core_globals,	core_globals)

//         STD_PHP_INI_ENTRY("auto_append_file",		nullptr,		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateString,			auto_append_file,		php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("auto_prepend_file",		nullptr,		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateString,			auto_prepend_file,		php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("doc_root",				nullptr,		PHP_INI_SYSTEM,		OnUpdateStringUnempty,	doc_root,				php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("default_charset",		PHP_DEFAULT_CHARSET,	PHP_INI_ALL,	OnUpdateDefaultCharset,			default_charset,		sapi_globals_struct, sapi_globals)
//         STD_PHP_INI_ENTRY("default_mimetype",		SAPI_DEFAULT_MIMETYPE,	PHP_INI_ALL,	OnUpdateString,			default_mimetype,		sapi_globals_struct, sapi_globals)
//         STD_PHP_INI_ENTRY("internal_encoding",		nullptr,			PHP_INI_ALL,	OnUpdateInternalEncoding,	internal_encoding,	php_core_globals, core_globals)
//         STD_PHP_INI_ENTRY("input_encoding",			nullptr,			PHP_INI_ALL,	OnUpdateInputEncoding,				input_encoding,		php_core_globals, core_globals)
//         STD_PHP_INI_ENTRY("output_encoding",		nullptr,			PHP_INI_ALL,	OnUpdateOutputEncoding,				output_encoding,	php_core_globals, core_globals)
//         STD_PHP_INI_ENTRY("error_log",				nullptr,		PHP_INI_ALL,		OnUpdateErrorLog,			error_log,				php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("extension_dir",			PHP_EXTENSION_DIR,		PHP_INI_SYSTEM,		OnUpdateStringUnempty,	extension_dir,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("sys_temp_dir",			nullptr,		PHP_INI_SYSTEM,		OnUpdateStringUnempty,	sys_temp_dir,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("include_path",			PHP_INCLUDE_PATH,		PHP_INI_ALL,		OnUpdateStringUnempty,	include_path,			php_core_globals,	core_globals)
//         PHP_INI_ENTRY("max_execution_time",			"30",		PHP_INI_ALL,			OnUpdateTimeout)
//         STD_PHP_INI_ENTRY("open_basedir",			nullptr,		PHP_INI_ALL,		OnUpdateBaseDir,			open_basedir,			php_core_globals,	core_globals)

//         STD_PHP_INI_BOOLEAN("file_uploads",			"1",		PHP_INI_SYSTEM,		OnUpdateBool,			file_uploads,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("upload_max_filesize",	"2M",		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateLong,			upload_max_filesize,	php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("post_max_size",			"8M",		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateLong,			post_max_size,			sapi_globals_struct,sapi_globals)
//         STD_PHP_INI_ENTRY("upload_tmp_dir",			nullptr,		PHP_INI_SYSTEM,		OnUpdateStringUnempty,	upload_tmp_dir,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("max_input_nesting_level", "64",		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateLongGEZero,	max_input_nesting_level,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("max_input_vars",			"1000",		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateLongGEZero,	max_input_vars,						php_core_globals,	core_globals)

//         STD_PHP_INI_ENTRY("user_dir",				nullptr,		PHP_INI_SYSTEM,		OnUpdateString,			user_dir,				php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("variables_order",		"EGPCS",	PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateStringUnempty,	variables_order,		php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("request_order",			nullptr,		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateString,	request_order,		php_core_globals,	core_globals)

//         STD_PHP_INI_ENTRY("error_append_string",	nullptr,		PHP_INI_ALL,		OnUpdateString,			error_append_string,	php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("error_prepend_string",	nullptr,		PHP_INI_ALL,		OnUpdateString,			error_prepend_string,	php_core_globals,	core_globals)

//         PHP_INI_ENTRY("SMTP",						"localhost",PHP_INI_ALL,		nullptr)
//         PHP_INI_ENTRY("smtp_port",					"25",		PHP_INI_ALL,		nullptr)
//         STD_PHP_INI_BOOLEAN("mail.add_x_header",			"0",		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateBool,			mail_x_header,			php_core_globals,	core_globals)
//         STD_PHP_INI_ENTRY("mail.log",					nullptr,		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateMailLog,			mail_log,			php_core_globals,	core_globals)
//         PHP_INI_ENTRY("browscap",					nullptr,		PHP_INI_SYSTEM,		OnChangeBrowscap)
//         PHP_INI_ENTRY("memory_limit",				"128M",		PHP_INI_ALL,		OnChangeMemoryLimit)
//         PHP_INI_ENTRY("precision",					"14",		PHP_INI_ALL,		OnSetPrecision)
//         PHP_INI_ENTRY("sendmail_from",				nullptr,		PHP_INI_ALL,		nullptr)
//         PHP_INI_ENTRY("sendmail_path",	DEFAULT_SENDMAIL_PATH,	PHP_INI_SYSTEM,		nullptr)
//         PHP_INI_ENTRY("mail.force_extra_parameters",nullptr,		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnChangeMailForceExtra)
//         PHP_INI_ENTRY("disable_functions",			"",			PHP_INI_SYSTEM,		nullptr)
//         PHP_INI_ENTRY("disable_classes",			"",			PHP_INI_SYSTEM,		nullptr)
//         PHP_INI_ENTRY("max_file_uploads",			"20",			PHP_INI_SYSTEM|PHP_INI_PERDIR,		nullptr)

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

namespace {
inline int cli_select(php_socket_t fd)
{
   fd_set wfd, dfd;
   struct timeval tv;
   int ret;
   ExecEnv &execEnv = retrieve_global_execenv();
   FD_ZERO(&wfd);
   FD_ZERO(&dfd);
   PHP_SAFE_FD_SET(fd, &wfd);
   tv.tv_sec = (long)execEnv.getDefaultSocketTimeout();
   tv.tv_usec = 0;
   ret = php_select(fd+1, &dfd, &wfd, &dfd, &tv);
   return ret != -1;
}
}

ssize_t cli_single_write(const char *str, size_t strLength)
{
   ssize_t ret;
   if (sg_cliShellCallbacks.cliShellWrite) {
      sg_cliShellCallbacks.cliShellWrite(str, strLength);
   }
#ifdef PHP_WRITE_STDOUT
   do {
      ret = ::write(STDOUT_FILENO, str, strLength);
   } while (ret <= 0 && errno == EAGAIN && cli_select(STDOUT_FILENO));
#else
   ret = ::fwrite(str, 1, MIN(strLength, 16384), stdout);
#endif
   return ret;
}

size_t cli_unbuffer_write(const char *str, size_t strLength)
{
   const char *ptr = str;
   size_t remaining = strLength;
   ssize_t ret;

   if (!strLength) {
      return 0;
   }

   if (sg_cliShellCallbacks.cliShellUnbufferWrite) {
      size_t ubWrote;
      ubWrote = sg_cliShellCallbacks.cliShellUnbufferWrite(str, strLength);
      if (ubWrote != (size_t) -1) {
         return ubWrote;
      }
   }

   while (remaining > 0)
   {
      ret = cli_single_write(ptr, remaining);
      if (ret < 0) {
#ifndef PHP_CLI_WIN32_NO_CONSOLE
         EG(exit_status) = 255;
#endif
         break;
      }
      ptr += ret;
      remaining -= ret;
   }

   return (ptr - str);
}

void cli_flush(void *server_context)
{
   /* Ignore EBADF here, it's caused by the fact that STDIN/STDOUT/STDERR streams
       * are/could be closed before fflush() is called.
       */
   fflush(stdout);
}

namespace internal {
void emit_fd_setsize_warning(int maxFd)
{

#ifdef POLAR_OS_WIN32
   php_error_docref(nullptr, E_WARNING,
                    "PHP needs to be recompiled with a larger value of FD_SETSIZE.\n"
                    "If this binary is from an official www.php.net package, file a bug report\n"
                    "at http://bugs.php.net, including the following information:\n"
                    "FD_SETSIZE=%d, but you are using %d.\n"
                    " --enable-fd-setsize=%d is recommended, but you may want to set it\n"
                    "to match to maximum number of sockets each script will work with at\n"
                    "one time, in order to avoid seeing this error again at a later date.",
                    FD_SETSIZE, maxFd, (maxFd + 128) & ~127);
#else
   php_error_docref(nullptr, E_WARNING,
                    "You MUST recompile PHP with a larger value of FD_SETSIZE.\n"
                    "It is set to %d, but you have descriptors numbered at least as high as %d.\n"
                    " --enable-fd-setsize=%d is recommended, but you may want to set it\n"
                    "to equal the maximum number of open files supported by your system,\n"
                    "in order to avoid seeing this error again at a later date.",
                    FD_SETSIZE, maxFd, (maxFd + 1024) & ~1023);
#endif
}
} // internal

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
   //   zend_string *replace_buffer = nullptr, *replace_origin = nullptr;
   //   char *buffer = nullptr, *docref_buf = nullptr, *target = nullptr;
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
   //      docref = nullptr;
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
   //      while((p = strchr(docref_buf, '_')) != nullptr) {
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

ZEND_COLD void php_error_callback(int type, const char *errorFilename,
                                  const uint32_t errorLineno, const char *format,
                                  va_list args)
{

}

size_t php_printf(const char *format, ...)
{

}

size_t php_output_wrapper(const char *str, size_t strLength)
{

}

FILE *php_fopen_wrapper_for_zend(const char *filename, zend_string **openedPath)
{

}

zval *php_get_configuration_directive_for_zend(zend_string *name)
{

}

POLAR_DECL_EXPORT void php_message_handler_for_zend(zend_long message, const void *data)
{

}

void php_on_timeout(int seconds)
{

}

int php_stream_open_for_zend(const char *filename, zend_file_handle *handle)
{

}

POLAR_DECL_EXPORT void php_printf_to_smart_string(smart_string *buf, const char *format, va_list ap)
{

}

POLAR_DECL_EXPORT void php_printf_to_smart_str(smart_str *buf, const char *format, va_list ap)
{

}

POLAR_DECL_EXPORT char *bootstrap_getenv(char *name, size_t nameLen)
{

}

zend_string *php_resolve_path_for_zend(const char *filename, size_t filenameLen)
{

}

bool seek_file_begin(zend_file_handle *fileHandle, const char *scriptFile, int *lineno)
{
   int c;
   *lineno = 1;
   fileHandle->type = ZEND_HANDLE_FP;
   fileHandle->opened_path = nullptr;
   fileHandle->free_filename = 0;
   if (!(fileHandle->handle.fp = VCWD_FOPEN(scriptFile, "rb"))) {
      php_printf("Could not open input file: %s\n", scriptFile);
      return false;
   }
   fileHandle->filename = scriptFile;
   /* #!php support */
   c = fgetc(fileHandle->handle.fp);
   if (c == '#' && (c = fgetc(fileHandle->handle.fp)) == '!') {
      while (c != '\n' && c != '\r' && c != EOF) {
         c = fgetc(fileHandle->handle.fp);	/* skip to end of line */
      }
      /* handle situations where line is terminated by \r\n */
      if (c == '\r') {
         if (fgetc(fileHandle->handle.fp) != '\n') {
            zend_long pos = zend_ftell(fileHandle->handle.fp);
            zend_fseek(fileHandle->handle.fp, pos - 1, SEEK_SET);
         }
      }
      *lineno = 2;
   } else {
      rewind(fileHandle->handle.fp);
   }
   return true;
}

bool php_hash_environment()
{
   //   memset(PG(http_globals), 0, sizeof(PG(http_globals)));
   //   zend_activate_auto_globals();
   //   if (PG(register_argc_argv)) {
   //      php_build_argv(SG(request_info).query_string, &PG(http_globals)[TRACK_VARS_SERVER]);
   //   }
   return true;
}

void cli_register_file_handles()
{
   /// TODO register std file fd
}

void ExecEnv::unbufferWrite(const char *str, int len)
{

}

} // polar
