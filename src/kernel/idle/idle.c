/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Nov 17 02:56:42 2007 texane
** Last update Tue Nov 20 02:56:06 2007 texane
*/



#include "../task/task.h"
#include "../sched/sched.h"
#include "../idle/idle.h"
#include "../sys/types.h"
#include "../arch/arch.h"
#include "../debug/debug.h"



/* create the idle task
 */

static error_t __attribute__((fastcall)) do_idle(task_t* task)
{
  serial_printl("%s(%d)\n", __FUNCTION__, task->id);

  while (1)
    {
      serial_printl("i");
      cpu_hlt();
    }

  return ERROR_SUCCESS;
}


error_t idle_initialize(void)
{
  task_t* task;

  task = task_create_with_entry(do_idle, NULL);
  if (task == NULL)
    return ERROR_FAILURE;

  task_set_state(task, TASK_STATE_READY);
  task_set_timeslice(task, TASK_SCHED_TIMESLICE);

  sched_add_task(task);

  return ERROR_SUCCESS;
}
