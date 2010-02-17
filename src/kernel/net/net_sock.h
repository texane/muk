/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Nov 18 00:51:05 2007 texane
** Last update Mon Nov 19 23:04:26 2007 texane
*/



#ifndef NET_SOCK_H_INCLUDED
# define NET_SOCK_H_INCLUDED



#include "../sys/types.h"



struct timeval;
struct net_sock;



/* internet adressing
 */

#define AF_UNSPEC 0
#define AF_UNIX 1
#define AF_LOCAL AF_UNIX
#define AF_INET 2


#define INADDR_ANY 0
#define INADDR_BROADCAST (~0)


struct sin_addr
{
  uint32_t s_addr;
};


struct sockaddr_in
{
  uint16_t sin_family;
  struct sin_addr sin_addr;
  uint16_t sin_port;
};



/* socket api
 */

error_t net_sock_create_udp(struct net_sock**);
error_t net_sock_create_icmp(struct net_sock**);
error_t net_sock_destroy(struct net_sock*);
error_t net_sock_bind(struct net_sock*, const struct sockaddr_in*);
error_t net_sock_unbind(struct net_sock*);
error_t net_sock_send(struct net_sock*, const uint8_t*, size_t, const struct sockaddr_in*, size_t*);
error_t net_sock_recv(volatile struct net_sock*, uint8_t*, size_t, struct sockaddr_in*, size_t*);
error_t net_sock_set_timeout(struct net_sock*, const struct timeval*);
error_t net_sock_get_local_port(const struct net_sock*, uint16_t*);
error_t net_sock_get_local_addr(const struct net_sock*, const struct sockaddr_in**);
error_t net_sock_push(struct net_sock*, struct net_buf*, size_t);



#endif /* ! NET_SOCK_H_INCLUDED */
