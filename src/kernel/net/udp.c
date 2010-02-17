/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Nov 12 17:22:07 2007 texane
** Last update Mon Nov 19 03:52:32 2007 texane
*/



#include "../net/net.h"
#include "../debug/debug.h"
#include "../mm/mm.h"
#include "../sys/types.h"
#include "../libc/libc.h"
#include "../net/inetd.h"



/* internal, manage udp connections
 */

#define UDP_CON_COUNT 128


struct udp_con
{
  struct sockaddr_in addr;
  struct net_sock* sock;
  bool_t used;
};


static struct udp_con* g_udp_cons = NULL;


static error_t udp_con_initialize(void)
{
  uint32_t i;
  struct udp_con* con;

  g_udp_cons = mm_alloc(UDP_CON_COUNT * sizeof(struct udp_con));
  if (g_udp_cons == NULL)
    return ERROR_NO_MEMORY;

  for (i = 0; i < UDP_CON_COUNT; ++i)
    {
      con = &g_udp_cons[i];
      memset(&con->addr, 0, sizeof(struct sockaddr_in));
      con->sock = NULL;
      con->used = false;
    }

  return ERROR_SUCCESS;
}


inline static bool_t addr_is_eq(const struct sockaddr_in* sa,
				const struct sockaddr_in* sb)
{
  if (sa->sin_family == sb->sin_family)
    if (sa->sin_port == sb->sin_port)
      {
	if ((sa->sin_addr.s_addr == sb->sin_addr.s_addr) ||
	    (sa->sin_addr.s_addr == INADDR_ANY) ||
	    (sb->sin_addr.s_addr == INADDR_ANY))
	  return true;
      }

  return false;
}


static error_t udp_con_find_by_addr(const struct sockaddr_in* addr,
				    struct udp_con** con)
{
  struct udp_con* pos;
  uint32_t i;

  *con = NULL;

  for (i = 0; i < UDP_CON_COUNT; ++i)
    {
      pos = &g_udp_cons[i];
      if ((pos->used == true) && (addr_is_eq(&pos->addr, addr) == true))
	{
	  *con = pos;
	  return ERROR_SUCCESS;
	}
    }

  return ERROR_NOT_FOUND;
}


static error_t udp_con_add_by_addr(const struct sockaddr_in* addr,
				   struct net_sock* sock)
{
  struct udp_con* con;
  uint32_t i;

  for (i = 0; i < UDP_CON_COUNT; ++i)
    {
      con = &g_udp_cons[i];
      if (con->used == false)
	{
	  memcpy(&con->addr, addr, sizeof(struct sockaddr_in));
	  con->sock = sock;
	  con->used = true;

	  return ERROR_SUCCESS;
	}
    }
  
  return ERROR_NO_MEMORY;
}


static error_t udp_con_del_by_addr(const struct sockaddr_in* addr)
{
  struct udp_con* con;
  error_t error;

  error = udp_con_find_by_addr(addr, &con);
  if (error_is_failure(error))
    return error;

  con->sock = NULL;
  memset(&con->addr, 0, sizeof(struct sockaddr_in));
  con->used = false;

  return ERROR_SUCCESS;
}



/* initialize udp protocol
 */

error_t udp_initialize(void)
{
  error_t error;

  error = udp_con_initialize();

  return error;
}



/* receive an udp packet
 */

error_t udp_recv(struct net_if* net_if, struct net_buf* nb)
{
  udp_hdr_t* udp_hdr;
  struct net_sock* net_sock;
  struct udp_con* udp_con;
  struct sockaddr_in local_addr;
  error_t error;
  size_t size;

  error = net_buf_read(nb, (uint8_t**)(void*)&udp_hdr, &size);
  if (error_is_failure(error))
    return error;

  if (size < sizeof(udp_hdr_t))
    return ERROR_INVALID_SIZE;

  /* meta
   */
  nb->dport = ntohs(udp_hdr->dport);
  nb->sport = ntohs(udp_hdr->sport);

  /* find socket
   */
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = udp_hdr->dport;
  local_addr.sin_addr.s_addr = htonl(nb->proto_dst);
  error = udp_con_find_by_addr(&local_addr, &udp_con);
  if (error_is_failure(error))
    return error;

  net_buf_advance(nb, sizeof(udp_hdr_t));

  /* push data to socket
   */
  net_sock = udp_con->sock;
  size = ntohs(udp_hdr->length) - sizeof(udp_hdr_t);
  net_sock_push(net_sock, nb, size);
  
  return ERROR_SUCCESS;
}



/* udp send operation
 */

error_t udp_send(struct net_if* net_if,
		 struct net_buf* net_buf,
		 const struct sockaddr_in* remote_addr,
		 uint16_t local_port)
{
  error_t error;
  udp_hdr_t udp_hdr;
  size_t size;

  /* build udp header
   */
  net_buf_get_size(net_buf, &size);
  memset(&udp_hdr, 0, sizeof(udp_hdr_t));
  udp_hdr.dport = remote_addr->sin_port;
  udp_hdr.sport = htons(local_port);
  udp_hdr.length = htons(sizeof(udp_hdr_t) + size);
  udp_hdr.check = 0x0;

  /* prepend udp header
   */
  net_buf_rewind(net_buf, sizeof(udp_hdr_t));
  error = net_buf_write(net_buf, (const uint8_t*)&udp_hdr, sizeof(udp_hdr_t));
  if (error_is_failure(error))
    return error;

  /* transmit to ip
   */
  error = ip_send(net_if, net_buf, remote_addr, IP_PROTO_UDP);

  return error;
}



/* udp bind operation
 */

error_t udp_bind(struct net_sock* net_sock,
		 const struct sockaddr_in* local_addr)
{
  struct udp_con* con;
  error_t error;

  /* already exists
   */
  error = udp_con_find_by_addr(local_addr, &con);
  if (error_is_success(error))
    return ERROR_ALREADY_USED;

  error = udp_con_add_by_addr(local_addr, net_sock);

  return error;
}



/* udp unbind operation
 */

error_t udp_unbind(struct net_sock* net_sock)
{
  const struct sockaddr_in* local_addr;
  error_t error;

  error = net_sock_get_local_addr(net_sock, &local_addr);
  if (error_is_failure(error))
    return error;

  error = udp_con_del_by_addr(local_addr);

  return error;
}
