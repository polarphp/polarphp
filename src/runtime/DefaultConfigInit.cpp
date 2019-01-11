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

#include "polarphp/runtime/Ini.h"
#include "polarphp/runtime/ExecEnv.h"
#include "polarphp/runtime/BuildinIniModifyHandler.h"

namespace polar {
namespace runtime {

ExecEnvInfo &sg_execEnvInfo = retrieve_global_execenv_runtime_info();

///
///TODO remove short opentags support
///
POLAR_INI_BEGIN()
   POLAR_INI_ENTRY_EX("highlight.comment",          HL_COMMENT_COLOR,       POLAR_INI_ALL,                     nullptr,                          zend_ini_color_displayer_cb)
   POLAR_INI_ENTRY_EX("highlight.default",          HL_DEFAULT_COLOR,       POLAR_INI_ALL,                     nullptr,                          zend_ini_color_displayer_cb)
   POLAR_INI_ENTRY_EX("highlight.html",             HL_HTML_COLOR,          POLAR_INI_ALL,                     nullptr,                          zend_ini_color_displayer_cb)
   POLAR_INI_ENTRY_EX("highlight.keyword",          HL_KEYWORD_COLOR,       POLAR_INI_ALL,                     nullptr,                          zend_ini_color_displayer_cb)
   POLAR_INI_ENTRY_EX("highlight.string",           HL_STRING_COLOR,        POLAR_INI_ALL,                     nullptr,                          zend_ini_color_displayer_cb)
   POLAR_STD_INI_ENTRY_EX("display_errors",         "1",                    POLAR_INI_ALL,                     update_display_errors_handler,    displayErrors,              ExecEnvInfo,           sg_execEnvInfo, display_errors_mode)
   POLAR_STD_INI_BOOLEAN("display_startup_errors",  "0",                    POLAR_INI_ALL,                     update_bool_handler,              displayStartupErrors,       ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_BOOLEAN("enable_dl",               "1",                    POLAR_INI_SYSTEM,                  update_bool_handler,              enableDl,                   ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("docref_root",               "",                     POLAR_INI_ALL,                     update_string_handler,            docrefRoot,                 ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("docref_ext",                "",                     POLAR_INI_ALL,                     update_string_handler,            docrefExt,                  ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_BOOLEAN("implicit_flush",          "0",                    POLAR_INI_ALL,                     update_bool_handler,              implicitFlush,              ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_BOOLEAN("log_errors",              "0",                    POLAR_INI_ALL,                     update_bool_handler,              logErrors,                  ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("log_errors_max_len",        "1024",                 POLAR_INI_ALL,                     update_long_handler,              logErrorsMaxLen,            ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_BOOLEAN("ignore_repeated_errors",  "0",                    POLAR_INI_ALL,                     update_bool_handler,              ignoreRepeatedErrors,       ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_BOOLEAN("ignore_repeated_source",  "0",                    POLAR_INI_ALL,                     update_bool_handler,              ignoreRepeatedSource,       ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_BOOLEAN("report_memleaks",         "1",                    POLAR_INI_ALL,                     update_bool_handler,              reportMemLeaks,             ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_BOOLEAN("report_zend_debug",       "1",                    POLAR_INI_ALL,                     update_bool_handler,              reportZendDebug,            ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("output_buffering",          "0",                    POLAR_INI_PERDIR|POLAR_INI_SYSTEM, update_long_handler,              outputBuffering,            ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("output_handler",            "",                     POLAR_INI_PERDIR|POLAR_INI_SYSTEM, update_string_handler,            outputHandler,              ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_BOOLEAN("register_argc_argv",      "1",                    POLAR_INI_PERDIR|POLAR_INI_SYSTEM, update_bool_handler,              registerArgcArgv,           ExecEnvInfo,           sg_execEnvInfo)
   ///POLAR_STD_INI_BOOLEAN("short_open_tag",       DEFAULT_SHORT_OPEN_TAG, POLAR_INI_SYSTEM|POLAR_INI_PERDIR, update_bool_handler,              shortTags,                  zend_compiler_globals, compiler_globals)
   POLAR_STD_INI_BOOLEAN("track_errors",            "0",                    POLAR_INI_ALL,                     update_bool_handler,              trackErrors,                ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("unserialize_callback_func", "",                     POLAR_INI_ALL,                     update_string_handler,            unserializeCallbackFunc,    ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("serialize_precision",       "-1",                   POLAR_INI_ALL,                     set_serialize_precision_handler,  serializePrecision,         ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("auto_append_file",          "",                     POLAR_INI_SYSTEM|POLAR_INI_PERDIR, update_string_handler,            autoAppendFile,             ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("auto_prepend_file",         "",                     POLAR_INI_SYSTEM|POLAR_INI_PERDIR, update_string_handler,            autoPrependFile,            ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("doc_root",                  "",                     POLAR_INI_SYSTEM,                  update_string_unempty_handler,    docRoot,                    ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("internal_encoding",         "",                     POLAR_INI_ALL,                     update_internal_encoding_handler, internalEncoding,           ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("error_log",                 "",                     POLAR_INI_ALL,                     update_error_log_handler,         errorLog,                   ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("extension_dir",             POLARPHP_EXTENSION_DIR, POLAR_INI_SYSTEM,                  update_string_unempty_handler,    extensionDir,               ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("sys_temp_dir",              "",                     POLAR_INI_SYSTEM,                  update_string_unempty_handler,    sysTempDir,                 ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("include_path",              POLARPHP_INCLUDE_PATH,  POLAR_INI_ALL,                     update_string_unempty_handler,    includePath,                ExecEnvInfo,           sg_execEnvInfo)
   POLAR_INI_ENTRY("max_execution_time",            "30",                   POLAR_INI_ALL,                     update_timeout_handler)
   POLAR_STD_INI_ENTRY("open_basedir",              "",                     POLAR_INI_ALL,                     update_base_dir_handler,          openBaseDir,                ExecEnvInfo,           sg_execEnvInfo)

   POLAR_STD_INI_ENTRY("user_dir",                  "",                     POLAR_INI_SYSTEM,                  update_string_handler,            userDir,                    ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("error_append_string",       "",                     POLAR_INI_ALL,                     update_string_handler,            errorAppendString,          ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("error_prepend_string",      "",                     POLAR_INI_ALL,                     update_string_handler,            errorPrependString,         ExecEnvInfo,           sg_execEnvInfo)

   POLAR_INI_ENTRY("memory_limit",                  "128M",                 POLAR_INI_ALL,                     change_memory_limit_handler)
   POLAR_INI_ENTRY("precision",                     "14",                   POLAR_INI_ALL,                     set_precision_handler)
   POLAR_INI_ENTRY("disable_functions",             "",                     POLAR_INI_SYSTEM,                  nullptr)
   POLAR_INI_ENTRY("disable_classes",               "",                     POLAR_INI_SYSTEM,                  nullptr)
   POLAR_INI_ENTRY("max_file_uploads",              "20",                   POLAR_INI_SYSTEM|POLAR_INI_PERDIR, nullptr)
   STD_ZEND_INI_ENTRY("realpath_cache_size",        "4096K",                POLAR_INI_SYSTEM,                  OnUpdateLong,                      realpath_cache_size_limit, virtual_cwd_globals,   cwd_globals)
   STD_ZEND_INI_ENTRY("realpath_cache_ttl",         "120",                  POLAR_INI_SYSTEM,                  OnUpdateLong,                      realpath_cache_ttl,        virtual_cwd_globals,   cwd_globals)

   POLAR_STD_INI_ENTRY("user_ini.filename",         ".user.ini",            POLAR_INI_SYSTEM,                  update_string_handler,             userIniFilename,           ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("user_ini.cache_ttl",        "300",                  POLAR_INI_SYSTEM,                  update_long_handler,               userIniCacheTtl,           ExecEnvInfo,           sg_execEnvInfo)
   STD_ZEND_INI_ENTRY("hard_timeout",               "2",                    POLAR_INI_SYSTEM,                  OnUpdateLong,                      hard_timeout,              zend_executor_globals, executor_globals)
   #ifdef POLAR_OS_WIN32
   POLAR_STD_INI_BOOLEAN("windows.show_crt_warning","0",                    POLAR_INI_ALL,                     update_bool_handler,               windowsShowCrtWarning,     ExecEnvInfo,           sg_execEnvInfo)
   #endif
   POLAR_STD_INI_ENTRY("syslog.facility",           "LOG_USER",             POLAR_INI_SYSTEM,                  set_facility_handler,              syslogFacility,            ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("syslog.ident",              "php",                  POLAR_INI_SYSTEM,                  update_string_handler,             syslogIdent,               ExecEnvInfo,           sg_execEnvInfo)
   POLAR_STD_INI_ENTRY("syslog.filter",             "no-ctrl",              POLAR_INI_ALL,                     set_log_filter_handler,            syslogFilter,              ExecEnvInfo,           sg_execEnvInfo)
POLAR_INI_END()

} //runtime
} //polar
