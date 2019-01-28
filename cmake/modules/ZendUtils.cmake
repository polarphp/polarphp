# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

macro(polar_check_libzend_bison)
   # we only support certain bison versions;
   # min: 2.4 (i.e. 204, major * 100 + minor for easier comparison)
   set(bisonVersionMin, 204)
   set(bsionVersionExclude "")
   set(bisonVersion "none")
   string(REPLACE "." ";" _versionParts ${BISON_VERSION})
   list(LENGTH _versionParts _bisonVersionPartsLen)

   list(GET _versionParts 0 _majorVersion)
   list(GET _versionParts 1 _minorVersion)
   if (_bisonVersionPartsLen EQUAL 3)
      list(GET _versionParts 2 _patchVersion)
   else()
      set(_patchVersion 0)
   endif()

   math(EXPR BSION_VERSION_NUM "${_majorVersion} * 100 + ${_minorVersion} * 10 + ${_patchVersion}")
   if (BSION_VERSION_NUM GREATER_EQUAL bisonVersionMin)
      message("${BSION_VERSION_NUM}xxxxxxx")
      set(bisonVersion "${BISON_VERSION} (ok)")
      foreach(excludeVersion ${bsionVersionExclude})
         if (excludeVersion STREQUAL BISON_VERSION)
            set(bisonVersion "invalid")
            break()
         endif()
      endforeach()
   endif()
   if (bisonVersion STREQUAL "invalid")
      message(FATAL "This bison version is not supported for regeneration of the Zend/PHP parsers (found: $bison_version, min: $bison_version_min, excluded: $bison_version_exclude).")
      set(POLAR_PROG_BISON "exit 0;")
      set(POLAR_PROG_YACC "exit 0;")
   endif()
endmacro()

macro(polar_check_zend_fp_except)
   check_c_source_compiles("#include <floatingpoint.h>
      int main(){
         fp_except x = (fp_except) 0;
         return 0;
      }" checkZendFpExcept)
   if (checkZendFpExcept)
      set(HAVE_FP_EXCEPT ON)
   endif()
endmacro()

macro(polar_check_zend_float_precision)
   check_c_source_compiles("#include <fpu_control.h>
      int main(){
         fpu_control_t fpu_oldcw, fpu_cw;
          volatile double result;
          double a = 2877.0;
          volatile double b = 1000000.0;

          _FPU_GETCW(fpu_oldcw);
          fpu_cw = (fpu_oldcw & ~_FPU_EXTENDED & ~_FPU_SINGLE) | _FPU_DOUBLE;
          _FPU_SETCW(fpu_cw);
          result = a / b;
          _FPU_SETCW(fpu_oldcw);
         return 0;
      }" checkFpuSetcw)
   if (checkFpuSetcw)
      set(HAVE__FPU_SETCW ON)
   endif()

   check_c_source_compiles("#include <machine/ieeefp.h>
      int main(){
         fp_prec_t fpu_oldprec;
         volatile double result;
         double a = 2877.0;
         volatile double b = 1000000.0;

         fpu_oldprec = fpgetprec();
         fpsetprec(FP_PD);
         result = a / b;
         fpsetprec(fpu_oldprec);
         return 0;
      }" checkFpsetprec)
   if (checkFpsetprec)
      set(HAVE_FPSETPREC ON)
   endif()

   check_c_source_compiles("#include <float.h>
      int main(){
          unsigned int fpu_oldcw;
          volatile double result;
          double a = 2877.0;
          volatile double b = 1000000.0;

          fpu_oldcw = _controlfp(0, 0);
          _controlfp(_PC_53, _MCW_PC);
          result = a / b;
          _controlfp(fpu_oldcw, _MCW_PC);
         return 0;
      }" checkControlfp)
   if (checkControlfp)
      set(HAVE__CONTROLFP ON)
   endif()

   check_c_source_compiles("
      #include <float.h>
      int main(){
         unsigned int fpu_oldcw, fpu_cw;
          volatile double result;
          double a = 2877.0;
          volatile double b = 1000000.0;

          _controlfp_s(&fpu_cw, 0, 0);
          fpu_oldcw = fpu_cw;
          _controlfp_s(&fpu_cw, _PC_53, _MCW_PC);
          result = a / b;
          _controlfp_s(&fpu_cw, fpu_oldcw, _MCW_PC);
         return 0;
      }" checkControlfpS)
   if (checkControlfpS)
      set(HAVE__CONTROLFP_S ON)
   endif()

   check_c_source_compiles("
       int main(){
         unsigned int oldcw, cw;
          volatile double result;
          double a = 2877.0;
          volatile double b = 1000000.0;
          __asm__ __volatile__ (\"fnstcw %0\" : \"=m\" (*&oldcw));
          cw = (oldcw & ~0x0 & ~0x300) | 0x200;
          __asm__ __volatile__ (\"fldcw %0\" : : \"m\" (*&cw));
          result = a / b;
          __asm__ __volatile__ (\"fldcw %0\" : : \"m\" (*&oldcw));
         return 0;
      }" checkFpuInlineAsmX86)
   if (checkFpuInlineAsmX86)
      set(HAVE_FPU_INLINE_ASM_X86 ON)
   endif()
endmacro()

macro(polar_check_libzend_basic_requires)
   polar_check_libzend_bison()
   # Ugly hack to get around a problem with gcc on AIX.
   check_c_compiler_flag(-g POLAR_GCC_SUPPORT_G)
   execute_process(COMMAND "uname" "-sv"
      RESULT_VARIABLE _retCode
      OUTPUT_VARIABLE _output
      ERROR_VARIABLE _errorMsg)
   if (NOT _retCode EQUAL 0)
      message(FATAL_ERROR "execute uname -sv error: ${_errorMsg}")
   endif()
   if (POLAR_CC_GCC AND POLAR_GCC_SUPPORT_G AND _output STREQUAL "AIX 4") # TODO right ?
      string(REPLACE "-g" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
   endif()

   # Hack to work around a Mac OS X cpp problem
   # Known versions needing this workaround are 5.3 and 5.4
   execute_process(COMMAND "uname" "-s"
      RESULT_VARIABLE _retCode
      OUTPUT_VARIABLE _output
      ERROR_VARIABLE _errorMsg)
   if (NOT _retCode EQUAL 0)
      message(FATAL_ERROR "execute uname -s error: ${_errorMsg}")
   endif()
   if (POLAR_CC_GCC AND _output STREQUAL "Rhapsody")
      polar_append_flag(-traditional-cpp CMAKE_CXX_FLAGS)
   endif()

   polar_check_headers(
      inttypes.h
      stdint.h
      limits.h
      malloc.h
      string.h
      unistd.h
      stdarg.h
      sys/types.h
      sys/time.h
      signal.h
      unix.h
      stdlib.h
      dlfcn.h)
   polar_check_signal()
   # TODO need if not exist we need define it
   polar_check_type(uint HAVE_UINT "")
   polar_check_type(ulong HAVE_ULONG "")

   # Check if int32_t and uint32_t are defined
   polar_check_libzend_int_type(int32_t)
   polar_check_libzend_int_type(uint32_t)

   # Checks for library functions.
   polar_check_func_vprintf()
   polar_check_func_alloca()
   polar_check_func_memcmp()
   polar_check_funcs(memcpy strdup getpid kill strtod strtol finite fpclass sigsetjmp)
   polar_check_broken_sprintf()

   polar_check_symbol_exists(isfinite math.h HAVE_DECL_ISFINITE)
   polar_check_symbol_exists(isnan math.h HAVE_DECL_ISNAN)
   polar_check_symbol_exists(isinf math.h HAVE_DECL_ISINF)

   polar_check_zend_fp_except()
   polar_check_zend_float_precision()

   # test whether double cast to long preserves least significant bits
   check_c_source_runs("
      #include <limits.h>

      int main()
      {
            if (sizeof(long) == 4) {
                  double d = (double) LONG_MIN * LONG_MIN + 2e9;

                  if ((long) d == 2e9 && (long) -d == -2e9) {
                        exit(0);
                  }
            } else if (sizeof(long) == 8) {
                  double correct = 18e18 - ((double) LONG_MIN * -2); /* Subtract ULONG_MAX + 1 */

                  if ((long) 18e18 == correct) { /* On 64-bit, only check between LONG_MAX and ULONG_MAX */
                        exit(0);
                  }
            }
            exit(1);
      }
      " checkDvalToLvalCastOK)
   if (checkDvalToLvalCastOK)
      set(ZEND_DVAL_TO_LVAL_CAST_OK ON)
   endif()
endmacro()

macro(polar_check_libzend_int_type type)
   if (HAVE_SYS_TYPES_H)
      polar_append_flag(-DHAVE_SYS_TYPES_H CMAKE_REQUIRED_DEFINITIONS)
   endif()
   if (HAVE_INTTYPES_H)
      polar_append_flag(-DHAVE_INTTYPES_H CMAKE_REQUIRED_DEFINITIONS)
   endif()
   if (HAVE_STDINT_H)
      polar_append_flag(-DHAVE_STDINT_H CMAKE_REQUIRED_DEFINITIONS)
   endif()
   check_c_source_runs(
      "#if HAVE_SYS_TYPES_H
      #include <sys/types.h>
      #endif
      #if HAVE_INTTYPES_H
      #include <inttypes.h>
      #elif HAVE_STDINT_H
      #include <stdint.h>
      #endif
      int main() {
         if ((${type} *) 1)
            return 0;
         if (sizeof(int))
           return 0;
      }
      " checkLibZendIntType)
   polar_define_have(${type})
   unset(CMAKE_REQUIRED_DEFINITIONS)
endmacro()

#
# Ugly hack to check if dlsym() requires a leading underscore in symbol name.
#
macro(polar_check_libzend_dlsym)
   message("whether dlsym() requires a leading underscore in symbol names")
   try_run(retCode compileResult
      ${CMAKE_CURRENT_BINARY_DIR} ${POLAR_CMAKE_TEST_CODE_DIR}/test_try_dlopen_self_code.c
      COMPILE_DEFINITIONS ${POLAR_COMPILE_DEFINITIONS}
      COMPILE_OUTPUT_VARIABLE compileOutput
      RUN_OUTPUT_VARIABLE runOutput
      LINK_LIBRARIES dl)
   if (NOT compileResult)
      message(FATAL_ERROR "compile test_try_dlopen_self_code.c error: ${compileOutput}")
   endif()
   if (retCode EQUAL 2)
      set(DLSYM_NEEDS_UNDERSCORE ON)
   endif()
endmacro()

macro(polar_check_libzend_cpp)
endmacro()

macro(polar_check_libzend_other_requires)
   set(ZEND_MAINTAINER_ZTS ON)
   if (NOT POLAR_DISABLE_INLINE_OPTIMIZATION)
      set(ZEND_INLINE_OPTIMIZATION ON)
      set(POLAR_INLINE_OPTIMIZATION ON)
   else()
      set(POLAR_INLINE_OPTIMIZATION OFF)
   endif()
   if (POLAR_DISABLE_INLINE_OPTIMIZATION)
      message("enable inline optimization for GCC")
   endif()
   if (POLAR_BUILD_TYPE STREQUAL "debug")
      # setup debug mode compile flags
      set(ZEND_DEBUG ON)
      if (POLAR_CC_GCC)
         polar_append_flag(-Wall CMAKE_C_FLAGS_DEBUG)
      endif()
   else()
      set(ZEND_DEBUG OFF)
   endif()
   polar_check_libzend_cpp()

   if (NOT POLAR_INLINE_OPTIMIZATION)
      string(REGEX REPLACE "-O[0-9s]*" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
      string(REGEX REPLACE "-O[0-9s]*" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS_${POLAR_BUILD_CONFIG}})
   endif()

   polar_check_inline()

   if (POLAR_SYSTEM_NORMAL_NAME STREQUAL "darwin")
      set(DARWIN ON)
   endif()

   # test and set the alignment define for ZEND_MM
   # this also does the logarithmic test for ZEND_MM.
   message("checking zend mm align")
   try_run(retCode compileResult
      ${CMAKE_CURRENT_BINARY_DIR} ${POLAR_CMAKE_TEST_CODE_DIR}/test_zendmm_code.c
      COMPILE_DEFINITIONS -DPOLAR_CONFIGURE_TEMP_DIR="${POLAR_CONFIGURE_TEMP_DIR}"
      COMPILE_OUTPUT_VARIABLE compileOutput
      RUN_OUTPUT_VARIABLE runOutput)
   if (NOT compileResult)
      message(FATAL_ERROR "${compileOutput}")
   endif()
   if (retCode EQUAL 0)
      file(READ ${POLAR_CONFIGURE_TEMP_DIR}/conftest.zend tempFileContent)
      string(REPLACE " " ";" tempFileContent ${tempFileContent})
      list(GET tempFileContent 0 ZEND_MM_ALIGNMENT)
      list(GET tempFileContent 1 ZEND_MM_ALIGNMENT_LOG2)
   else()
      set(ZEND_MM_ALIGNMENT 8)
   endif()

   # test for memory allocation using mmap(MAP_ANON)
   check_c_source_runs(
      "#include <sys/types.h>
      #include <sys/stat.h>
      #include <fcntl.h>
      #include <sys/mman.h>
      #include <stdlib.h>
      #include <stdio.h>
      #ifndef MAP_ANON
      # ifdef MAP_ANONYMOUS
      #  define MAP_ANON MAP_ANONYMOUS
      # endif
      #endif
      #ifndef MREMAP_MAYMOVE
      # define MREMAP_MAYMOVE 0
      #endif
      #ifndef MAP_FAILED
      # define MAP_FAILED ((void*)-1)
      #endif

      #define SEG_SIZE (256*1024)

      int main()
      {
            void *seg = mmap(NULL, SEG_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
            if (seg == MAP_FAILED) {
                  return 1;
            }
            if (munmap(seg, SEG_SIZE) != 0) {
                  return 2;
            }
            return 0;
      }" checkMemMMapAnon)
   if (checkMemMMapAnon)
      set(HAVE_MEM_MMAP_ANON ON)
   endif()

   # test for memory allocation using mmap("/dev/zero")
   check_c_source_runs(
      "#include <sys/types.h>
      #include <sys/stat.h>
      #include <fcntl.h>
      #include <sys/mman.h>
      #include <stdlib.h>
      #include <stdio.h>
      #ifndef MAP_ANON
      # ifdef MAP_ANONYMOUS
      #  define MAP_ANON MAP_ANONYMOUS
      # endif
      #endif
      #ifndef MREMAP_MAYMOVE
      # define MREMAP_MAYMOVE 0
      #endif
      #ifndef MAP_FAILED
      # define MAP_FAILED ((void*)-1)
      #endif

      #define SEG_SIZE (256*1024)

      int main()
      {
            int fd;
            void *seg;

            fd = open(\"/dev/zero\", O_RDWR, S_IRUSR | S_IWUSR);
            if (fd < 0) {
                  return 1;
            }
            seg = mmap(NULL, SEG_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
            if (seg == MAP_FAILED) {
                  return 2;
            }
            if (munmap(seg, SEG_SIZE) != 0) {
                  return 3;
            }
            if (close(fd) != 0) {
                  return 4;
            }
            return 0;
      }" checkMemMmapZero)
   if (checkMemMmapZero)
      set(HAVE_MEM_MMAP_ZERO ON)
   endif()

   check_function_exists(mremap HAVE_MREMAP)
   if (NOT POLAR_DISABLE_ZEND_SIGNALS)
      set(ZEND_SIGNALS ON)
   else()
      set(ZEND_SIGNALS OFF)
   endif()
   check_function_exists(sigaction HAVE_SIGACTION)
   if (NOT HAVE_SIGACTION)
      set(ZEND_SIGNALS OFF)
   endif()
endmacro()

