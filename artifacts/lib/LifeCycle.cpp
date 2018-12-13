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

#include "LifeCycle.h"
#include "polarphp/global/SystemDetection.h"

namespace polar {

/* True globals (no need for thread safety */
/* But don't make them a single int bitfield */
static int sg_moduleInitialized = 0;
static int sg_moduleStartup = 1;
static int sg_moduleShutdown = 0;

bool php_moduleStartup(zend_module_entry *additionalModules, uint32_t numAdditionalModules)
{
   zend_utility_functions zuf;
   zend_utility_values zuv;
   bool retval = true;
   int moduleNumber=0;	/* for REGISTER_INI_ENTRIES() */
   char *phpOs;
   zend_module_entry *module;

#ifdef POLAR_OS_WIN32
   WORD wVersionRequested = MAKEWORD(2, 0);
   WSADATA wsaData;

   phpOs = "WINNT";

   old_invalid_parameter_handler =
         _set_invalid_parameter_handler(dummy_invalid_parameter_handler);
   if (old_invalid_parameter_handler != NULL) {
      _set_invalid_parameter_handler(old_invalid_parameter_handler);
   }

   /* Disable the message box for assertions.*/
   _CrtSetReportMode(_CRT_ASSERT, 0);
#else
   phpOs = PHP_OS;
#endif

#ifdef ZTS
   (void)ts_resource(0);
#endif

#ifdef POLAR_OS_WIN32
   if (!php_win32_init_random_bytes()) {
      fprintf(stderr, "\ncrypt algorithm provider initialization failed\n");
      return false;
   }
#endif

   sg_moduleShutdown = 0;
   sg_moduleStartup = 1;

   if (sg_moduleInitialized) {
      return true;
   }
//   php_output_startup();
//#ifdef ZTS
//   ts_allocate_id(&core_globals_id, sizeof(php_core_globals), (ts_allocate_ctor) core_globals_ctor, (ts_allocate_dtor) core_globals_dtor);
//#ifdef PHP_WIN32
//   ts_allocate_id(&php_win32_core_globals_id, sizeof(php_win32_core_globals), (ts_allocate_ctor) php_win32_core_globals_ctor, (ts_allocate_dtor) php_win32_core_globals_dtor);
//#endif
//#else
//   memset(&core_globals, 0, sizeof(core_globals));
//   php_startup_ticks();
//#endif
//   gc_globals_ctor();

//   zuf.error_function = php_error_cb;
//   zuf.printf_function = php_printf;
//   zuf.write_function = php_output_wrapper;
//   zuf.fopen_function = php_fopen_wrapper_for_zend;
//   zuf.message_handler = php_message_handler_for_zend;
//   zuf.get_configuration_directive = php_get_configuration_directive_for_zend;
//   zuf.ticks_function = php_run_ticks;
//   zuf.on_timeout = php_on_timeout;
//   zuf.stream_open_function = php_stream_open_for_zend;
//   zuf.printf_to_smart_string_function = php_printf_to_smart_string;
//   zuf.printf_to_smart_str_function = php_printf_to_smart_str;
//   zuf.getenv_function = sapi_getenv;
//   zuf.resolve_path_function = php_resolve_path_for_zend;
//   zend_startup(&zuf, NULL);

//#if HAVE_SETLOCALE
//   setlocale(LC_CTYPE, "");
//   zend_update_current_locale();
//#endif

//#if HAVE_TZSET
//   tzset();
//#endif

//#ifdef PHP_WIN32
//   /* start up winsock services */
//   if (WSAStartup(wVersionRequested, &wsaData) != 0) {
//      php_printf("\nwinsock.dll unusable. %d\n", WSAGetLastError());
//      return FAILURE;
//   }
//#endif

//   le_index_ptr = zend_register_list_destructors_ex(NULL, NULL, "index pointer", 0);

//   /* Register constants */
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_VERSION", PHP_VERSION, sizeof(PHP_VERSION)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_MAJOR_VERSION", PHP_MAJOR_VERSION, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_MINOR_VERSION", PHP_MINOR_VERSION, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_RELEASE_VERSION", PHP_RELEASE_VERSION, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_EXTRA_VERSION", PHP_EXTRA_VERSION, sizeof(PHP_EXTRA_VERSION) - 1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_VERSION_ID", PHP_VERSION_ID, CONST_PERSISTENT | CONST_CS);
//#ifdef ZTS
//   REGISTER_MAIN_LONG_CONSTANT("PHP_ZTS", 1, CONST_PERSISTENT | CONST_CS);
//#else
//   REGISTER_MAIN_LONG_CONSTANT("PHP_ZTS", 0, CONST_PERSISTENT | CONST_CS);
//#endif
//   REGISTER_MAIN_LONG_CONSTANT("PHP_DEBUG", PHP_DEBUG, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_OS", phpOs, strlen(phpOs), CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_OS_FAMILY", PHP_OS_FAMILY, sizeof(PHP_OS_FAMILY)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_SAPI", sapi_module.name, strlen(sapi_module.name), CONST_PERSISTENT | CONST_CS | CONST_NO_FILE_CACHE);
//   REGISTER_MAIN_STRINGL_CONSTANT("DEFAULT_INCLUDE_PATH", PHP_INCLUDE_PATH, sizeof(PHP_INCLUDE_PATH)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PEAR_INSTALL_DIR", PEAR_INSTALLDIR, sizeof(PEAR_INSTALLDIR)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PEAR_EXTENSION_DIR", PHP_EXTENSION_DIR, sizeof(PHP_EXTENSION_DIR)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_EXTENSION_DIR", PHP_EXTENSION_DIR, sizeof(PHP_EXTENSION_DIR)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_PREFIX", PHP_PREFIX, sizeof(PHP_PREFIX)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_BINDIR", PHP_BINDIR, sizeof(PHP_BINDIR)-1, CONST_PERSISTENT | CONST_CS);
//#ifndef PHP_WIN32
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_MANDIR", PHP_MANDIR, sizeof(PHP_MANDIR)-1, CONST_PERSISTENT | CONST_CS);
//#endif
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_LIBDIR", PHP_LIBDIR, sizeof(PHP_LIBDIR)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_DATADIR", PHP_DATADIR, sizeof(PHP_DATADIR)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_SYSCONFDIR", PHP_SYSCONFDIR, sizeof(PHP_SYSCONFDIR)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_LOCALSTATEDIR", PHP_LOCALSTATEDIR, sizeof(PHP_LOCALSTATEDIR)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_CONFIG_FILE_PATH", PHP_CONFIG_FILE_PATH, strlen(PHP_CONFIG_FILE_PATH), CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_CONFIG_FILE_SCAN_DIR", PHP_CONFIG_FILE_SCAN_DIR, sizeof(PHP_CONFIG_FILE_SCAN_DIR)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_SHLIB_SUFFIX", PHP_SHLIB_SUFFIX, sizeof(PHP_SHLIB_SUFFIX)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_STRINGL_CONSTANT("PHP_EOL", PHP_EOL, sizeof(PHP_EOL)-1, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_MAXPATHLEN", MAXPATHLEN, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_INT_MAX", ZEND_LONG_MAX, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_INT_MIN", ZEND_LONG_MIN, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_INT_SIZE", SIZEOF_ZEND_LONG, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_FD_SETSIZE", FD_SETSIZE, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_FLOAT_DIG", DBL_DIG, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_DOUBLE_CONSTANT("PHP_FLOAT_EPSILON", DBL_EPSILON, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_DOUBLE_CONSTANT("PHP_FLOAT_MAX", DBL_MAX, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_DOUBLE_CONSTANT("PHP_FLOAT_MIN", DBL_MIN, CONST_PERSISTENT | CONST_CS);

//#ifdef PHP_WIN32
//   REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_MAJOR",      EG(windows_version_info).dwMajorVersion, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_MINOR",      EG(windows_version_info).dwMinorVersion, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_BUILD",      EG(windows_version_info).dwBuildNumber, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_PLATFORM",   EG(windows_version_info).dwPlatformId, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_SP_MAJOR",   EG(windows_version_info).wServicePackMajor, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_SP_MINOR",   EG(windows_version_info).wServicePackMinor, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_SUITEMASK",  EG(windows_version_info).wSuiteMask, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_VERSION_PRODUCTTYPE", EG(windows_version_info).wProductType, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_NT_DOMAIN_CONTROLLER", VER_NT_DOMAIN_CONTROLLER, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_NT_SERVER", VER_NT_SERVER, CONST_PERSISTENT | CONST_CS);
//   REGISTER_MAIN_LONG_CONSTANT("PHP_WINDOWS_NT_WORKSTATION", VER_NT_WORKSTATION, CONST_PERSISTENT | CONST_CS);
//#endif

//   php_binary_init();
//   if (PG(php_binary)) {
//      REGISTER_MAIN_STRINGL_CONSTANT("PHP_BINARY", PG(php_binary), strlen(PG(php_binary)), CONST_PERSISTENT | CONST_CS | CONST_NO_FILE_CACHE);
//   } else {
//      REGISTER_MAIN_STRINGL_CONSTANT("PHP_BINARY", "", 0, CONST_PERSISTENT | CONST_CS | CONST_NO_FILE_CACHE);
//   }

//   php_output_register_constants();
//   php_rfc1867_register_constants();

//   /* this will read in php.ini, set up the configuration parameters,
//         load zend extensions and register php function extensions
//         to be loaded later */
//   if (php_init_config() == FAILURE) {
//      return FAILURE;
//   }

//   /* Register PHP core ini entries */
//   REGISTER_INI_ENTRIES();

//   /* Register Zend ini entries */
//   zend_register_standard_ini_entries();

//#ifdef ZEND_WIN32
//   /* Until the current ini values was setup, the current cp is 65001.
//         If the actual ini vaues are different, some stuff needs to be updated.
//         It concerns at least main_cwd_state and there might be more. As we're
//         still in the startup phase, lets use the chance and reinit the relevant
//         item according to the current codepage. Still, if ini_set() is used
//         later on, a more intelligent way to update such stuff is needed.
//         Startup/shutdown routines could involve touching globals and thus
//         can't always be used on demand. */
//   if (!php_win32_cp_use_unicode()) {
//      virtual_cwd_main_cwd_init(1);
//   }
//#endif

//   /* Disable realpath cache if an open_basedir is set */
//   if (PG(open_basedir) && *PG(open_basedir)) {
//      CWDG(realpath_cache_size_limit) = 0;
//   }

//   PG(have_called_openlog) = 0;

//   /* initialize stream wrappers registry
//       * (this uses configuration parameters from php.ini)
//       */
//   if (php_init_stream_wrappers(moduleNumber) == FAILURE)	{
//      php_printf("PHP:  Unable to initialize stream url wrappers.\n");
//      return FAILURE;
//   }

//   zuv.html_errors = 1;
//   zuv.import_use_extension = ".php";
//   zuv.import_use_extension_length = (uint32_t)strlen(zuv.import_use_extension);
//   php_startup_auto_globals();
//   zend_set_utility_values(&zuv);
//   php_startup_sapi_content_types();

//   /* startup extensions statically compiled in */
//   if (php_register_internal_extensions_func() == FAILURE) {
//      php_printf("Unable to start builtin modules\n");
//      return FAILURE;
//   }

//   /* start additional PHP extensions */
//   php_register_extensions_bc(additional_modules, num_additional_modules);

//   /* load and startup extensions compiled as shared objects (aka DLLs)
//         as requested by php.ini entries
//         these are loaded after initialization of internal extensions
//         as extensions *might* rely on things from ext/standard
//         which is always an internal extension and to be initialized
//         ahead of all other internals
//       */
//   php_ini_register_extensions();
//   zend_startup_modules();

//   /* start Zend extensions */
//   zend_startup_extensions();

//   zend_collect_module_handlers();

//   /* register additional functions */
//   if (sapi_module.additional_functions) {
//      if ((module = zend_hash_str_find_ptr(&module_registry, "standard", sizeof("standard")-1)) != NULL) {
//         EG(current_module) = module;
//         zend_register_functions(NULL, sapi_module.additional_functions, NULL, MODULE_PERSISTENT);
//         EG(current_module) = NULL;
//      }
//   }

//   /* disable certain classes and functions as requested by php.ini */
//   php_disable_functions();
//   php_disable_classes();

//   /* make core report what it should */
//   if ((module = zend_hash_str_find_ptr(&module_registry, "core", sizeof("core")-1)) != NULL) {
//      module->version = PHP_VERSION;
//      module->info_func = PHP_MINFO(php_core);
//   }

//   if (zend_post_startup() != SUCCESS) {
//      return FAILURE;
//   }

//   module_initialized = 1;

//   /* Check for deprecated directives */
//   /* NOTE: If you add anything here, remember to add it to Makefile.global! */
//   {
//      struct {
//         const long error_level;
//         const char *phrase;
//         const char *directives[17]; /* Remember to change this if the number of directives change */
//      } directives[2] = {
//      {
//         E_DEPRECATED,
//               "Directive '%s' is deprecated",
//         {
//            "track_errors",
//            NULL
//         }
//      },
//      {
//         E_CORE_ERROR,
//               "Directive '%s' is no longer available in PHP",
//         {
//            "allow_call_time_pass_reference",
//            "asp_tags",
//            "define_syslog_variables",
//            "highlight.bg",
//            "magic_quotes_gpc",
//            "magic_quotes_runtime",
//            "magic_quotes_sybase",
//            "register_globals",
//            "register_long_arrays",
//            "safe_mode",
//            "safe_mode_gid",
//            "safe_mode_include_dir",
//            "safe_mode_exec_dir",
//            "safe_mode_allowed_env_vars",
//            "safe_mode_protected_env_vars",
//            "zend.ze1_compatibility_mode",
//            NULL
//         }
//      }
//   };

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
//         retval = FAILURE;
//      } zend_end_try();
//   }

//   virtual_cwd_deactivate();

//   sapi_deactivate();
//   moduleStartup = 0;

//   shutdown_memory_manager(1, 0);
//   virtual_cwd_activate();

//   zend_interned_strings_switch_storage(1);

//#if ZEND_RC_DEBUG
//   zend_rc_debug = 1;
//#endif

//   /* we're done */
//   return retval;
}

} // polar
