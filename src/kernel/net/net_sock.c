/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Nov 18 01:03:09 2007 texane
** Last update Mon Nov 19 23:57:56 2007 texane
*/



#include "../net/net.h"
#include "../task/task.h"
#include "../event/event.h"
#include "../mm/mm.h"
#include "../arch/arch.h"
#include "../sched/sched.h"
#include "../libc/libc.h"
#include "../debug/debug.h"
#include "../sys/types.h"
#include "../sys/time.h"



/* socket type
 */

typedef struct net_sock
{
  struct task* task;
  struct net_if* net_if;
  struct sockaddr_in local_addr;
  struct sockaddr_in remote_addr;
  bool_t is_bound;
  uint32_t proto;
  struct timeval timeout;
  struct net_buf* net_buf;
  struct event* event;
} net_sock_t;



/* socket creation
 */

inline static void net_sock_reset(net_sock_t* net_sock)
{
  net_sock->task = NULL;
  net_sock->net_if = NULL;
  memset(&net_sock->local_addr, 0, sizeof(net_sock->local_addr));
  memset(&net_sock->remote_addr, 0, sizeof(net_sock->remote_addr));
  net_sock->proto = IP_PROTO_INVALID;
  net_sock->is_bound = false;
  net_sock->net_buf = NULL;
  timeval_make_infinite(&net_sock->timeout);
  net_sock->event = NULL;
}


error_t net_sock_create_udp(struct net_sock** res)
{
  net_sock_t* net_sock;

  *res = NULL;

  net_sock = mm_alloc(sizeof(net_sock_t));
  if (net_sock == NULL)
    return ERROR_NO_MEMORY;

  net_sock_reset(net_sock);

  net_sock->proto = IP_PROTO_UDP;

  event_create(&net_sock->event);

  *res = net_sock;

  return ERROR_SUCCESS;
}


error_t net_sock_create_icmp(struct net_sock** res)
{
  net_sock_t* net_sock;

  *res = NULL;

  net_sock = mm_alloc(sizeof(net_sock_t));
  if (net_sock == NULL)
    return ERROR_NO_MEMORY;

  net_sock_reset(net_sock);

  net_sock->proto = IP_PROTO_ICMP;

  event_create(&net_sock->event);

  *res = net_sock;

  return ERROR_SUCCESS;
}


error_t net_sock_destroy(net_sock_t* net_sock)
{
  net_sock_unbind(net_sock);

  event_destroy(net_sock->event);

  mm_free(net_sock);

  return ERROR_SUCCESS;
}



/* bind socket to address
 */

error_t net_sock_bind(net_sock_t* net_sock,
		      const struct sockaddr_in* local_addr)
{
  error_t error;
  struct net_if* net_if;

  /* already bound
   */
  if (net_sock->is_bound == true)
    return ERROR_ALREADY_USED;
  
  /* find local interface
   */
  error = ip_route_find_if(ntohl(local_addr->sin_addr.s_addr), &net_if);
  if (error_is_failure(error))
    return error;

  /* call protocol bind operation
   */
  switch (net_sock->proto)
    {
    case IP_PROTO_UDP:
      error = udp_bind(net_sock, local_addr);
      break;

    case IP_PROTO_ICMP:
      error = icmp_bind(net_sock, local_addr);
      break;

    default:
      error = ERROR_INVALID_PROTO;
      break;
    }

  /* update socket
   */
  if (error_is_success(error))
    {
      net_sock->is_bound = true;
      net_sock->net_if = net_if;
      memcpy(&net_sock->local_addr, local_addr, sizeof(struct sockaddr_in));
      return ERROR_SUCCESS;
    }

  return error;
}



/* unbind socket
 */

error_t net_sock_unbind(net_sock_t* net_sock)
{
  error_t error;

  /* not bound
   */
  if (net_sock->is_bound == false)
    return ERROR_NOT_SUPPORTED;

  /* call protocol bind operation
   */
  switch (net_sock->proto)
    {
    case IP_PROTO_UDP:
      error = udp_unbind(net_sock);
      break;

    case IP_PROTO_ICMP:
      error = icmp_unbind(net_sock);
      break;

    default:
      error = ERROR_INVALID_PROTO;
      break;
    }

  if (error_is_success(error))
    net_sock->is_bound = false;

  return error;
}



/* send data to the socket
 */

error_t net_sock_send(net_sock_t* net_sock,
		      const uint8_t* buf,
		      size_t size,
		      const struct sockaddr_in* remote_addr,
		      size_t* count)
{
  uint16_t local_port;
  struct net_buf* nb;
  struct net_if* net_if;
  error_t error;
  size_t mtu;

  *count = 0;

  /* find the local interface
   */
  net_if = net_sock->net_if;
  if (net_if == NULL)
    {
      error = ip_route_find_if(ntohl(remote_addr->sin_addr.s_addr), &net_if);
      if (error_is_failure(error))
	return error;
      net_sock->net_if = net_if;
    }

  /* buffer of the mtu size
   */
  error = net_if_get_mtu(net_if, &mtu);
  if (error_is_failure(error))
    return error;

  error = net_buf_alloc_with_size(&nb, mtu);
  if (error_is_failure(error))
    return error;

  /* write content at the end
   */
  net_buf_advance(nb, mtu - size);
  net_buf_write(nb, buf, size);

  /* send to protocol
   */
  switch (net_sock->proto)
    {
    case IP_PROTO_UDP:
      net_sock_get_local_port(net_sock, &local_port);
      error = udp_send(net_if, nb, remote_addr, local_port);
      break;

    case IP_PROTO_ICMP:
      error = icmp_send(net_if, nb, remote_addr);
      break;

    default:
      error = ERROR_INVALID_PROTO;
      break;
    }

  /* free net buf
   */
  net_buf_free(nb);

  if (error_is_success(error))
    {
      *count = size;
      return ERROR_SUCCESS;
    }

  return error;
}



/* receive data from the socket
 */

error_t net_sock_recv(volatile net_sock_t* net_sock,
		      uint8_t* buf,
		      size_t size,
		      struct sockaddr_in* addr_from,
		      size_t* count)
{
  struct net_buf* nb;
  uint8_t* nb_buf;
  size_t nb_size;
  error_t error;

  if (addr_from != NULL)
    memset(addr_from, 0, sizeof(struct sockaddr_in));

  *count = 0;

  if (net_sock->net_if == NULL)
    return ERROR_INVALID_ADDR;

  /* block until data or timeout
   */
  event_wait(net_sock->event);

  nb = net_sock->net_buf;
  net_sock->net_buf = NULL;

  if (nb == NULL)
    return ERROR_NO_DATA;

  /* fill buffer
   */
  error = net_buf_read(nb, &nb_buf, &nb_size);
  if (error_is_failure(error))
    return error;

  if (nb_size < size)
    size = nb_size;
  memcpy(buf, nb_buf, size);
  *count = size;

  /* fill addr_from
   */
  if (addr_from != NULL)
    {
      addr_from->sin_family = AF_INET;
      addr_from->sin_addr.s_addr = htonl(nb->proto_src);
      addr_from->sin_port = htons(nb->sport);
    }

  net_buf_free(nb);

  return ERROR_SUCCESS;
}



/* get local port
 */

error_t net_sock_get_local_port(const struct net_sock* net_sock, uint16_t* port)
{
  *port = ntohs(net_sock->local_addr.sin_port);

  return ERROR_SUCCESS;
}



/* get local address
 */

error_t net_sock_get_local_addr(const struct net_sock* net_sock,
				const struct sockaddr_in** local_addr)
{
  *local_addr = &net_sock->local_addr;

  return ERROR_SUCCESS;
}



/* push data to socket
 */

error_t net_sock_push(net_sock_t* net_sock,
		      struct net_buf* net_buf,
		      size_t size)
{
  if (net_sock->net_buf != NULL)
    {
      BUG();
      return ERROR_FAILURE;
    }

  net_buf_clone(net_buf, &net_sock->net_buf, size);

  event_signal(net_sock->event);

  return ERROR_SUCCESS;
}
