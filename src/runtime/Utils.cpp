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

namespace {
inline int php_charmask(const unsigned char *input, size_t len, char *mask)
{
   const unsigned char *end;
   unsigned char c;
   int result = SUCCESS;
   memset(mask, 0, 256);
   for (end = input+len; input < end; input++) {
      c=*input;
      if ((input+3 < end) && input[1] == '.' && input[2] == '.'
          && input[3] >= c) {
         memset(mask+c, 1, input[3] - c + 1);
         input+=3;
      } else if ((input+1 < end) && input[0] == '.' && input[1] == '.') {
         /* Error, try to be as helpful as possible:
            (a range ending/starting with '.' won't be captured here) */
         if (end-len >= input) { /* there was no 'left' char */
            php_error_docref(NULL, E_WARNING, "Invalid '..'-range, no character to the left of '..'");
            result = FAILURE;
            continue;
         }
         if (input+2 >= end) { /* there is no 'right' char */
            php_error_docref(NULL, E_WARNING, "Invalid '..'-range, no character to the right of '..'");
            result = FAILURE;
            continue;
         }
         if (input[-1] > input[2]) { /* wrong order */
            php_error_docref(NULL, E_WARNING, "Invalid '..'-range, '..'-range needs to be incrementing");
            result = FAILURE;
            continue;
         }
         /* FIXME: better error (a..b..c is the only left possibility?) */
         php_error_docref(NULL, E_WARNING, "Invalid '..'-range");
         result = FAILURE;
         continue;
      } else {
         mask[c]=1;
      }
   }
   return result;
}
} // anonymous namespace

zend_string *php_addcslashes_str(const char *str, size_t len, char *what, size_t wlength)
{
   char flags[256];
   char *target;
   const char *source, *end;
   char c;
   size_t  newlen;
   zend_string *new_str = zend_string_safe_alloc(4, len, 0, 0);

   php_charmask((unsigned char *)what, wlength, flags);

   for (source = str, end = source + len, target = ZSTR_VAL(new_str); source < end; source++) {
      c = *source;
      if (flags[(unsigned char)c]) {
         if ((unsigned char) c < 32 || (unsigned char) c > 126) {
            *target++ = '\\';
            switch (c) {
            case '\n': *target++ = 'n'; break;
            case '\t': *target++ = 't'; break;
            case '\r': *target++ = 'r'; break;
            case '\a': *target++ = 'a'; break;
            case '\v': *target++ = 'v'; break;
            case '\b': *target++ = 'b'; break;
            case '\f': *target++ = 'f'; break;
            default: target += sprintf(target, "%03o", (unsigned char) c);
            }
            continue;
         }
         *target++ = '\\';
      }
      *target++ = c;
   }
   *target = 0;
   newlen = target - ZSTR_VAL(new_str);
   if (newlen < len * 4) {
      new_str = zend_string_truncate(new_str, newlen, 0);
   }
   return new_str;
}

zend_string *php_addcslashes(zend_string *str, char *what, size_t wlength)
{
   return php_addcslashes_str(ZSTR_VAL(str), ZSTR_LEN(str), what, wlength);
}

zend_string *php_str_to_str(const char *haystack, size_t length, const char *needle,
                            size_t needle_len, const char *str, size_t str_len)
{
   zend_string *new_str;

   if (needle_len < length) {
      const char *end;
      const char *s, *p;
      char *e, *r;

      if (needle_len == str_len) {
         new_str = zend_string_init(haystack, length, 0);
         end = ZSTR_VAL(new_str) + length;
         for (p = ZSTR_VAL(new_str); (r = const_cast<char*>(php_memnstr(p, needle, needle_len, end))); p = r + needle_len) {
            memcpy(r, str, str_len);
         }
         return new_str;
      } else {
         if (str_len < needle_len) {
            new_str = zend_string_alloc(length, 0);
         } else {
            size_t count = 0;
            const char *o = haystack;
            const char *n = needle;
            const char *endp = o + length;

            while ((o = const_cast<char*>(php_memnstr(o, n, needle_len, endp)))) {
               o += needle_len;
               count++;
            }
            if (count == 0) {
               /* Needle doesn't occur, shortcircuit the actual replacement. */
               new_str = zend_string_init(haystack, length, 0);
               return new_str;
            } else {
               if (str_len > needle_len) {
                  new_str = zend_string_safe_alloc(count, str_len - needle_len, length, 0);
               } else {
                  new_str = zend_string_alloc(count * (str_len - needle_len) + length, 0);
               }
            }
         }

         s = e = ZSTR_VAL(new_str);
         end = haystack + length;
         for (p = haystack; (r = const_cast<char*>(php_memnstr(p, needle, needle_len, end))); p = r + needle_len) {
            memcpy(e, p, r - p);
            e += r - p;
            memcpy(e, str, str_len);
            e += str_len;
         }

         if (p < end) {
            memcpy(e, p, end - p);
            e += end - p;
         }

         *e = '\0';
         new_str = zend_string_truncate(new_str, e - s, 0);
         return new_str;
      }
   } else if (needle_len > length || memcmp(haystack, needle, length)) {
      new_str = zend_string_init(haystack, length, 0);
      return new_str;
   } else {
      new_str = zend_string_init(str, str_len, 0);

      return new_str;
   }
}

zend_string *php_string_toupper(zend_string *s)
{
   unsigned char *c;
   const unsigned char *e;

   c = (unsigned char *)ZSTR_VAL(s);
   e = c + ZSTR_LEN(s);

   while (c < e) {
      if (islower(*c)) {
         unsigned char *r;
         zend_string *res = zend_string_alloc(ZSTR_LEN(s), 0);

         if (c != (unsigned char*)ZSTR_VAL(s)) {
            memcpy(ZSTR_VAL(res), ZSTR_VAL(s), c - (unsigned char*)ZSTR_VAL(s));
         }
         r = c + (ZSTR_VAL(res) - ZSTR_VAL(s));
         while (c < e) {
            *r = toupper(*c);
            r++;
            c++;
         }
         *r = '\0';
         return res;
      }
      c++;
   }
   return zend_string_copy(s);
}

zend_string *php_string_tolower(zend_string *s)
{
   unsigned char *c;
   const unsigned char *e;

   c = (unsigned char *)ZSTR_VAL(s);
   e = c + ZSTR_LEN(s);

   while (c < e) {
      if (isupper(*c)) {
         unsigned char *r;
         zend_string *res = zend_string_alloc(ZSTR_LEN(s), 0);

         if (c != (unsigned char*)ZSTR_VAL(s)) {
            memcpy(ZSTR_VAL(res), ZSTR_VAL(s), c - (unsigned char*)ZSTR_VAL(s));
         }
         r = c + (ZSTR_VAL(res) - ZSTR_VAL(s));
         while (c < e) {
            *r = tolower(*c);
            r++;
            c++;
         }
         *r = '\0';
         return res;
      }
      c++;
   }
   return zend_string_copy(s);
}

namespace {
int compare_right(char const **a, char const *aend, char const **b, char const *bend)
{
   int bias = 0;

   /* The longest run of digits wins.  That aside, the greatest
      value wins, but we can't know that it will until we've scanned
      both numbers to know that they have the same magnitude, so we
      remember it in BIAS. */
   for(;; (*a)++, (*b)++) {
      if ((*a == aend || !isdigit((int)(unsigned char)**a)) &&
          (*b == bend || !isdigit((int)(unsigned char)**b))) {
         return bias;
      } else if (*a == aend || !isdigit((int)(unsigned char)**a)) {
         return -1;
      } else if (*b == bend || !isdigit((int)(unsigned char)**b)) {
         return +1;
      } else if (**a < **b) {
         if (!bias) {
            bias = -1;
         }
      } else if (**a > **b) {
         if (!bias) {
            bias = +1;
         }
      }
   }
   return 0;
}

int compare_left(char const **a, char const *aend, char const **b, char const *bend)
{
   /* Compare two left-aligned numbers: the first to have a
        different value wins. */
   for(;; (*a)++, (*b)++) {
      if ((*a == aend || !isdigit((int)(unsigned char)**a)) &&
          (*b == bend || !isdigit((int)(unsigned char)**b))) {
         return 0;
      } else if (*a == aend || !isdigit((int)(unsigned char)**a)) {
         return -1;
      } else if (*b == bend || !isdigit((int)(unsigned char)**b)) {
         return +1;
      } else if (**a < **b) {
         return -1;
      } else if (**a > **b) {
         return +1;
      }
   }

   return 0;
}
} // anonymous namespace

int strnatcmp_ex(char const *a, size_t a_len, char const *b, size_t b_len, int fold_case)
{
   unsigned char ca, cb;
   char const *ap, *bp;
   char const *aend = a + a_len,
         *bend = b + b_len;
   int fractional, result;
   short leading = 1;

   if (a_len == 0 || b_len == 0) {
      return (a_len == b_len ? 0 : (a_len > b_len ? 1 : -1));
   }

   ap = a;
   bp = b;
   while (1) {
      ca = *ap; cb = *bp;

      /* skip over leading zeros */
      while (leading && ca == '0' && (ap+1 < aend) && isdigit((int)(unsigned char)*(ap+1))) {
         ca = *++ap;
      }

      while (leading && cb == '0' && (bp+1 < bend) && isdigit((int)(unsigned char)*(bp+1))) {
         cb = *++bp;
      }

      leading = 0;

      /* Skip consecutive whitespace */
      while (isspace((int)(unsigned char)ca)) {
         ca = *++ap;
      }

      while (isspace((int)(unsigned char)cb)) {
         cb = *++bp;
      }

      /* process run of digits */
      if (isdigit((int)(unsigned char)ca)  &&  isdigit((int)(unsigned char)cb)) {
         fractional = (ca == '0' || cb == '0');

         if (fractional) {
            result = compare_left(&ap, aend, &bp, bend);
         } else {
            result = compare_right(&ap, aend, &bp, bend);
         }
         if (result != 0) {
            return result;
         } else if (ap == aend && bp == bend) {
            /* End of the strings. Let caller sort them out. */
            return 0;
         } else if (ap == aend) {
            return -1;
         } else if (bp == bend) {
            return 1;
         } else {
            /* Keep on comparing from the current point. */
            ca = *ap; cb = *bp;
         }
      }

      if (fold_case) {
         ca = toupper((int)(unsigned char)ca);
         cb = toupper((int)(unsigned char)cb);
      }

      if (ca < cb) {
         return -1;
      } else if (ca > cb) {
         return +1;
      }

      ++ap; ++bp;
      if (ap >= aend && bp >= bend) {
         /* The strings compare the same.  Perhaps the caller
            will want to call strcmp to break the tie. */
         return 0;
      } else if (ap >= aend) {
         return -1;
      } else if (bp >= bend) {
         return 1;
      }
   }
}

zend_long mt_rand_range(zend_long min, zend_long max)
{
   zend_ulong umax = max - min;
   std::random_device rd;
#if ZEND_ULONG_MAX > UINT32_MAX
   if (umax > UINT32_MAX) {
      std::mt19937_64 gen(rd());
      std::uniform_int_distribution<uint64_t> dis(min, max);
      return static_cast<zend_long>(dis(gen));
   }
#endif
   std::mt19937 gen(rd());
   std::uniform_int_distribution<uint32_t> dis(min, max);
   return static_cast<zend_long>(dis(gen));
}

std::uint32_t mt_rand_32()
{
   std::random_device rd;
   std::mt19937 gen(rd());
   return gen();
}

std::uint64_t mt_rand_64()
{
   std::random_device rd;
   std::mt19937_64 gen(rd());
   return gen();
}

} // runtime
} // utils
