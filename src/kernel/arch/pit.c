/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Aug 26 13:47:33 2007 texane
** Last update Sun Nov 18 20:43:50 2007 texane
*/



#include "../debug/debug.h"
#include "../sys/types.h"
#include "../arch/cpu.h"
#include "../arch/pit.h"
#include "../arch/pic.h"



#define I8254_MAX_FREQ 1193180
#define I8254_TIMER0 0x40
#define I8254_TIMER1 0x41
#define I8254_TIMER2 0x42
#define I8254_CONTROL 0x43


static void i8254_set_freq(uint32_t freq)
{
  uint_t nticks;

  nticks = I8254_MAX_FREQ / freq;
  if (nticks == 65536)
    nticks = 0;

  cpu_outb(I8254_CONTROL, 0x34);
  cpu_outb(I8254_TIMER0, nticks & 0xff);
  cpu_outb(I8254_TIMER0, (nticks >> 8) & 0xff);
}


static void i8254_initialize(uint32_t freq)
{
  i8254_set_freq(freq);
}


int pit_initialize(uint32_t freq)
{
  i8254_initialize(freq);

  return 0;
}
