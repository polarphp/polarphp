include(ZendUtils)

polar_check_libzend_basic_requires()
polar_check_libzend_dlsym()
polar_check_libzend_other_requires()

# here we force thread safe
set(ZTS ON)
set(PHP_THREAD_SAFETY ON)

message("whether /dev/urandom exists")
execute_process(COMMAND test -r "/dev/urandom"
   RESULT_VARIABLE haveDevUrandomReadPerm
   OUTPUT_VARIABLE _output
   ERROR_VARIABLE _errorMsg)

execute_process(COMMAND test -c "/dev/urandom"
   RESULT_VARIABLE isDevUrandomCharDevice
   OUTPUT_VARIABLE _output
   ERROR_VARIABLE _errorMsg)

if (haveDevUrandomReadPerm EQUAL 0 AND isDevUrandomCharDevice EQUAL 0)
   # Define if the target system has /dev/urandom device
   set(HAVE_DEV_URANDOM ON)
endif()

message("whether /dev/arandom exists")
execute_process(COMMAND test -r "/dev/arandom"
   RESULT_VARIABLE haveDevArandomReadPerm
   OUTPUT_VARIABLE _output
   ERROR_VARIABLE _errorMsg)

execute_process(COMMAND test -c "/dev/arandom"
   RESULT_VARIABLE isDevArandomCharDevice
   OUTPUT_VARIABLE _output
   ERROR_VARIABLE _errorMsg)

if (haveDevArandomReadPerm EQUAL 0 AND isDevArandomCharDevice EQUAL 0)
   # Define if the target system has /dev/arandom device
   set(HAVE_DEV_ARANDOM ON)
endif()

if (NOT POLAR_DISABLE_GCC_GLOABL_REGS)
   set(ZEND_GCC_GLOBAL_REGS ON)
else()
   set(ZEND_GCC_GLOBAL_REGS OFF)
endif()
if (ZEND_GCC_GLOBAL_REGS)
   message("for global register variables support")
   check_c_source_compiles("
      #if defined(__GNUC__)
         # define ZEND_GCC_VERSION (__GNUC__ * 1000 + __GNUC_MINOR__)
      #else
         # define ZEND_GCC_VERSION 0
      #endif
      #if defined(__GNUC__) && ZEND_GCC_VERSION >= 4008 && defined(i386)
         # define ZEND_VM_FP_GLOBAL_REG \"%esi\"
         # define ZEND_VM_IP_GLOBAL_REG \"%edi\"
      #elif defined(__GNUC__) && ZEND_GCC_VERSION >= 4008 && defined(__x86_64__)
         # define ZEND_VM_FP_GLOBAL_REG \"%r14\"
         # define ZEND_VM_IP_GLOBAL_REG \"%r15\"
      #elif defined(__GNUC__) && ZEND_GCC_VERSION >= 4008 && defined(__powerpc64__)
         # define ZEND_VM_FP_GLOBAL_REG \"r28\"
         # define ZEND_VM_IP_GLOBAL_REG \"r29\"
      #elif defined(__IBMC__) && ZEND_GCC_VERSION >= 4002 && defined(__powerpc64__)
         # define ZEND_VM_FP_GLOBAL_REG \"r28\"
         # define ZEND_VM_IP_GLOBAL_REG \"r29\"
      #else
         # error \"global register variables are not supported\"
      #endif
      typedef int (*opcode_handler_t)(void);
      register void *FP  __asm__(ZEND_VM_FP_GLOBAL_REG);
      register const opcode_handler_t *IP __asm__(ZEND_VM_IP_GLOBAL_REG);
      int emu(const opcode_handler_t *ip, void *fp) {
            const opcode_handler_t *orig_ip = IP;
            void *orig_fp = FP;
            IP = ip;
            FP = fp;
            while ((*ip)());
            FP = orig_fp;
            IP = orig_ip;
      }
      int main() {
         return 0;
      }" CheckZendGccGlobalRegs)
   if(CheckZendGccGlobalRegs)
      set(HAVE_GCC_GLOBAL_REGS ON)
   else()
      set(HAVE_GCC_GLOBAL_REGS OFF)
   endif()
endif()

message("Check if atof() accepts NAN")
check_c_source_runs(
   "#include <math.h>
    #include <stdlib.h>

    #ifdef HAVE_ISNAN
      #define zend_isnan(a) isnan(a)
    #elif defined(HAVE_FPCLASS)
      #define zend_isnan(a) ((fpclass(a) == FP_SNAN) || (fpclass(a) == FP_QNAN))
    #else
      #define zend_isnan(a) 0
    #endif

   int main(int argc, char** argv)
   {
         return zend_isnan(atof(\"NAN\")) ? 0 : 1;
   }" CheckAtofAcceptInf)
if(CheckAtofAcceptInf)
   set(HAVE_ATOF_ACCEPTS_INF ON)
endif()

message("Check if HUGE_VAL == INF")
check_c_source_runs(
   "#include <math.h>
   #include <stdlib.h>

   #ifdef HAVE_ISINF
   #define zend_isinf(a) isinf(a)
   #elif defined(INFINITY)
   /* Might not work, but is required by ISO C99 */
   #define zend_isinf(a) (((a)==INFINITY)?1:0)
   #elif defined(HAVE_FPCLASS)
   #define zend_isinf(a) ((fpclass(a) == FP_PINF) || (fpclass(a) == FP_NINF))
   #else
   #define zend_isinf(a) 0
   #endif

   int main(int argc, char** argv)
   {
         return zend_isinf(HUGE_VAL) ? 0 : 1;
   }" CheckHugeValInf)
if(CheckHugeValInf)
   set(HAVE_HUGE_VAL_INF ON)
endif()

message("Check if HUGE_VAL + -HUGEVAL == NAN")
check_c_source_runs(
   "#include <math.h>
   #include <stdlib.h>
   #include <stdio.h>

   #ifdef HAVE_ISNAN
   #define zend_isnan(a) isnan(a)
   #elif defined(HAVE_FPCLASS)
   #define zend_isnan(a) ((fpclass(a) == FP_SNAN) || (fpclass(a) == FP_QNAN))
   #else
   #define zend_isnan(a) 0
   #endif

   int main(int argc, char** argv)
   {
   #if defined(__sparc__) && !(__GNUC__ >= 3)
         /* prevent bug #27830 */
         return 1;
   #else
         return zend_isnan(HUGE_VAL + -HUGE_VAL) ? 0 : 1;
   #endif
   }" CheckHugeValNan)

if(CheckHugeValNan)
   set(HAVE_HUGE_VAL_NAN ON)
endif()
