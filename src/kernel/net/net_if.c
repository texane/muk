/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Nov 17 18:17:36 2007 texane
** Last update Mon Nov 19 03:12:58 2007 texane
*/



#include "../net/net.h"
#include "../sys/types.h"
#include "../task/task.h"
#include "../sched/sched.h"
#include "../mm/mm.h"
#include "../debug/debug.h"
#include "../arch/arch.h"
#include "../libc/libc.h"



/* network interface
 */

typedef struct net_if
{
  struct task* task;

  uint32_t if_addr;
  uint32_t net_mask;
  uint32_t gw_addr;
  uint32_t dns_addr;

  net_dev_t* net_dev;

  net_buf_t* tx_bufs;
  net_buf_t* rx_bufs;

  bool_t is_up;

} net_if_t;



/* network interface task
 */

struct net_if_param
{
  struct net_if* net_if;
  struct net_dev* net_dev;
};


static error_t __attribute__((fastcall)) do_net_if(task_t* task)
{
  struct net_if_param* param;
  error_t error;
  net_buf_t* pos;
  net_buf_t* tmp;
  net_if_t* net_if;

  param = task->param;
  net_if = param->net_if;

  /* bind the interface
   */
  net_if->net_dev = param->net_dev;
  net_dev_bind_if(param->net_dev, net_if);

  /* interface addressing
   */
  error = bootp_create_client_task(net_if);
  if (error_is_failure(error))
    return error;

  /* poll network buffers
   */
  while (1)
    {
      pos = NULL;

      /* cli, no races with isr
       */
      cpu_cli();
      if (net_if->rx_bufs != NULL)
	{
	  pos = net_if->rx_bufs;
	  net_if->rx_bufs = NULL;
	}
      cpu_sti();

      /* process buffers
       */
      while (pos != NULL)
	{
	  tmp = pos;
	  pos = pos->next;
	  eth_recv(net_if, tmp);
	  net_buf_free(tmp);
	}

      /* wait to be scheduled
       */
      cpu_hlt();
    }

  return ERROR_SUCCESS;
}



/* net if destructor
 */

static void net_if_dtor(task_t* task)
{
  if (task->param != NULL)
    {
      mm_free(task->param);
      task->param = NULL;
    }
}



/* create a network interface
 */

static net_if_t* g_net_if = NULL;


inline static void net_if_reset(net_if_t* net_if)
{
  net_if->task = NULL;
  net_if->if_addr = 0;
  net_if->net_mask = 0;
  net_if->gw_addr = 0;
  net_if->dns_addr = 0;
  net_if->net_dev = NULL;
  net_if->tx_bufs = NULL;
  net_if->rx_bufs = NULL;
  net_if->is_up = false;
}


error_t net_if_create(net_if_t** res, net_dev_t* net_dev)
{
  struct net_if_param* param;
  net_if_t* net_if;

  *res = NULL;

  /* allocate interface
   */
  net_if = mm_alloc(sizeof(net_if_t));
  if (net_if == NULL)
    return ERROR_NO_MEMORY;

  /* task param
   */
  param = mm_alloc(sizeof(struct net_if_param));
  if (param == NULL)
    {
      mm_free(net_if);
      return ERROR_NO_MEMORY;
    }
  param->net_if = net_if;
  param->net_dev = net_dev;

  /* interface task
   */
  net_if->task = task_create_with_entry(do_net_if, param);
  if (net_if->task == NULL)
    {
      mm_free(net_if);
      mm_free(param);
      return ERROR_FAILURE;
    }

  task_set_state(net_if->task, TASK_STATE_READY);
  task_set_timeslice(net_if->task, TASK_SCHED_TIMESLICE);
  task_set_dtor(net_if->task, net_if_dtor);
  sched_add_task(net_if->task);

  *res = net_if;

  /* to remove */
  g_net_if = net_if;
  
  return ERROR_SUCCESS;
}



/* destroy a network interface
 */

error_t net_if_destroy(net_if_t* net_if)
{
  net_buf_t* pos;
  net_buf_t* tmp;

  pos = net_if->tx_bufs;
  while (pos != NULL)
    {
      tmp = pos;
      pos = pos->next;
      net_buf_free(tmp);
    }

  pos = net_if->rx_bufs;
  while (pos != NULL)
    {
      tmp = pos;
      pos = pos->next;
      net_buf_free(tmp);
    }

  mm_free(net_if);
  
  return ERROR_SUCCESS;
}



/* called by isr upon buffer rx
 */

error_t net_if_rx(net_if_t* net_if, net_buf_t* net_buf)
{
  net_buf_t* pos;

  /* push back
   */
  if (net_if->rx_bufs != NULL)
    {
      pos = net_if->rx_bufs;
      while (pos->next != NULL)
	pos = pos->next;
      pos->next = net_buf;
    }
  else
    {
      net_if->rx_bufs = net_buf;
    }

  net_buf->next = NULL;

  return ERROR_SUCCESS;
}



/* called by protocol to tx
 */

error_t net_if_tx(net_if_t* net_if, net_buf_t* net_buf)
{
  NOT_IMPLEMENTED();

  return ERROR_NOT_IMPLEMENTED;
}



/* interface activation
 */

error_t net_if_up(net_if_t* net_if)
{
  volatile uint32_t* if_addr;

  /* wait for the interface to be up
   */
  if_addr = &net_if->if_addr;
  while (*if_addr == 0)
    ;

  return ERROR_SUCCESS;
}


error_t net_if_down(net_if_t* net_if)
{
  /* fixme: release the address
   */

  net_if->is_up = false;

  return ERROR_SUCCESS;
}


bool_t net_if_is_up(const net_if_t* net_if)
{
  return net_if->is_up;
}



/* interface addressing
 */

error_t net_if_set_if_addr(net_if_t* net_if, uint32_t if_addr)
{
  net_if->if_addr = if_addr;

  return ERROR_SUCCESS;
}


error_t net_if_get_if_addr(net_if_t* net_if, uint32_t* if_addr)
{
  *if_addr = net_if->if_addr;

  if (*if_addr == 0)
    return ERROR_INVALID_ADDR;

  return ERROR_SUCCESS;
}


error_t net_if_get_hw_addr(net_if_t* net_if, uint8_t* hw_addr)
{
  if (net_if->net_dev == NULL)
    return ERROR_NO_DEVICE;

  if (net_if->net_dev->get_addr != NULL)
    return net_if->net_dev->get_addr(net_if->net_dev, hw_addr);

  return ERROR_NOT_SUPPORTED;
}



/* get mtu
 */

error_t net_if_get_mtu(struct net_if* net_if, size_t* mtu)
{
  /* fixme: should be net_dev->get_mtu() */

  *mtu = 1500;

  return ERROR_SUCCESS;
}



/* get associated device
 */

struct net_dev* net_if_get_dev(net_if_t* net_if)
{
  return net_if->net_dev;
}



/* send buffer to device
 */

error_t net_if_send(struct net_if* net_if, struct net_buf* net_buf)
{
  struct net_dev* net_dev;

  net_dev = net_if_get_dev(net_if);
  if (net_dev == NULL)
    return ERROR_NO_DEVICE;

  return net_dev_send(net_dev, net_buf);
}



/* get unique interface
 */

struct net_if* net_if_get_instance(void)
{
  return g_net_if;
}
