/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Aug 25 20:37:44 2007 texane
** Last update Tue Nov 13 01:46:31 2007 texane
*/



#ifndef RTL8139_H_INCLUDED
# define RTL8139_H_INCLUDED


#include "../sys/types.h"


struct net_dev;


error_t rtl8139_initialize(struct net_dev**);
error_t rtl8139_cleanup(void);
void rtl8139_test(void);


#endif /* ! RTL8139_H_INCLUDED */
