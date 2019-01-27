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

#include "polarphp/global/SystemDetection.h"
#include "polarphp/runtime/LifeCycle.h"
#include "polarphp/runtime/ExecEnv.h"
#include "polarphp/runtime/Output.h"
#include "polarphp/runtime/RtDefs.h"
#include "polarphp/runtime/Ini.h"
#include "polarphp/runtime/Reentrancy.h"
#include "polarphp/runtime/Spprintf.h"

#include "polarphp/runtime/Ticks.h"
#include "polarphp/global/Config.h"

#include <cstring>
#include <sys/wait.h>

namespace polar {
namespace runtime {

/// True globals (no need for thread safety
/// But don't make them a single int bitfield
/// TODO review polarphp bootstrap
///
bool sg_moduleInitialized = false;
bool sg_moduleStartup = true;
bool sg_moduleShutdown = false;

extern zend_ini_entry_def ini_entries[];

namespace {

void php_binary_init()
{
   char *binaryLocation = nullptr;
#ifdef POLAR_OS_WIN32
   binaryLocation = (char *)malloc(MAXPATHLEN);
   if (binaryLocation && GetModuleFileName(0, binaryLocation, MAXPATHLEN) == 0) {
      free(binaryLocation);
      PG(php_binary) = nullptr;
   }
#else
   binaryLocation = reinterpret_cast<char *>(malloc(MAXPATHLEN));
   ExecEnv &execEnv = retrieve_global_execenv();
   ExecEnvInfo &execEnvInfo = execEnv.getRuntimeInfo();
   if (binaryLocation &&  execEnv.getExecutableFilepath().find('/') == StringRef::npos) {
      char *envpath, *path;
      int found = 0;
      if ((envpath = getenv("PATH")) != nullptr) {
         char *searchDir, search_path[MAXPATHLEN];
         char *last = nullptr;
         zend_stat_t s;
         path = estrdup(envpath);
         searchDir = polar_strtok_r(path, ":", &last);
         while (searchDir) {
            snprintf(search_path, MAXPATHLEN, "%s/%s", searchDir, execEnv.getExecutableFilepath().getData());
            if (VCWD_REALPATH(search_path, binaryLocation) && !VCWD_ACCESS(binaryLocation, X_OK) && VCWD_STAT(binaryLocation, &s) == 0 && S_ISREG(s.st_mode)) {
               found = 1;
               break;
            }
            searchDir = polar_strtok_r(nullptr, ":", &last);
         }
         efree(path);
      }
      if (!found) {
         free(binaryLocation);
         binaryLocation = nullptr;
      }
   } else if (!VCWD_REALPATH(execEnv.getExecutableFilepath().getData(), binaryLocation) || VCWD_ACCESS(binaryLocation, X_OK)) {
      free(binaryLocation);
      binaryLocation = nullptr;
   }
#endif
   execEnvInfo.polarBinary = binaryLocation;
}

} // anonymous namespace

int php_during_module_startup()
{
   return sg_moduleStartup;
}

int php_during_module_shutdown()
{
   return sg_moduleShutdown;
}

int php_get_module_initialized()
{
   return sg_moduleInitialized;
}

void cli_ini_defaults(HashTable *configuration_hash)
{
   zval tmp;
   POLAR_INI_DEFAULT("report_zend_debug", "0");
   POLAR_INI_DEFAULT("display_errors", "1");
}

void php_disable_functions()
{

}

void php_disable_classes()
{

}

bool php_register_extensions(std::list<zend_module_entry *> extensions)
{
   for (zend_module_entry *extension : extensions) {
      if (extension != nullptr) {
         if (zend_register_internal_module(extension) == nullptr) {
            return false;
         }
      }
   }
   return true;
}

namespace {
void php_register_buildin_constants();
} // anonymous namespace

bool php_module_startup()
{
   zend_utility_functions zuf;
   zend_utility_values zuv;
   bool retval = true;
   int module_number = 0;	/* for REGISTER_INI_ENTRIES() */
   ExecEnv &execEnv = retrieve_global_execenv();
   ExecEnvInfo &execEnvInfo = execEnv.getRuntimeInfo();
#ifdef POLAR_OS_WIN32
   WORD wVersionRequested = MAKEWORD(2, 0);
   WSADATA wsaData;
   old_invalid_parameter_handler =
         _set_invalid_parameter_handler(dummy_invalid_parameter_handler);
   if (old_invalid_parameter_handler != nullptr) {
      _set_invalid_parameter_handler(old_invalid_parameter_handler);
   }

   /* Disable the message box for assertions.*/
   _CrtSetReportMode(_CRT_ASSERT, 0);
#endif

   (void)ts_resource(0);

#ifdef POLAR_OS_WIN32
   if (!php_win32_init_random_bytes()) {
      fprintf(stderr, "\ncrypt algorithm provider initialization failed\n");
      return false;
   }
#endif

   sg_moduleShutdown = false;
   sg_moduleStartup = true;
   execEnv.activate();

   if (sg_moduleInitialized) {
      return true;
   }
   php_output_startup();
   startup_ticks();
   gc_globals_ctor();

   zuf.error_function = php_error_callback;
   zuf.printf_function = php_printf;
   zuf.write_function = php_output_wrapper;
   /// polarphp does not use php stream
   zuf.fopen_function = nullptr;
   zuf.stream_open_function = nullptr;
   /// need review whether need execute timeout mechanism
   zuf.on_timeout = nullptr;
   zuf.message_handler = php_message_handler_for_zend;
   zuf.get_configuration_directive = php_get_configuration_directive_for_zend;
   zuf.ticks_function = run_ticks;
   zuf.printf_to_smart_string_function = php_printf_to_smart_string;
   zuf.printf_to_smart_str_function = php_printf_to_smart_str;
   zuf.getenv_function = bootstrap_getenv;
   zuf.resolve_path_function = php_resolve_path_for_zend;
   zend_startup(&zuf, nullptr);

#if HAVE_SETLOCALE
   setlocale(LC_CTYPE, "");
   zend_update_current_locale();
#endif

#if HAVE_TZSET
   tzset();
#endif

#ifdef POLAR_OS_WIN32
   /* start up winsock services */
   if (WSAStartup(wVersionRequested, &wsaData) != 0) {
      php_printf("\nwinsock.dll unusable. %d\n", WSAGetLastError());
      return FAILURE;
   }
#endif

   /// zendVM global variable
   le_index_ptr = zend_register_list_destructors_ex(nullptr, nullptr, "index pointer", 0);

   /* Register constants */
   php_register_buildin_constants();

   php_binary_init();
   std::string &polarBinary = execEnvInfo.polarBinary;
   if (!polarBinary.empty()) {
      REGISTER_MAIN_STRINGL_CONSTANT("POLAR_BINARY", const_cast<char *>(polarBinary.c_str()), polarBinary.size(), CONST_PERSISTENT | CONST_CS | CONST_NO_FILE_CACHE);
   } else {
      REGISTER_MAIN_STRINGL_CONSTANT("POLAR_BINARY", PHP_EMPTY_STR, 0, CONST_PERSISTENT | CONST_CS | CONST_NO_FILE_CACHE);
   }
   php_output_register_constants();

   /// this will read in php.ini, set up the configuration parameters,
   /// load zend extensions and register php function extensions
   /// to be loaded later
   if (!php_init_config()) {
      return false;
   }
   /// Register PHP core ini entries
   /// TODO refactor
   ///
   REGISTER_INI_ENTRIES();
   /* Register Zend ini entries */
   zend_register_standard_ini_entries();

#ifdef POLAR_OS_WIN32
   /* Until the current ini values was setup, the current cp is 65001.
            If the actual ini vaues are different, some stuff needs to be updated.
            It concerns at least main_cwd_state and there might be more. As we're
            still in the startup phase, lets use the chance and reinit the relevant
            item according to the current codepage. Still, if ini_set() is used
            later on, a more intelligent way to update such stuff is needed.
            Startup/shutdown routines could involve touching globals and thus
            can't always be used on demand. */
   if (!php_win32_cp_use_unicode()) {
      virtual_cwd_main_cwd_init(1);
   }
#endif

   /// Disable realpath cache if an open_basedir is set
   std::string &openBaseDir = execEnvInfo.openBaseDir;
   if (!openBaseDir.empty()) {
      CWDG(realpath_cache_size_limit) = 0;
   }
   execEnvInfo.haveCalledOpenlog = false;
   /// polarphp does not use html errors
   zuv.html_errors = 0;
   zuv.import_use_extension = const_cast<char *>(".php");
   zuv.import_use_extension_length = (uint32_t)strlen(zuv.import_use_extension);
   zend_set_utility_values(&zuv);

   /* startup extensions statically compiled in */
   if (!php_register_internal_extensions()) {
      php_printf("Unable to start polarphp standard library (libpdk)\n");
      return false;
   }

   /// load and startup extensions compiled as shared objects (aka DLLs)
   /// as requested by php.ini entries
   /// these are loaded after initialization of internal extensions
   /// as extensions *might* rely on things from ext/standard
   /// which is always an internal extension and to be initialized
   /// ahead of all other internals

   php_ini_register_extensions();
   zend_startup_modules();
   /* start Zend extensions */
   zend_startup_extensions();
   zend_collect_module_handlers();
   /* disable certain classes and functions as requested by php.ini */
   php_disable_functions();
   php_disable_classes();

   if (zend_post_startup() != SUCCESS) {
      return false;
   }
   sg_moduleInitialized = true;
   /* Check for deprecated directives */
   /* NOTE: If you add anything here, remember to add it to Makefile.global! */
   {
      struct {
         const long error_level;
         const char *phrase;
         const char *directives[17]; /* Remember to change this if the number of directives change */
      } directives[2] = {
      {
         E_DEPRECATED,
               "Directive '%s' is deprecated",
         {
            "track_errors",
            nullptr
         }
      },
      {
         E_CORE_ERROR,
               "Directive '%s' is no longer available in PHP",
         {
            "allow_call_time_pass_reference",
            "asp_tags",
            "define_syslog_variables",
            "highlight.bg",
            "magic_quotes_gpc",
            "magic_quotes_runtime",
            "magic_quotes_sybase",
            "register_globals",
            "register_long_arrays",
            "safe_mode",
            "safe_mode_gid",
            "safe_mode_include_dir",
            "safe_mode_exec_dir",
            "safe_mode_allowed_env_vars",
            "safe_mode_protected_env_vars",
            "zend.ze1_compatibility_mode",
            nullptr
         }
      }
   };
      unsigned int i;
      polar_try {
         /* 2 = Count of deprecation structs */
         for (i = 0; i < 2; i++) {
            const char **p = directives[i].directives;
            while(*p) {
               zend_long value;
               if (cfg_get_long(const_cast<char*>(*p), &value) == SUCCESS && value) {
                  zend_error(directives[i].error_level, const_cast<char *>(directives[i].phrase), *p);
               }
               ++p;
            }
         }
      } polar_catch {
         retval = false;
      } polar_end_try;
   }
   virtual_cwd_deactivate();
   execEnv.deactivate();
   sg_moduleStartup = false;
   shutdown_memory_manager(1, 0);
   virtual_cwd_activate();
   zend_interned_strings_switch_storage(1);
#if ZEND_RC_DEBUG
   zend_rc_debug = 1;
#endif
   /* we're done */
   return retval;
}

namespace {
void php_register_buildin_constants()
{
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_VERSION", const_cast<char *>(POLARPHP_VERSION), sizeof(POLARPHP_VERSION)-1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_VERSION_MAJOR", POLARPHP_VERSION_MAJOR, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_VERSION_MINOR", POLARPHP_VERSION_MINOR, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_VERSION_PATCH", POLARPHP_VERSION_PATCH, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_VERSION_SUFFIX", const_cast<char *>(POLARPHP_VERSION_SUFFIX), sizeof(POLARPHP_VERSION_SUFFIX) - 1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_VERSION_ID", POLARPHP_VERSION_ID, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_ZTS", 1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_DEBUG", PHP_DEBUG, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_OS", const_cast<char *>(PHP_OS), strlen(PHP_OS), CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_OS_FAMILY", const_cast<char *>(POLAR_SYSTEM_NAME), sizeof(POLAR_SYSTEM_NAME)-1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_EXEC_ENV", const_cast<char *>("cli"), strlen("cli"), CONST_PERSISTENT | CONST_CS | CONST_NO_FILE_CACHE);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_DEFAULT_INCLUDE_PATH", const_cast<char *>(POLARPHP_INCLUDE_PATH), sizeof(POLARPHP_INCLUDE_PATH)-1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_EXTENSION_DIR", const_cast<char *>(POLARPHP_EXTENSION_DIR), sizeof(POLARPHP_EXTENSION_DIR)-1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_PREFIX", const_cast<char *>(POLARPHP_PREFIX), sizeof(POLARPHP_PREFIX)-1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_BINDIR", const_cast<char *>(POLARPHP_BINDIR), sizeof(POLARPHP_BINDIR)-1, CONST_PERSISTENT | CONST_CS);
#ifndef POLAR_OS_WIN32
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_MANDIR", const_cast<char *>(POLARPHP_MAN_DIR), sizeof(POLARPHP_MAN_DIR)-1, CONST_PERSISTENT | CONST_CS);
#endif
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_LIBDIR", const_cast<char *>(POLARPHP_LIBDIR), sizeof(POLARPHP_LIBDIR)-1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_DATADIR", const_cast<char *>(POLARPHP_DATADIR), sizeof(POLARPHP_DATADIR)-1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_SYSCONFDIR", const_cast<char *>(POLARPHP_SYSCONFDIR), sizeof(POLARPHP_SYSCONFDIR)-1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_LOCALSTATEDIR", const_cast<char *>(POLARPHP_LOCALSTATEDIR), sizeof(POLARPHP_LOCALSTATEDIR)-1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_CONFIG_FILE_PATH", const_cast<char *>(POLARPHP_CONFIG_FILE_PATH), strlen(POLARPHP_CONFIG_FILE_PATH), CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_CONFIG_FILE_SCAN_DIR", const_cast<char *>(POLARPHP_CONFIG_FILE_SCAN_DIR), sizeof(POLARPHP_CONFIG_FILE_SCAN_DIR)-1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_SHLIB_SUFFIX", const_cast<char *>(POLARPHP_SHLIB_SUFFIX), sizeof(POLARPHP_SHLIB_SUFFIX)-1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_STRINGL_CONSTANT("POLARPHP_EOL", const_cast<char *>(PHP_EOL), sizeof(PHP_EOL)-1, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_MAXPATHLEN", MAXPATHLEN, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_INT_MAX", ZEND_LONG_MAX, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_INT_MIN", ZEND_LONG_MIN, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_INT_SIZE", SIZEOF_ZEND_LONG, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_FD_SETSIZE", FD_SETSIZE, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_FLOAT_DIG", DBL_DIG, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_DOUBLE_CONSTANT("POLARPHP_FLOAT_EPSILON", DBL_EPSILON, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_DOUBLE_CONSTANT("POLARPHP_FLOAT_MAX", DBL_MAX, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_DOUBLE_CONSTANT("POLARPHP_FLOAT_MIN", DBL_MIN, CONST_PERSISTENT | CONST_CS);

#ifdef POLAR_OS_WIN32
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_WINDOWS_VERSION_MAJOR",      EG(windows_version_info).dwMajorVersion, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_WINDOWS_VERSION_MINOR",      EG(windows_version_info).dwMinorVersion, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_WINDOWS_VERSION_BUILD",      EG(windows_version_info).dwBuildNumber, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_WINDOWS_VERSION_PLATFORM",   EG(windows_version_info).dwPlatformId, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_WINDOWS_VERSION_SP_MAJOR",   EG(windows_version_info).wServicePackMajor, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_WINDOWS_VERSION_SP_MINOR",   EG(windows_version_info).wServicePackMinor, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_WINDOWS_VERSION_SUITEMASK",  EG(windows_version_info).wSuiteMask, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_WINDOWS_VERSION_PRODUCTTYPE", EG(windows_version_info).wProductType, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_WINDOWS_NT_DOMAIN_CONTROLLER", VER_NT_DOMAIN_CONTROLLER, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_WINDOWS_NT_SERVER", VER_NT_SERVER, CONST_PERSISTENT | CONST_CS);
   REGISTER_MAIN_LONG_CONSTANT("POLARPHP_WINDOWS_NT_WORKSTATION", VER_NT_WORKSTATION, CONST_PERSISTENT | CONST_CS);
#endif
}
} // anonymous namespace

void php_module_shutdown()
{
   int module_number = 0;	/* for UNREGISTER_INI_ENTRIES() */
   sg_moduleShutdown = true;
   if (!sg_moduleInitialized) {
      return;
   }
   zend_interned_strings_switch_storage(0);
   ts_free_worker_threads();

#if ZEND_RC_DEBUG
   zend_rc_debug = 0;
#endif

#ifdef POLAR_OS_WIN32
   (void)php_win32_shutdown_random_bytes();
#endif
   zend_shutdown();
#ifdef POLAR_OS_WIN32
   /*close winsock */
   WSACleanup();
#endif
   UNREGISTER_INI_ENTRIES();
   /* close down the ini config */
   php_shutdown_config();
   zend_ini_global_shutdown();
   php_output_shutdown();
   /// interned_strings
   /// shutdown by zend_startup
   /// tsrm_set_shutdown_handler(zend_interned_strings_dtor);
   sg_moduleInitialized = false;
   shutdown_ticks();
   /// ZTS mode dtor call by tsrm
   // gc_globals_dtor();
#ifdef PHP_WIN32
   if (old_invalid_parameter_handler == NULL) {
      _set_invalid_parameter_handler(old_invalid_parameter_handler);
   }
#endif
}


#ifdef PHP_SIGCHILD
namespace {
void sigchld_handler(int)
{
   int errnoSave = errno;
   while (::waitpid(-1, nullptr, WNOHANG) > 0);
   ::signal(SIGCHLD, sigchld_handler);
   errno = errnoSave;
}
} // anonymous namespace
#endif

bool php_exec_env_startup()
{
   int retval = true;
   zend_interned_strings_activate();
   ExecEnv &execEnv = retrieve_global_execenv();
   ExecEnvInfo &execEnvInfo = execEnv.getRuntimeInfo();

   /// TODO dtrace
   //#ifdef HAVE_DTRACE
   //   DTRACE_REQUEST_STARTUP(SAFE_FILENAME(SG(request_info).path_translated), SAFE_FILENAME(SG(request_info).request_uri), (char *)SAFE_FILENAME(SG(request_info).request_method));
   //#endif /* HAVE_DTRACE */
#ifdef POLAR_OS_WIN32
   _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
   execEnv.setComInitialized(false);
#endif

#ifdef PHP_SIGCHILD
   ::signal(SIGCHLD, sigchld_handler);
#endif

   polar_try {
      execEnvInfo.inErrorLog = false;
      execEnvInfo.duringExecEnvStartup = true;
      php_output_activate();
      /* initialize global variables */
      execEnvInfo.modulesActivated = false;
      execEnvInfo.inUserInclude = false;
      zend_activate();
      execEnv.activate();

#ifdef ZEND_SIGNALS
      zend_signal_activate();
#endif
      /* Disable realpath cache if an open_basedir is set */
      if (!execEnvInfo.openBaseDir.empty()) {
         CWDG(realpath_cache_size_limit) = 0;
      }
      std::string &outputHandler = execEnvInfo.outputHandler;
      zend_long outputBuffering = execEnvInfo.outputBuffering;
      bool implicitFlush = execEnvInfo.implicitFlush;
      if (!outputHandler.empty()) {
         zval oh;
         ZVAL_STRING(&oh, outputHandler.c_str());
         php_output_start_user(&oh, 0, PHP_OUTPUT_HANDLER_STDFLAGS);
         zval_ptr_dtor(&oh);
      } else if (outputBuffering) {
         php_output_start_user(nullptr, outputBuffering > 1 ? outputBuffering : 0, PHP_OUTPUT_HANDLER_STDFLAGS);
      } else if (implicitFlush) {
         php_output_set_implicit_flush(1);
      }

      /* We turn this off in php_execute_script() */
      /* PG(during_request_startup) = 0; */

      php_hash_environment();
      zend_activate_modules();
      execEnvInfo.modulesActivated = true;
   } polar_catch {
      retval = FAILURE;
   } polar_end_try;
   execEnv.setEnvReady(true);
   return retval;
}

///
/// current we do not use this function
///
void php_free_cli_exec_globals()
{

}

void php_exec_env_shutdown()
{
   ExecEnv &execEnv = retrieve_global_execenv();
   ExecEnvInfo &execEnvInfo = execEnv.getRuntimeInfo();
   EG(flags) |= EG_FLAGS_IN_SHUTDOWN;
   bool reportMemleaks = execEnvInfo.reportMemLeaks;
   /* EG(current_execute_data) points into nirvana and therefore cannot be safely accessed
       * inside zend_executor callback functions.
       */
   EG(current_execute_data) = nullptr;
   deactivate_ticks();
   bool modulesActivated = execEnvInfo.modulesActivated;
   /* 1. Call all possible shutdown functions registered with register_shutdown_function() */
   if (modulesActivated) {
      polar_try {
         ///
         /// TODO review we need support this hook mechanism in libpdk, not here
         /// but we need from here to invoke libpdk functions
         ///
         /// php_call_shutdown_functions();
         ///
      } polar_end_try;
   }

   /* 2. Call all possible __destruct() functions */
   polar_try {
      zend_call_destructors();
   } polar_end_try;

   /* 3. Flush all output buffers */
   polar_try {
      bool sendBuffer = true;
      if (CG(unclean_shutdown) && execEnvInfo.lastErrorType == E_ERROR &&
          static_cast<size_t>(execEnvInfo.memoryLimit) < zend_memory_usage(1)
          ) {
         sendBuffer = false;
      }
      if (!sendBuffer) {
         php_output_discard_all();
      } else {
         php_output_end_all();
      }
   } polar_end_try;

   /* 4. Reset max_execution_time (no longer executing php code after response sent) */
   polar_try {
      ///
      /// review we does not use timeout
      ///
      zend_unset_timeout();
   } polar_end_try;
   /* 5. Call all extensions RSHUTDOWN functions */
   if (modulesActivated) {
      zend_deactivate_modules();
   }

   /* 6. Shutdown output layer (send the set HTTP headers, cleanup output handlers, etc.) */
   polar_try {
      php_output_deactivate();
   } polar_end_try;

   /* 7. Free shutdown functions */
   if (modulesActivated) {
      ///
      /// TODO review we need support this hook mechanism in libpdk, not here
      /// but we need from here to invoke libpdk functions
      ///
      /// php_free_shutdown_functions();
   }

   /// polarphp wether support
   //   /* 8. Destroy super-globals */
   //   polar_try {
   //      int i;

   //      for (i=0; i<NUM_TRACK_VARS; i++) {
   //         zval_ptr_dtor(&PG(http_globals)[i]);
   //      }
   //   } polar_end_try;

   /* 9. free request-bound globals */
   php_free_cli_exec_globals();

   /* 10. Shutdown scanner/executor/compiler and restore ini entries */
   zend_deactivate();

   /* 11. Call all extensions post-RSHUTDOWN functions */
   polar_try {
      zend_post_deactivate_modules();
   } polar_end_try;

   /* 12. exec env related shutdown (free stuff) */
   polar_try {
      execEnv.deactivate();
   } polar_end_try;

   /* 13. free virtual CWD memory */
   virtual_cwd_deactivate();

   /* 14. Free Willy (here be crashes) */
   zend_interned_strings_deactivate();
   polar_try {
      shutdown_memory_manager(CG(unclean_shutdown) || !reportMemleaks, 0);
   } polar_end_try;

   /* 15. Reset max_execution_time */
   polar_try {
      zend_unset_timeout();
   } polar_end_try;

#ifdef POLAR_OS_WIN32
   if (PG(com_initialized) ) {
      CoUninitialize();
      PG(com_initialized) = 0;
   }
#endif

   ///
   /// TODO add dtrace support
   ///
#ifdef HAVE_DTRACE
   // DTRACE_REQUEST_SHUTDOWN(SAFE_FILENAME(SG(request_info).path_translated), SAFE_FILENAME(SG(request_info).request_uri), (char *)SAFE_FILENAME(SG(request_info).request_method));
#endif /* HAVE_DTRACE */
}

} // runtime
} // polar
