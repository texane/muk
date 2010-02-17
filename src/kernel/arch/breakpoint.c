/*
** Made by texane <texane@gmail.com>
** 
** Started on  Thu Sep 13 01:00:32 2007 texane
** Last update Fri Nov 16 17:46:23 2007 texane
*/



#include "../sched/sched.h"
#include "../task/task.h"
#include "../debug/debug.h"
#include "../sys/types.h"
#include "../arch/idt.h"
#include "../arch/cpu.h"



/* interrupt handler
 */

static void handle_interrupt(void)
{
  task_t* task;

  serial_printl("[?] breakpoint\n");

  task = sched_get_current();
  if (task != NULL)
    serial_printl("[?] task->id: %d\n", task->id);
  else
    serial_printl("[?] task->id: null\n");

  cpu_hlt();
}



/* initialize subsystem
 */

error_t breakpoint_initialize(void)
{
  idt_set_handler(3, handle_interrupt);

  return ERROR_SUCCESS;
}



/* cleanup subsystem
 */

error_t breakpoint_cleanup(void)
{
  idt_set_handler(3, null);
  
  return ERROR_SUCCESS;
}



/* set a breakpoint at addr
 */

error_t breakpoint_set(uintptr_t addr, void (*handler)())
{
  NOT_IMPLEMENTED();

  return ERROR_NOT_IMPLEMENTED;
}



/* unset breakpoint from addr
 */

error_t breakpoint_unset(uintptr_t addr)
{
  NOT_IMPLEMENTED();

  return ERROR_NOT_IMPLEMENTED;
}
