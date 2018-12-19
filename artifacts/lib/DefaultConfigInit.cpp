// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/19.

#include "ZendHeaders.h"
#include "Ini.h"

namespace polar {

POLAR_INI_BEGIN()
   POLAR_INI_ENTRY_EX("highlight.comment",		HL_COMMENT_COLOR,	POLAR_INI_ALL,	NULL,			zend_ini_color_displayer_cb)
   POLAR_INI_ENTRY_EX("highlight.default",		HL_DEFAULT_COLOR,	POLAR_INI_ALL,	NULL,			zend_ini_color_displayer_cb)
   POLAR_INI_ENTRY_EX("highlight.html",			HL_HTML_COLOR,		POLAR_INI_ALL,	NULL,			zend_ini_color_displayer_cb)
   POLAR_INI_ENTRY_EX("highlight.keyword",		HL_KEYWORD_COLOR,	POLAR_INI_ALL,	NULL,			zend_ini_color_displayer_cb)
   POLAR_INI_ENTRY_EX("highlight.string",		HL_STRING_COLOR,	POLAR_INI_ALL,	NULL,			zend_ini_color_displayer_cb)

//   STD_POLAR_INI_ENTRY_EX("display_errors",		"1",		POLAR_INI_ALL,		OnUpdateDisplayErrors,	display_errors,			php_core_globals,	core_globals, display_errors_mode)
//   STD_POLAR_INI_BOOLEAN("display_startup_errors",	"0",	POLAR_INI_ALL,		OnUpdateBool,			display_startup_errors,	php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("enable_dl",			"1",		POLAR_INI_SYSTEM,		OnUpdateBool,			enable_dl,				php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("expose_php",			"1",		POLAR_INI_SYSTEM,		OnUpdateBool,			expose_php,				php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("docref_root", 			"", 		POLAR_INI_ALL,		OnUpdateString,			docref_root,			php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("docref_ext",				"",			POLAR_INI_ALL,		OnUpdateString,			docref_ext,				php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("html_errors",			"1",		POLAR_INI_ALL,		OnUpdateBool,			html_errors,			php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("xmlrpc_errors",		"0",		POLAR_INI_SYSTEM,		OnUpdateBool,			xmlrpc_errors,			php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("xmlrpc_error_number",	"0",		POLAR_INI_ALL,		OnUpdateLong,			xmlrpc_error_number,	php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("max_input_time",			"-1",	POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		OnUpdateLong,			max_input_time,	php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("ignore_user_abort",	"0",		POLAR_INI_ALL,		OnUpdateBool,			ignore_user_abort,		php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("implicit_flush",		"0",		POLAR_INI_ALL,		OnUpdateBool,			implicit_flush,			php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("log_errors",			"0",		POLAR_INI_ALL,		OnUpdateBool,			log_errors,				php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("log_errors_max_len",	 "1024",		POLAR_INI_ALL,		OnUpdateLong,			log_errors_max_len,		php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("ignore_repeated_errors",	"0",	POLAR_INI_ALL,		OnUpdateBool,			ignore_repeated_errors,	php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("ignore_repeated_source",	"0",	POLAR_INI_ALL,		OnUpdateBool,			ignore_repeated_source,	php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("report_memleaks",		"1",		POLAR_INI_ALL,		OnUpdateBool,			report_memleaks,		php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("report_zend_debug",	"1",		POLAR_INI_ALL,		OnUpdateBool,			report_zend_debug,		php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("output_buffering",		"0",		POLAR_INI_PERDIR|POLAR_INI_SYSTEM,	OnUpdateLong,	output_buffering,		php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("output_handler",			NULL,		POLAR_INI_PERDIR|POLAR_INI_SYSTEM,	OnUpdateString,	output_handler,		php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("register_argc_argv",	"1",		POLAR_INI_PERDIR|POLAR_INI_SYSTEM,	OnUpdateBool,	register_argc_argv,		php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("auto_globals_jit",		"1",		POLAR_INI_PERDIR|POLAR_INI_SYSTEM,	OnUpdateBool,	auto_globals_jit,	php_core_globals,	core_globals)
//   STD_POLAR_INI_BOOLEAN("short_open_tag",	DEFAULT_SHORT_OPEN_TAG,	POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		OnUpdateBool,			short_tags,				zend_compiler_globals,	compiler_globals)
//   STD_POLAR_INI_BOOLEAN("track_errors",			"0",		POLAR_INI_ALL,		OnUpdateBool,			track_errors,			php_core_globals,	core_globals)

//   STD_POLAR_INI_ENTRY("unserialize_callback_func",	NULL,	POLAR_INI_ALL,		OnUpdateString,			unserialize_callback_func,	php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("serialize_precision",	"-1",	POLAR_INI_ALL,		OnSetSerializePrecision,			serialize_precision,	php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("arg_separator.output",	"&",		POLAR_INI_ALL,		OnUpdateStringUnempty,	arg_separator.output,	php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("arg_separator.input",	"&",		POLAR_INI_SYSTEM|POLAR_INI_PERDIR,	OnUpdateStringUnempty,	arg_separator.input,	php_core_globals,	core_globals)

//   STD_POLAR_INI_ENTRY("auto_append_file",		NULL,		POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		OnUpdateString,			auto_append_file,		php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("auto_prepend_file",		NULL,		POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		OnUpdateString,			auto_prepend_file,		php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("doc_root",				NULL,		POLAR_INI_SYSTEM,		OnUpdateStringUnempty,	doc_root,				php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("default_charset",		PHP_DEFAULT_CHARSET,	POLAR_INI_ALL,	OnUpdateDefaultCharset,			default_charset,		sapi_globals_struct, sapi_globals)
//   STD_POLAR_INI_ENTRY("default_mimetype",		SAPI_DEFAULT_MIMETYPE,	POLAR_INI_ALL,	OnUpdateString,			default_mimetype,		sapi_globals_struct, sapi_globals)
//   STD_POLAR_INI_ENTRY("internal_encoding",		NULL,			POLAR_INI_ALL,	OnUpdateInternalEncoding,	internal_encoding,	php_core_globals, core_globals)
//   STD_POLAR_INI_ENTRY("input_encoding",			NULL,			POLAR_INI_ALL,	OnUpdateInputEncoding,				input_encoding,		php_core_globals, core_globals)
//   STD_POLAR_INI_ENTRY("output_encoding",		NULL,			POLAR_INI_ALL,	OnUpdateOutputEncoding,				output_encoding,	php_core_globals, core_globals)
//   STD_POLAR_INI_ENTRY("error_log",				NULL,		POLAR_INI_ALL,		OnUpdateErrorLog,			error_log,				php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("extension_dir",			PHP_EXTENSION_DIR,		POLAR_INI_SYSTEM,		OnUpdateStringUnempty,	extension_dir,			php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("sys_temp_dir",			NULL,		POLAR_INI_SYSTEM,		OnUpdateStringUnempty,	sys_temp_dir,			php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("include_path",			PHP_INCLUDE_PATH,		POLAR_INI_ALL,		OnUpdateStringUnempty,	include_path,			php_core_globals,	core_globals)
//   POLAR_INI_ENTRY("max_execution_time",			"30",		POLAR_INI_ALL,			OnUpdateTimeout)
//   STD_POLAR_INI_ENTRY("open_basedir",			NULL,		POLAR_INI_ALL,		OnUpdateBaseDir,			open_basedir,			php_core_globals,	core_globals)

//   STD_POLAR_INI_BOOLEAN("file_uploads",			"1",		POLAR_INI_SYSTEM,		OnUpdateBool,			file_uploads,			php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("upload_max_filesize",	"2M",		POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		OnUpdateLong,			upload_max_filesize,	php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("post_max_size",			"8M",		POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		OnUpdateLong,			post_max_size,			sapi_globals_struct,sapi_globals)
//   STD_POLAR_INI_ENTRY("upload_tmp_dir",			NULL,		POLAR_INI_SYSTEM,		OnUpdateStringUnempty,	upload_tmp_dir,			php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("max_input_nesting_level", "64",		POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		OnUpdateLongGEZero,	max_input_nesting_level,			php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("max_input_vars",			"1000",		POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		OnUpdateLongGEZero,	max_input_vars,						php_core_globals,	core_globals)

//   STD_POLAR_INI_ENTRY("user_dir",				NULL,		POLAR_INI_SYSTEM,		OnUpdateString,			user_dir,				php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("variables_order",		"EGPCS",	POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		OnUpdateStringUnempty,	variables_order,		php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("request_order",			NULL,		POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		OnUpdateString,	request_order,		php_core_globals,	core_globals)

//   STD_POLAR_INI_ENTRY("error_append_string",	NULL,		POLAR_INI_ALL,		OnUpdateString,			error_append_string,	php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("error_prepend_string",	NULL,		POLAR_INI_ALL,		OnUpdateString,			error_prepend_string,	php_core_globals,	core_globals)

//   POLAR_INI_ENTRY("SMTP",						"localhost",POLAR_INI_ALL,		NULL)
//   POLAR_INI_ENTRY("smtp_port",					"25",		POLAR_INI_ALL,		NULL)
//   STD_POLAR_INI_BOOLEAN("mail.add_x_header",			"0",		POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		OnUpdateBool,			mail_x_header,			php_core_globals,	core_globals)
//   STD_POLAR_INI_ENTRY("mail.log",					NULL,		POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		OnUpdateMailLog,			mail_log,			php_core_globals,	core_globals)
//   POLAR_INI_ENTRY("browscap",					NULL,		POLAR_INI_SYSTEM,		OnChangeBrowscap)
//   POLAR_INI_ENTRY("memory_limit",				"128M",		POLAR_INI_ALL,		OnChangeMemoryLimit)
//   POLAR_INI_ENTRY("precision",					"14",		POLAR_INI_ALL,		OnSetPrecision)
//   POLAR_INI_ENTRY("sendmail_from",				NULL,		POLAR_INI_ALL,		NULL)
//   POLAR_INI_ENTRY("sendmail_path",	DEFAULT_SENDMAIL_PATH,	POLAR_INI_SYSTEM,		NULL)
//   POLAR_INI_ENTRY("mail.force_extra_parameters",NULL,		POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		OnChangeMailForceExtra)
//   POLAR_INI_ENTRY("disable_functions",			"",			POLAR_INI_SYSTEM,		NULL)
//   POLAR_INI_ENTRY("disable_classes",			"",			POLAR_INI_SYSTEM,		NULL)
//   POLAR_INI_ENTRY("max_file_uploads",			"20",			POLAR_INI_SYSTEM|POLAR_INI_PERDIR,		NULL)

//   STD_POLAR_INI_BOOLEAN("allow_url_fopen",		"1",		POLAR_INI_SYSTEM,		OnUpdateBool,		allow_url_fopen,		php_core_globals,		core_globals)
//   STD_POLAR_INI_BOOLEAN("allow_url_include",	"0",		POLAR_INI_SYSTEM,		OnUpdateBool,		allow_url_include,		php_core_globals,		core_globals)
//   STD_POLAR_INI_BOOLEAN("enable_post_data_reading",	"1",	POLAR_INI_SYSTEM|POLAR_INI_PERDIR,	OnUpdateBool,	enable_post_data_reading,	php_core_globals,	core_globals)

//   STD_POLAR_INI_ENTRY("realpath_cache_size",	"4096K",	POLAR_INI_SYSTEM,		OnUpdateLong,	realpath_cache_size_limit,	virtual_cwd_globals,	cwd_globals)
//   STD_POLAR_INI_ENTRY("realpath_cache_ttl",		"120",		POLAR_INI_SYSTEM,		OnUpdateLong,	realpath_cache_ttl,			virtual_cwd_globals,	cwd_globals)

//   STD_POLAR_INI_ENTRY("user_ini.filename",		".user.ini",	POLAR_INI_SYSTEM,		OnUpdateString,		user_ini_filename,	php_core_globals,		core_globals)
//   STD_POLAR_INI_ENTRY("user_ini.cache_ttl",		"300",			POLAR_INI_SYSTEM,		OnUpdateLong,		user_ini_cache_ttl,	php_core_globals,		core_globals)
//   STD_POLAR_INI_ENTRY("hard_timeout",			"2",			POLAR_INI_SYSTEM,		OnUpdateLong,		hard_timeout,		zend_executor_globals,	executor_globals)
//#ifdef POLAR_OS_WIN32
//   STD_POLAR_INI_BOOLEAN("windows.show_crt_warning",		"0",		POLAR_INI_ALL,		OnUpdateBool,			windows_show_crt_warning,			php_core_globals,	core_globals)
//#endif
//   STD_POLAR_INI_ENTRY("syslog.facility",		"LOG_USER",		POLAR_INI_SYSTEM,		OnSetFacility,		syslog_facility,	php_core_globals,		core_globals)
//   STD_POLAR_INI_ENTRY("syslog.ident",		"php",			POLAR_INI_SYSTEM,		OnUpdateString,		syslog_ident,		php_core_globals,		core_globals)
//   STD_POLAR_INI_ENTRY("syslog.filter",		"no-ctrl",		POLAR_INI_ALL,		OnSetLogFilter,		syslog_filter,		php_core_globals, 		core_globals)
POLAR_INI_END()

} // polar
