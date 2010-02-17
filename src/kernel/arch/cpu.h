/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Aug 19 20:18:29 2007 texane
** Last update Tue Nov 20 01:34:26 2007 texane
*/



#ifndef CPU_H_INCLUDED
# define CPU_H_INCLUDED



#include "../sys/types.h"



#define cpu_cli() __asm__ ("cli \n\t")


#define cpu_sti() __asm__ ("sti \n\t")


inline static bool_t cpu_irqs_enabled(void)
{
  uint32_t flags;

  __asm__ ("pushfl \n\t"
	   "popl %0\n\t"
	   : "=r"(flags)
	   :
	   : "memory"
	   );

  if (flags & (1 << 9))
    return true;

  return false;
}


inline static bool_t cpu_irqs_disabled(void)
{
  return cpu_irqs_disabled() == true ? false : true;
}


inline static void cpu_disable_irqs(bool_t* state)
{
  *state = false;

  if (cpu_irqs_enabled())
    {
      *state = true;
      cpu_cli();
    }
}


inline static void cpu_restore_irqs(bool_t state)
{
  if (state == true)
    cpu_sti();
}


#define cpu_hlt() __asm__ ("hlt \n\t")


#define cpu_nop() __asm__ ("nop \n\t")


#define cpu_brk() __asm__ ("int $0x3 \n\t")


static inline void cpu_divide_error(void)
{						
  asm("xorb %%dl, %%dl\n\t"			
      "movb %%dl, %%al\n\t"			
      "divb %%dl\n\t"			
      :					
      :					
      : "al", "dl");			
}


static inline uchar_t cpu_inb(ushort_t addr)
{
  uchar_t val;

  asm("movw %1, %%dx\n\t"
      "inb %%dx, %%al\n\t"
      "movb %%al, %0\n\t"
      :"=m"(val)
      :"m"(addr)
      :"memory", "eax", "edx");

  return val;
}


static inline uint_t cpu_inl(ushort_t addr)
{
  uint_t val;

  asm("movw %1, %%dx\n\t"
      "inl %%dx, %%eax\n\t"
      "movl %%eax, %0\n\t"
      :"=m"(val)
      :"m"(addr)
      :"memory", "eax", "edx");

  return val;
}


static inline void cpu_outb(ushort_t addr, uchar_t val)
{
  asm("movb %0, %%al\n\t"
      "movw %1, %%dx\n\t"
      "outb %%al, %%dx\n\t"
      :
      :"m"(val), "m"(addr)
      :"memory", "eax", "edx");
}


static inline void cpu_outl(ushort_t addr, uint_t val)
{
  asm("movl %0, %%eax\n\t"
      "movw %1, %%dx\n\t"
      "outl %%eax, %%dx\n\t"
      :
      :"m"(val), "m"(addr)
      :"memory", "eax", "edx");
}


uint_t cpu_get_freq(void);


#endif /* ! CPU_H_INCLUDED */
