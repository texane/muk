/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Sep 10 01:01:22 2007 texane
** Last update Tue Nov 20 00:01:50 2007 texane
*/



#include "../mm/mm.h"
#include "../libc/libc.h"
#include "../net/net.h"
#include "../sys/types.h"
#include "../debug/debug.h"



/* initialize icmp protocol
 */

error_t icmp_initialize(void)
{
  return ERROR_SUCCESS;
}



/* icmp echo packet building
 */

inline static uint16_t icmp_compute_checksum(uchar_t* buf, size_t size)
{
  uint32_t sum;
  uint16_t* p;
  
  sum = 0;
  p = (uint16_t*)buf;

  while (size > 1)
    {
      sum += *p;
      ++p;
      size -= 2;
    }

  if (size)
    sum += *(uchar_t*)p;

  sum = (sum & 0xffff) + (sum >> 16);
  sum += sum >> 16;

  return ~(uint16_t)sum;
}


inline static void build_icmp_echo_query(uchar_t* buf, uchar_t* buf_data, size_t sz_data)
{
  icmp_hdr_t* icmp;
  icmp_echo_t* echo;
  size_t size;

  size = sizeof(icmp_hdr_t) + sizeof(icmp_echo_t) + sz_data;

  memset(buf, 0, size);
  if (sz_data)
    memcpy(buf + sizeof(icmp_hdr_t) + sizeof(icmp_echo_t), buf_data, sz_data);

  icmp = (icmp_hdr_t*)buf;
  icmp->type = ICMP_TYPE_ECHO_QUERY;

  echo = (icmp_echo_t*)(buf + sizeof(icmp_hdr_t));
  echo->id = 0x2a;
  echo->seq = 0x2a;

  icmp->check = htons(icmp_compute_checksum(buf, size));
}


inline static void build_icmp_echo_reply(uchar_t* buf, uint16_t id, uint16_t seq, uchar_t* buf_data, size_t sz_data)
{
  icmp_hdr_t* icmp;
  icmp_echo_t* echo;
  size_t size;

  size = sizeof(icmp_hdr_t) + sizeof(icmp_echo_t) + sz_data;

  memset(buf, 0, size);
  memcpy(buf + sizeof(icmp_hdr_t) + sizeof(icmp_echo_t), buf_data, sz_data);

  icmp = (icmp_hdr_t*)buf;
  icmp->type = ICMP_TYPE_ECHO_REPLY;

  echo = (icmp_echo_t*)(buf + sizeof(icmp_hdr_t));
  echo->id = id;
  echo->seq = seq;

  icmp->check = icmp_compute_checksum(buf, size);
}



/* icmp echo query
 */

inline static error_t process_icmp_echo_query(struct net_if* net_if,
					      struct net_buf* nb_query)
{
  error_t err;
  struct net_buf* nb_reply;
  size_t sz_data;
  uchar_t* buf;

  /* build reply packet */
  {
    size_t size;
    eth_hdr_t* eth;
    ip_hdr_t* ip;
    icmp_echo_t* echo;

    net_buf_read(nb_query, (uchar_t**)(void*)&echo, &size);
    sz_data = size - sizeof(icmp_echo_t);

    buf = mm_alloc(sizeof(eth_hdr_t) + sizeof(ip_hdr_t) + sizeof(icmp_hdr_t) + sizeof(icmp_echo_t) + sz_data);
    if (buf == NULL)
      return ERROR_NO_MEMORY;

    /* icmp */
    build_icmp_echo_reply(buf + sizeof(eth_hdr_t) + sizeof(ip_hdr_t), echo->id, echo->seq, (uchar_t*)echo + sizeof(icmp_echo_t), sz_data);

    /* ip */
    ip = (ip_hdr_t*)(buf + sizeof(eth_hdr_t));
    memset(ip, 0, sizeof(ip_hdr_t));
    ip->ihl = sizeof(ip_hdr_t) / sizeof(uint32_t);
    ip->version = 4;
    ip->ttl = 128;
    ip->saddr = htonl(nb_query->proto_dst);
    ip->daddr = htonl(nb_query->proto_src);
    ip->proto = IP_PROTO_ICMP;
    ip->tot_len = htons(sizeof(ip_hdr_t) + sizeof(icmp_hdr_t) + sizeof(icmp_echo_t) + sz_data);
    ip->check = ip_compute_checksum((uchar_t*)ip, ip->ihl * sizeof(uint32_t));

    /* ethernet */
    eth = (eth_hdr_t*)buf;
    memcpy(eth->dest, nb_query->hw_src, ETH_ADDR_LEN);
    memcpy(eth->src, nb_query->hw_dst, ETH_ADDR_LEN);
    eth->type = htons(ETH_PROTO_IP);
  }

  /* send net buffer
   */
  err = net_buf_alloc(&nb_reply);
  if (error_is_failure(err))
    return err;

  nb_reply->size = sizeof(eth_hdr_t) + sizeof(ip_hdr_t) + sizeof(icmp_hdr_t) + sizeof(icmp_echo_t) + sz_data;
  nb_reply->buf = buf;

  err = net_if_send(net_if, nb_reply);

  net_buf_free(nb_reply);

  return err;
}



/* icmp echo reply
 */

inline static error_t process_icmp_echo_reply(struct net_if* net_if,
					      struct net_buf* nb)
{
  NOT_IMPLEMENTED();

  return ERROR_NOT_IMPLEMENTED;
}



/* receive an icmp packet
 */

error_t icmp_recv(struct net_if* net_if,
		  struct net_buf* net_buf)
{
  uchar_t* buf;
  icmp_hdr_t* icmp;
  size_t size;
  error_t err;

  net_buf_read(net_buf, &buf, &size);
  if (size < sizeof(icmp_hdr_t))
    return ERROR_INVALID_SIZE;

  net_buf_advance(net_buf, sizeof(icmp_hdr_t));

  err = ERROR_SUCCESS;
  icmp = (icmp_hdr_t*)buf;
  switch (icmp->type)
    {
    case ICMP_TYPE_ECHO_REPLY:
      err = process_icmp_echo_reply(net_if, net_buf);
      break;

    case ICMP_TYPE_ECHO_QUERY:
      err = process_icmp_echo_query(net_if, net_buf);
      break;

    default:
      NOT_IMPLEMENTED();
      err = ERROR_NOT_IMPLEMENTED;
      break;
    }

  return err;
}



/* send icmp
 */

error_t icmp_send(struct net_if* net_if,
		  struct net_buf* net_buf,
		  const struct sockaddr_in* remote_addr)
{
  NOT_IMPLEMENTED();

  return ERROR_NOT_IMPLEMENTED;
}



/* icmp bind operation
 */

error_t icmp_bind(struct net_sock* net_sock,
		  const struct sockaddr_in* local_addr)
{
  return ERROR_SUCCESS;
}



/* icmp unbind operation
 */

error_t icmp_unbind(struct net_sock* net_sock)
{
  return ERROR_SUCCESS;
}



/* ping a remote host
 */

error_t icmp_ping(struct net_if* net_if, uint32_t proto_addr)
{
  error_t err;
  struct net_buf* nb;
  arp_entry_t* ent;
  uchar_t* buf;
  size_t sz_data;
  eth_hdr_t* eth;
  ip_hdr_t* ip;
  uchar_t* buf_data;
  uchar_t hw_local[ETH_ADDR_LEN];

  err = arp_find_entry_by_proto(proto_addr, &ent);
  if (error_is_failure(err))
    {
      arp_resolve(net_if, proto_addr);
      return err;
    }

  net_if_get_hw_addr(net_if, hw_local);

  buf_data = (uchar_t*)"toto";
  sz_data = sizeof("toto") - 1;

  buf =
    mm_alloc(sizeof(eth_hdr_t) +
	     sizeof(ip_hdr_t) +
	     sizeof(icmp_hdr_t) +
	     sizeof(icmp_echo_t) +
	     sz_data);
  if (buf == NULL)
    return ERROR_NO_MEMORY;

  /* icmp */
  build_icmp_echo_query(buf +
			sizeof(eth_hdr_t) +
			sizeof(ip_hdr_t),
			buf_data,
			sz_data);

  /* ip */
  ip = (ip_hdr_t*)(buf + sizeof(eth_hdr_t));
  memset(ip, 0, sizeof(ip_hdr_t));
  ip->ihl = sizeof(ip_hdr_t) / sizeof(uint32_t);
  ip->version = 4;
  ip->ttl = 128;
  net_if_get_if_addr(net_if, &ip->saddr);
  ip->saddr = htonl(ip->saddr);
  ip->daddr = proto_addr;
  ip->proto = IP_PROTO_ICMP;
  ip->tot_len = htons(sizeof(ip_hdr_t) + sizeof(icmp_hdr_t) + sizeof(icmp_echo_t) + sz_data);
  ip->check = htons(ip_compute_checksum((uchar_t*)ip, ip->ihl * sizeof(uint32_t)));

  /* ethernet */
  eth = (eth_hdr_t*)buf;
  memcpy(eth->dest, ent->hw_addr, ETH_ADDR_LEN);
  memcpy(eth->src, hw_local, ETH_ADDR_LEN);
  eth->type = htons(ETH_PROTO_IP);

  /* net buffer */
  err = net_buf_alloc(&nb);
  if (error_is_failure(err))
    return err;
  nb->size = sizeof(eth_hdr_t) + sizeof(ip_hdr_t) + sizeof(icmp_hdr_t) + sizeof(icmp_echo_t) + sz_data;
  nb->buf = buf;

  err = net_if_send(net_if, nb);
  net_buf_free(nb);

  return err;
}
