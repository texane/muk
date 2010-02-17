/*
** Made by texane <texane@gmail.com>
** 
** Started on  Fri Sep  7 23:45:17 2007 texane
** Last update Sun Nov 18 20:44:45 2007 texane
*/



#include "../debug/debug.h"
#include "../sched/sched.h"
#include "../arch/idt.h"
#include "../arch/pic.h"
#include "../arch/pit.h"
#include "../sys/types.h"



static void handle_timer_interrupt(void)
{
  sched_tick();
}


int timer_initialize(uint32_t freq)
{
  pic_enable_irq(0, handle_timer_interrupt);

  pit_initialize(freq);

  return 0;
}
