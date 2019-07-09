/* this is a simple hello world program */
#include <stdio.h>

char __ramfs_runtime_data[1024];
char flash_block_array[1024];
int main(void)
{
  printf("Hello, eCos world!\n");
  return 0;
}
