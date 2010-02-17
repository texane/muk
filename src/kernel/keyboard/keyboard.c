/*
** Made by texane <texane@gmail.com>
** 
** Started on  Tue Dec 11 02:31:01 2007 texane
** Last update Wed Dec 12 03:25:46 2007 texane
*/



#include "../sys/types.h"
#include "../debug/debug.h"
#include "../task/task.h"
#include "../event/event.h"
#include "../arch/cpu.h"
#include "../mm/mm.h"
#include "../keyboard/keyboard.h"
#include "../keyboard/i8042.h"
#include "../libc/libc.h"




struct keyboard
{
  struct event* event;
  bool_t is_overflow;
  uint32_t size;
  uint32_t count;
  uint8_t buffer[0];
};


static struct keyboard* g_keyboard;


error_t keyboard_initialize(void)
{
  error_t error;

  g_keyboard = mm_alloc(0x1000);
  if (g_keyboard == NULL)
    return ERROR_NO_MEMORY;

  g_keyboard->count = 0;
  g_keyboard->size = 0x1000 - sizeof(struct keyboard);
  g_keyboard->is_overflow = false;

  error = i8042_initialize();
  if (error_is_failure(error))
    {
      mm_free(g_keyboard);
      g_keyboard = NULL;
      return error;
    }

  error = event_create(&g_keyboard->event);
  if (error_is_failure(error))
    {
      mm_free(g_keyboard);
      g_keyboard = NULL;
      return error;
    }

  return ERROR_SUCCESS;
}



error_t keyboard_release(void)
{
  struct keyboard* k;

  cpu_cli();

  k = g_keyboard;
  g_keyboard = NULL;

  cpu_sti();

  if (k->event != NULL)
    event_destroy(k->event);
  mm_free(k);

  return ERROR_SUCCESS;
}



error_t keyboard_read(uchar_t* buf,
		      uint32_t size,
		      uint32_t* nread)
{
  error_t error;

  *nread = 0;

  error = event_wait(g_keyboard->event);
  if (error_is_failure(error))
    return error;

  event_clear_signal(g_keyboard->event);

  cpu_cli();

  if (g_keyboard->count)
    {
      if (g_keyboard->count < size)
	size = g_keyboard->count;

      memcpy(buf, g_keyboard->buffer, size);

      *nread = size;
    }

  cpu_sti();

  return ERROR_SUCCESS;
}



error_t keyboard_flush(void)
{
  cpu_cli();

  g_keyboard->count = 0;

  cpu_sti();

  return ERROR_SUCCESS;
}



error_t keyboard_putc(uint8_t c)
{
  if (g_keyboard->count >= g_keyboard->size)
    {
      g_keyboard->is_overflow = true;
      return ERROR_NO_MEMORY;
    }

  g_keyboard->buffer[g_keyboard->count++] = c;

  event_signal(g_keyboard->event);

  return ERROR_SUCCESS;
}
