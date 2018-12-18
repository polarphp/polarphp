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
#include "ZendHeaders.h"

#include <cstdio>

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

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

extern int sg_moduleInitialized;
extern int sg_moduleStartup;
extern int sg_moduleShutdown;

CliShellCallbacksType sg_cliShellCallbacks = {nullptr, nullptr, nullptr};

POLAR_DECL_EXPORT CliShellCallbacksType *php_cli_get_shell_callbacks()
{
   return &sg_cliShellCallbacks;
}

ExecEnv &retrieve_global_execenv()
{
   thread_local ExecEnv execEnv;
   return execEnv;
}

ExecEnv::ExecEnv()
   : m_started(false),
     /// TODO read from cfg file
     m_defaultSocketTimeout(60),
     m_logErrorsMaxLen(1024),
     m_ignoreRepeatedErrors(false),
     m_ignoreRepeatedSource(false),
     m_displayErrors(true),
     m_logErrors(true),
     m_syslogFacility(LOG_USER),
     m_syslogIdent("polarphp"),
     m_syslogFilter(PHP_SYSLOG_FILTER_NO_CTRL)
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

/// polarphp don't use timeout at vm level
///
void php_on_timeout(int seconds)
{

}

int php_execute_script(zend_file_handle *primaryFile)
{
   ExecEnv &execEnv = retrieve_global_execenv();
   zend_file_handle *prepend_file_p = nullptr, *append_file_p = nullptr;
   zend_file_handle prepend_file = {{0}, nullptr, nullptr, zend_stream_type(0), zend_stream_type(0)}, append_file = {{0}, nullptr, nullptr, zend_stream_type(0), zend_stream_type(0)};
#if HAVE_BROKEN_GETCWD
   volatile int old_cwd_fd = -1;
#else
   char *old_cwd;
   ALLOCA_FLAG(use_heap)
      #endif
         int retval = 0;

   EG(exit_status) = 0;
#ifndef HAVE_BROKEN_GETCWD
# define OLD_CWD_SIZE 4096
   old_cwd = reinterpret_cast<char *>(do_alloca(OLD_CWD_SIZE, use_heap));
   old_cwd[0] = '\0';
#endif

   polar_try {
      char realfile[MAXPATHLEN];
#ifdef POALR_OS_WIN32
      if(primaryFile->filename) {
         UpdateIniFromRegistry((char*)primaryFile->filename);
      }
#endif
      execEnv.setDuringExecEnvStartup(false);
      //      if (primaryFile->filename && !(SG(options) & SAPI_OPTION_NO_CHDIR)) {
      //#if HAVE_BROKEN_GETCWD
      //         /* this looks nasty to me */
      //         old_cwd_fd = open(".", 0);
      //#else
      //         php_ignore_value(VCWD_GETCWD(old_cwd, OLD_CWD_SIZE-1));
      //#endif
      //         VCWD_CHDIR_FILE(primaryFile->filename);
      //      }

      /* Only lookup the real file path and add it to the included_files list if already opened
       *   otherwise it will get opened and added to the included_files list in zend_execute_scripts
       */
      if (primaryFile->filename &&
          strcmp("Standard input code", primaryFile->filename) &&
          primaryFile->opened_path == nullptr &&
          primaryFile->type != ZEND_HANDLE_FILENAME
          ) {
         //         if (expand_filepath(primaryFile->filename, realfile)) {
         //            primaryFile->opened_path = zend_string_init(realfile, strlen(realfile), 0);
         //            zend_hash_add_empty_element(&EG(included_files), primaryFile->opened_path);
         //         }
      }

      //      if (PG(auto_prepend_file) && PG(auto_prepend_file)[0]) {
      //         prepend_file.filename = PG(auto_prepend_file);
      //         prepend_file.opened_path = nullptr;
      //         prepend_file.free_filename = 0;
      //         prepend_file.type = ZEND_HANDLE_FILENAME;
      //         prepend_file_p = &prepend_file;
      //      } else {
      //         prepend_file_p = nullptr;
      //      }

      //      if (PG(auto_append_file) && PG(auto_append_file)[0]) {
      //         append_file.filename = PG(auto_append_file);
      //         append_file.opened_path = nullptr;
      //         append_file.free_filename = 0;
      //         append_file.type = ZEND_HANDLE_FILENAME;
      //         append_file_p = &append_file;
      //      } else {
      //         append_file_p = nullptr;
      //      }


      /*
         If cli primary file has shabang line and there is a prepend file,
         the `start_lineno` will be used by prepend file but not primary file,
         save it and restore after prepend file been executed.
       */
      if (CG(start_lineno) && prepend_file_p) {
         int orig_start_lineno = CG(start_lineno);

         CG(start_lineno) = 0;
         if (zend_execute_scripts(ZEND_REQUIRE, nullptr, 1, prepend_file_p) == SUCCESS) {
            CG(start_lineno) = orig_start_lineno;
            retval = (zend_execute_scripts(ZEND_REQUIRE, nullptr, 2, primaryFile, append_file_p) == SUCCESS);
         }
      } else {
         retval = (zend_execute_scripts(ZEND_REQUIRE, nullptr, 3, prepend_file_p, primaryFile, append_file_p) == SUCCESS);
      }
   } polar_end_try;

   if (EG(exception)) {
      polar_try {
         zend_exception_error(EG(exception), E_ERROR);
      } polar_end_try;
   }

#if HAVE_BROKEN_GETCWD
   if (old_cwd_fd != -1) {
      fchdir(old_cwd_fd);
      close(old_cwd_fd);
   }
#else
   if (old_cwd[0] != '\0') {
      php_ignore_value(VCWD_CHDIR(old_cwd));
   }
   free_alloca(old_cwd, use_heap);
#endif
   return retval;
}

int php_execute_simple_script(zend_file_handle *primaryFile, zval *ret)
{
   return 0;
}

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

void cli_flush()
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
   ExecEnv &execEnv = retrieve_global_execenv();
   /// need protected for memory leak
   /// TODO find elegance way
   ///
   char *bufferRaw = nullptr;
   bool display;
   int bufferLen = static_cast<int>(zend_vspprintf(&bufferRaw, execEnv.getLogErrorsMaxLen(), format, args));
   std::shared_ptr<char> buffer;
   if (bufferRaw) {
      buffer.reset(bufferRaw, [](void *ptr){
         efree(ptr);
      });
   }
   /* check for repeated errors to be ignored */
   if (execEnv.getIgnoreRepeatedErrors() && !execEnv.getLastErrorMessage().empty()) {
      /* no check for execEnv.getLastErrorFile() is needed since it cannot
          * be empty if execEnv.getLastErrorMessage() is not empty */
      StringRef lastErrorMsg = execEnv.getLastErrorMessage();
      if (lastErrorMsg != buffer.get()
          || (!execEnv.getIgnoreRepeatedSource()
              && ((execEnv.getLastErrorLineno() != (int)errorLineno)
                  || execEnv.getLastErrorFile() != errorFilename))) {
         display = true;
      } else {
         display = false;
      }
   } else {
      display = true;
   }

   /* according to error handling mode, throw exception or show it */
   if (EG(error_handling) == EH_THROW) {
      switch (type) {
      case E_ERROR:
      case E_CORE_ERROR:
      case E_COMPILE_ERROR:
      case E_USER_ERROR:
      case E_PARSE:
         /* fatal errors are real errors and cannot be made exceptions */
         break;
      case E_STRICT:
      case E_DEPRECATED:
      case E_USER_DEPRECATED:
         /* for the sake of BC to old damaged code */
         break;
      case E_NOTICE:
      case E_USER_NOTICE:
         /* notices are no errors and are not treated as such like E_WARNINGS */
         break;
      default:
         /* throw an exception if we are in EH_THROW mode
                * but DO NOT overwrite a pending exception
                */
         if (!EG(exception)) {
            zend_throw_error_exception(EG(exception_class), bufferRaw, 0, type);
         }
         return;
      }
   }

   /* store the error if it has changed */
   if (display) {
      if (!errorFilename) {
         errorFilename = "Unknown";
      }
      execEnv.setLastErrorType(type);
      execEnv.setLastErrorMessage(std::string(bufferRaw, bufferLen));
      // errorFilename must be c string ?
      execEnv.setLastErrorFile(errorFilename);
      execEnv.setLastErrorLineno(errorLineno);
   }

//   /* display/log the error if necessary */
//   if (display && (EG(error_reporting) & type || (type & E_CORE))
//       && (execEnv.getLogErrors() || execEnv.getDisplayErrors() || (!sg_moduleInitialized))) {
//      char *error_type_str;
//      int syslog_type_int = LOG_NOTICE;

//      switch (type) {
//      case E_ERROR:
//      case E_CORE_ERROR:
//      case E_COMPILE_ERROR:
//      case E_USER_ERROR:
//         error_type_str = "Fatal error";
//         syslog_type_int = LOG_ERR;
//         break;
//      case E_RECOVERABLE_ERROR:
//         error_type_str = "Recoverable fatal error";
//         syslog_type_int = LOG_ERR;
//         break;
//      case E_WARNING:
//      case E_CORE_WARNING:
//      case E_COMPILE_WARNING:
//      case E_USER_WARNING:
//         error_type_str = "Warning";
//         syslog_type_int = LOG_WARNING;
//         break;
//      case E_PARSE:
//         error_type_str = "Parse error";
//         syslog_type_int = LOG_EMERG;
//         break;
//      case E_NOTICE:
//      case E_USER_NOTICE:
//         error_type_str = "Notice";
//         syslog_type_int = LOG_NOTICE;
//         break;
//      case E_STRICT:
//         error_type_str = "Strict Standards";
//         syslog_type_int = LOG_INFO;
//         break;
//      case E_DEPRECATED:
//      case E_USER_DEPRECATED:
//         error_type_str = "Deprecated";
//         syslog_type_int = LOG_INFO;
//         break;
//      default:
//         error_type_str = "Unknown error";
//         break;
//      }

//      if (!module_initialized || PG(log_errors)) {
//         char *log_buffer;
//#ifdef PHP_WIN32
//         if (type == E_CORE_ERROR || type == E_CORE_WARNING) {
//            syslog(LOG_ALERT, "PHP %s: %s (%s)", error_type_str, buffer, GetCommandLine());
//         }
//#endif
//         spprintf(&log_buffer, 0, "PHP %s:  %s in %s on line %" PRIu32, error_type_str, buffer, error_filename, error_lineno);
//         php_log_err_with_severity(log_buffer, syslog_type_int);
//         efree(log_buffer);
//      }

//      if (PG(display_errors) && ((module_initialized && !PG(during_request_startup)) || (PG(display_startup_errors)))) {
//         if (PG(xmlrpc_errors)) {
//            php_printf("<?xml version=\"1.0\"?><methodResponse><fault><value><struct><member><name>faultCode</name><value><int>" ZEND_LONG_FMT "</int></value></member><member><name>faultString</name><value><string>%s:%s in %s on line %" PRIu32 "</string></value></member></struct></value></fault></methodResponse>", PG(xmlrpc_error_number), error_type_str, buffer, error_filename, error_lineno);
//         } else {
//            char *prepend_string = INI_STR("error_prepend_string");
//            char *append_string = INI_STR("error_append_string");

//            if (PG(html_errors)) {
//               if (type == E_ERROR || type == E_PARSE) {
//                  zend_string *buf = php_escape_html_entities((unsigned char*)buffer, buffer_len, 0, ENT_COMPAT, get_safe_charset_hint());
//                  php_printf("%s<br />\n<b>%s</b>:  %s in <b>%s</b> on line <b>%" PRIu32 "</b><br />\n%s", STR_PRINT(prepend_string), error_type_str, ZSTR_VAL(buf), error_filename, error_lineno, STR_PRINT(append_string));
//                  zend_string_free(buf);
//               } else {
//                  php_printf("%s<br />\n<b>%s</b>:  %s in <b>%s</b> on line <b>%" PRIu32 "</b><br />\n%s", STR_PRINT(prepend_string), error_type_str, buffer, error_filename, error_lineno, STR_PRINT(append_string));
//               }
//            } else {
//               /* Write CLI/CGI errors to stderr if display_errors = "stderr" */
//               if ((!strcmp(sapi_module.name, "cli") || !strcmp(sapi_module.name, "cgi")) &&
//                   PG(display_errors) == PHP_DISPLAY_ERRORS_STDERR
//                   ) {
//                  fprintf(stderr, "%s: %s in %s on line %" PRIu32 "\n", error_type_str, buffer, error_filename, error_lineno);
//#ifdef PHP_WIN32
//                  fflush(stderr);
//#endif
//               } else {
//                  php_printf("%s\n%s: %s in %s on line %" PRIu32 "\n%s", STR_PRINT(prepend_string), error_type_str, buffer, error_filename, error_lineno, STR_PRINT(append_string));
//               }
//            }
//         }
//      }
//#if ZEND_DEBUG
//      if (PG(report_zend_debug)) {
//         zend_bool trigger_break;

//         switch (type) {
//         case E_ERROR:
//         case E_CORE_ERROR:
//         case E_COMPILE_ERROR:
//         case E_USER_ERROR:
//            trigger_break=1;
//            break;
//         default:
//            trigger_break=0;
//            break;
//         }
//         zend_output_debug_string(trigger_break, "%s(%" PRIu32 ") : %s - %s", error_filename, error_lineno, error_type_str, buffer);
//      }
//#endif
//   }
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

int php_stream_open_for_zend(const char *filename, zend_file_handle *handle)
{

}

//POLAR_DECL_EXPORT void php_printf_to_smart_string(smart_string *buf, const char *format, va_list ap)
//{

//}

//POLAR_DECL_EXPORT void php_printf_to_smart_str(smart_str *buf, const char *format, va_list ap)
//{

//}

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
