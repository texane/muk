/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Nov 17 15:45:07 2007 texane
** Last update Sat Nov 17 15:46:02 2007 texane
*/



#ifndef TFTP_H_INCLUDED
# define TFTP_H_INCLUDED



#include "../sys/types.h"



struct net_dev;
struct net_buf;



error_t tftp_recv(struct net_dev*, struct net_buf*);



#endif /* ! TFTP_H_INCLUDED */
