#include <stdio.h>
#include <stdlib.h>

typedef union _mm_align_test {
   void *ptr;
   double dbl;
   long lng;
} mm_align_test;

#if (defined (__GNUC__) && __GNUC__ >= 2)
#define ZEND_MM_ALIGNMENT (__alignof__ (mm_align_test))
#else
#define ZEND_MM_ALIGNMENT (sizeof(mm_align_test))
#endif

int main()
{
   int i = ZEND_MM_ALIGNMENT;
   int zeros = 0;
   FILE *fp;

   while (i & ~0x1) {
      zeros++;
      i = i >> 1;
   }

   fp = fopen(POLAR_CONFIGURE_TEMP_DIR"/conftest.zend", "w");
   fprintf(fp, "%d %d\n", ZEND_MM_ALIGNMENT, zeros);
   fclose(fp);
   exit(0);
}
