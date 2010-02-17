/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 15 21:46:54 2007 texane
** Last update Wed Sep  5 00:42:20 2007 texane
*/



#include "../arch/mmu.h"
#include "../sys/types.h"
#include "../debug/debug.h"



static void enable_paging(void)
{
  asm("");
}



int mmu_initialize(void)
{
  enable_paging();
  return -1;
}



void mmu_debug(void)
{
}



int mmu_handle_page_fault(vaddr_t addr)
{
  return -1;
}



void mmu_test(void)
{
}
