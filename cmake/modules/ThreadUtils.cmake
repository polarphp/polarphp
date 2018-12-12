# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

# Copyright (c) 1999, 2000 Sascha Schumann. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# THIS SOFTWARE IS PROVIDED BY SASCHA SCHUMANN ``AS IS'' AND ANY
# EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL SASCHA SCHUMANN OR
# HIS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

include(CheckCSourceCompiles)

macro(polar_setup_pthreads_flags)
   set(_pthreadsFlags "")
   if(POLAR_SYSTEM_NORMAL_NAME MATCHES ".*solaris.*")
      set(_pthreadsFlags _POSIX_PTHREAD_SEMANTICS -D_REENTRANT)
   elseif(POLAR_SYSTEM_NORMAL_NAME MATCHES ".*freebsd.*")
      set(_pthreadsFlags _REENTRANT -D_THREAD_SAFE)
   elseif(POLAR_SYSTEM_NORMAL_NAME MATCHES ".*linux.*")
      set(_pthreadsFlags _REENTRANT)
   elseif(POLAR_SYSTEM_NORMAL_NAME MATCHES ".*aix.*")
      set(_pthreadsFlags _THREAD_SAFE)
   elseif(POLAR_SYSTEM_NORMAL_NAME MATCHES ".*irix.*")
      set(_pthreadsFlags _POSIX_THREAD_SAFE_FUNCTIONS)
   elseif(POLAR_SYSTEM_NORMAL_NAME MATCHES ".*hpux.*")
      set(_pthreadsFlags _REENTRANT)
   elseif(POLAR_SYSTEM_NORMAL_NAME MATCHES ".*sco.*")
      set(_pthreadsFlags _REENTRANT)
   endif()
   # Solves sigwait() problem, creates problems with u_long etc.
   # _pthreadsFlags="-D_REENTRANT -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=199506 -D_XOPEN_SOURCE_EXTENDED=1";;
   if (_pthreadsFlags)
      set(${_pthreadsFlags} ON)
   endif()
endmacro()

macro(polar_pthreads_check_compiler result)
   check_c_source_compiles("
      #include <pthread.h>
      #include <stddef.h>

      void *thread_routine(void *data) {
          return data;
      }

      int main() {
          pthread_t thd;
          pthread_mutexattr_t mattr;
          int data = 1;
          pthread_mutexattr_init(&mattr);
          return pthread_create(&thd, NULL, thread_routine, &data);
      }" ${result})
endmacro()

macro(polar_pthreads_check)
   set(pthreadWorking OFF)
   set(pthreadWorkingFlag "")
   set(pthreadWorkingLib "")
   if (POLAR_BEOS_THREADS)
      set(POLAR_THREADS_WORKING ON)
   endif()
   if (NOT pthreadWorking)
      foreach(flag -kthread -pthread -pthreads -mthreads -Kthread -threads -mt -qthreaded)
         set(CMAKE_REQUIRED_FLAGS ${flag})
         polar_pthreads_check_compiler(checkingPthreadFor${flag})
         if (checkingPthreadFor${flag})
            set(pthreadWorkingFlag ${flag})
            set(pthreadWorking ON)
            break()
         endif()
      endforeach()
   endif()
   set(CMAKE_REQUIRED_FLAGS "")
   # flag is not working, try libpthread
   if (NOT pthreadWorking)
      foreach(lib pthread pthreads c_r)
         set(CMAKE_REQUIRED_LIBRARIES -l${lib})
         polar_pthreads_check_compiler(checkingPthreadFor${lib})
         if (checkingPthreadFor${lib})
            set(pthreadWorkingLib -l${lib})
            break()
         endif()
      endforeach()
   endif()
   if (pthreadWorkingFlag OR pthreadWorkingLib)
      set(POLAR_THREADS_WORKING ON)
      # TODO refactor
#      if (pthreadWorkingFlag)
#         set(POLAR_THREADS_LIBRARY ${pthreadWorkingFlag})
#      else(pthreadWorkingLib)
#         set(POLAR_THREADS_LIBRARY ${pthreadWorkingLib})
#      endif()
      set(POLAR_THREADS_LIBRARY pthread)
   endif()
   if (POLAR_THREADS_WORKING)
      message("POSIX-Threads found")
   else()
      message("POSIX-Threads not found")
      message(FATAL_ERROR "ZTS currently requires working POSIX threads. We were unable to verify that your system supports Pthreads.")
   endif()
endmacro()
