/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 15 19:29:55 2007 texane
** Last update Wed Aug 15 21:35:22 2007 texane
*/


/* machine specific registers
 */


#include "../arch/msr.h"
#include "../sys/types.h"


int msr_read(msr_reg_t reg, uint64_t* val)
{
  *val = 0;

  __asm__("movl %1, %%ecx\n\t"
	  "rdmsr\n\t"
	  "movl %0, %%ecx\n\t"
	  "movl %%eax, (%%ecx)\n\t"
	  "add $4, %%ecx\n\t"
	  "movl %%edx, (%%ecx)\n\t"
	  :"=m"(val)
	  :"m"(reg)
	  :"eax", "ecx", "edx");

  return 0;
}


int msr_write(msr_reg_t reg, uint64_t val)
{
  uint_t lo;
  uint_t hi;

  lo = (uint_t)((val >>  0) & 0xffffffff);
  hi = (uint_t)((val >> 32) & 0xffffffff);

  __asm__("movl %0, %%ecx\n\t"
	  "movl %1, %%eax\n\t"
	  "movl %2, %%edx\n\t"
	  "wrmsr\n\t"
	  :
	  :"m"(reg), "m"(lo), "m"(hi)
	  :"eax", "ecx", "edx");

  return 0;
}
