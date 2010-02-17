/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Sep  9 01:53:56 2007 texane
** Last update Mon Nov 19 02:56:49 2007 texane
*/



#include "../task/task.h"
#include "../sched/sched.h"
#include "../sys/types.h"
#include "../debug/debug.h"
#include "../libc/libc.h"
#include "../mm/mm.h"
#include "../net/net.h"
#include "../net/arp.h"
#include "../net/eth.h"
#include "../net/conf.h"
#include "../drivers/rtl8139.h"
#include "../arch/arch.h"



/* network task
 */

static error_t __attribute__((fastcall)) do_network(task_t* task)
{
  struct net_dev* net_dev;
  struct net_if* net_if;
  error_t error;

  /* protocols
   */
  eth_initialize();
  arp_initialize();
  ip_initialize();
  icmp_initialize();
  udp_initialize();

  /* device
   */
  error = rtl8139_initialize(&net_dev);
  if (error_is_failure(error))
    return error;

  /* interface
   */
  error = net_if_create(&net_if, net_dev);
  if (error_is_failure(error))
    return error;

  /* inetd
   */
  error = inetd_create_task();
  if (error_is_failure(error))
    return error;

  /* network loop
   */
  while (1)
    {
      serial_printl("n");
      cpu_hlt();
    }
  
  return ERROR_SUCCESS;
}



/* initialize networking
 */

error_t net_initialize(void)
{
  task_t* task;

  task = task_create_with_entry(do_network, NULL);
  if (task == NULL)
    return ERROR_FAILURE;

  task_set_state(task, TASK_STATE_READY);
  task_set_timeslice(task, TASK_SCHED_TIMESLICE);
  sched_add_task(task);

  return ERROR_SUCCESS;
}



/* cleanup networking
 */

error_t net_cleanup(void)
{
  rtl8139_cleanup();

  return ERROR_SUCCESS;
}



/* test networking
 */

error_t net_test(void)
{
  TRACE_ENTRY();

  rtl8139_test();

  return ERROR_SUCCESS;
}
