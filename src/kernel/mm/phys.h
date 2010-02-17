/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Oct 13 19:14:25 2007 texane
** Last update Sun Oct 14 11:18:20 2007 texane
*/



#ifndef PHYS_H_INCLUDED
# define PHYS_H_INCLUDED



#include "../sys/types.h"



struct multiboot_info;


#define PHYS_FRAME_SIZE 0x1000


error_t phys_init(const struct multiboot_info*);
error_t phys_release(void);
paddr_t phys_alloc(void);
void phys_free(paddr_t);
paddr_t phys_alloc_range(uint32_t);
void phys_free_range(paddr_t, uint32_t);
uint32_t phys_get_mem_size(void);
void phys_get_allocator_range(paddr_t*, uint32_t*);
void phys_test(void);
void phys_debug(void);



#endif /* ! PHYS_H_INCLUDED */
