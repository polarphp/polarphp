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

#include "polarphp/runtime/ExecEnv.h"
#include "polarphp/global/CompilerFeature.h"
#include "polarphp/global/Config.h"
#include "polarphp/runtime/RtDefs.h"
#include "polarphp/runtime/LifeCycle.h"
#include "polarphp/runtime/Spprintf.h"
#include "polarphp/runtime/internal/DepsZendVmHeaders.h"
#include "polarphp/runtime/Reentrancy.h"
#include "polarphp/runtime/Output.h"
#include "polarphp/runtime/Utils.h"
#include "polarphp/runtime/Ini.h"

#include <filesystem>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
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

namespace polar {
namespace runtime {

namespace fs = std::filesystem;

extern bool sg_moduleInitialized;
extern bool sg_moduleStartup;
extern bool sg_moduleShutdown;

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

ExecEnvInfo &retrieve_global_execenv_runtime_info()
{
   return retrieve_global_execenv().getRuntimeInfo();
}

ExecEnv::ExecEnv()
   : m_moduleStarted(false),
     m_execEnvStarted(false),
     m_execEnvReady(false),
     m_execEnvDestroyed(false)
{
   /// TODO read from cfg file
   m_runtimeInfo.defaultSocketTimeout = 60;
   m_runtimeInfo.logErrorsMaxLen = 1024;
   m_runtimeInfo.ignoreRepeatedErrors = false;
   m_runtimeInfo.ignoreRepeatedSource = false;
   m_runtimeInfo.displayErrors = PHP_DISPLAY_ERRORS_STDOUT;
   m_runtimeInfo.logErrors = true;
   m_runtimeInfo.trackErrors = true;
   m_runtimeInfo.syslogFacility = LOG_USER;
   m_runtimeInfo.syslogIdent = "polarphp";
   m_runtimeInfo.syslogFilter = PHP_SYSLOG_FILTER_NO_CTRL;
   m_runtimeInfo.memoryLimit = Z_L(1)<<30;
   m_runtimeInfo.docrefRoot = "/phpmanual/";
   m_runtimeInfo.docrefExt = ".html";
   m_runtimeInfo.includePath = ".:/php/includes";
   m_runtimeInfo.reportMemLeaks = true;
   m_runtimeInfo.serializePrecision = -1;
}

ExecEnv::~ExecEnv()
{
   shutdown();
}

bool ExecEnv::bootup()
{
#ifdef HAVE_SIGNAL_H
#if defined(SIGPIPE) && defined(SIG_IGN)
   signal(SIGPIPE, SIG_IGN);
   /// ignore SIGPIPE in standalone mode so
   /// that sockets created via fsockopen()
   /// don't kill PHP if the remote site
   /// closes it.  in apache|apxs mode apache
   /// does that for us!  thies@thieso.net
   /// 20000419
#endif
#endif
   tsrm_startup(1, 1, 0, nullptr);
   (void)ts_resource(0);
   ZEND_TSRMLS_CACHE_UPDATE();
   zend_signal_startup();
   if (!polar::runtime::php_module_startup()) {
      // there is no way to see if we must call zend_ini_deactivate()
      // since we cannot check if EG(ini_directives) has been initialised
      // because the executor's constructor does not set initialize it.
      // Apart from that there seems no need for zend_ini_deactivate() yet.
      // So we goto out_err.
      return false;
   }
   m_moduleStarted = true;
   polar_try {
      CG(in_compilation) = 0; /* not initialized but needed for several options */
      if (!php_exec_env_startup()) {
         std::cerr << "Could not startup." << std::endl;
         return false;
      }
   } polar_end_try;
   m_execEnvStarted = true;
   m_runtimeInfo.duringExecEnvStartup = false;
   return true;
}

void ExecEnv::shutdown()
{
   if (m_execEnvDestroyed) {
      return;
   }
   if (m_execEnvStarted) {
      deactivate();
      zend_ini_deactivate();
      php_exec_env_shutdown();
      m_execEnvStarted = false;
   }
   if (m_moduleStarted) {
      php_module_shutdown();
      m_moduleStarted = false;
   }
   tsrm_shutdown();
}

/// default log handler
///
void ExecEnv::logMessage(const char *logMessage, int syslogTypeInt)
{

}

void ExecEnv::initDefaultConfig(HashTable *configuration_hash)
{
   zval tmp;
   POLAR_INI_DEFAULT("report_zend_debug", "1");
   POLAR_INI_DEFAULT("display_errors", "1");
}

void ExecEnv::activate()
{
   m_execEnvReady = true;
}

void ExecEnv::deactivate()
{
   m_execEnvReady = false;
}

ExecEnv &ExecEnv::setCompileOptions(int opts)
{
   CG(compiler_options) = opts;
   return *this;
}

ExecEnv &ExecEnv::setContainerArgc(int argc)
{
   m_argc = argc;
   return *this;
}

ExecEnv &ExecEnv::setContainerArgv(const std::vector<StringRef> &argv)
{
   m_argv = argv;
   return *this;
}

ExecEnv &ExecEnv::setContainerArgv(char *argv[])
{
   std::vector<StringRef> tempArgv;
   char **arg = argv;
   while (*arg != nullptr) {
      tempArgv.push_back(*arg);
      ++arg;
   }
   if (!tempArgv.empty()) {
      m_argv = tempArgv;
   }
   return *this;
}

ExecEnv &ExecEnv::setEnvReady(bool flag)
{
   m_execEnvReady = flag;
   return *this;
}

bool ExecEnv::isEnvReady() const
{
   return m_execEnvReady;
}

const std::vector<StringRef> &ExecEnv::getContainerArgv() const
{
   return m_argv;
}

int ExecEnv::getContainerArgc() const
{
   return m_argc;
}

uint32_t ExecEnv::getCompileOptions() const
{
   return CG(compiler_options);
}

ExecEnvInfo &ExecEnv::getRuntimeInfo()
{
   return m_runtimeInfo;
}

StringRef ExecEnv::getExecutableFilepath() const
{
   assert(m_argv.size() > 0);
   return m_argv[0];
}

int ExecEnv::getVmExitStatus() const
{
   return EG(exit_status);
}

bool ExecEnv::execScript(StringRef filename, int &exitStatus)
{
   bool useStdin = false;
   if (!filename.empty()) {
      if (!fs::exists(filename.getStr())) {
         std::cerr << "script: " << filename.getData() << " is not exist" << std::endl;
         exitStatus = 1;
         return false;
      }
   } else {
      filename = PHP_STDIN_FILENAME_MARK;
      useStdin = true;
   }
   zend_file_handle fileHandle;
   StringRef translatedPath;
   int lineno = 0;
   polar_try {
      CG(in_compilation) = 0; /* not initialized but needed for several options */
      if (!useStdin) {
         if (!seek_file_begin(&fileHandle, filename.getData(), &lineno)) {
            std::cerr << "seek_file_begin error: " << strerror(errno) << std::endl;
            exitStatus = 1;
            return false;
         } else {
            char realPath[MAXPATHLEN];
            if (VCWD_REALPATH(filename.getData(), realPath)) {
               translatedPath = realPath;
            }
         }
      } else {
         /// We could handle PHP_MODE_PROCESS_STDIN in a different manner
         /// here but this would make things only more complicated. And it
         /// is consitent with the way -R works where the stdin file handle
         /// is also accessible.
         fileHandle.filename = PHP_STDIN_FILENAME_MARK;
         fileHandle.handle.fp = stdin;
      }
      fileHandle.type = ZEND_HANDLE_FP;
      fileHandle.opened_path = nullptr;
      fileHandle.free_filename = 0;
      if (!translatedPath.empty()) {
         m_runtimeInfo.entryScriptFilename = translatedPath;
      } else {
         m_runtimeInfo.entryScriptFilename = fileHandle.filename;
      }
      CG(start_lineno) = lineno;
      if (filename == "Standard input code") {
         cli_register_file_handles();
      }
      php_execute_script(&fileHandle);
      exitStatus = EG(exit_status);
   } polar_end_try;
   return true;
}

/// polarphp don't use timeout at vm level
///
void php_on_timeout(int seconds)
{

}


///
/// TODO support prepend and append file
///
int php_execute_script(zend_file_handle *primaryFile)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   zend_file_handle *prependFilePointer = nullptr;
   zend_file_handle *appendFilePointer = nullptr;
   zend_file_handle prependFile = {{0}, nullptr, nullptr, zend_stream_type(0), zend_stream_type(0)};
   zend_file_handle appendFile = {{0}, nullptr, nullptr, zend_stream_type(0), zend_stream_type(0)};
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
         if (expand_filepath(primaryFile->filename, realfile)) {
            primaryFile->opened_path = zend_string_init(realfile, strlen(realfile), 0);
            zend_hash_add_empty_element(&EG(included_files), primaryFile->opened_path);
         }
      }
      std::string &autoPrependFile = execEnvInfo.autoAppendFile;
      if (!autoPrependFile.empty()) {
         prependFile.filename = autoPrependFile.c_str();
         prependFile.opened_path = nullptr;
         prependFile.free_filename = 0;
         prependFile.type = ZEND_HANDLE_FILENAME;
         prependFilePointer = &prependFile;
      } else {
         prependFilePointer = nullptr;
      }
      std::string &autoAppendFile = execEnvInfo.autoAppendFile;
      if (!autoAppendFile.empty()) {
         appendFile.filename = autoAppendFile.c_str();
         appendFile.opened_path = nullptr;
         appendFile.free_filename = 0;
         appendFile.type = ZEND_HANDLE_FILENAME;
         appendFilePointer = &appendFile;
      } else {
         appendFilePointer = nullptr;
      }

      /*
         If cli primary file has shabang line and there is a prepend file,
         the `start_lineno` will be used by prepend file but not primary file,
         save it and restore after prepend file been executed.
       */
      if (CG(start_lineno) && prependFilePointer) {
         int orig_start_lineno = CG(start_lineno);

         CG(start_lineno) = 0;
         if (zend_execute_scripts(ZEND_REQUIRE, nullptr, 1, prependFilePointer) == SUCCESS) {
            CG(start_lineno) = orig_start_lineno;
            retval = (zend_execute_scripts(ZEND_REQUIRE, nullptr, 2, primaryFile, appendFilePointer) == SUCCESS);
         }
      } else {
         retval = (zend_execute_scripts(ZEND_REQUIRE, nullptr, 3, prependFilePointer, primaryFile, appendFilePointer) == SUCCESS);
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
   fd_set wfd, dfd, efd;
   struct timeval tv;
   int ret;
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   FD_ZERO(&wfd);
   FD_ZERO(&dfd);
   FD_ZERO(&efd);
   PHP_SAFE_FD_SET(fd, &wfd);
   tv.tv_sec = (long)execEnvInfo.defaultSocketTimeout;
   tv.tv_usec = 0;
   ret = php_select(fd+1, &dfd, &wfd, &efd, &tv);
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

///
/// TODO review memory leak
///
ZEND_COLD void php_verror(const char *docref, const char *params,
                          int type, const char *format,
                          va_list args)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   zend_string *replace_buffer = nullptr;
   zend_string *replace_origin = nullptr;
   char *buffer = nullptr;
   char *docref_buf = nullptr;
   char *target = nullptr;
   char *docref_target = PHP_EMPTY_STR;
   char *docref_root = PHP_EMPTY_STR;
   char *p;
   int buffer_len = 0;
   const char *space = PHP_EMPTY_STR;
   const char *class_name = PHP_EMPTY_STR;
   const char *function;
   char *origin;
   char *message;
   int is_function = 0;

   /* get error text into buffer and escape for html if necessary */
   buffer_len = (int)polar_vspprintf(&buffer, 0, format, args);

   /* which function caused the problem if any at all */
   if (php_during_module_startup()) {
      function = const_cast<char *>("polarphp startup");
   } else if (php_during_module_shutdown()) {
      function = const_cast<char *>("polarphp shutdown");
   } else if (EG(current_execute_data) &&
              EG(current_execute_data)->func &&
              ZEND_USER_CODE(EG(current_execute_data)->func->common.type) &&
              EG(current_execute_data)->opline &&
              EG(current_execute_data)->opline->opcode == ZEND_INCLUDE_OR_EVAL
              ) {
      switch (EG(current_execute_data)->opline->extended_value) {
      case ZEND_EVAL:
         function = const_cast<char *>("eval");
         is_function = 1;
         break;
      case ZEND_INCLUDE:
         function = const_cast<char *>("include");
         is_function = 1;
         break;
      case ZEND_INCLUDE_ONCE:
         function = const_cast<char *>("include_once");
         is_function = 1;
         break;
      case ZEND_REQUIRE:
         function = const_cast<char *>("require");
         is_function = 1;
         break;
      case ZEND_REQUIRE_ONCE:
         function = const_cast<char *>("require_once");
         is_function = 1;
         break;
      default:
         function = const_cast<char *>("Unknown");
      }
   } else {
      function = get_active_function_name();
      if (!function || !strlen(function)) {
         function = const_cast<char *>("Unknown");
      } else {
         is_function = 1;
         class_name = get_active_class_name(&space);
      }
   }

   /* if we still have memory then format the origin */
   if (is_function) {
      polar_spprintf(&origin, 0, "%s%s%s(%s)", class_name, space, function, params);
   } else {
      polar_spprintf(&origin, 0, "%s", function);
   }

   /* origin and buffer available, so lets come up with the error message */
   if (docref && docref[0] == '#') {
      docref_target = const_cast<char *>(strchr(docref, '#'));
      docref = nullptr;
   }

   /* no docref given but function is known (the default) */
   if (!docref && is_function) {
      int doclen;
      while (*function == '_') {
         function++;
      }
      if (space[0] == '\0') {
         doclen = (int)polar_spprintf(&docref_buf, 0, "function.%s", function);
      } else {
         doclen = (int)polar_spprintf(&docref_buf, 0, "%s.%s", class_name, function);
      }
      while((p = strchr(docref_buf, '_')) != nullptr) {
         *p = '-';
      }
      docref = php_strtolower(docref_buf, doclen);
   }

   /* we have a docref for a function AND
       * - we show errors in html mode AND
       * - the user wants to see the links
       */
   std::string &docrefRoot = execEnvInfo.docrefRoot;
   if (docref && is_function  && !docrefRoot.empty()) {
      if (strncmp(docref, "http://", 7)) {
         /* We don't have 'http://' so we use docref_root */

         char *ref;  /* temp copy for duplicated docref */

         ref = estrdup(docref);
         if (docref_buf) {
            efree(docref_buf);
         }
         docref_buf = ref;
         /* strip of the target if any */
         p = strrchr(ref, '#');
         if (p) {
            target = estrdup(p);
            if (target) {
               docref_target = target;
               *p = '\0';
            }
         }
         std::string &docrefExt = execEnvInfo.docrefExt;
         /* add the extension if it is set in ini */
         if (!docrefExt.empty()) {
            polar_spprintf(&docref_buf, 0, "%s%s", ref, docrefExt.c_str());
            efree(ref);
         }
         docref = docref_buf;
      }
      polar_spprintf(&message, 0, "%s [%s%s%s]: %s", origin, docref_root, docref, docref_target, buffer);
      if (target) {
         efree(target);
      }
   } else {
      polar_spprintf(&message, 0, "%s: %s", origin, buffer);
   }
   if (replace_origin) {
      zend_string_free(replace_origin);
   } else {
      efree(origin);
   }
   if (docref_buf) {
      efree(docref_buf);
   }

   if (execEnvInfo.trackErrors && sg_moduleInitialized && EG(active) &&
       (Z_TYPE(EG(user_error_handler)) == IS_UNDEF || !(EG(user_error_handler_error_reporting) & type))) {
      zval tmp;
      ZVAL_STRINGL(&tmp, buffer, buffer_len);
      if (EG(current_execute_data)) {
         if (zend_set_local_var_str("php_errormsg", sizeof("php_errormsg")-1, &tmp, 0) == FAILURE) {
            zval_ptr_dtor(&tmp);
         }
      } else {
         zend_hash_str_update_ind(&EG(symbol_table), "php_errormsg", sizeof("php_errormsg")-1, &tmp);
      }
   }
   if (replace_buffer) {
      zend_string_free(replace_buffer);
   } else {
      if (buffer_len > 0) {
         efree(buffer);
      }
   }

   php_error(type, "%s", message);
   efree(message);
}

ZEND_COLD void php_error_callback(int type, const char *errorFilename,
                                  const uint32_t errorLineno, const char *format,
                                  va_list args)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   /// need protected for memory leak
   /// TODO find elegance way
   ///
   char *bufferRaw = nullptr;
   bool display;
   int bufferLen = static_cast<int>(zend_vspprintf(&bufferRaw, execEnvInfo.logErrorsMaxLen, format, args));
   std::shared_ptr<char> buffer;
   if (bufferRaw) {
      buffer.reset(bufferRaw, [](void *ptr){
         efree(ptr);
      });
   }
   /* check for repeated errors to be ignored */
   if (execEnvInfo.ignoreRepeatedErrors && !execEnvInfo.lastErrorMessage.empty()) {
      /* no check for execEnv.getLastErrorFile() is needed since it cannot
          * be empty if execEnv.getLastErrorMessage() is not empty */
      std::string &lastErrorMsg = execEnvInfo.lastErrorMessage;
      if (lastErrorMsg != buffer.get()
          || (!execEnvInfo.ignoreRepeatedSource
              && ((execEnvInfo.lastErrorLineno != (int)errorLineno)
                  || execEnvInfo.lastErrorFile != errorFilename))) {
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
      execEnvInfo.lastErrorType = type;
      execEnvInfo.lastErrorMessage = std::string(bufferRaw, bufferLen);
      // errorFilename must be c string ?
      execEnvInfo.lastErrorFile = errorFilename;
      execEnvInfo.lastErrorLineno = errorLineno;
   }

   /* display/log the error if necessary */
   if (display && (EG(error_reporting) & type || (type & E_CORE))
       && (execEnvInfo.logErrors || execEnvInfo.displayErrors || (!sg_moduleInitialized))) {
      char *error_type_str;
      int syslogTypeInt = LOG_NOTICE;

      switch (type) {
      case E_ERROR:
      case E_CORE_ERROR:
      case E_COMPILE_ERROR:
      case E_USER_ERROR:
         error_type_str = const_cast<char *>("Fatal error");
         syslogTypeInt = LOG_ERR;
         break;
      case E_RECOVERABLE_ERROR:
         error_type_str = const_cast<char *>("Recoverable fatal error");
         syslogTypeInt = LOG_ERR;
         break;
      case E_WARNING:
      case E_CORE_WARNING:
      case E_COMPILE_WARNING:
      case E_USER_WARNING:
         error_type_str = const_cast<char *>("Warning");
         syslogTypeInt = LOG_WARNING;
         break;
      case E_PARSE:
         error_type_str = const_cast<char *>("Parse error");
         syslogTypeInt = LOG_EMERG;
         break;
      case E_NOTICE:
      case E_USER_NOTICE:
         error_type_str = const_cast<char *>("Notice");
         syslogTypeInt = LOG_NOTICE;
         break;
      case E_STRICT:
         error_type_str = const_cast<char *>("Strict Standards");
         syslogTypeInt = LOG_INFO;
         break;
      case E_DEPRECATED:
      case E_USER_DEPRECATED:
         error_type_str = const_cast<char *>("Deprecated");
         syslogTypeInt = LOG_INFO;
         break;
      default:
         error_type_str = const_cast<char *>("Unknown error");
         break;
      }

      if (!sg_moduleInitialized || execEnvInfo.logErrors) {
         /// TODO maybe memory leak
         char *log_buffer;
#ifdef POLAR_OS_WIN32
         if (type == E_CORE_ERROR || type == E_CORE_WARNING) {
            syslog(LOG_ALERT, "PHP %s: %s (%s)", error_type_str, buffer, GetCommandLine());
         }
#endif
         polar_spprintf(&log_buffer, 0, "polarphp %s:  %s in %s on line %" PRIu32, error_type_str, buffer.get(), errorFilename, errorLineno);
         php_log_err_with_severity(log_buffer, syslogTypeInt);
         efree(log_buffer);
      }

      if (execEnvInfo.displayErrors && ((sg_moduleInitialized && !execEnvInfo.duringExecEnvStartup) || execEnvInfo.displayStartupErrors)) {
         char *prepend_string = INI_STR(const_cast<char *>("error_prepend_string"));
         char *append_string = INI_STR(const_cast<char *>("error_append_string"));
         /* Write CLI/CGI errors to stderr if display_errors = "stderr" */
         if (execEnvInfo.displayErrors == PHP_DISPLAY_ERRORS_STDERR) {
            fprintf(stderr, "%s: %s in %s on line %" PRIu32 "\n", error_type_str, buffer.get(), errorFilename, errorLineno);
#ifdef POLAR_OS_WIN32
            fflush(stderr);
#endif
         } else {
            php_printf("%s\n%s: %s in %s on line %" PRIu32 "\n%s", PHP_STR_PRINT(prepend_string), error_type_str, buffer.get(), errorFilename, errorLineno, PHP_STR_PRINT(append_string));
         }
      }
#if ZEND_DEBUG
      if (execEnvInfo.reportZendDebug) {
         zend_bool trigger_break;

         switch (type) {
         case E_ERROR:
         case E_CORE_ERROR:
         case E_COMPILE_ERROR:
         case E_USER_ERROR:
            trigger_break=1;
            break;
         default:
            trigger_break=0;
            break;
         }
         zend_output_debug_string(trigger_break, "%s(%" PRIu32 ") : %s - %s %s\n", errorFilename, errorLineno, error_type_str, buffer.get(), errorFilename);
      }
#endif
   }

   /* Bail out if we can't recover */
   switch (type) {
   case E_CORE_ERROR:
      if(!sg_moduleInitialized) {
         /* bad error in module startup - no way we can live with this */
         exit(-2);
      }
      POLAR_FALLTHROUGH;
      /* no break - intentionally */
   case E_ERROR:
   case E_RECOVERABLE_ERROR:
   case E_PARSE:
   case E_COMPILE_ERROR:
   case E_USER_ERROR:
      EG(exit_status) = 255;
      if (sg_moduleInitialized) {
         /* the parser would return 1 (failure), we can bail out nicely */
         if (type != E_PARSE) {
            /* restore memory limit */
            zend_set_memory_limit(execEnvInfo.memoryLimit);
            zend_objects_store_mark_destructed(&EG(objects_store));
            zend_bailout();
            return;
         }
      }
      break;
   }

   /* Log if necessary */
   if (!display) {
      return;
   }

   if (execEnvInfo.trackErrors && sg_moduleInitialized && EG(active)) {
      zval tmp;

      ZVAL_STRINGL(&tmp, buffer.get(), bufferLen);
      if (EG(current_execute_data)) {
         if (zend_set_local_var_str("php_errormsg", sizeof("php_errormsg")-1, &tmp, 0) == FAILURE) {
            zval_ptr_dtor(&tmp);
         }
      } else {
         zend_hash_str_update_ind(&EG(symbol_table), "php_errormsg", sizeof("php_errormsg")-1, &tmp);
      }
   }
}

///
/// TODO maybe memory leak in this function
///
ZEND_COLD void php_log_err_with_severity(char *logMessage, int syslogTypeInt)
{
   ExecEnv &execEnv = retrieve_global_execenv();
   ExecEnvInfo &execEnvInfo = execEnv.getRuntimeInfo();
   int fd = -1;
   time_t error_time;

   if (execEnvInfo.inErrorLog) {
      /* prevent recursive invocation */
      return;
   }
   execEnvInfo.inErrorLog = true;
   std::string &errorLog = execEnvInfo.errorLog;
   /* Try to use the specified logging location. */
   if (!errorLog.empty()) {
#ifdef HAVE_SYSLOG_H
      if (errorLog == "syslog") {
         php_syslog(syslogTypeInt, "%s", logMessage);
         execEnvInfo.inErrorLog = false;
         return;
      }
#endif
      fd = VCWD_OPEN_MODE(errorLog.c_str(), O_CREAT | O_APPEND | O_WRONLY, 0644);
      if (fd != -1) {
         char *tmp;
         size_t len;
         /// TODO which size enough here
         char errorTimeBuffer[128];
         time(&error_time);
         if (!php_during_module_startup()) {
            php_format_date(errorTimeBuffer, 128, "d-M-Y H:i:s e", error_time, true);
         } else {
            php_format_date(errorTimeBuffer, 128, "d-M-Y H:i:s e", error_time, false);
         }
         len = polar_spprintf(&tmp, 0, "[%s] %s%s", errorTimeBuffer, logMessage, PHP_EOL);
#ifdef POLAR_OS_WIN32
         php_flock(fd, 2);
         /* XXX should eventually write in a loop if len > UINT_MAX */
         php_ignore_value(write(fd, tmp, (unsigned)len));
#else
         php_ignore_value(write(fd, tmp, len));
#endif
         efree(tmp);
         close(fd);
         execEnvInfo.inErrorLog = false;
         return;
      }
   }

   /* Otherwise fall back to the default logging location, if we have one */
   /// maybe here we need user hook
   /// TODO
   execEnv.logMessage(logMessage, syslogTypeInt);
   execEnvInfo.inErrorLog = false;
}

size_t php_write(void *buf, size_t size)
{
   return PHPWRITE(reinterpret_cast<char *>(buf), size);
}

///
/// review check memory leak
///
size_t php_printf(const char *format, ...)
{
   va_list args;
   size_t ret;
   char *buffer;
   size_t size;

   va_start(args, format);
   size = polar_vspprintf(&buffer, 0, format, args);
   ret = PHPWRITE(buffer, size);
   efree(buffer);
   va_end(args);

   return ret;
}

size_t php_output_wrapper(const char *str, size_t strLength)
{
   return php_output_write(str, strLength);
}

zval *php_get_configuration_directive_for_zend(zend_string *name)
{
   return cfg_get_entry_ex(name);
}

POLAR_DECL_EXPORT void php_message_handler_for_zend(zend_long message, const void *data)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   switch (message) {
   case ZMSG_FAILED_INCLUDE_FOPEN:
      php_error_docref("function.include", E_WARNING, "Failed opening '%s' for inclusion (include_path='%s')", php_strip_url_passwd(reinterpret_cast<char *>(const_cast<void *>(data))), PHP_STR_PRINT(execEnvInfo.includePath.c_str()));
      break;
   case ZMSG_FAILED_REQUIRE_FOPEN:
      php_error_docref("function.require", E_COMPILE_ERROR, "Failed opening required '%s' (include_path='%s')", php_strip_url_passwd(reinterpret_cast<char *>(const_cast<void *>(data))), PHP_STR_PRINT(execEnvInfo.includePath.c_str()));
      break;
   case ZMSG_FAILED_HIGHLIGHT_FOPEN:
      php_error_docref(NULL, E_WARNING, "Failed opening '%s' for highlighting", php_strip_url_passwd(reinterpret_cast<char *>(const_cast<void *>(data))));
      break;
   case ZMSG_MEMORY_LEAK_DETECTED:
   case ZMSG_MEMORY_LEAK_REPEATED:
#if ZEND_DEBUG
      if (EG(error_reporting) & E_WARNING) {
         char memoryLeakBuf[1024];
         if (message==ZMSG_MEMORY_LEAK_DETECTED) {
            zend_leak_info *t = reinterpret_cast<zend_leak_info *>(const_cast<void *>(data));
            if (!execEnvInfo.entryScriptFilename.empty()) {
               std::snprintf(memoryLeakBuf, 512, "%s(%" PRIu32 ") :  Freeing " ZEND_ADDR_FMT " (%zu bytes), script=%s\n", t->filename, t->lineno, (size_t)t->addr, t->size, SAFE_FILENAME(execEnvInfo.entryScriptFilename.c_str()));
               if (t->orig_filename) {
                  char relayBuf[512];
                  std::snprintf(relayBuf, 512, "%s(%" PRIu32 ") : Actual location (location was relayed)\n", t->orig_filename, t->orig_lineno);
                  strlcat(memoryLeakBuf, relayBuf, sizeof(memoryLeakBuf));
               }
            } else {
               std::snprintf(memoryLeakBuf, 512, "Freeing " ZEND_ADDR_FMT " (%zu bytes), script=%s\n", (size_t)t->addr, t->size, SAFE_FILENAME(execEnvInfo.entryScriptFilename.c_str()));
            }
         } else {
            unsigned long leak_count = (zend_uintptr_t) data;
            std::snprintf(memoryLeakBuf, 512, "Last leak repeated %lu time%s\n", leak_count, (leak_count>1?"s":""));
         }
#	if defined(POLAR_OS_WIN32)
         OutputDebugString(memoryLeakBuf);
#	else
         std::fprintf(stderr, "%s", memoryLeakBuf);
#	endif
      }
#endif
      break;
   case ZMSG_MEMORY_LEAKS_GRAND_TOTAL:
#if ZEND_DEBUG
      if (EG(error_reporting) & E_WARNING) {
         char memoryLeakBuf[512];
         std::snprintf(memoryLeakBuf, 512, "=== Total %d memory leaks detected ===\n", *(reinterpret_cast<uint32_t *>(const_cast<void *>(data))));
#	if defined(POLAR_OS_WIN32)
         OutputDebugString(memoryLeakBuf);
#	else
         std::fprintf(stderr, "%s", memoryLeakBuf);
#	endif
      }
#endif
      break;
   case ZMSG_LOG_SCRIPT_NAME: {
      struct tm *ta, tmbuf;
      time_t curtime;
      char *datetime_str, asctimebuf[52];
      char memoryLeakBuf[4096];
      time(&curtime);
      ta = php_localtime_r(&curtime, &tmbuf);
      datetime_str = polar_asctime_r(ta, asctimebuf);
      if (datetime_str) {
         datetime_str[strlen(datetime_str)-1]=0;	/* get rid of the trailing newline */
         std::snprintf(memoryLeakBuf, sizeof(memoryLeakBuf), "[%s]  Script:  '%s'\n", datetime_str, SAFE_FILENAME(execEnvInfo.entryScriptFilename.c_str()));
      } else {
         std::snprintf(memoryLeakBuf, sizeof(memoryLeakBuf), "[null]  Script:  '%s'\n", SAFE_FILENAME(execEnvInfo.entryScriptFilename.c_str()));
      }
#	if defined(POLAR_OS_WIN32)
      OutputDebugString(memoryLeakBuf);
#	else
      std::fprintf(stderr, "%s", memoryLeakBuf);
#	endif
   }
      break;
   }
}

///
/// here we does not use it, but need review
///
POLAR_DECL_EXPORT char *bootstrap_getenv(char *, size_t)
{
   return nullptr;
}

///
/// need review for memory leak, we remove stream, so we need check the result
///
zend_string *php_resolve_path(const char *filename, size_t filenameLen, const char *path)
{
   char resolvedPath[MAXPATHLEN];
   char trypath[MAXPATHLEN];
   const char *ptr;
   const char *end;
   const char *p;
   const char *actualPath;
   zend_string *execFilename;
   if (!filename || CHECK_NULL_PATH(filename, filenameLen)) {
      return nullptr;
   }
   /// polarphp does not handle stream protocol
   for (p = filename; isalnum((int)*p) || *p == '+' || *p == '-' || *p == '.'; p++);
   if ((*filename == '.' &&
        (IS_SLASH(filename[1]) ||
         ((filename[1] == '.') && IS_SLASH(filename[2])))) ||
       IS_ABSOLUTE_PATH(filename, filenameLen) ||
    #ifdef POLAR_OS_WIN32
       /* This should count as an absolute local path as well, however
                                                                                                                                                                                                                        IS_ABSOLUTE_PATH doesn't care about this path form till now. It
                                                                                                                                                                                                                        might be a big thing to extend, thus just a local handling for
                                                                                                                                                                                                                        now. */
       filenameLen >=2 && IS_SLASH(filename[0]) && !IS_SLASH(filename[1]) ||
    #endif
       !path ||
       !*path) {
      if (tsrm_realpath(filename, resolvedPath)) {
         return zend_string_init(resolvedPath, strlen(resolvedPath), 0);
      } else {
         return nullptr;
      }
   }
   ptr = path;
   while (ptr && *ptr) {
      /// polarphp does not use stream
      for (p = ptr; isalnum((int)*p) || *p == '+' || *p == '-' || *p == '.'; p++);
      end = strchr(p, DEFAULT_DIR_SEPARATOR);
      if (end) {
         if (filenameLen > (MAXPATHLEN - 2) || (end-ptr) > MAXPATHLEN || (end-ptr) + 1 + filenameLen + 1 >= MAXPATHLEN) {
            ptr = end + 1;
            continue;
         }
         memcpy(trypath, ptr, end-ptr);
         trypath[end-ptr] = '/';
         memcpy(trypath+(end-ptr)+1, filename, filenameLen+1);
         ptr = end+1;
      } else {
         size_t len = strlen(ptr);
         if (filenameLen > (MAXPATHLEN - 2) || len > MAXPATHLEN || len + 1 + filenameLen + 1 >= MAXPATHLEN) {
            break;
         }
         memcpy(trypath, ptr, len);
         trypath[len] = '/';
         memcpy(trypath+len+1, filename, filenameLen+1);
         ptr = nullptr;
      }
      actualPath = trypath;
      if (tsrm_realpath(actualPath, resolvedPath)) {
         return zend_string_init(resolvedPath, strlen(resolvedPath), 0);
      }
   } /* end provided path */

   /* check in calling scripts' current working directory as a fall back case
       */
   if (zend_is_executing() &&
       (execFilename = zend_get_executed_filename_ex()) != nullptr) {
      const char *exec_fname = ZSTR_VAL(execFilename);
      size_t execFnameLength = ZSTR_LEN(execFilename);
      while ((--execFnameLength < SIZE_MAX) && !IS_SLASH(exec_fname[execFnameLength]));
      if (execFnameLength > 0 &&
          filenameLen < (MAXPATHLEN - 2) &&
          execFnameLength + 1 + filenameLen + 1 < MAXPATHLEN) {
         memcpy(trypath, exec_fname, execFnameLength + 1);
         memcpy(trypath+execFnameLength + 1, filename, filenameLen+1);
         actualPath = trypath;
         /* Check for stream wrapper */
         for (p = trypath; isalnum((int)*p) || *p == '+' || *p == '-' || *p == '.'; p++);
         if (tsrm_realpath(actualPath, resolvedPath)) {
            return zend_string_init(resolvedPath, strlen(resolvedPath), 0);
         }
      }
   }
   return nullptr;
}

zend_string *php_resolve_path_for_zend(const char *filename, size_t filenameLen)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   return php_resolve_path(filename, filenameLen, execEnvInfo.includePath.c_str());
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

///
/// we mybe not use this function
///
void cli_register_file_handles()
{}

size_t ExecEnv::unbufferWrite(const char *str, int len)
{
   const char *ptr = str;
   size_t remaining = len;
   ssize_t ret;

   if (!len) {
      return 0;
   }

   if (sg_cliShellCallbacks.cliShellUnbufferWrite) {
      size_t ubWrote;
      ubWrote = sg_cliShellCallbacks.cliShellUnbufferWrite(str, len);
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

} // runtime
} // polar
