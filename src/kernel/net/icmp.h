/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Sep 10 01:01:28 2007 texane
** Last update Sun Nov 18 18:19:24 2007 texane
*/



#ifndef ICMP_H_INCLUDED
# define ICMP_H_INCLUDED



#include "../sys/types.h"



struct net_if;
struct net_dev;
struct net_buf;
struct net_sock;
struct sockaddr_in;



/* icmp types
 */

#define ICMP_TYPE_ECHO_REPLY 0
#define ICMP_TYPE_ECHO_QUERY 8



/* icmp header
 */

typedef struct
{
  uint8_t type;
  uint8_t code;
  uint16_t check;
} __attribute__((packed)) icmp_hdr_t;



/* icmp echo
 */

typedef struct
{
  uint16_t id;
  uint16_t seq;
} __attribute__((packed)) icmp_echo_t;



/* routines
 */

error_t icmp_initialize(void);
error_t icmp_recv(struct net_if*, struct net_buf*);
error_t icmp_send(struct net_if*, struct net_buf*, const struct sockaddr_in*);
error_t icmp_ping(struct net_if*, uint32_t);
error_t icmp_bind(struct net_sock*, const struct sockaddr_in*);
error_t icmp_unbind(struct net_sock*);



#endif /* ! ICMP_H_INCLUDED */
