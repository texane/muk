/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Nov 19 04:46:50 2007 texane
** Last update Tue Nov 20 01:39:39 2007 texane
*/



#include "../event/event.h"
#include "../task/task.h"
#include "../sched/sched.h"
#include "../sys/types.h"
#include "../sys/time.h"
#include "../libc/libc.h"
#include "../mm/mm.h"
#include "../arch/arch.h"
#include "../debug/debug.h"




/* event waiting task
 */

struct event_task
{
  struct event_task* next;
  struct task* task;
};



/* event object
 */

struct event
{
  struct event_task* tasks;
  bool_t is_signaled;
  struct timeval timeout;
  struct event* next;
};



/* event list
 */

struct event* g_event_list = NULL;



/* event subsystem initialization
 */

error_t event_initialize(void)
{
  g_event_list = NULL;

  return ERROR_SUCCESS;
}



/* event creation
 */

static void link_event(struct event* event)
{
  cpu_cli();

  event->next = g_event_list;
  g_event_list = event;

  cpu_sti();
}


error_t event_create(struct event** event)
{
  struct event* tmp;

  tmp = mm_alloc(sizeof(struct event));
  if (tmp == NULL)
    return ERROR_NO_MEMORY;
  
  tmp->tasks = NULL;
  tmp->is_signaled = false;
  timeval_make_infinite(&tmp->timeout);

  *event = tmp;

  link_event(tmp);

  return ERROR_SUCCESS;
}



/* event destruction
 */

static void unlink_event(struct event* event)
{
  struct event* pos;

  cpu_cli();

  if (event != g_event_list)
    {
      pos = g_event_list;
      while (pos->next != event)
	pos = pos->next;
      pos->next = event->next;
    }
  else
    {
      g_event_list = event->next;
    }

  cpu_sti();
}


error_t event_destroy(struct event* event)
{
  struct event_task* pos;
  struct event_task* tmp;

  unlink_event(event);

  /* no need be atomic: by definition, only
     one task can be destroying an event.
  */

  pos = event->tasks;

  while (pos != NULL)
    {
      tmp = pos;
      pos = pos->next;
      mm_free(tmp);
    }

  mm_free(event);

  return ERROR_SUCCESS;
}



/* signal the event
 */

error_t event_signal(struct event* event)
{
  bool_t state;

  cpu_disable_irqs(&state);

  event->is_signaled = true;

  cpu_restore_irqs(state);

  return ERROR_SUCCESS;
}



/* is event signaled
 */

bool_t event_is_signaled(const struct event* event)
{
  return event->is_signaled;
}



/* reset the event signal
 */

void event_clear_signal(struct event* event)
{
  bool_t state;

  cpu_disable_irqs(&state);

  event->is_signaled = false;

  cpu_restore_irqs(state);
}



/* wait for signal
 */

error_t event_wait(struct event* event)
{
  struct task* task;
  struct event_task* node;
  struct event_task* pos;
  bool_t is_linked;

  task = g_current_task;

  node = mm_alloc(sizeof(struct event_task));
  if (node == NULL)
    return ERROR_NO_MEMORY;

  node->task = task;
  node->next = NULL;

  is_linked = false;

  cpu_cli();

  if (event->is_signaled == false)
    {
      task_wait_event(task, event);
      node->next = event->tasks;
      event->tasks = node;
      is_linked = true;
    }

  cpu_sti();

  /* race here
   */
  while (task_get_state(task) == TASK_STATE_WAIT)
    cpu_hlt();

  /* unlink task node
   */
  if (is_linked == true)
    {
      cpu_cli();

      if (event->tasks == node)
	{
	  event->tasks = node->next;
	}
      else
	{
	  pos = event->tasks;
	  while (pos->next != node)
	    pos = pos->next;
	  pos->next = node->next;
	}

      cpu_sti();
    }

  /* free task node
   */
  mm_free(node);

  return ERROR_SUCCESS;
}



/* set event timeout
 */

void event_set_timeout(struct event* event, const struct timeval* timeout)
{
  memcpy(&event->timeout, timeout, sizeof(struct timeval));
}



/* get event timeout
 */

void event_get_timeout(struct event* event, struct timeval* timeout)
{
  memcpy(timeout, &event->timeout, sizeof(struct timeval));
}



/* get next event
 */

struct event* event_get_next(struct event* event)
{
  return event->next;
}



/* called by sched to signal tasks
 */

error_t event_signal_tasks(struct event* event)
{
  struct event_task* pos;

  for (pos = event->tasks; pos != NULL; pos = pos->next)
    task_set_state(pos->task, TASK_STATE_READY);

  return ERROR_SUCCESS;
}



/* event testing
 */

static error_t __attribute__((fastcall, unused)) do_waiter(task_t* task)
{
  struct event* event;

  event = task->param;

  while (1)
    {
      serial_printl("w(%d)", task->id);
      event_wait(event);
    }

  return ERROR_SUCCESS;
}


static error_t __attribute__((fastcall, unused)) do_signaler(task_t* task)
{
  struct event* event;
  int i;

  event = task->param;

  i = 0;

  while (1)
    {
      cpu_hlt();
      ++i;

      if ((i & 0xf) == 0xf)
	{
	  serial_printl("s");
	  event_signal(event);
	}
    }

  return ERROR_SUCCESS;
}


static error_t __attribute__((fastcall, unused)) do_timeouter(task_t* task)
{
  struct timeval timeout;
  struct event* event;
  error_t error;

  error = event_create(&event);
  if (error_is_failure(error))
    return error;

  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  while (1)
    {
      serial_printl("t");
      event_set_timeout(event, &timeout);
      event_wait(event);
    }

  event_destroy(event);

  return ERROR_SUCCESS;
}


void event_test(void)
{
  struct event* event;
  struct task* task;
  uint32_t count;

  event_create(&event);

  /* waiter tasks
   */
  for (count = 0; count < 2; ++count)
    {
      task = task_create_with_entry(do_waiter, event);
      if (task != NULL)
	{
	  task_set_timeslice(task, TASK_SCHED_TIMESLICE);
	  task_set_state(task, TASK_STATE_READY);
	  sched_add_task(task);
	}
    }

  /* signaler task
   */
  task = task_create_with_entry(do_signaler, event);
  if (task == NULL)
    return ;

  task_set_timeslice(task, TASK_SCHED_TIMESLICE);
  task_set_state(task, TASK_STATE_READY);
  sched_add_task(task);

  /* timeouter task
   */
  task = task_create_with_entry(do_timeouter, event);
  if (task == NULL)
    return ;

  task_set_timeslice(task, TASK_SCHED_TIMESLICE);
  task_set_state(task, TASK_STATE_READY);
  sched_add_task(task);

  /* start scheduler
   */
  sched_start();

  /* infinite loop
   */
  while (1)
    {
      serial_printl("e");
      cpu_hlt();
    }
}
