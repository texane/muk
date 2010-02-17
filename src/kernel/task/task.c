/*
** Made by texane <texane@gmail.com>
** 
** Started on  Thu Nov 15 01:21:49 2007 texane
** Last update Tue Nov 20 03:46:12 2007 texane
*/



#include "../task/task.h"
#include "../sched/sched.h"
#include "../sys/types.h"
#include "../mm/mm.h"
#include "../arch/arch.h"
#include "../debug/debug.h"



/* get next task identifier
 */

static task_id_t g_task_id = 0;

inline static task_id_t get_next_id(void)
{
  return g_task_id++;
}



/* initialize the task subsystem
 */

error_t task_initialize(void)
{
  return ERROR_SUCCESS;
}



/* reset task internals
 */

inline static void task_reset(task_t* task)
{
  task->sp = 0;
  task->state = TASK_STATE_INVALID;
  task->event = NULL;
  task->id = 0;
  task->timeslice = 0;
  task->error = ERROR_SUCCESS;
  task->next = NULL;
  task->prev = NULL;
  task->entry = NULL;
  task->param = NULL;
  task->dtor = NULL;
}



/* task destruction
 */

static void task_destructor(task_t* task)
{
  /* error code is stored in eax
   */
  __asm__ __volatile__ ("movl %%eax, %0\n\t"
			: "=m" (task->error));

  /* invoke user supplied dtor
   */
  if (task->dtor != NULL)
    task->dtor(task);

  /* mark the task as invalid
   */
  task_set_state(task, TASK_STATE_INVALID);

  /* wait to be reaped
   */
  while (1)
    {
      serial_printl("z(%d)", task->error);
      cpu_hlt();
    }
}


void task_destroy(task_t* task)
{
  void* frame;

  if (task_get_state(task) != TASK_STATE_INVALID)
    {
      BUG();
      return ;
    }

  /* get task page frame
   */
  frame = (void*)((uintptr_t)task & ~(PHYS_FRAME_SIZE - 1));
  mm_free(frame);
}



/* set task destructor
 */

void task_set_dtor(task_t* task, task_dtor_t dtor)
{
  task->dtor = dtor;
}



/* create a task given the entry point
 */

task_t* task_create_with_entry(task_entry_t entry, void* param)
{
  register task_t* task;
  void* frame;

  frame = mm_alloc(PHYS_FRAME_SIZE);
  if (frame == NULL)
    return NULL;

  task = (task_t*)((uintptr_t)frame + PHYS_FRAME_SIZE - sizeof(task_t));
  task_reset(task);
  task->id = get_next_id();
  task->entry = entry;
  task->param = param;
  task->sp = (uintptr_t)task - sizeof(uint32_t);

  /* initial stack frame
     use eax, ebx, ecx as temporary registers
   */
  __asm__ (
	  /* save current */
	  "movl %%esp, %%eax\n\t"
	  "movl %%ebp, %%ebx\n\t"

	  /* set stack pointer */
	  "movl %2, %%esp\n\t"
	  "movl %%esp, %%ebp\n\t"

	  /* on exit, invoke destructor(task) */
	  "movl %3, %%ecx\n\t"
	  "pushl %%ecx\n\t"
	  "pushl $0\n\t"
	  "pushl %4\n\t"

	  /* eflags, cs, eip */
	  "pushf\n\t"
	  "pushl %%cs\n\t"
	  "movl %1, %%ecx\n\t"
	  "pushl %%ecx\n\t"

	  /* entry point is a fastcall (ecx) */
	  "movl %3, %%ecx\n\t"

	  /* save cpu context */
	  "pusha\n\t"
	  "subl  $2,%%esp\n\t"
	  "pushw %%ss\n\t"
	  "pushw %%ds\n\t"
	  "pushw %%es\n\t"
	  "pushw %%fs\n\t"
	  "pushw %%gs\n\t"

	  /* update task stack pointer */
	  "movl %%esp, %0\n\t"

	  /* restore stack pointer */
	  "movl %%ebx, %%ebp\n\t"
	  "movl %%eax, %%esp\n\t"

	  : "=m"(task->sp)
	  : "m"(task->entry), "m"(task->sp), "g"(task), "i"(task_destructor)
	  : "eax", "ebx", "ecx"

	  );

  return task;
}



/* switch tasks
 */

#define switch_to( to )					\
do {							\
  __asm__ (						\
	   /* save context				\
	    */						\
	   "pushf\n\t"					\
	   "pushl %%cs\n\t"				\
	   "call 1f\n\t"				\
	   "1:\n\t"					\
	   "addl $(2f - 1b), (%%esp)\n\t"		\
	   "pusha\n\t"					\
	   "subl $2, %%esp\n\t"				\
	   "pushw %%ss\n\t"				\
	   "pushw %%ds\n\t"				\
	   "pushw %%es\n\t"				\
	   "pushw %%fs\n\t"				\
	   "pushw %%gs\n\t"				\
	   "movl %0, %%eax\n\t"				\
	   "movl %%esp, (%%eax)\n\t"			\
							\
	   /* update current				\
	    */						\
	   "movl %1, %%eax\n\t"				\
	   "movl %%eax, %0\n\t"				\
							\
	   /* switch stacks				\
	    */						\
	   "movl %2, %%esp\n\t"				\
	   						\
	   /* restore context				\
	    */						\
	   "popw %%gs\n\t"				\
	   "popw %%fs\n\t"				\
	   "popw %%es\n\t"				\
	   "popw %%ds\n\t"				\
	   "popw %%ss\n\t"				\
	   "addl $2, %%esp\n\t"				\
	   "popa\n\t"					\
	   "iretl\n\t"					\
							\
	   "2:\n\t"					\
							\
	   : "=g"(g_current_task)			\
	   : "g"(to), "g"(to->sp)			\
	   : "memory"					\
	   );						\
} while (0)


void task_switch(struct task* from, struct task* to)
{
  cpu_cli();
  task_set_timeslice(from, 0);
  switch_to(to);
}



/* set task state
 */

error_t task_set_state(task_t* task,
		       task_state_t state)
{
  task->state = state;

  return ERROR_SUCCESS;
}



/* get task state
 */

task_state_t task_get_state(task_t* task)
{
  return task->state;
}



/* set task param
 */

void task_set_param(struct task* task, void* param)
{
  task->param = param;
}



/* set task timeslice
 */

error_t task_set_timeslice(task_t* task,
			   uint32_t timeslice)
{
  task->timeslice = timeslice;

  return ERROR_SUCCESS;
}



/* wait for event to be signaled
 */

error_t task_wait_event(struct task* task, struct event* event)
{
  cpu_cli();

  task->state = TASK_STATE_WAIT;
  task->event = event;

  cpu_sti();

  return ERROR_SUCCESS;
}



/* test task subsystem
 */

static error_t __attribute__((fastcall, unused)) do_printer(task_t* task)
{
  struct task* task_switcher;

  task_switcher = task->param;

  while (1)
    {
      serial_printl("p");
      cpu_hlt();

/*       cpu_cli(); */
/*       task_set_timeslice(task, 0); */
/*       cpu_sti(); */

/*       task_switch_to(task_switcher); */
    }

  return ERROR_SUCCESS;
}


static error_t __attribute__((fastcall, unused)) do_switcher(task_t* task)
{
  struct task* task_printer;
  volatile uint32_t count;

  task_printer = task->param;

  count = 0;

  while (1)
    {
      serial_printl("s(%d:%d)", task->id, count);
      ++count;
      sched_yield();
    }

  return ERROR_SUCCESS;
}


void task_test(void)
{
  task_t* task_switcher;
  task_t* task_printer;
  task_t* task;

#if 0
  {
    uint32_t i;

    for (i = 0; i < 10; ++i)
      {
	task = task_create_with_entry(do_printer, NULL);
	if (task != NULL)
	  {
	    serial_printl("[?] task %d created\n", task->id);

	    task_set_state(task, TASK_STATE_READY);
	    task_set_timeslice(task, TASK_SCHED_TIMESLICE);
	    sched_add_task(task);
	  }
      }
  }
#else
  {
    task = task_create_with_entry(do_printer, NULL);
    if (task != NULL)
      {
	task_set_state(task, TASK_STATE_READY);
	task_set_timeslice(task, TASK_SCHED_TIMESLICE);
	sched_add_task(task);
      }
    task_printer = task;

    task = task_create_with_entry(do_switcher, NULL);
    if (task != NULL)
      {
	task_set_state(task, TASK_STATE_READY);
	task_set_timeslice(task, TASK_SCHED_TIMESLICE);
	sched_add_task(task);
      }
    task_switcher = task;

    task_set_param(task_switcher, task_printer);
    task_set_param(task_printer, task_switcher);
  }
#endif

  sched_start();

  while (1)
    {
      if (g_current_task == NULL)
	serial_printl("-");
      else
	serial_printl("k");
      cpu_hlt();
    }
}
