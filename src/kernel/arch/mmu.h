/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 15 21:46:16 2007 texane
** Last update Wed Sep  5 00:41:33 2007 texane
*/


#ifndef MMU_H_INCLUDED
# define MMU_H_INCLUDED


#include "../sys/types.h"


int mmu_init(void);
void mmu_debug(void);
void mmu_enable_paging(void);
int mmu_handle_page_fault(vaddr_t);
void mmu_test(void);


#endif /* ! MMU_H_INCLUDED */
