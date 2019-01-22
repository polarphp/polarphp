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

#include "polarphp/runtime/Ini.h"
#include "polarphp/runtime/Output.h"
#include "polarphp/runtime/ExecEnv.h"
#include "polarphp/runtime/internal/DepsZendVmHeaders.h"
#include "polarphp/runtime/Spprintf.h"
#include "polarphp/runtime/Utils.h"
#include "polarphp/runtime/ScanDir.h"

#ifdef POLAR_OS_WIN32
#include "win32/php_registry.h"
#endif

#if HAVE_SCANDIR && HAVE_ALPHASORT && HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifdef POLAR_OS_WIN32
#define TRANSLATE_SLASHES_LOWER(path) \
{ \
   char *tmp = path; \
   while (*tmp) { \
   if (*tmp == '\\') *tmp = '/'; \
   else *tmp = tolower(*tmp); \
   tmp++; \
   } \
   }
#else
#define TRANSLATE_SLASHES_LOWER(path)
#endif

struct php_extension_lists
{
   zend_llist engine;
   zend_llist functions;
};

/* True globals */
static bool sg_isSpecialSection = false;
static HashTable *sg_activeIniHash;
static HashTable sg_configurationHash;
static int sg_hasPerDirConfig = 0;
static int sg_hasPerHostConfig = 0;
char *sg_phpIniOpenedPath = nullptr;
static php_extension_lists sg_extensionLists;
char *sg_phpIniScannedPath = nullptr;
char *sg_phpIniScannedFiles = nullptr;

namespace polar {
namespace runtime {

namespace {
void php_ini_displayer_callback(zend_ini_entry *ini_entry, int type)
{
   if (ini_entry->displayer) {
      ini_entry->displayer(ini_entry, type);
   } else {
      char *display_string;
      size_t display_string_length;
      if (type == ZEND_INI_DISPLAY_ORIG && ini_entry->modified) {
         if (ini_entry->orig_value && ZSTR_VAL(ini_entry->orig_value)[0]) {
            display_string = ZSTR_VAL(ini_entry->orig_value);
            display_string_length = ZSTR_LEN(ini_entry->orig_value);
         } else {
            display_string = const_cast<char *>("no value");
            display_string_length = sizeof("no value") - 1;
         }
      } else if (ini_entry->value && ZSTR_VAL(ini_entry->value)[0]) {
         display_string = ZSTR_VAL(ini_entry->value);
         display_string_length = ZSTR_LEN(ini_entry->value);
      } else {
         display_string = const_cast<char *>("no value");
         display_string_length = sizeof("no value") - 1;
      }
      PHPWRITE(display_string, display_string_length);
   }
}

int php_ini_displayer(zval *el, void *arg)
{
   zend_ini_entry *ini_entry = (zend_ini_entry*)Z_PTR_P(el);
   int module_number = *(int *)arg;

   if (ini_entry->module_number != module_number) {
      return 0;
   }
   PHPWRITE(ZSTR_VAL(ini_entry->name), ZSTR_LEN(ini_entry->name));
   PUTS(" => ");
   php_ini_displayer_callback(ini_entry, ZEND_INI_DISPLAY_ACTIVE);
   PUTS(" => ");
   php_ini_displayer_callback(ini_entry, ZEND_INI_DISPLAY_ORIG);
   PUTS("\n");
   return 0;
}

int php_ini_available(zval *el, void *arg)
{
   zend_ini_entry *ini_entry = (zend_ini_entry *)Z_PTR_P(el);
   int *module_number_available = (int *)arg;
   if (ini_entry->module_number == *(int *)module_number_available) {
      *(int *)module_number_available = -1;
      return ZEND_HASH_APPLY_STOP;
   } else {
      return ZEND_HASH_APPLY_KEEP;
   }
}

} // anonymous namespace

void display_ini_entries(zend_module_entry *module)
{
   int module_number, module_number_available;

   if (module) {
      module_number = module->module_number;
   } else {
      module_number = 0;
   }
   module_number_available = module_number;
   zend_hash_apply_with_argument(EG(ini_directives), php_ini_available, &module_number_available);
   if (module_number_available == -1) {
      //      php_info_print_table_start();
      //      php_info_print_table_header(3, "Directive", "Local Value", "Master Value");
      zend_hash_apply_with_argument(EG(ini_directives), php_ini_displayer, (void *)&module_number);
      //      php_info_print_table_end();
   }
}

/* php.ini support */
#define PHP_EXTENSION_TOKEN		"extension"
#define ZEND_EXTENSION_TOKEN	"zend_extension"

void config_zval_dtor(zval *zvalue)
{
   if (Z_TYPE_P(zvalue) == IS_ARRAY) {
      zend_hash_destroy(Z_ARRVAL_P(zvalue));
      free(Z_ARR_P(zvalue));
   } else if (Z_TYPE_P(zvalue) == IS_STRING) {
      zend_string_release_ex(Z_STR_P(zvalue), 1);
   }
}
/* Reset / free active_ini_sectin global */
#define RESET_ACTIVE_INI_HASH() do { \
   sg_activeIniHash = nullptr;          \
   sg_isSpecialSection = false;          \
} while (0)

static void php_ini_parser_callback(zval *arg1, zval *arg2, zval *arg3, int callback_type, HashTable *target_hash)
{
   zval *entry;
   HashTable *active_hash;
   char *extension_name;

   if (sg_activeIniHash) {
      active_hash = sg_activeIniHash;
   } else {
      active_hash = target_hash;
   }

   switch (callback_type) {
   case ZEND_INI_PARSER_ENTRY: {
      if (!arg2) {
         /* bare string - nothing to do */
         break;
      }

      /* PHP and Zend extensions are not added into configuration hash! */
      if (!sg_isSpecialSection && !strcasecmp(Z_STRVAL_P(arg1), PHP_EXTENSION_TOKEN)) { /* load PHP extension */
         extension_name = estrndup(Z_STRVAL_P(arg2), Z_STRLEN_P(arg2));
         zend_llist_add_element(&sg_extensionLists.functions, &extension_name);
      } else if (!sg_isSpecialSection && !strcasecmp(Z_STRVAL_P(arg1), ZEND_EXTENSION_TOKEN)) { /* load Zend extension */
         extension_name = estrndup(Z_STRVAL_P(arg2), Z_STRLEN_P(arg2));
         zend_llist_add_element(&sg_extensionLists.engine, &extension_name);

         /* All other entries are added into either sg_configurationHash or active ini section array */
      } else {
         /* Store in active hash */
         entry = zend_hash_update(active_hash, Z_STR_P(arg1), arg2);
         Z_STR_P(entry) = zend_string_dup(Z_STR_P(entry), 1);
      }
   }
      break;

   case ZEND_INI_PARSER_POP_ENTRY: {
      zval option_arr;
      zval *find_arr;

      if (!arg2) {
         /* bare string - nothing to do */
         break;
      }

      /* fprintf(stdout, "ZEND_INI_PARSER_POP_ENTRY: %s[%s] = %s\n",Z_STRVAL_P(arg1), Z_STRVAL_P(arg3), Z_STRVAL_P(arg2)); */

      /* If option not found in hash or is not an array -> create array, otherwise add to existing array */
      if ((find_arr = zend_hash_find(active_hash, Z_STR_P(arg1))) == nullptr || Z_TYPE_P(find_arr) != IS_ARRAY) {
         ZVAL_NEW_PERSISTENT_ARR(&option_arr);
         zend_hash_init(Z_ARRVAL(option_arr), 8, nullptr, config_zval_dtor, 1);
         find_arr = zend_hash_update(active_hash, Z_STR_P(arg1), &option_arr);
      }

      /* arg3 is possible option offset name */
      if (arg3 && Z_STRLEN_P(arg3) > 0) {
         entry = zend_symtable_update(Z_ARRVAL_P(find_arr), Z_STR_P(arg3), arg2);
      } else {
         entry = zend_hash_next_index_insert(Z_ARRVAL_P(find_arr), arg2);
      }
      Z_STR_P(entry) = zend_string_dup(Z_STR_P(entry), 1);
   }
      break;

   case ZEND_INI_PARSER_SECTION: { /* Create an array of entries of each section */

      /* fprintf(stdout, "ZEND_INI_PARSER_SECTION: %s\n",Z_STRVAL_P(arg1)); */

      char *key = nullptr;
      size_t key_len;

      /* PATH sections */
      if (!zend_binary_strncasecmp(Z_STRVAL_P(arg1), Z_STRLEN_P(arg1), "PATH", sizeof("PATH") - 1, sizeof("PATH") - 1)) {
         key = Z_STRVAL_P(arg1);
         key = key + sizeof("PATH") - 1;
         key_len = Z_STRLEN_P(arg1) - sizeof("PATH") + 1;
         sg_isSpecialSection = true;
         sg_hasPerDirConfig = 1;

         /* make the path lowercase on Windows, for case insensitivity. Does nothing for other platforms */
         TRANSLATE_SLASHES_LOWER(key);

         /* HOST sections */
      } else if (!zend_binary_strncasecmp(Z_STRVAL_P(arg1), Z_STRLEN_P(arg1), "HOST", sizeof("HOST") - 1, sizeof("HOST") - 1)) {
         key = Z_STRVAL_P(arg1);
         key = key + sizeof("HOST") - 1;
         key_len = Z_STRLEN_P(arg1) - sizeof("HOST") + 1;
         sg_isSpecialSection = true;
         sg_hasPerHostConfig = 1;
         zend_str_tolower(key, key_len); /* host names are case-insensitive. */

      } else {
         sg_isSpecialSection = false;
      }

      if (key && key_len > 0) {
         /* Strip any trailing slashes */
         while (key_len > 0 && (key[key_len - 1] == '/' || key[key_len - 1] == '\\')) {
            key_len--;
            key[key_len] = 0;
         }

         /* Strip any leading whitespace and '=' */
         while (*key && (
                   *key == '=' ||
                   *key == ' ' ||
                   *key == '\t'
                   )) {
            key++;
            key_len--;
         }
         /* Search for existing entry and if it does not exist create one */
         if ((entry = zend_hash_str_find(target_hash, key, key_len)) == nullptr) {
            zval section_arr;
            ZVAL_NEW_PERSISTENT_ARR(&section_arr);
            zend_hash_init(Z_ARRVAL(section_arr), 8, nullptr, (dtor_func_t) config_zval_dtor, 1);
            entry = zend_hash_str_update(target_hash, key, key_len, &section_arr);
         }
         if (Z_TYPE_P(entry) == IS_ARRAY) {
            sg_activeIniHash = Z_ARRVAL_P(entry);
         }
      }
   }
      break;
   }
}

static void php_load_php_extension_callback(void *arg)
{
#ifdef HAVE_LIBDL
   //   php_load_extension(*((char **) arg), MODULE_PERSISTENT, 0);
#endif
}

#ifdef HAVE_LIBDL
static void php_load_zend_extension_callback(void *arg)
{
//   char *filename = *((char **) arg);
//   const size_t length = strlen(filename);

//#ifndef POLAR_OS_WIN32
//   (void) length;
//#endif

//   if (IS_ABSOLUTE_PATH(filename, length)) {
//      zend_load_extension(filename);
//   } else {
//      DL_HANDLE handle;
//      char *libpath;
//      char *extension_dir = INI_STR("extension_dir");
//      int slash_suffix = 0;
//      char *err1, *err2;

//      if (extension_dir && extension_dir[0]) {
//         slash_suffix = IS_SLASH(extension_dir[strlen(extension_dir)-1]);
//      }

//      /* Try as filename first */
//      if (slash_suffix) {
//         zend_spprintf(&libpath, 0, "%s%s", extension_dir, filename); /* SAFE */
//      } else {
//         zend_spprintf(&libpath, 0, "%s%c%s", extension_dir, DEFAULT_SLASH, filename); /* SAFE */
//      }

//      handle = (DL_HANDLE)php_load_shlib(libpath, &err1);
//      if (!handle) {
//         /* If file does not exist, consider as extension name and build file name */
//         char *orig_libpath = libpath;

//         if (slash_suffix) {
//            zend_spprintf(&libpath, 0, "%s%s." POLARPHP_SHLIB_SUFFIX, extension_dir, filename); /* SAFE */
//         } else {
//            zend_spprintf(&libpath, 0, "%s%c%s." POLARPHP_SHLIB_SUFFIX, extension_dir, DEFAULT_SLASH, filename); /* SAFE */
//         }

//         handle = (DL_HANDLE)php_load_shlib(libpath, &err2);
//         if (!handle) {
//            php_error(E_CORE_WARNING, "Failed loading Zend extension '%s' (tried: %s (%s), %s (%s))",
//                      filename, orig_libpath, err1, libpath, err2);
//            efree(orig_libpath);
//            efree(err1);
//            efree(libpath);
//            efree(err2);
//            return;
//         }

//         efree(orig_libpath);
//         efree(err1);
//      }

//      zend_load_extension_handle(handle, libpath);
//      efree(libpath);
//   }
}
#else
static void php_load_zend_extension_callback(void *arg) { }
#endif

///
/// need review for memory leak
///
bool php_init_config()
{
   StringRef phpIniFileName;
   char *phpIniSearchPath;
   int phpIniScannedPathLen;
   StringRef openBaseDir;
   int freeIniSearchPath = 0;
   zend_file_handle fh;
   zend_string *openedPath = nullptr;
   ExecEnv &execEnv = retrieve_global_execenv();
   ExecEnvInfo &execEnvInfo = execEnv.getRuntimeInfo();
   zend_hash_init(&sg_configurationHash, 8, nullptr, config_zval_dtor, 1);
   /// invoke exec env to setup default config
   execEnv.initDefaultConfig(&sg_configurationHash);
   zend_llist_init(&sg_extensionLists.engine, sizeof(char *), reinterpret_cast<llist_dtor_func_t>(free_estring), 1);
   zend_llist_init(&sg_extensionLists.functions, sizeof(char *), reinterpret_cast<llist_dtor_func_t>(free_estring), 1);
   openBaseDir = execEnvInfo.openBaseDir;
   std::string &phpIniexecPathOverride = execEnvInfo.phpIniPathOverride;
   if (!phpIniexecPathOverride.empty()) {
      phpIniFileName = phpIniexecPathOverride;
      phpIniSearchPath = const_cast<char *>(phpIniexecPathOverride.c_str());
      freeIniSearchPath = 0;
   } else if (!execEnvInfo.phpIniIgnore) {
      int searchPathSize;
      StringRef defaultLocation;
      StringRef envLocation;
      static const char pathsSeparator[] = { ZEND_PATHS_SEPARATOR, 0 };
#ifdef POLAR_OS_WIN32
      char *reg_location;
      char phprc_path[MAXPATHLEN];
#endif

      envLocation = getenv("PHPRC");

#ifdef POLAR_OS_WIN32
      if (!envLocation) {
         char dummybuf;
         int size;

         SetLastError(0);

         /*If the given buffer is not large enough to hold the data, the return value is
            the buffer size,  in characters, required to hold the string and its terminating
            null character. We use this return value to alloc the final buffer. */
         size = GetEnvironmentVariableA("PHPRC", &dummybuf, 0);
         if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
            /* The environment variable doesn't exist. */
            envLocation = "";
         } else {
            if (size == 0) {
               envLocation = "";
            } else {
               size = GetEnvironmentVariableA("PHPRC", phprc_path, size);
               if (size == 0) {
                  envLocation = "";
               } else {
                  envLocation = phprc_path;
               }
            }
         }
      }
#else
      if (!envLocation.empty()) {
         envLocation = "";
      }
#endif
      /*
          * Prepare search path
          */

      searchPathSize = MAXPATHLEN * 4 + (int)envLocation.size() + 3 + 1;
      phpIniSearchPath = reinterpret_cast<char *>(emalloc(searchPathSize));
      freeIniSearchPath = 1;
      phpIniSearchPath[0] = 0;

      /* Add environment location */
      if (!envLocation.empty()) {
         if (*phpIniSearchPath) {
            strlcat(phpIniSearchPath, pathsSeparator, searchPathSize);
         }
         strlcat(phpIniSearchPath, envLocation.getData(), searchPathSize);
         phpIniFileName = envLocation;
      }

#ifdef POLAR_OS_WIN32
      /* Add registry location */
      reg_location = GetIniPathFromRegistry();
      if (reg_location != nullptr) {
         if (*phpIniSearchPath) {
            strlcat(phpIniSearchPath, pathsSeparator, searchPathSize);
         }
         strlcat(phpIniSearchPath, reg_location, searchPathSize);
         efree(reg_location);
      }
#endif

      /* Add cwd (not with CLI) */
      if (!execEnvInfo.phpIniIgnoreCwd) {
         if (*phpIniSearchPath) {
            strlcat(phpIniSearchPath, pathsSeparator, searchPathSize);
         }
         strlcat(phpIniSearchPath, ".", searchPathSize);
      }
      std::string &polarBinary = execEnvInfo.polarBinary;
      if (!polarBinary.empty()) {
         char *separatorLocation;
         char *binaryLocation;
         binaryLocation = estrdup(polarBinary.c_str());
         separatorLocation = strrchr(binaryLocation, DEFAULT_SLASH);
         if (separatorLocation && separatorLocation != binaryLocation) {
            *(separatorLocation) = 0;
         }
         if (*phpIniSearchPath) {
            strlcat(phpIniSearchPath, pathsSeparator, searchPathSize);
         }
         strlcat(phpIniSearchPath, binaryLocation, searchPathSize);
         efree(binaryLocation);
      }

      /* Add default location */
#ifdef POLAR_OS_WIN32
      defaultLocation = (char *) emalloc(MAXPATHLEN + 1);
      if (0 < GetWindowsDirectory(defaultLocation, MAXPATHLEN)) {
         if (*phpIniSearchPath) {
            strlcat(phpIniSearchPath, pathsSeparator, searchPathSize);
         }
         strlcat(phpIniSearchPath, defaultLocation, searchPathSize);
      }
      /* For people running under terminal services, GetWindowsDirectory will
          * return their personal Windows directory, so lets add the system
          * windows directory too */
      if (0 < GetSystemWindowsDirectory(defaultLocation, MAXPATHLEN)) {
         if (*phpIniSearchPath) {
            strlcat(phpIniSearchPath, pathsSeparator, searchPathSize);
         }
         strlcat(phpIniSearchPath, defaultLocation, searchPathSize);
      }
      efree(defaultLocation);
#else
      defaultLocation = const_cast<char *>(POLARPHP_CONFIG_FILE_PATH);
      if (*phpIniSearchPath) {
         strlcat(phpIniSearchPath, pathsSeparator, searchPathSize);
      }
      strlcat(phpIniSearchPath, defaultLocation.getData(), searchPathSize);
#endif
   }
   execEnvInfo.openBaseDir.clear();
   /// Find and open actual ini file
   memset(&fh, 0, sizeof(fh));
   /* If SAPI does not want to ignore all ini files OR an overriding file/path is given.
       * This allows disabling scanning for ini files in the PHP_CONFIG_FILE_SCAN_DIR but still
       * load an optional ini file. */

   if (!execEnvInfo.phpIniIgnore || !phpIniexecPathOverride.empty()) {
      /* Check if phpIniFileName is a file and can be opened */
      if (!phpIniFileName.empty()) {
         zend_stat_t statbuf;
         if (!VCWD_STAT(phpIniFileName.getData(), &statbuf)) {
            if (!((statbuf.st_mode & S_IFMT) == S_IFDIR)) {
               fh.handle.fp = VCWD_FOPEN(phpIniFileName.getData(), "r");
               if (fh.handle.fp) {
                  fh.filename = expand_filepath(phpIniFileName.getData(), nullptr);
               }
            }
         }
      }

      ///
      /// polarphp does not use SAPI mechanism
      ///
      /* Otherwise search for php-cli.ini file in search path */
      if (!fh.handle.fp) {
         const char *fmt = "php-%s.ini";
         char *ini_fname;
         polar_spprintf(&ini_fname, 0, fmt, "cli");
         fh.handle.fp = php_fopen_with_path(ini_fname, "r", phpIniSearchPath, &openedPath);
         efree(ini_fname);
         if (fh.handle.fp) {
            fh.filename = ZSTR_VAL(openedPath);
         }
      }

      /* If still no ini file found, search for php.ini file in search path */
      if (!fh.handle.fp) {
         fh.handle.fp = php_fopen_with_path("php.ini", "r", phpIniSearchPath, &openedPath);
         if (fh.handle.fp) {
            fh.filename = ZSTR_VAL(openedPath);
         }
      }
   }

   if (freeIniSearchPath) {
      efree(phpIniSearchPath);
   }
   execEnvInfo.openBaseDir = openBaseDir;
   if (fh.handle.fp) {
      fh.type = ZEND_HANDLE_FP;
      RESET_ACTIVE_INI_HASH();
      zend_parse_ini_file(&fh, 1, ZEND_INI_SCANNER_NORMAL, (zend_ini_parser_cb_t) php_ini_parser_callback, &sg_configurationHash);

      {
         zval tmp;
         ZVAL_NEW_STR(&tmp, zend_string_init(fh.filename, strlen(fh.filename), 1));
         zend_hash_str_update(&sg_configurationHash, const_cast<char *>("cfg_file_path"), sizeof("cfg_file_path")-1, &tmp);
         if (openedPath) {
            zend_string_release_ex(openedPath, 0);
         } else {
            efree(const_cast<char *>(fh.filename));
         }
         sg_phpIniOpenedPath = zend_strndup(Z_STRVAL(tmp), Z_STRLEN(tmp));
      }
   }

   /* Check for PHP_INI_SCAN_DIR environment variable to override/set config file scan directory */
   sg_phpIniScannedPath = getenv("PHP_INI_SCAN_DIR");
   if (!sg_phpIniScannedPath) {
      /* Or fall back using possible --with-config-file-scan-dir setting (defaults to empty string!) */
      sg_phpIniScannedPath = const_cast<char *>(POLARPHP_CONFIG_FILE_SCAN_DIR);
   }
   phpIniScannedPathLen = (int)strlen(sg_phpIniScannedPath);

   /* Scan and parse any .ini files found in scan path if path not empty. */
   if (!execEnvInfo.phpIniIgnore && phpIniScannedPathLen) {
      struct dirent **namelist;
      int ndir, i;
      zend_stat_t sb;
      char iniFile[MAXPATHLEN + 2];
      char *p;
      zend_file_handle fh2;
      zend_llist scanned_ini_list;
      zend_llist_element *element;
      int l, total_l = 0;
      char *bufpath, *debpath, *endpath;
      int lenpath;
      zend_llist_init(&scanned_ini_list, sizeof(char *), (llist_dtor_func_t) free_estring, 1);
      memset(&fh2, 0, sizeof(fh2));
      bufpath = estrdup(sg_phpIniScannedPath);
      for (debpath = bufpath ; debpath ; debpath=endpath) {
         endpath = strchr(debpath, DEFAULT_DIR_SEPARATOR);
         if (endpath) {
            *(endpath++) = 0;
         }
         if (!debpath[0]) {
            /* empty string means default builtin value
                  to allow "/foo/php.d:" or ":/foo/php.d" */
            debpath = const_cast<char *>(POLARPHP_CONFIG_FILE_SCAN_DIR);
         }
         lenpath = (int)strlen(debpath);
         if (lenpath > 0 && (ndir = php_scandir(debpath, &namelist, 0, php_alphasort)) > 0) {
            for (i = 0; i < ndir; i++) {
               /* check for any file with .ini extension */
               if (!(p = strrchr(namelist[i]->d_name, '.')) || (p && strcmp(p, ".ini"))) {
                  free(namelist[i]);
                  continue;
               }
               /* Reset active ini section */
               RESET_ACTIVE_INI_HASH();

               if (IS_SLASH(debpath[lenpath - 1])) {
                  std::snprintf(iniFile, MAXPATHLEN, "%s%s", debpath, namelist[i]->d_name);
               } else {
                  ///
                  /// TODO review print result
                  ///
                  std::snprintf(iniFile, MAXPATHLEN + 2, "%s%c%s", debpath, DEFAULT_SLASH, namelist[i]->d_name);
               }
               if (VCWD_STAT(iniFile, &sb) == 0) {
                  if (S_ISREG(sb.st_mode)) {
                     if ((fh2.handle.fp = VCWD_FOPEN(iniFile, "r"))) {
                        fh2.filename = iniFile;
                        fh2.type = ZEND_HANDLE_FP;

                        if (zend_parse_ini_file(&fh2, 1, ZEND_INI_SCANNER_NORMAL, (zend_ini_parser_cb_t) php_ini_parser_callback, &sg_configurationHash) == SUCCESS) {
                           /* Here, add it to the list of ini files read */
                           l = (int)strlen(iniFile);
                           total_l += l + 2;
                           p = estrndup(iniFile, l);
                           zend_llist_add_element(&scanned_ini_list, &p);
                        }
                     }
                  }
               }
               free(namelist[i]);
            }
            free(namelist);
         }
      }
      efree(bufpath);

      if (total_l) {
         int phpIniScannedFilesLen = (sg_phpIniScannedFiles) ? (int)strlen(sg_phpIniScannedFiles) + 1 : 0;
         sg_phpIniScannedFiles = (char *) realloc(sg_phpIniScannedFiles, phpIniScannedFilesLen + total_l + 1);
         if (!phpIniScannedFilesLen) {
            *sg_phpIniScannedFiles = '\0';
         }
         total_l += phpIniScannedFilesLen;
         for (element = scanned_ini_list.head; element; element = element->next) {
            if (phpIniScannedFilesLen) {
               strlcat(sg_phpIniScannedFiles, ",\n", total_l);
            }
            strlcat(sg_phpIniScannedFiles, *(char **)element->data, total_l);
            strlcat(sg_phpIniScannedFiles, element->next ? ",\n" : "\n", total_l);
         }
      }
      zend_llist_destroy(&scanned_ini_list);
   } else {
      /* Make sure an empty sg_phpIniScannedPath ends up as nullptr */
      sg_phpIniScannedPath = nullptr;
   }
   std::string &iniEntries = execEnvInfo.iniEntries;
   if (!iniEntries.empty()) {
      /* Reset active ini section */
      RESET_ACTIVE_INI_HASH();
      zend_parse_ini_string(const_cast<char *>(iniEntries.c_str()), 1, ZEND_INI_SCANNER_NORMAL, (zend_ini_parser_cb_t) php_ini_parser_callback, &sg_configurationHash);
   }
   return true;
}

int php_shutdown_config(void)
{
   zend_hash_destroy(&sg_configurationHash);
   if (sg_phpIniOpenedPath) {
      free(sg_phpIniOpenedPath);
      sg_phpIniOpenedPath = nullptr;
   }
   if (sg_phpIniScannedFiles) {
      free(sg_phpIniScannedFiles);
      sg_phpIniScannedFiles = nullptr;
   }
   return SUCCESS;
}

void php_ini_register_extensions(void)
{
   zend_llist_apply(&sg_extensionLists.engine, php_load_zend_extension_callback);
   zend_llist_apply(&sg_extensionLists.functions, php_load_php_extension_callback);
   zend_llist_destroy(&sg_extensionLists.engine);
   zend_llist_destroy(&sg_extensionLists.functions);
}

int php_parse_user_ini_file(const char *dirname, char *ini_filename, HashTable *target_hash)
{
   zend_stat_t sb;
   char iniFile[MAXPATHLEN];
   zend_file_handle fh;
   snprintf(iniFile, MAXPATHLEN, "%s%c%s", dirname, DEFAULT_SLASH, ini_filename);
   if (VCWD_STAT(iniFile, &sb) == 0) {
      if (S_ISREG(sb.st_mode)) {
         memset(&fh, 0, sizeof(fh));
         if ((fh.handle.fp = VCWD_FOPEN(iniFile, "r"))) {
            fh.filename = iniFile;
            fh.type = ZEND_HANDLE_FP;
            /* Reset active ini section */
            RESET_ACTIVE_INI_HASH();
            if (zend_parse_ini_file(&fh, 1, ZEND_INI_SCANNER_NORMAL, (zend_ini_parser_cb_t) php_ini_parser_callback, target_hash) == SUCCESS) {
               /* FIXME: Add parsed file to the list of user files read? */
               return SUCCESS;
            }
            return FAILURE;
         }
      }
   }
   return FAILURE;
}

void php_ini_activate_config(HashTable *source_hash, int modify_type, int stage)
{
   zend_string *str;
   zval *data;
   /* Walk through config hash and alter matching ini entries using the values found in the hash */
   ZEND_HASH_FOREACH_STR_KEY_VAL(source_hash, str, data) {
      zend_alter_ini_entry_ex(str, Z_STR_P(data), modify_type, stage, 0);
   } ZEND_HASH_FOREACH_END();
}

int php_ini_has_per_dir_config(void)
{
   return sg_hasPerDirConfig;
}

void php_ini_activate_per_dir_config(char *path, size_t path_len)
{
   zval *tmp2;
   char *ptr;
#ifdef POLAR_OS_WIN32
   char path_bak[MAXPATHLEN];
#endif

#ifdef POLAR_OS_WIN32
   /* MAX_PATH is \0-terminated, path_len == MAXPATHLEN would overrun path_bak */
   if (path_len >= MAXPATHLEN) {
#else
   if (path_len > MAXPATHLEN) {
#endif
      return;
   }

#ifdef POLAR_OS_WIN32
   memcpy(path_bak, path, path_len);
   path_bak[path_len] = 0;
   TRANSLATE_SLASHES_LOWER(path_bak);
   path = path_bak;
#endif

   /* Walk through each directory in path and apply any found per-dir-system-configuration from sg_configurationHash */
   if (sg_hasPerDirConfig && path && path_len) {
      ptr = path + 1;
      while ((ptr = strchr(ptr, '/')) != nullptr) {
         *ptr = 0;
         /* Search for source array matching the path from sg_configurationHash */
         if ((tmp2 = zend_hash_str_find(&sg_configurationHash, path, strlen(path))) != nullptr) {
            php_ini_activate_config(Z_ARRVAL_P(tmp2), POLAR_INI_SYSTEM, POLAR_INI_STAGE_ACTIVATE);
         }
         *ptr = '/';
         ptr++;
      }
   }
}

int php_ini_has_per_host_config()
{
   return sg_hasPerHostConfig;
}

void php_ini_activate_per_host_config(const char *host, size_t host_len)
{
   zval *tmp;
   if (sg_hasPerHostConfig && host && host_len) {
      /* Search for source array matching the host from sg_configurationHash */
      if ((tmp = zend_hash_str_find(&sg_configurationHash, host, host_len)) != nullptr) {
         php_ini_activate_config(Z_ARRVAL_P(tmp), POLAR_INI_SYSTEM, POLAR_INI_STAGE_ACTIVATE);
      }
   }
}

zval *cfg_get_entry_ex(zend_string *name)
{
   return zend_hash_find(&sg_configurationHash, name);
}

zval *cfg_get_entry(const char *name, size_t name_length)
{
   return zend_hash_str_find(&sg_configurationHash, name, name_length);
}

int cfg_get_long(const char *varname, zend_long *result)
{
   zval *tmp;
   if ((tmp = zend_hash_str_find(&sg_configurationHash, varname, strlen(varname))) == nullptr) {
      *result = 0;
      return FAILURE;
   }
   *result = zval_get_long(tmp);
   return SUCCESS;
}

int cfg_get_double(const char *varname, double *result)
{
   zval *tmp;
   if ((tmp = zend_hash_str_find(&sg_configurationHash, varname, strlen(varname))) == nullptr) {
      *result = (double) 0;
      return FAILURE;
   }
   *result = zval_get_double(tmp);
   return SUCCESS;
}

int cfg_get_string(const char *varname, char **result)
{
   zval *tmp;
   if ((tmp = zend_hash_str_find(&sg_configurationHash, varname, strlen(varname))) == nullptr) {
      *result = nullptr;
      return FAILURE;
   }
   *result = Z_STRVAL_P(tmp);
   return SUCCESS;
}

HashTable* php_ini_get_configuration_hash()
{
   return &sg_configurationHash;
}

} // runtime
} // polar
