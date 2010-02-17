/*
** Made by texane <texane@gmail.com>
** 
** Started on  Thu Nov 15 01:51:40 2007 texane
** Last update Tue Nov 20 03:42:01 2007 texane
*/



#include "../sched/sched.h"
#include "../task/task.h"
#include "../mm/mm.h"
#include "../event/event.h"
#include "../sys/types.h"
#include "../sys/time.h"
#include "../arch/arch.h"
#include "../debug/debug.h"



/* scheduler
 */

struct sched
{
  struct task* head;
  struct task* tail;
  bool_t is_started;
};

static struct sched g_sched;



/* current task
 */

task_t* g_current_task = NULL;



/* initialize the scheduler
 */

error_t sched_initialize(void)
{
  g_sched.head = NULL;
  g_sched.tail = NULL;

  g_sched.is_started = false;

  return ERROR_SUCCESS;
}



/* start scheduling
 */

error_t sched_start(void)
{
  if (g_sched.is_started == true)
    return ERROR_FAILURE;

  cpu_cli();

  g_sched.is_started = true;

  timer_initialize(SCHED_TIMER_FREQ);

  cpu_sti();

  return ERROR_SUCCESS;
}



/* add a taks to the scheduler
 */

error_t sched_add_task(task_t* task)
{
  bool_t state;

  cpu_disable_irqs(&state);

  /* link task back
   */
  if (g_sched.head == NULL)
    {
      task->next = task;
      g_sched.head = task;
      g_sched.tail = task;
    }
  else
    {
      task->next = g_sched.head;
      g_sched.tail->next = task;
      g_sched.tail = task;
    }

  cpu_restore_irqs(state);

  return ERROR_SUCCESS;
}



/* process a timer tick
 */

static void process_events(void)
{
  struct event* pos;
  struct timeval timeout;

  for (pos = g_event_list; pos != NULL; pos = event_get_next(pos))
    {
      /* event timeout
       */
      event_get_timeout(pos, &timeout);
      if (timeval_is_infinite(&timeout) == false)
	{
	  timeval_sub_one_tick(&timeout);
	  event_set_timeout(pos, &timeout);

	  if (timeval_is_null(&timeout) == true)
	    event_signal(pos);
	}

      /* event was signaled
       */
      if (event_is_signaled(pos))
	{
	  event_signal_tasks(pos);
	  event_clear_signal(pos);
	}
    }
}


error_t sched_tick(void)
{
  struct task* first;
  struct task* pos;
  struct task* ready;
  bool_t is_done;

  /* set current task
   */
  if (g_current_task == NULL)
    g_current_task = g_sched.head;

  /* process events
   */
  process_events();

  /* find next ready task
   */
  if ((g_current_task->state != TASK_STATE_READY) || !g_current_task->timeslice)
    {
      is_done = false;
      ready = NULL;
      first = g_current_task->next;
      pos = g_current_task->next;
      while (is_done == false)
	{
	  if (pos->state == TASK_STATE_READY)
	    {
	      ready = pos;
	      ready->timeslice = TASK_SCHED_TIMESLICE;
	      is_done = true;
	    }
	  else
	    {
	      pos = pos->next;
	      if (pos == first)
		is_done = true;
	    }
	}

      /* update current task
       */
      if (ready != NULL)
	g_current_task = ready;

      /* update timeslice
       */
      if (g_current_task->timeslice == 0)
	g_current_task->timeslice = TASK_SCHED_TIMESLICE;
    }

  --g_current_task->timeslice;

  return ERROR_SUCCESS;
}



/* yield the cpu
 */

#if 0

error_t sched_yield(void)
{
  task_t* current;
  volatile uint32_t* timeslice;

  current = sched_get_current();
  timeslice = &current->timeslice;
  *timeslice = 0;

  /* fixme: race here
   */
  while (*timeslice == 0)
    cpu_hlt();

  return ERROR_SUCCESS;
}


#else

error_t sched_yield(void)
{
  struct task* first;
  struct task* pos;
  struct task* ready;
  bool_t is_done;

  cpu_cli();

  task_set_timeslice(g_current_task, 0);

  /* find next ready task
   */
  is_done = false;
  ready = NULL;
  first = g_current_task->next;
  pos = g_current_task->next;
  while (is_done == false)
    {
      if (pos->state == TASK_STATE_READY)
	{
	  ready = pos;
	  ready->timeslice = TASK_SCHED_TIMESLICE;
	  is_done = true;
	}
      else
	{
	  pos = pos->next;
	  if (pos == first)
	    is_done = true;
	}
    }

  /* switch to ready
   */
  if (ready != NULL)
    {
      if (ready->timeslice == 0)
	task_set_timeslice(ready, TASK_SCHED_TIMESLICE);
      task_switch(g_current_task, ready);
    }
  else
    {
      cpu_sti();
    }
  
  return ERROR_SUCCESS;
}

#endif
