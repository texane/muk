/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Aug 26 20:37:54 2007 texane
** Last update Mon Nov 19 03:07:25 2007 texane
*/



#ifndef NET_H_INCLUDED
# define NET_H_INCLUDED



#include "../sys/types.h"
#include "net_if.h"
#include "net_dev.h"
#include "net_buf.h"
#include "net_sock.h"
#include "eth.h"
#include "arp.h"
#include "ip.h"
#include "icmp.h"
#include "udp.h"
#include "bootp.h"
#include "tftp.h"
#include "inetd.h"
#include "conf.h"



struct net_if;
struct net_buf;
struct net_dev;



/* network subsystem
 */

error_t net_initialize(void);
error_t net_cleanup(void);
error_t net_add_dev(net_dev_t**);
error_t net_test(void);



/* utilities
 */

inline static ushort_t ntohs(ushort_t s)
{
  return (s >> 8) | (s << 8);
}


inline static ushort_t htons(ushort_t s)
{
  return (s >> 8) | (s << 8);
}


inline static ulong_t htonl(ulong_t l)
{
  return ( ((l & 0xff000000) >> 24) |
	   ((l & 0x00ff0000) >>  8) |
	   ((l & 0x0000ff00) <<  8) |
	   ((l & 0x000000ff) << 24));
}


inline static ulong_t ntohl(ulong_t l)
{
  return ( ((l & 0xff000000) >> 24) |
	   ((l & 0x00ff0000) >>  8) |
	   ((l & 0x0000ff00) <<  8) |
	   ((l & 0x000000ff) << 24));
}



#endif /* ! NET_H_INCLUDED */
