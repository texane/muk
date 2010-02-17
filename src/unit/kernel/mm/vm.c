/*
** Made by texane <texane@gmail.com>
** 
** Started on  Mon Oct 15 14:51:32 2007 texane
** Last update Mon Oct 15 15:10:45 2007 texane
*/



#include "../../../kernel/mm/mm.h"
#include "../../../kernel/sys/types.h"
#include "../../../kernel/libc/libc.h"
#include "../../../kernel/debug/debug.h"



/* cpu related 
 */

typedef struct pde
{
  uint8_t present : 1;
  uint8_t writable : 1;
  uint8_t user : 1;
  uint8_t pwt : 1;
  uint8_t cachable : 1;
  uint8_t accessed : 1;
  uint8_t zero : 1;
  uint8_t size : 1;
  uint8_t global : 1;
  uint8_t avail : 3;
  uint32_t address : 20;
} __attribute__((packed)) pde_t;


typedef struct pde4m
{
  uint8_t present : 1;
  uint8_t writable : 1;
  uint8_t user : 1;
  uint8_t pwt : 1;
  uint8_t cachable : 1;
  uint8_t accessed : 1;
  uint8_t dirty : 1;
  uint8_t size : 1;
  uint8_t global : 1;
  uint8_t avail : 3;
  uint8_t pat : 1;
  uint32_t reserved : 9;
  uint32_t address : 10;
} __attribute__((packed)) pde4m_t;


typedef struct pte
{
  uint8_t present : 1;
  uint8_t writable : 1;
  uint8_t user : 1;
  uint8_t pwt : 1;
  uint8_t cachable : 1;
  uint8_t accessed : 1;
  uint8_t dirty : 1;
  uint8_t pat : 1;
  uint8_t global : 1;
  uint8_t avail : 3;
  uint32_t address : 20;
} __attribute__((packed)) pte_t;


inline static uint32_t vaddr_get_pd_index(vaddr_t vaddr)
{
  return (uint32_t)vaddr >> 22;
}


inline static uint32_t vaddr_get_pt_index(vaddr_t vaddr)
{
  return ((uint32_t)vaddr >> 12) & 0x3ff;
}


inline static uint32_t vaddr_get_pg_offset(vaddr_t vaddr)
{
  return (uint32_t)vaddr & 0xfff;
}



/* util routines
 */

static paddr_t vaddr_to_paddr(vm_as_t* as,
			      vaddr_t vaddr)
{
  pte_t* pte;
  pde_t* pde;
  vaddr_t tmp;

  pde = &as->pagedir[vaddr_get_pd_index(vaddr)];
  if (pde->present == 0)
    return (paddr_t)NULL;

  tmp = as->identity + (pde->address << 12);
  pte = &((pte_t*)tmp)[vaddr_get_pt_index(vaddr)];
  if (pte->present == 0)
    return (paddr_t)NULL;

  return (paddr_t)((pte->address << 12) + vaddr_get_pg_offset(vaddr));
}



/* test routines
 */

static void test_identity_zone(void)
{
  uint32_t i;
  uint32_t count;
  paddr_t paddr;
  vm_as_t* as;

  as = vm_get_kernel_as();

  count = phys_get_mem_size();
  for (i = 0; i < count; ++i)
    {
      paddr = vaddr_to_paddr(as, (vaddr_t)i);
      if (paddr != NULL)
	  {
	    if (*(uint8_t*)paddr != ((uint8_t*)as->identity)[i])
	      serial_printl("[!] error @ 0x%x\n", i);
	  }
    }
}


static void test_page_fault(void)
{
  volatile uint32_t* vaddr_0 = (uint32_t*)0xb0000000;
  volatile uint32_t* vaddr_1 = (uint32_t*)0xb0001000;
  volatile uint32_t* vaddr_2 = (uint32_t*)0xb0002000;
  paddr_t paddr = phys_alloc();
  vm_as_t* as;

  as = vm_get_kernel_as();

  vm_map(as, (vaddr_t)vaddr_0, paddr);
  vm_map(as, (vaddr_t)vaddr_1, paddr);
  vm_map(as, (vaddr_t)vaddr_2, paddr);

  *vaddr_0 = 0x2a;

  if ((*vaddr_0 != 0x2a) ||
      (*vaddr_0 != *vaddr_1) ||
      (*vaddr_0 != *vaddr_2))
    BUG();
}


static void test_vm_map(void)
{
  paddr_t paddr;
  vaddr_t vaddr;
  vm_as_t* as;

  as = vm_get_kernel_as();

  vaddr = (vaddr_t)0xb0000000;
  paddr = phys_alloc();

  vm_map(as, vaddr + PHYS_FRAME_SIZE * 0, paddr);
  *(volatile uint32_t*)(vaddr + PHYS_FRAME_SIZE * 0) = 0x2a;
  serial_printl("[?] --> %x\n",
		*(volatile uint32_t*)(vaddr + PHYS_FRAME_SIZE * 0));

  vm_map(as, vaddr + PHYS_FRAME_SIZE * 1, paddr);
  *(volatile uint32_t*)(vaddr + PHYS_FRAME_SIZE * 1) = 0x2a;
  serial_printl("[?] --> %x\n",
		*(volatile uint32_t*)(vaddr + PHYS_FRAME_SIZE * 1));

  vm_map(as, vaddr + PHYS_FRAME_SIZE * 2, paddr);
  *(volatile uint32_t*)(vaddr + PHYS_FRAME_SIZE * 2) = 0x2a;
  serial_printl("[?] --> %x\n",
		*(volatile uint32_t*)(vaddr + PHYS_FRAME_SIZE * 2));

  phys_free(paddr);
}


static void test_vm_unmap(void)
{
  paddr_t paddr;
  vaddr_t vaddr;
  vm_as_t* as;

  as = vm_get_kernel_as();

  serial_printl("[?] test unmap\n");

  vaddr = (vaddr_t)0xb0000000;
  paddr = phys_alloc();
  vm_map(as, vaddr, paddr);

  /* no page fault
   */
  *(uint32_t*)vaddr = 0x2a;

  vm_unmap(as, vaddr);
  phys_free(paddr);

  /* page fault
   */
  *(uint32_t*)vaddr = 0x2a;
}


static void test_vm_unmap_range(void)
{
#define VM_RANGE_SIZE (10 * VM_PAGE_SIZE)
#define VM_MAGIC_KEY 0x2a
  vm_as_t* as;
  vaddr_t vaddr_0;
  vaddr_t vaddr_1;
  paddr_t paddr;
  uint32_t size;
  uint32_t i;

  serial_printl("[?] > vm/test/range\n");

  as = vm_get_kernel_as();

  size = VM_RANGE_SIZE;
  vaddr_0 = (vaddr_t)0xa0000000 + VM_RANGE_SIZE * 0;
  vaddr_1 = (vaddr_t)0xb0000000 + VM_RANGE_SIZE * 1;
  paddr = phys_alloc_range(size / VM_PAGE_SIZE);
  if (paddr == (paddr_t)NULL)
    {
      BUG();
    }
  else
    {
      serial_printl(" + vaddr_0: 0x%x\n", (uint32_t)vaddr_0);
      serial_printl(" + vaddr_1: 0x%x\n", (uint32_t)vaddr_1);
      serial_printl(" + paddr  : 0x%x\n", (uint32_t)paddr);
      serial_printl(" + size   : 0x%x\n", size);

      vm_map_range(as, vaddr_0, paddr, size);
      vm_map_range(as, vaddr_1, paddr, size);

      memset(vaddr_0, VM_MAGIC_KEY, size);
      for (i = 0; i < size; ++i)
	{
	  if ((((uint8_t*)vaddr_0)[i] != VM_MAGIC_KEY) ||
	      (((uint8_t*)vaddr_0)[i] != ((uint8_t*)vaddr_1)[i]))
	    {
	      BUG();
	      serial_printl(" --> %x\n", ((uint8_t*)vaddr_0)[i]);
	      serial_printl(" --> %x\n", ((uint8_t*)vaddr_1)[i]);
	    }
	}
    }

  /* should page fault
   */
  vm_unmap_range(as, vaddr_0, size);
  phys_free_range(paddr, size / VM_PAGE_SIZE);
  *(uint8_t*)vaddr_0 = VM_MAGIC_KEY;
  if (*(uint8_t*)vaddr_0 != VM_MAGIC_KEY)
    BUG();

  serial_printl("[?] < vm/test/range\n");
}


/* exported
 */

void unit_test_vm(void)
{
  TRACE_ENTRY();

  test_identity_zone();
  test_page_fault();
  test_vm_map();
  test_vm_unmap();
  test_vm_unmap_range();

  TRACE_EXIT();
}
