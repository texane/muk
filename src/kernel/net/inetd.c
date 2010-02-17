/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Nov 12 17:55:54 2007 texane
** Last update Mon Nov 19 03:33:33 2007 texane
*/



#include "../net/net.h"
#include "../task/task.h"
#include "../debug/debug.h"
#include "../sys/types.h"
#include "../mm/mm.h"
#include "../sched/sched.h"
#include "../libc/libc.h"



/* echod
 */

static error_t __attribute__((fastcall)) do_echod(struct task* task)
{
  struct sockaddr_in local_addr;
  struct sockaddr_in remote_addr;
  struct net_sock* net_sock;
  size_t count;
  error_t error;
  uint8_t buf[32];

  /* create udp socket
   */
  error = net_sock_create_udp(&net_sock);
  if (error_is_failure(error))
    return error;

  /* local is INADDR_ANY:ECHO
   */
  memset(&local_addr, 0, sizeof(struct sockaddr_in));
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(UDP_PROTO_ECHO);
  local_addr.sin_addr.s_addr = INADDR_ANY;

  error = net_sock_bind(net_sock, &local_addr);
  if (error_is_failure(error))
    {
      net_sock_destroy(net_sock);
      return error;
    }

  /* message loop
   */
  while (1)
    {
      error = net_sock_recv(net_sock, buf, sizeof(buf), &remote_addr, &count);
      if (error_is_success(error))
	error = net_sock_send(net_sock, buf, count, &remote_addr, &count);
    }

  /* destroy socket
   */
  net_sock_destroy(net_sock);

  return ERROR_SUCCESS;
}


/* discard
 */

static error_t __attribute__((fastcall)) do_discard(struct task* task)
{
  struct sockaddr_in local_addr;
  struct sockaddr_in remote_addr;
  struct net_sock* net_sock;
  size_t count;
  error_t error;
  uint8_t buf[32];

  /* create udp socket
   */
  error = net_sock_create_udp(&net_sock);
  if (error_is_failure(error))
    return error;

  /* local is INADDR_ANY:DISCARD
   */
  memset(&local_addr, 0, sizeof(struct sockaddr_in));
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(UDP_PROTO_DISCARD);
  local_addr.sin_addr.s_addr = INADDR_ANY;

  error = net_sock_bind(net_sock, &local_addr);
  if (error_is_failure(error))
    {
      net_sock_destroy(net_sock);
      return error;
    }

  /* message loop
   */
  while (1)
    {
      error = net_sock_recv(net_sock, buf, sizeof(buf), &remote_addr, &count);
    }

  /* destroy socket
   */
  net_sock_destroy(net_sock);

  return ERROR_SUCCESS;
}


/* create inetd task
 */

error_t inetd_create_task(void)
{
  struct task* task;

  /* echod
   */
  task = task_create_with_entry(do_echod, NULL);
  if (task == NULL)
    return ERROR_NO_MEMORY;

  task_set_timeslice(task, TASK_SCHED_TIMESLICE);
  task_set_state(task, TASK_STATE_READY);
  sched_add_task(task);

  /* discard
   */
  task = task_create_with_entry(do_discard, NULL);
  if (task == NULL)
    return ERROR_NO_MEMORY;

  task_set_timeslice(task, TASK_SCHED_TIMESLICE);
  task_set_state(task, TASK_STATE_READY);
  sched_add_task(task);

  return ERROR_SUCCESS;
}
