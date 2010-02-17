/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Nov 12 23:00:24 2007 texane
** Last update Sun Nov 18 03:43:35 2007 texane
*/



#ifndef BOOTP_H_INCLUDED
# define BOOTP_H_INCLUDED



#include "../sys/types.h"



struct net_dev;
struct net_buf;
struct net_if;



typedef struct
{
  uchar_t op;
  uchar_t htype;
  uchar_t hlen;
  uchar_t hops;
  uint_t xid;
  ushort_t secs;
  ushort_t zero;
  uint_t ciaddr;
  uint_t yiaddr;
  uint_t siaddr;
  uint_t giaddr;
  uchar_t chaddr[16];
  uchar_t sname[64];
  uchar_t file[128];
  uchar_t vend[64];
} __attribute__((packed)) bootp_hdr_t;



error_t bootp_recv(struct net_if*, struct net_buf*, const uchar_t*, size_t);
error_t bootp_send_request(struct net_if*);
error_t bootp_create_client_task(struct net_if*);



#endif /* ! BOOTP_H_INCLUDED */
