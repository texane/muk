/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Sep  9 13:13:12 2007 texane
** Last update Mon Nov 19 02:12:14 2007 texane
*/


#ifndef IP_H_INCLUDED
# define IP_H_INCLUDED



#include "../sys/types.h"



struct net_if;
struct net_dev;
struct net_buf;
struct sockaddr_in;



/* macros
 */

#define IP_ADDR_LEN 4
#define IP_ADDR_ANY 0xffffffff



/* protocols
 */

#define IP_PROTO_INVALID 0x0
#define IP_PROTO_ICMP 0x1
#define IP_PROTO_TCP 0x6
#define IP_PROTO_UDP 0x11



/* ip header
 */

typedef struct
{
  uchar_t ihl : 4;
  uchar_t version : 4;
  uchar_t tos;
  ushort_t tot_len;
  ushort_t id;
  ushort_t frag_off;
  uchar_t ttl;
  uchar_t proto;
  ushort_t check;
  uint32_t saddr;
  uint32_t daddr;
} __attribute__((packed)) ip_hdr_t;



/* routines
 */

error_t ip_initialize(void);
uint16_t ip_compute_checksum(uchar_t*, ssize_t);
error_t ip_recv(struct net_if*, struct net_buf*);
error_t ip_send(struct net_if*, struct net_buf*, const struct sockaddr_in*, uint8_t);
error_t ip_route_find_if(uint32_t, struct net_if**);



#endif /* ! IP_H_INCLUDED */
