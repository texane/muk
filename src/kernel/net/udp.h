/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Nov 12 17:22:14 2007 texane
** Last update Sun Nov 18 18:26:12 2007 texane
*/



#ifndef UDP_H_INCLUDED
# define UDP_H_INCLUDED



#include "../sys/types.h"



struct net_if;
struct net_dev;
struct net_buf;
struct net_sock;
struct sockaddr_in;



typedef struct
{
  ushort_t sport;
  ushort_t dport;
  ushort_t length;
  ushort_t check;
} __attribute__((packed)) udp_hdr_t;



#define UDP_PROTO_ECHO 7
#define UDP_PROTO_DISCARD 9
#define UDP_PROTO_BOOTPS 67
#define UDP_PROTO_BOOTPC 68



error_t udp_initialize(void);
error_t udp_recv(struct net_if*, struct net_buf*);
error_t udp_bind(struct net_sock*, const struct sockaddr_in*);
error_t udp_unbind(struct net_sock*);
error_t udp_send(struct net_if*, struct net_buf*, const struct sockaddr_in*, uint16_t);



#endif /* ! UDP_H_INCLUDED */
