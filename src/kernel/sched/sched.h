/*
** Made by texane <texane@gmail.com>
** 
** Started on  Fri Sep  7 23:48:37 2007 texane
** Last update Mon Nov 19 23:40:24 2007 texane
*/


#ifndef SCHED_H_INCLUDED
# define SCHED_H_INCLUDED



#include "../sched/timer.h"
#include "../sys/types.h"
#include "../task/task.h"



#define SCHED_TIMER_FREQ 10



/* get the current task
 */

extern task_t* g_current_task;

static inline task_t* sched_get_current(void)
{
  return g_current_task;
}



/* scheduler prototypes
 */

error_t sched_initialize(void);
error_t sched_add_task(task_t*);
error_t sched_tick(void);
error_t sched_yield(void);
error_t sched_start(void);
error_t sched_stop(void);



#endif /* ! SCHED_H_INCLUDED */
