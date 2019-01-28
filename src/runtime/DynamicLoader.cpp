// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/26.

#include "polarphp/runtime/DynamicLoader.h"
#include "polarphp/runtime/Spprintf.h"
#include "polarphp/runtime/ExecEnv.h"
#include "polarphp/runtime/internal/DepsZendVmHeaders.h"
#include "polarphp/basic/adt/Twine.h"

#if defined(HAVE_LIBDL)
#include <cstdlib>
#include <cstdio>
#ifdef HAVE_STRING_H
#include <cstring>
#else
#include <strings.h>
#endif
#ifdef POLAR_OS_WIN32
#include "win32/param.h"
#include "win32/winutil.h"
#define GET_DL_ERROR()	php_win_err()
#else
#include <sys/param.h>
#define GET_DL_ERROR()	DL_ERROR()
#endif
#endif /* defined(HAVE_LIBDL) */

namespace polar {
namespace runtime {

using polar::basic::Twine;

#if defined(HAVE_LIBDL)

void *php_load_shlib(StringRef path, std::string &errp)
{
   void *handle;
   char *err;

   handle = DL_LOAD(path.getData());
   if (!handle) {
      err = GET_DL_ERROR();
#ifdef POLAR_OS_WIN32
      if (err && (*err)) {
         size_t i = strlen(err);
         errp = err;
         LocalFree(err);
         while (i > 0 && isspace(errp[i-1])) { errp[i-1] = '\0'; i--; }
      } else {
         errp = "<No message>";
      }
#else
      errp = err;
      GET_DL_ERROR(); /* free the buffer storing the error */
#endif
   }
   return handle;
}

bool php_load_extension(StringRef filename, int type, bool startNow)
{
   void *handle;
   std::string libpath;
   zend_module_entry *module_entry;
   zend_module_entry *(*get_module)(void);
   int errorType;
   int slashSuffix = 0;
   StringRef extensionDir;
   if (type == MODULE_PERSISTENT) {
      extensionDir = "extension_dir";
   } else {
      ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
      extensionDir = execEnvInfo.extensionDir;
   }

   if (type == MODULE_TEMPORARY) {
      errorType = E_WARNING;
   } else {
      errorType = E_CORE_WARNING;
   }

   /* Check if passed filename contains directory separators */
   if (filename.find("/") != StringRef::npos || filename.find(DEFAULT_SLASH) != StringRef::npos) {
      /* Passing modules with full path is not supported for dynamically loaded extensions */
      if (type == MODULE_TEMPORARY) {
         php_error_docref(nullptr, E_WARNING, "Temporary module name should contain only filename");
         return false;
      }
      libpath = filename.getStr();
   } else if (!extensionDir.empty()) {
      slashSuffix = IS_SLASH(extensionDir[extensionDir.size() - 1]);
      /* Try as filename first */
      if (slashSuffix) {
         libpath = Twine(extensionDir).concat(filename).getStr();
      } else {
         libpath = Twine(extensionDir).concat(std::string(DEFAULT_SLASH, 1)).concat(filename).getStr();
      }
   } else {
      return false; /* Not full path given or extension_dir is not set */
   }
   std::string err1;
   handle = php_load_shlib(libpath, err1);
   if (!handle) {
      /* Now, consider 'filename' as extension name and build file name */
      std::string origLibpath = libpath;

      if (slashSuffix) {
         libpath = Twine(extensionDir)
               .concat(filename)
               .concat(".")
               .concat(POLARPHP_SHLIB_SUFFIX).getStr();
      } else {
         libpath = Twine(extensionDir)
               .concat(std::string(DEFAULT_SLASH, 1))
               .concat(filename)
               .concat(POLARPHP_SHLIB_SUFFIX).getStr();
      }
      std::string err2;
      handle = php_load_shlib(libpath, err2);
      if (!handle) {
         php_error_docref(nullptr, errorType, "Unable to load dynamic library '%s' (tried: %s (%s), %s (%s))",
                          filename.getData(), origLibpath.c_str(), err1.c_str(), libpath.c_str(), err2.c_str());
         return false;
      }
   }
   get_module = (zend_module_entry *(*)(void)) DL_FETCH_SYMBOL(handle, "get_module");
   /* Some OS prepend _ to symbol names while their dynamic linker
    * does not do that automatically. Thus we check manually for
    * _get_module. */
   if (!get_module) {
      get_module = (zend_module_entry *(*)(void)) DL_FETCH_SYMBOL(handle, "_get_module");
   }
   if (!get_module) {
      if (DL_FETCH_SYMBOL(handle, "zend_extension_entry") || DL_FETCH_SYMBOL(handle, "_zend_extension_entry")) {
         DL_UNLOAD(handle);
         php_error_docref(nullptr, errorType,
                          "Invalid library (appears to be a Zend Extension, try loading using zend_extension=%s from php.ini)",
                          filename.getData());
         return false;
      }
      DL_UNLOAD(handle);
      php_error_docref(nullptr, errorType, "Invalid library (maybe not a PHP library) '%s'", filename.getData());
      return false;
   }
   module_entry = get_module();
   if (module_entry->zend_api != ZEND_MODULE_API_NO) {
      php_error_docref(nullptr, errorType,
                       "%s: Unable to initialize module\n"
                       "Module compiled with module API=%d\n"
                       "PHP    compiled with module API=%d\n"
                       "These options need to match\n",
                       module_entry->name, module_entry->zend_api, ZEND_MODULE_API_NO);
      DL_UNLOAD(handle);
      return false;
   }
   if(strcmp(module_entry->build_id, ZEND_MODULE_BUILD_ID)) {
      php_error_docref(nullptr, errorType,
                       "%s: Unable to initialize module\n"
                       "Module compiled with build ID=%s\n"
                       "PHP    compiled with build ID=%s\n"
                       "These options need to match\n",
                       module_entry->name, module_entry->build_id, ZEND_MODULE_BUILD_ID);
      DL_UNLOAD(handle);
      return false;
   }
   module_entry->type = type;
   module_entry->module_number = zend_next_free_module();
   module_entry->handle = handle;
   if ((module_entry = zend_register_module_ex(module_entry)) == nullptr) {
      DL_UNLOAD(handle);
      return false;
   }
   if ((type == MODULE_TEMPORARY || startNow) && zend_startup_module_ex(module_entry) == false) {
      DL_UNLOAD(handle);
      return false;
   }
   if ((type == MODULE_TEMPORARY || startNow) && module_entry->request_startup_func) {
      if (module_entry->request_startup_func(type, module_entry->module_number) == false) {
         php_error_docref(nullptr, errorType, "Unable to initialize module '%s'", module_entry->name);
         DL_UNLOAD(handle);
         return false;
      }
   }
   return true;
}

void php_dl(StringRef file, int type, zval *return_value, int startNow)
{
   /* Load extension */
   if (php_load_extension(file, type, startNow) == false) {
      RETVAL_FALSE;
   } else {
      RETVAL_TRUE;
   }
}

#else

void php_dl(StringRef file, int type, zval *return_value, bool startNow)
{
   php_error_docref(nullptr, E_WARNING, "Cannot dynamically load %s - dynamic modules are not supported", file.getData());
   RETVAL_FALSE;
}

#endif

} // runtime
} // polar
