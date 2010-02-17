/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 22 23:21:34 2007 texane
** Last update Mon Oct 15 15:08:12 2007 texane
*/



#ifndef MM_H_INCLUDED
# define MM_H_INCLUDED



#include "vm.h"
#include "phys.h"
#include "../sys/types.h"
#include "../boot/multiboot.h"



/* macros
 */

#define MM_PFRAME_SIZE 0x1000



/* physical memory
 */

int mm_initialize(addr_t, size_t);
void* mm_alloc(size_t);
void mm_free(void*);
void mm_debug(void);
void mm_test(void);



/* page frames
 */

paddr_t pframe_alloc(count_t);
void pframe_free(paddr_t);
int pframe_initialize_with_mbi(multiboot_info_t*);
void pframe_test(void);


/* segments
 */
typedef struct
{
  paddr_t base;
  size_t size;
  unsigned int attrs;
} mm_segment_t;

int mm_segment_initialize(paddr_t, size_t);
int mm_segment_initialize_from_mbi(multiboot_info_t*);
void mm_segment_cleanup(void);
int mm_segment_reserve(void);
mm_segment_t* mm_segment_new(size_t);
int mm_segment_delete(mm_segment_t*);
uint32_t mm_get_size(void);


#endif /* ! MM_H_INCLUDED */
