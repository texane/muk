/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Nov 17 21:14:45 2007 texane
** Last update Mon Nov 19 03:50:46 2007 texane
*/



#ifndef NET_BUF_H_INCLUDED
# define NET_BUF_H_INCLUDED



#include "../sys/types.h"
#include "../net/eth.h"



typedef struct net_buf
{
  struct net_buf* next;
  uchar_t* buf;
  size_t size;
  off_t off;

  /* meta */
  ushort_t dport;
  ushort_t sport;
  uint32_t proto_src;
  uint32_t proto_dst;
  uchar_t hw_src[ETH_ADDR_LEN];
  uchar_t hw_dst[ETH_ADDR_LEN];
} net_buf_t;



error_t net_buf_alloc(struct net_buf**);
error_t net_buf_alloc_with_size(struct net_buf**, size_t);
error_t net_buf_free(struct net_buf*);
error_t net_buf_advance(struct net_buf*, off_t);
error_t net_buf_rewind(struct net_buf*, off_t);
error_t net_buf_append(struct net_buf*, uchar_t*, size_t);
error_t net_buf_read(struct net_buf*, uchar_t**, size_t*);
error_t net_buf_write(struct net_buf*, const uint8_t*, size_t);
error_t net_buf_get_size(const struct net_buf*, size_t*);
error_t net_buf_clone(struct net_buf*, struct net_buf**, size_t);



#endif /* ! NET_BUF_H_INCLUDED */
