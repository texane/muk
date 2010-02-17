/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Nov 17 20:52:11 2007 texane
** Last update Sun Nov 18 03:29:51 2007 texane
*/



#include "../net/net.h"
#include "../mm/mm.h"
#include "../sys/types.h"



error_t net_dev_create(struct net_dev** res, void* priv)
{
  net_dev_t* net_dev;

  *res = NULL;

  net_dev = mm_alloc(sizeof(net_dev_t));
  if (net_dev == NULL)
    return ERROR_NO_MEMORY;

  net_dev->next = NULL;
  net_dev->prev = NULL;
  net_dev->send = NULL;
  net_dev->get_addr = NULL;
  net_dev->set_addr = NULL;
  net_dev->net_if = NULL;
  net_dev->priv = priv;

  *res = net_dev;

  return ERROR_SUCCESS;
}



error_t net_dev_destroy(struct net_dev* net_dev)
{
  return ERROR_NOT_IMPLEMENTED;
}



error_t net_dev_bind_if(net_dev_t* net_dev, struct net_if* net_if)
{
  net_dev->net_if = net_if;

  return ERROR_SUCCESS;
}



error_t net_dev_send(struct net_dev* net_dev, struct net_buf* net_buf)
{
  if (net_dev->send == NULL)
    return ERROR_NOT_SUPPORTED;

  return net_dev->send(net_dev, net_buf);
}
