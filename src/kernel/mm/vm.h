/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Sep 10 23:16:24 2007 texane
** Last update Mon Oct 15 15:05:37 2007 texane
*/



#ifndef VM_H_INCLUDED
# define VM_H_INCLUDED



#include "../sys/types.h"



/* forward
 */

struct pde;
struct vm_as;



/* macros
 */

#define VM_PAGE_SIZE (0x1000)
#define VM_PAGE_MASK ~(VM_PAGE_SIZE - 1)



/* address space
 */

typedef struct vm_as
{
  struct pde* pagedir;
  vaddr_t identity;
} vm_as_t;



/* vm operations
 */

error_t vm_init(void);
error_t vm_switch_as(vm_as_t*);
error_t vm_map(vm_as_t*, vaddr_t, paddr_t);
error_t vm_unmap(vm_as_t*, vaddr_t);
error_t vm_map_range(vm_as_t*, vaddr_t, paddr_t, uint32_t);
error_t vm_unmap_range(vm_as_t*, vaddr_t, uint32_t);
vm_as_t* vm_get_kernel_as(void);
vm_as_t* vm_get_current_as(void);
void vm_dump_as(vm_as_t*);



#endif /* ! VM_H_INCLUDED */
