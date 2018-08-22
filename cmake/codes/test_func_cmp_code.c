#include <string.h>
int main() {
   char c0 = 0100, c1 = 0200, c2 = 0201;
   if (memcmp(&c0, &c2, 1) >= 0 || memcmp(&c1, &c2, 1) >= 0)
      return 1;
   {
      char foo[21];
      char bar[21];
      int i;
      for (i = 0; i < 4; i++)
      {
         char *a = foo + i;
         char *b = bar + i;
         strcpy (a, "--------01111111");
         strcpy (b, "--------10000000");
         if (memcmp (a, b, 16) >= 0)
            return 1;
      }
      return 0;
   }
}
