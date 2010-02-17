/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 22 23:24:11 2007 texane
** Last update Sat Aug 25 18:24:56 2007 texane
*/



#include "../sys/types.h"
#include "../arch/cpu.h"


uint_t cpu_get_freq(void)
{
  /* return the cpu frequency in mhz */
#define CPU_FREQ 500000000
  return CPU_FREQ;
}
