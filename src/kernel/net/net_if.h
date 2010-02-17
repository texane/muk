/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Nov 17 18:17:42 2007 texane
** Last update Sun Nov 18 14:34:18 2007 texane
*/



#ifndef NET_IF_INCLUDED_H
# define NET_IF_INCLUDED_H



#include "../sys/types.h"



struct net_if;
struct net_dev;
struct net_buf;



error_t net_if_create(struct net_if**, struct net_dev*);
error_t net_if_destroy(struct net_if*);
error_t net_if_rx(struct net_if*, struct net_buf*);
error_t net_if_tx(struct net_if*, struct net_buf*);
error_t net_if_up(struct net_if*);
error_t net_if_down(struct net_if*);
bool_t net_if_is_up(const struct net_if*);
error_t net_if_set_if_addr(struct net_if*, uint32_t);
error_t net_if_get_if_addr(struct net_if*, uint32_t*);
error_t net_if_get_hw_addr(struct net_if*, uint8_t*);
error_t net_if_get_mtu(struct net_if*, size_t*);
struct net_dev* net_if_get_dev(struct net_if*);
error_t net_if_send(struct net_if*, struct net_buf*);
struct net_if* net_if_get_instance(void);



#endif /* ! NET_IF_INCLUDED_H */
