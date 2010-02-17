/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 15 21:51:03 2007 texane
** Last update Wed Aug 15 23:13:16 2007 texane
*/



#include "../debug/debug.h"
#include "../arch/trap.h"



/* timer handler
 */
static void timer_handler(void)
{
  serial_printl("%s\n", __FUNCTION__);
}


/* init interrupts
 */
static void intr_init(void)
{
  /* todos:
     . build the idt
     . program the pic
  */
}


/* exported
 */
int trap_init(void)
{
  intr_init();
  trap_add_handler(TRAP_ID_TIMER, timer_handler);

  return 0;
}


/* exported
 */
int trap_add_handler(trap_id_t id, trap_handler_t handler)
{
  return -1;
}


/* exported
 */
int trap_remove_handler(trap_id_t id, trap_handler_t handler)
{
  return -1;
}


/* exported
 */
void trap_debug(void)
{
}
