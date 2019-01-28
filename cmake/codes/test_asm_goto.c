int main(void)
{
   __asm__ goto("jmp %l0\n" :::: end);
end:
   return 0;
}
