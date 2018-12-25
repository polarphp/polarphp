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

#include "polarphp/runtime/Utils.h"
#include "polarphp/runtime/internal/DepsZendVmHeaders.h"
#include "polarphp/runtime/ExecEnv.h"

#include <cctype>
#include <fcntl.h>

namespace polar {
namespace runtime {

std::size_t php_format_date(char* str, std::size_t count, const char* format,
                            time_t ts, bool localtime)
{
   /// TODO modify for multithread
   std::tm tm;
   std::tm *tempTm;
   if (localtime) {
      tempTm = std::localtime(&ts);
   } else {
      tempTm = std::gmtime(&ts);
   }
   tm = *tempTm;
   return std::strftime(str, count, format, &tm);
}

char *php_strtolower(char *s, size_t len)
{
   unsigned char *c;
   const unsigned char *e;

   c = (unsigned char *)s;
   e = c+len;

   while (c < e) {
      *c = std::tolower(*c);
      c++;
   }
   return s;
}

char *php_strtoupper(char *s, size_t len)
{
   unsigned char *c;
   const unsigned char *e;

   c = (unsigned char *)s;
   e = (unsigned char *)c+len;

   while (c < e) {
      *c = toupper(*c);
      c++;
   }
   return s;
}

char *php_strip_url_passwd(char *url)
{
   char *p = nullptr;
   char *url_start = nullptr;
   if (url == nullptr) {
      return PHP_EMPTY_STR;
   }
   p = url;
   while (*p) {
      if (*p == ':' && *(p + 1) == '/' && *(p + 2) == '/') {
         /* found protocol */
         url_start = p = p + 3;
         while (*p) {
            if (*p == '@') {
               int i;
               for (i = 0; i < 3 && url_start < p; i++, url_start++) {
                  *url_start = '.';
               }
               for (; *p; p++) {
                  *url_start++ = *p;
               }
               *url_start=0;
               break;
            }
            p++;
         }
         return url;
      }
      p++;
   }
   return url;
}

char *expand_filepath(const char *filePath, char *realPath)
{
   return expand_filepath(filePath, realPath, nullptr, 0);
}

char *expand_filepath(const char *filePath, char *realPath,
                      const char *relativeTo, size_t relativeToLen)
{
   return expand_filepath(filePath, realPath, relativeTo, relativeToLen, CWD_FILEPATH);
}

int php_check_open_basedir(const char *path)
{
   return php_check_open_basedir(path, 1);
}

///
/// review memory leak
///
int php_check_open_basedir(const char *path, int warn)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   /// Only check when open_basedir is available
   std::string &openBaseDir = execEnvInfo.openBaseDir;
   if (!openBaseDir.empty()) {
      char *pathbuf;
      char *ptr;
      char *end;
      /* Check if the path is too long so we can give a more useful error
         * message. */
      if (strlen(path) > (MAXPATHLEN - 1)) {
         php_error_docref(nullptr, E_WARNING, "File name is longer than the maximum allowed path length on this platform (%d): %s", MAXPATHLEN, path);
         errno = EINVAL;
         return -1;
      }
      pathbuf = estrdup(openBaseDir.c_str());
      ptr = pathbuf;
      while (ptr && *ptr) {
         end = strchr(ptr, DEFAULT_DIR_SEPARATOR);
         if (end != nullptr) {
            *end = '\0';
            end++;
         }
         if (php_check_specific_open_basedir(ptr, path) == 0) {
            efree(pathbuf);
            return 0;
         }
         ptr = end;
      }
      if (warn) {
         php_error_docref(nullptr, E_WARNING, "open_basedir restriction in effect. File(%s) is not within the allowed path(s): (%s)", path, openBaseDir.c_str());
      }
      efree(pathbuf);
      errno = EPERM; /* we deny permission to open it */
      return -1;
   }
   /* Nothing to check... */
   return 0;
}

///
/// review memory leak
///
int php_check_specific_open_basedir(const char *basedir, const char *path)
{
   char resolvedName[MAXPATHLEN];
   char resolvedBaseDir[MAXPATHLEN];
   char localOpenBaseDir[MAXPATHLEN];
   char pathTmp[MAXPATHLEN];
   char *pathFile;
   size_t resolvedBaseDirLen;
   size_t resolvedNameLen;
   size_t pathLen;
   int nestingLevel = 0;

   /* Special case basedir==".": Use script-directory */
   if (strcmp(basedir, ".") || !VCWD_GETCWD(localOpenBaseDir, MAXPATHLEN)) {
      /* Else use the unmodified path */
      strlcpy(localOpenBaseDir, basedir, sizeof(localOpenBaseDir));
   }
   pathLen = strlen(path);
   if (pathLen > (MAXPATHLEN - 1)) {
      /* empty and too long paths are invalid */
      return -1;
   }

   /* normalize and expand path */
   if (expand_filepath(path, resolvedName) == NULL) {
      return -1;
   }

   pathLen = strlen(resolvedName);
   memcpy(pathTmp, resolvedName, pathLen + 1); /* safe */

   while (VCWD_REALPATH(pathTmp, resolvedName) == NULL) {
#if defined(POLAR_OS_WIN32) || defined(HAVE_SYMLINK)
      if (nestingLevel == 0) {
         ssize_t ret;
         char buf[MAXPATHLEN];

         ret = php_sys_readlink(pathTmp, buf, MAXPATHLEN - 1);
         if (ret == -1) {
            /* not a broken symlink, move along.. */
         } else {
            /* put the real path into the path buffer */
            memcpy(pathTmp, buf, ret);
            pathTmp[ret] = '\0';
         }
      }
#endif

#ifdef POLAR_OS_WIN32
      pathFile = strrchr(pathTmp, DEFAULT_SLASH);
      if (!pathFile) {
         pathFile = strrchr(pathTmp, '/');
      }
#else
      pathFile = strrchr(pathTmp, DEFAULT_SLASH);
#endif
      if (!pathFile) {
         /* none of the path components exist. definitely not in open_basedir.. */
         return -1;
      } else {
         pathLen = pathFile - pathTmp + 1;
#ifdef POLAR_OS_WIN32
         if (pathLen > 1 && pathTmp[pathLen - 2] == ':') {
            if (pathLen != 3) {
               return -1;
            }
            /* this is c:\ */
            pathTmp[pathLen] = '\0';
         } else {
            pathTmp[pathLen - 1] = '\0';
         }
#else
         pathTmp[pathLen - 1] = '\0';
#endif
      }
      nestingLevel++;
   }

   /* Resolve open_basedir to resolvedBaseDir */
   if (expand_filepath(localOpenBaseDir, resolvedBaseDir) != NULL) {
      size_t baseDirLen = strlen(basedir);
      /* Handler for basedirs that end with a / */
      resolvedBaseDirLen = strlen(resolvedBaseDir);
#ifdef POLAR_OS_WIN32
      if (basedir[baseDirLen - 1] == PHP_DIR_SEPARATOR || basedir[baseDirLen - 1] == '/') {
#else
      if (basedir[baseDirLen - 1] == PHP_DIR_SEPARATOR) {
#endif
         if (resolvedBaseDir[resolvedBaseDirLen - 1] != PHP_DIR_SEPARATOR) {
            resolvedBaseDir[resolvedBaseDirLen] = PHP_DIR_SEPARATOR;
            resolvedBaseDir[++resolvedBaseDirLen] = '\0';
         }
      } else {
         resolvedBaseDir[resolvedBaseDirLen++] = PHP_DIR_SEPARATOR;
         resolvedBaseDir[resolvedBaseDirLen] = '\0';
      }

      resolvedNameLen = strlen(resolvedName);
      if (pathTmp[pathLen - 1] == PHP_DIR_SEPARATOR) {
         if (resolvedName[resolvedNameLen - 1] != PHP_DIR_SEPARATOR) {
            resolvedName[resolvedNameLen] = PHP_DIR_SEPARATOR;
            resolvedName[++resolvedNameLen] = '\0';
         }
      }

      /* Check the path */
#ifdef POLAR_OS_WIN32
      if (strncasecmp(resolvedBaseDir, resolvedName, resolvedBaseDirLen) == 0) {
#else
      if (strncmp(resolvedBaseDir, resolvedName, resolvedBaseDirLen) == 0) {
#endif
         if (resolvedNameLen > resolvedBaseDirLen &&
             resolvedName[resolvedBaseDirLen - 1] != PHP_DIR_SEPARATOR) {
            return -1;
         } else {
            /* File is in the right directory */
            return 0;
         }
      } else {
         /* /openbasedir/ and /openbasedir are the same directory */
         if (resolvedBaseDirLen == (resolvedNameLen + 1) && resolvedBaseDir[resolvedBaseDirLen - 1] == PHP_DIR_SEPARATOR) {
#ifdef POLAR_OS_WIN32
            if (strncasecmp(resolvedBaseDir, resolvedName, resolvedNameLen) == 0) {
#else
            if (strncmp(resolvedBaseDir, resolvedName, resolvedNameLen) == 0) {
#endif
               return 0;
            }
         }
         return -1;
      }
   } else {
      /* Unable to resolve the real path, return -1 */
      return -1;
   }
}

namespace {
///
/// review memory leak
///
FILE *php_fopen_and_set_opened_path(const char *path, const char *mode,
                                    zend_string **opened_path)
{
   FILE *fp;
   if (php_check_open_basedir(const_cast<char *>(path))) {
      return nullptr;
   }
   fp = VCWD_FOPEN(path, mode);
   if (fp && opened_path) {
      //TODO :avoid reallocation
      char *tmp = expand_filepath(path, nullptr, nullptr, 0, CWD_EXPAND);
      if (tmp) {
         *opened_path = zend_string_init(tmp, strlen(tmp), 0);
         efree(tmp);
      }
   }
   return fp;
}
} // anonymous namespace

///
/// review memory leak
///
char *expand_filepath(const char *filePath, char *realPath,
                      const char *relativeTo, size_t relativeToLen,
                      int realPathMode)
{
   cwd_state newState;
   char cwd[MAXPATHLEN];
   size_t copyLen;
   size_t pathLen;
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   if (!filePath[0]) {
      return nullptr;
   }
   pathLen = strlen(filePath);
   if (IS_ABSOLUTE_PATH(filePath, pathLen)) {
      cwd[0] = '\0';
   } else {
      const char *iam = execEnvInfo.entryScriptFilename.c_str();
      const char *result;
      if (relativeTo) {
         if (relativeToLen > MAXPATHLEN-1U) {
            return nullptr;
         }
         result = relativeTo;
         memcpy(cwd, relativeTo, relativeToLen+1U);
      } else {
         result = VCWD_GETCWD(cwd, MAXPATHLEN);
      }

      if (!result && (iam != filePath)) {
         int fdtest = -1;

         fdtest = VCWD_OPEN(filePath, O_RDONLY);
         if (fdtest != -1) {
            /* return a relative file path if for any reason
                * we cannot cannot getcwd() and the requested,
                * relatively referenced file is accessible */
            copyLen = pathLen > MAXPATHLEN - 1 ? MAXPATHLEN - 1 : pathLen;
            if (realPath) {
               memcpy(realPath, filePath, copyLen);
               realPath[copyLen] = '\0';
            } else {
               realPath = estrndup(filePath, copyLen);
            }
            close(fdtest);
            return realPath;
         } else {
            cwd[0] = '\0';
         }
      } else if (!result) {
         cwd[0] = '\0';
      }
   }

   newState.cwd = estrdup(cwd);
   newState.cwd_length = strlen(cwd);

   if (virtual_file_ex(&newState, filePath, nullptr, realPathMode)) {
      efree(newState.cwd);
      return nullptr;
   }

   if (realPath) {
      copyLen = newState.cwd_length > MAXPATHLEN - 1 ? MAXPATHLEN - 1 : newState.cwd_length;
      memcpy(realPath, newState.cwd, copyLen);
      realPath[copyLen] = '\0';
   } else {
      realPath = estrndup(newState.cwd, newState.cwd_length);
   }
   efree(newState.cwd);

   return realPath;
}

///
/// review memory leak
///
FILE *php_fopen_with_path(const char *filename, const char *mode,
                          const char *path, zend_string **openedPath)
{
   char *pathbuf;
   char *ptr;
   char *end;
   char trypath[MAXPATHLEN];
   FILE *fp;
   size_t filename_length;
   zend_string *exec_filename;
   if (openedPath) {
      *openedPath = nullptr;
   }
   if (!filename) {
      return nullptr;
   }
   filename_length = strlen(filename);
#ifndef POLAR_OS_WIN32
   (void) filename_length;
#endif

   /* Relative path open */
   if ((*filename == '.')
       /* Absolute path open */
       || IS_ABSOLUTE_PATH(filename, filename_length)
       || (!path || !*path)
       ) {
      return php_fopen_and_set_opened_path(filename, mode, openedPath);
   }

   /* check in provided path */
   /* append the calling scripts' current working directory
       * as a fall back case
       */
   if (zend_is_executing() &&
       (exec_filename = zend_get_executed_filename_ex()) != nullptr) {
      const char *exec_fname = ZSTR_VAL(exec_filename);
      size_t exec_fname_length = ZSTR_LEN(exec_filename);

      while ((--exec_fname_length < SIZE_MAX) && !IS_SLASH(exec_fname[exec_fname_length]));
      if ((exec_fname && exec_fname[0] == '[') || exec_fname_length <= 0) {
         /* [no active file] or no path */
         pathbuf = estrdup(path);
      } else {
         size_t path_length = strlen(path);
         pathbuf = (char *) emalloc(exec_fname_length + path_length + 1 + 1);
         memcpy(pathbuf, path, path_length);
         pathbuf[path_length] = DEFAULT_DIR_SEPARATOR;
         memcpy(pathbuf + path_length + 1, exec_fname, exec_fname_length);
         pathbuf[path_length + exec_fname_length + 1] = '\0';
      }
   } else {
      pathbuf = estrdup(path);
   }
   ptr = pathbuf;
   while (ptr && *ptr) {
      end = strchr(ptr, DEFAULT_DIR_SEPARATOR);
      if (end != nullptr) {
         *end = '\0';
         end++;
      }
      if (snprintf(trypath, MAXPATHLEN, "%s/%s", ptr, filename) >= MAXPATHLEN) {
         php_error_docref(nullptr, E_NOTICE, "%s/%s path was truncated to %d", ptr, filename, MAXPATHLEN);
      }
      fp = php_fopen_and_set_opened_path(trypath, mode, openedPath);
      if (fp) {
         efree(pathbuf);
         return fp;
      }
      ptr = end;
   } /* end provided path */
   efree(pathbuf);
   return nullptr;
}

} // runtime
} // utils
