/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Oct 15 21:41:51 2007 texane
** Last update Mon Oct 15 21:51:18 2007 texane
*/



#ifndef KERNEL_H_INCLUDED
# define KERNEL_H_INCLUDED



#include "sys/types.h"



/* kernel info
 */

typedef struct
{
  paddr_t kernel_addr;
  uint32_t kernel_size;
} kernel_info_t;


extern kernel_info_t* g_kernel_info;



#endif /* ! KERNEL_H_INCLUDED */
