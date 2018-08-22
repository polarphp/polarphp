#include <stdio.h>

int main()
{
   /* Ultrix mips cc rejects this sort of thing.  */
   typedef int charset[2];
   const charset cs = { 0, 0 };
   /* SunOS 4.1.1 cc rejects this.  */
   char const *const *pcpcc;
   char **ppc;
   /* NEC SVR4.0.2 mips cc rejects this.  */
   struct point {int x, y;};
   static struct point const zero = {0,0};
   /* AIX XL C 1.02.0.0 rejects this.
           It does not let you subtract one const X* pointer from another in
           an arm of an if-expression whose if-part is not a constant
           expression */
   const char *g = "string";
   pcpcc = &g + (g ? g-g : 0);
   /* HPUX 7.0 cc rejects these. */
   ++pcpcc;
   ppc = (char**) pcpcc;
   pcpcc = (char const *const *) ppc;
   { /* SCO 3.2v4 cc rejects this sort of thing.  */
      char tx;
      char *t = &tx;
      char const *s = 0 ? (char *) 0 : (char const *) 0;

      *t++ = 0;
      if (s) return 0;
   }
   { /* Someone thinks the Sun supposedly-ANSI compiler will reject this.  */
      int x[] = {25, 17};
      const int *foo = &x[0];
      ++foo;
   }
   { /* Sun SC1.0 ANSI compiler rejects this -- but not the above. */
      typedef const int *iptr;
      iptr p = 0;
      ++p;
   }
   { /* AIX XL C 1.02.0.0 rejects this sort of thing, saying
             "k.c", line 2.27: 1506-025 (S) Operand must be a modifiable lvalue. */
      struct s { int j; const int *ap[3]; } bx;
      struct s *b = &bx; b->j = 5;
   }
   { /* ULTRIX-32 V3.1 (Rev 9) vcc rejects this */
      const int foo = 10;
      if (!foo) return 0;
   }
   printf("xiuxixixuix");
   return !(!cs[0] && !zero.x);
}
