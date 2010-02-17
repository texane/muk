/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Sep  9 13:14:45 2007 texane
** Last update Mon Nov 19 03:37:19 2007 texane
*/



#include "../debug/debug.h"
#include "../sys/types.h"
#include "../net/net.h"
#include "../libc/libc.h"



/* initialize ip protocol
 */

error_t ip_initialize(void)
{
  return ERROR_SUCCESS;
}



/* compute the ip checksum
 */

uint16_t ip_compute_checksum(uchar_t* buf, ssize_t size)
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

  while (sum >> 16)
    sum = (sum & 0xffff) + (sum >> 16);

  return ~(uint16_t)sum;
}



/* build a basic ip header
 */

inline static void build_ip_hdr(ip_hdr_t* ip)
{
  memset(ip, 0, sizeof(ip_hdr_t));
  ip->ihl = 5;
  ip->version = 4;
  ip->ttl = 32;
}



/* receive an ip packet
 */

error_t ip_recv(struct net_if* net_if, struct net_buf* nb)
{
  error_t err;
  uchar_t* buf;
  size_t size;
  ip_hdr_t* ip;

  net_buf_read(nb, &buf, &size);
  if (size < sizeof(ip_hdr_t))
    return ERROR_INVALID_SIZE;

  /* check size */
  ip = (ip_hdr_t*)buf;
  if ((ip->ihl * 4) > size)
    return ERROR_INVALID_SIZE;

  /* meta */
  nb->proto_src = ntohl(ip->saddr);
  nb->proto_dst = ntohl(ip->daddr);

  net_buf_advance(nb, ip->ihl * 4);

  err = ERROR_SUCCESS;

  switch (ip->proto)
    {
    case IP_PROTO_ICMP:
      err = icmp_recv(net_if, nb);
      break;

    case IP_PROTO_UDP:
      err = udp_recv(net_if, nb);
      break;

    case IP_PROTO_TCP:
    default:
      NOT_IMPLEMENTED();
      err = ERROR_NOT_IMPLEMENTED;
      break;
    }

  return err;
}



/* ip send operation
 */

error_t ip_send(struct net_if* net_if,
		struct net_buf* net_buf,
		const struct sockaddr_in* remote_addr,
		uint8_t proto_id)
{
  error_t error;
  size_t size;
  uint32_t local_addr;
  arp_entry_t* arp_entry;
  ip_hdr_t ip_hdr;
  uint8_t hw_addr[ETH_ADDR_LEN];

  /* resolve ethernet address
   */
  if (remote_addr->sin_addr.s_addr != INADDR_BROADCAST)
    {
      error = arp_resolve(net_if, remote_addr->sin_addr.s_addr);
      if (error_is_failure(error))
	return error;

      error = arp_find_entry_by_proto(remote_addr->sin_addr.s_addr,
				      &arp_entry);
      if (error_is_failure(error))
	return error;

      memcpy(hw_addr, arp_entry->hw_addr, ETH_ADDR_LEN);
    }
  else
    {
      memset(hw_addr, ~0, ETH_ADDR_LEN);
    }

  /* build ip header
   */
  memset(&ip_hdr, 0, sizeof(ip_hdr_t));
  ip_hdr.ihl = 5;
  ip_hdr.version = 4;
  ip_hdr.ttl = 32;
  net_if_get_if_addr(net_if, &local_addr);
  ip_hdr.saddr = htonl(local_addr);
  ip_hdr.daddr = remote_addr->sin_addr.s_addr;
  ip_hdr.proto = proto_id;
  net_buf_get_size(net_buf, &size);
  ip_hdr.tot_len = htons(sizeof(ip_hdr_t) + size);
  ip_hdr.check = ip_compute_checksum((uchar_t*)&ip_hdr,
				     ip_hdr.ihl * sizeof(uint32_t));

  /* prepend ip header
   */
  net_buf_rewind(net_buf, sizeof(ip_hdr_t));
  error = net_buf_write(net_buf, (const uint8_t*)&ip_hdr, sizeof(ip_hdr_t));
  if (error_is_failure(error))
    return error;

  /* pass to ethernet
   */
  error = eth_send(net_if, net_buf, hw_addr, ETH_PROTO_IP);

  return error;
}



/* find outgoing interface
 */

error_t ip_route_find_if(uint32_t addr, struct net_if** res)
{
  struct net_if* net_if;

  *res = NULL;

  /* fixme: should walk the routing table
   */
  net_if = net_if_get_instance();
  if (net_if == NULL)
    return ERROR_NOT_FOUND;

  /* fixme: perform checks on address
   */

  *res = net_if;

  return ERROR_SUCCESS;
}
