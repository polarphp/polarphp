#include <stdio.h>
int main(int argc, char *argv[])
{
  FILE *fp;
  long position;
  char *filename = tmpnam(NULL);

  fp = fopen(filename, "w");
  if (fp == NULL) {
    perror("fopen");
    exit(2);
  }
  fputs("foobar", fp);
  fclose(fp);

  fp = fopen(filename, "a+");
  position = ftell(fp);
  fclose(fp);
  unlink(filename);
  if (position == 0)
  return 1;
  return 0;
}
