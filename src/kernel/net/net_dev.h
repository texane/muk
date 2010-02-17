/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Nov 17 20:52:30 2007 texane
** Last update Sun Nov 18 03:28:59 2007 texane
*/



#ifndef NET_DEV_H_INCLUDED
# define NET_DEV_H_INCLUDED



#include "../sys/types.h"



struct net_if;



typedef struct net_dev
{
  /* linked list */
  struct net_dev* next;
  struct net_dev* prev;

  /* device operations */
  error_t (*send)(struct net_dev*, struct net_buf*);
  error_t (*get_addr)(struct net_dev*, uchar_t*);
  error_t (*set_addr)(struct net_dev*, uchar_t*);

  struct net_if* net_if;

  /* private data */
  void* priv;
} net_dev_t;



error_t net_dev_create(struct net_dev**, void*);
error_t net_dev_destroy(struct net_dev*);
error_t net_dev_bind_if(net_dev_t*, struct net_if*);
error_t net_dev_send(struct net_dev*, struct net_buf*);



#endif /* ! NET_DEV_H_INCLUDED */
