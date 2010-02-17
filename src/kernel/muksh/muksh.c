/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Nov 17 03:01:40 2007 texane
** Last update Wed Dec 12 03:49:00 2007 texane
*/



#include "../task/task.h"
#include "../sched/sched.h"
#include "../muksh/muksh.h"
#include "../sys/types.h"
#include "../arch/arch.h"
#include "../debug/debug.h"
#include "../keyboard/keyboard.h"



#if 0

static void translate(const uint8_t* buf,
		      uint32_t size,
		      char* s)
{
  const uint16_t* p;
  uint32_t i;
  uint32_t j;
  bool_t is_break;

  is_break = false;
  i = 0;
  p = (const uint16_t*)buf;

  for (j = 0; j < (size / 2); ++j)
    {
      switch (p[i])
	{
	  /* break code
	   */
	case 0x07e0:
	  is_break = true;
	  break;

	default:
	  if (is_break == false)
	    s[i++] = 0;
	  is_break = false;
	  break;
	}
    }

  s[i] = 0;
}

#endif


/* create muksh task
 */

static error_t __attribute__((fastcall)) do_muksh(task_t* task)
{
  uint32_t size;
  error_t error;
  uint8_t buf[256];
/*   char string[sizeof(buf)]; */

  serial_printl("%s(%d)\n", __FUNCTION__, task->id);

  keyboard_initialize();

  while (1)
    {
      error = keyboard_read(buf, sizeof(buf), &size);
      if (error_is_failure(error))
	return error;

      if (size)
	{
#if 0
	  translate(buf, size, string);
	  if (string[strlen(string) - 1] == '\n')
	    {
	      serial_printl("%s\n", string);
	      keyboard_flush();
	    }
#endif
	}
    }

  return ERROR_SUCCESS;
}


error_t muksh_initialize(void)
{
  task_t* task;

  task = task_create_with_entry(do_muksh, NULL);
  if (task == NULL)
    return ERROR_FAILURE;

  task_set_state(task, TASK_STATE_READY);
  task_set_timeslice(task, TASK_SCHED_TIMESLICE);

  sched_add_task(task);

  return ERROR_SUCCESS;
}
