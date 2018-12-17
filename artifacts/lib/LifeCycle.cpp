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

#include "polarphp/global/SystemDetection.h"
#include "LifeCycle.h"
#include "ExecEnv.h"
#include "Output.h"
#include "Ticks.h"
#include "Defs.h"
#include "Reentrancy.h"
#include "Ini.h"

#include <cstring>

namespace polar {

/* True globals (no need for thread safety */
/* But don't make them a single int bitfield */
static int sg_moduleInitialized = 0;
static int sg_moduleStartup = 1;
static int sg_moduleShutdown = 0;

POLAR_DECL_EXPORT int (*php_register_internal_extensions_func)(void) = php_register_internal_extensions;

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
   if (binaryLocation && execEnv.getExecutableFilepath().find('/') == StringRef::npos) {
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
   execEnv.setPolarBinary(binaryLocation);
}

} // anonymous namespace

void cli_ini_defaults(HashTable *configuration_hash)
{
   zval tmp;
   POLAR_INI_DEFAULT("report_zend_debug", "0");
   POLAR_INI_DEFAULT("display_errors", "1");
}

bool php_module_startup(zend_module_entry *additionalModules, uint32_t numAdditionalModules)
{
   zend_utility_functions zuf;
   zend_utility_values zuv;
   bool retval = true;
   int moduleNumber = 0;	/* for REGISTER_INI_ENTRIES() */
   char *phpOs;
   zend_module_entry *module;
   ExecEnv &execEnv = retrieve_global_execenv();
#ifdef POLAR_OS_WIN32
   WORD wVersionRequested = MAKEWORD(2, 0);
   WSADATA wsaData;

   phpOs = "WINNT";

   old_invalid_parameter_handler =
         _set_invalid_parameter_handler(dummy_invalid_parameter_handler);
   if (old_invalid_parameter_handler != nullptr) {
      _set_invalid_parameter_handler(old_invalid_parameter_handler);
   }

   /* Disable the message box for assertions.*/
   _CrtSetReportMode(_CRT_ASSERT, 0);
#else
   phpOs = const_cast<char *>(PHP_OS);
#endif

   (void)ts_resource(0);

#ifdef POLAR_OS_WIN32
   if (!php_win32_init_random_bytes()) {
      fprintf(stderr, "\ncrypt algorithm provider initialization failed\n");
      return false;
   }
#endif

   sg_moduleShutdown = 0;
   sg_moduleStartup = 1;
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
   zuf.fopen_function = php_fopen_wrapper_for_zend;
   zuf.message_handler = php_message_handler_for_zend;
   zuf.get_configuration_directive = php_get_configuration_directive_for_zend;
   zuf.ticks_function = run_ticks;
   zuf.on_timeout = php_on_timeout;
   zuf.stream_open_function = php_stream_open_for_zend;
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

   le_index_ptr = zend_register_list_destructors_ex(nullptr, nullptr, "index pointer", 0);

   /* Register constants */
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_VERSION", PHP_VERSION, sizeof(PHP_VERSION)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_MAJOR_VERSION", PHP_MAJOR_VERSION, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_MINOR_VERSION", PHP_MINOR_VERSION, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_RELEASE_VERSION", PHP_RELEASE_VERSION, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_EXTRA_VERSION", PHP_EXTRA_VERSION, sizeof(PHP_EXTRA_VERSION) - 1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_VERSION_ID", PHP_VERSION_ID, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_ZTS", 1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_DEBUG", PHP_DEBUG, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_OS", phpOs, strlen(phpOs), CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_OS_FAMILY", PHP_OS_FAMILY, sizeof(PHP_OS_FAMILY)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_SAPI", sapi_module.name, strlen(sapi_module.name), CONST_PERSISTENT | CONST_CS | CONST_NO_FILE_CACHE);
   //      REGISTER_MAIN_STRINGL_CONSTANT("DEFAULT_INCLUDE_PATH", PHP_INCLUDE_PATH, sizeof(PHP_INCLUDE_PATH)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PEAR_INSTALL_DIR", PEAR_INSTALLDIR, sizeof(PEAR_INSTALLDIR)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PEAR_EXTENSION_DIR", PHP_EXTENSION_DIR, sizeof(PHP_EXTENSION_DIR)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_EXTENSION_DIR", PHP_EXTENSION_DIR, sizeof(PHP_EXTENSION_DIR)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_PREFIX", PHP_PREFIX, sizeof(PHP_PREFIX)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_BINDIR", PHP_BINDIR, sizeof(PHP_BINDIR)-1, CONST_PERSISTENT | CONST_CS);
   //#ifndef PHP_WIN32
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_MANDIR", PHP_MANDIR, sizeof(PHP_MANDIR)-1, CONST_PERSISTENT | CONST_CS);
   //#endif
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_LIBDIR", PHP_LIBDIR, sizeof(PHP_LIBDIR)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_DATADIR", PHP_DATADIR, sizeof(PHP_DATADIR)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_SYSCONFDIR", PHP_SYSCONFDIR, sizeof(PHP_SYSCONFDIR)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_LOCALSTATEDIR", PHP_LOCALSTATEDIR, sizeof(PHP_LOCALSTATEDIR)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_CONFIG_FILE_PATH", PHP_CONFIG_FILE_PATH, strlen(PHP_CONFIG_FILE_PATH), CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_CONFIG_FILE_SCAN_DIR", PHP_CONFIG_FILE_SCAN_DIR, sizeof(PHP_CONFIG_FILE_SCAN_DIR)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_SHLIB_SUFFIX", PHP_SHLIB_SUFFIX, sizeof(PHP_SHLIB_SUFFIX)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_STRINGL_CONSTANT("PHP_EOL", PHP_EOL, sizeof(PHP_EOL)-1, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_MAXPATHLEN", MAXPATHLEN, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_INT_MAX", ZEND_LONG_MAX, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_INT_MIN", ZEND_LONG_MIN, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_INT_SIZE", SIZEOF_ZEND_LONG, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_FD_SETSIZE", FD_SETSIZE, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_FLOAT_DIG", DBL_DIG, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_DOUBLE_CONSTANT("PHP_FLOAT_EPSILON", DBL_EPSILON, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_DOUBLE_CONSTANT("PHP_FLOAT_MAX", DBL_MAX, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_DOUBLE_CONSTANT("PHP_FLOAT_MIN", DBL_MIN, CONST_PERSISTENT | CONST_CS);

   //#ifdef PHP_WIN32
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_MAJOR",      EG(windows_version_info).dwMajorVersion, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_MINOR",      EG(windows_version_info).dwMinorVersion, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_BUILD",      EG(windows_version_info).dwBuildNumber, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_PLATFORM",   EG(windows_version_info).dwPlatformId, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_SP_MAJOR",   EG(windows_version_info).wServicePackMajor, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_SP_MINOR",   EG(windows_version_info).wServicePackMinor, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_SUITEMASK",  EG(windows_version_info).wSuiteMask, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_PRODUCTTYPE", EG(windows_version_info).wProductType, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_NT_DOMAIN_CONTROLLER", VER_NT_DOMAIN_CONTROLLER, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_NT_SERVER", VER_NT_SERVER, CONST_PERSISTENT | CONST_CS);
   //      REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_NT_WORKSTATION", VER_NT_WORKSTATION, CONST_PERSISTENT | CONST_CS);
   //#endif

   //php_binary_init();
   if (!execEnv.getPolarBinary().empty()) {
      //REGISTER_MAIN_STRINGL_CONSTANT("PHP_BINARY", PG(php_binary), strlen(PG(php_binary)), CONST_PERSISTENT | CONST_CS | CONST_NO_FILE_CACHE);
   } else {
      //REGISTER_MAIN_STRINGL_CONSTANT("PHP_BINARY", "", 0, CONST_PERSISTENT | CONST_CS | CONST_NO_FILE_CACHE);
   }

   php_output_register_constants();

   /* this will read in php.ini, set up the configuration parameters,
            load zend extensions and register php function extensions
            to be loaded later */
   //   if (php_init_config() == FAILURE) {
   //      return FAILURE;
   //   }

   //   /* Register PHP core ini entries */
   //REGISTER_INI_ENTRIES();
   /* Register Zend ini entries */
   // zend_register_standard_ini_entries();
#ifdef ZEND_WIN32
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

   /* Disable realpath cache if an open_basedir is set */
   //   if (PG(open_basedir) && *PG(open_basedir)) {
   //      CWDG(realpath_cache_size_limit) = 0;
   //   }

   // PG(have_called_openlog) = 0;

   /* initialize stream wrappers registry
          * (this uses configuration parameters from php.ini)
          */
   //   if (php_init_stream_wrappers(moduleNumber) == FAILURE)	{
   //      php_printf("PHP:  Unable to initialize stream url wrappers.\n");
   //      return FAILURE;
   //   }
   zuv.html_errors = 1;
   zuv.import_use_extension = ".php";
   zuv.import_use_extension_length = (uint32_t)strlen(zuv.import_use_extension);
   zend_set_utility_values(&zuv);
   /* startup extensions statically compiled in */
   if (php_register_internal_extensions_func() == FAILURE) {
      php_printf("Unable to start builtin modules\n");
      return false;
   }
   /* start additional PHP extensions */
   // php_register_extensions_bc(additional_modules, num_additional_modules);

   /* load and startup extensions compiled as shared objects (aka DLLs)
            as requested by php.ini entries
            these are loaded after initialization of internal extensions
            as extensions *might* rely on things from ext/standard
            which is always an internal extension and to be initialized
            ahead of all other internals
          */
   // php_ini_register_extensions();
   zend_startup_modules();
   /* start Zend extensions */
   zend_startup_extensions();
   zend_collect_module_handlers();
   /* register additional functions */
   //   if (sapi_module.additional_functions) {
   //      if ((module = zend_hash_str_find_ptr(&module_registry, "standard", sizeof("standard")-1)) != nullptr) {
   //         EG(current_module) = module;
   //         zend_register_functions(nullptr, sapi_module.additional_functions, nullptr, MODULE_PERSISTENT);
   //         EG(current_module) = nullptr;
   //      }
   //   }
   /* disable certain classes and functions as requested by php.ini */
   //   php_disable_functions();
   //   php_disable_classes();

   /* make core report what it should */
   //   if ((module = zend_hash_str_find_ptr(&module_registry, "core", sizeof("core")-1)) != nullptr) {
   //      module->version = PHP_VERSION;
   //      module->info_func = PHP_MINFO(php_core);
   //   }
   if (zend_post_startup() != SUCCESS) {
      return false;
   }
   sg_moduleInitialized = 1;
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
      //      unsigned int i;
      //      zend_try {
      //         /* 2 = Count of deprecation structs */
      //         for (i = 0; i < 2; i++) {
      //            const char **p = directives[i].directives;
      //            while(*p) {
      //               zend_long value;
      //               if (cfg_get_long((char*)*p, &value) == SUCCESS && value) {
      //                  zend_error(directives[i].error_level, directives[i].phrase, *p);
      //               }
      //               ++p;
      //            }
      //         }
      //      } zend_catch {
      //         retval = false;
      //      } zend_end_try();
   }
   virtual_cwd_deactivate();
   execEnv.deactivate();
   sg_moduleStartup = 0;
   shutdown_memory_manager(1, 0);
   virtual_cwd_activate();
   zend_interned_strings_switch_storage(1);
#if ZEND_RC_DEBUG
   zend_rc_debug = 1;
#endif
   /* we're done */
   return retval;
}

void php_module_shutdown()
{
   int module_number = 0;	/* for UNREGISTER_INI_ENTRIES() */
   sg_moduleShutdown = 1;
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
   sg_moduleInitialized = 0;
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

   zend_try {
      execEnv.setInErrorLog(false);
      execEnv.setDuringExecEnvStartup(true);
      php_output_activate();
      /* initialize global variables */
      execEnv.setModulesActivated(false);
      execEnv.setInUserInclude(false);
      zend_activate();
      execEnv.activate();

#ifdef ZEND_SIGNALS
      zend_signal_activate();
#endif
      /* Disable realpath cache if an open_basedir is set */
      if (!execEnv.getOpenBaseDir().empty()) {
         CWDG(realpath_cache_size_limit) = 0;
      }
      StringRef outputHandler = execEnv.getOutputHandler();
      zend_long outputBuffering = execEnv.getOutputBuffering();
      bool implicitFlush = execEnv.getImplicitFlush();
      if (!outputHandler.empty()) {
         zval oh;
         ZVAL_STRING(&oh, outputHandler.getData());
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
      execEnv.setModulesActivated(true);
   } zend_catch {
      retval = FAILURE;
   } zend_end_try()
   execEnv.setStarted(true);
   return retval;
}

void php_exec_env_shutdown()
{

}

} // polar
