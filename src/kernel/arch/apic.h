/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 15 18:39:37 2007 texane
** Last update Sun Aug 19 21:42:36 2007 texane
*/


#ifndef APIC_H_INCLUDED
# define APIC_H_INCLUDED


#include "../sys/types.h"


int apic_initialize(void);
int apic_release(void);
int apic_get_addr(paddr_t*);
int apic_set_addr(paddr_t);
void apic_ack(void);
void apic_debug(void);


#endif /* ! APIC_H_INCLUDED */
