#include<stdio.h>

void main(void)
{
  unsigned long a1= 0x800820f0ec;
  unsigned long a2= 0x8008096eb4;
  int a3= (int)(a2-a1)/4;
  int off= 0xf3fa1f72;
  int test= -1;
  //ffffff800820f0c4:   97fa1f7c    bl  ffffff8008096eb4 <_mcount>
  //ffffff800820f0ec:   97fa1f72    bl  ffffff8008096eb4 <_mcount>
  //ffffff8008096eb4 T _mcount
  printf("a1-a2:%x\n",a1 - a2);
  printf("a1-a2:%x\n",(a1 - a2)/4);
  printf("a3:%x\n",a3);
  printf("off:%d\n",off);
  printf("off:%x\n",off*4);
  printf("test:%x\n",test);
}
